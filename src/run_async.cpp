#include "run_async.h"

#include "duktapevm.h"
#include "callback.h"

#include <node.h>
#include <v8.h>

#include <string>
#include <vector>

using namespace v8;
using node::FatalException;

namespace {

// Forward declaration for APICallbackSignaling destructor.
void cleanupUvAsync(uv_handle_s* handle);

// Forward declaration for CallbackHelper.
void callV8FunctionOnMainThread(uv_async_t* handle, int status);

struct WorkRequest
{
	WorkRequest(std::string functionName, std::string parameters, std::string script, Persistent<Function> callback):
	 functionName(std::move(functionName))
	,parameters(std::move(parameters))
	,script(std::move(script))
	,callback(callback)
	,hasError(false)
	,returnValue()
	{
	};

	~WorkRequest()
	{
		for(auto it = apiCallbackFunctions.begin(); it != apiCallbackFunctions.end(); ++it)
		{
			it->Dispose();
			it->Clear();
		}
		callback.Dispose();
		callback.Clear();
	}

	duktape::DuktapeVM vm;
	std::vector< Persistent<Function> > apiCallbackFunctions;

	// in
	std::string functionName;
	std::string parameters;
	std::string script;

	// out
	Persistent<Function> callback;
	bool hasError;
	std::string returnValue;
};

struct ScopedUvWorkRequest
{
	ScopedUvWorkRequest(uv_work_t* work):
	 m_work(work)
	,m_workRequest(static_cast<WorkRequest*> (m_work->data))
	{
	}

	~ScopedUvWorkRequest()
	{
		delete m_workRequest;
		delete m_work;
	}

	WorkRequest* getWorkRequest()
	{
		return m_workRequest;
	}

private:
	uv_work_t* m_work;
	WorkRequest* m_workRequest;
};

struct APICallbackSignaling
{	
	APICallbackSignaling(Persistent<Function> callback, std::string parameter, uv_async_cb cbFunc):
	 callback(callback)
	,parameter(parameter)
	,returnValue("")
	,cbFunc(cbFunc)
	,async(new uv_async_t)
	{
		uv_mutex_init(&mutex);
		uv_cond_init(&cv);
		uv_async_init(uv_default_loop(), async, cbFunc);	
	}

	~APICallbackSignaling()
	{
		uv_mutex_destroy(&mutex);
		uv_cond_destroy(&cv);
		uv_close((uv_handle_t*) async, &cleanupUvAsync);
	}

	Persistent<Function> callback;
	std::string parameter;
	std::string returnValue;

	uv_cond_t cv;
	uv_mutex_t mutex;
	uv_async_cb cbFunc;

	// Has to be on heap, because of closing logic.
	uv_async_t* async;
};

struct CallbackHelper
{
	CallbackHelper(Persistent<Function> persistentApiCallbackFunc):
	m_persistentApiCallbackFunc(persistentApiCallbackFunc)
	{
	}

	std::string operator()(const std::string& paramString)
	{
		// We're on not on libuv/V8 main thread. Signal main to run 
		// callback function and wait for an answer.
		APICallbackSignaling cbsignaling(m_persistentApiCallbackFunc, 
										paramString,
										callV8FunctionOnMainThread);
						
		uv_mutex_lock(&cbsignaling.mutex);

		cbsignaling.async->data = (void*) &cbsignaling;
		uv_async_send(cbsignaling.async);
		uv_cond_wait(&cbsignaling.cv, &cbsignaling.mutex);
		std::string retStr(cbsignaling.returnValue);

		uv_mutex_unlock(&cbsignaling.mutex);

		return retStr;
	}

private:
	Persistent<Function> m_persistentApiCallbackFunc;
};

void cleanupUvAsync(uv_handle_s* handle)
{
	// "handle" is "async"-parameter passed to uv_close
	delete (uv_async_t*) handle;
}

void callV8FunctionOnMainThread(uv_async_t* handle, int status) 
{
	auto signalData = static_cast<APICallbackSignaling*> (handle->data);
	uv_mutex_lock(&signalData->mutex);

	HandleScope scope;
	Handle<Value> argv[1];
	argv[0] = String::New(signalData->parameter.c_str());
	auto retVal = signalData->callback->Call(Context::GetCurrent()->Global(), 1, argv);
	String::Utf8Value retString(retVal);
	signalData->returnValue = std::string(*retString);

	uv_mutex_unlock(&signalData->mutex);
	uv_cond_signal(&signalData->cv);
}

void onWork(uv_work_t* req)
{
	// Do not use scoped-wrapper as req is still needed in onWorkDone.
	WorkRequest* work = static_cast<WorkRequest*> (req->data);

	auto ret = work->vm.run(work->functionName, work->parameters, work->script);
	work->hasError = ret.errorCode != 0;
	work->returnValue = ret.value;
}

void onWorkDone(uv_work_t* req, int status)
{
	ScopedUvWorkRequest uvReq(req);
	WorkRequest* work = uvReq.getWorkRequest();

	HandleScope scope;

	Handle<Value> argv[2];
	argv[0] = Boolean::New(work->hasError);
	argv[1] = String::New(work->returnValue.c_str());

	TryCatch try_catch;
	work->callback->Call(Context::GetCurrent()->Global(), 2, argv);

	if (try_catch.HasCaught()) 
	{
		FatalException(try_catch);
	}
	scope.Close(Undefined());
}

} // unnamed namespace

namespace duktape {

Handle<Value> run(const Arguments& args) 
{
	HandleScope scope;
	if(args.Length() < 5) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString() || !args[4]->IsFunction()) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong arguments")));
		return scope.Close(Undefined());
	}

	String::Utf8Value functionName(args[0]->ToString());
	String::Utf8Value parameters(args[1]->ToString());
	String::Utf8Value script(args[2]->ToString());
	Local<Function> returnCallback = Local<Function>::Cast(args[4]);

	WorkRequest* workReq = new WorkRequest(	std::string(*functionName), 
											std::string(*parameters), 
											std::string(*script), 
											Persistent<Function>::New(returnCallback));

	// API Handling
	if(args[3]->IsObject())
	{
		auto object = Handle<Object>::Cast(args[3]);
		auto properties = object->GetPropertyNames();

		auto len = properties->Length();
		for(unsigned int i = 0; i < len; ++i)
		{
			Local<Value> key = properties->Get(i);
			Local<Value> value = object->Get(key);
			if(!key->IsString() || !value->IsFunction())
			{
				ThrowException(Exception::Error(String::New("Error in API-definition")));
				return scope.Close(Undefined());
			}

			auto apiCallbackFunc = Local<Function>::Cast(value);
			auto persistentApiCallbackFunc = Persistent<Function>::New(apiCallbackFunc);
			auto duktapeToNodeBridge = duktape::Callback(CallbackHelper(persistentApiCallbackFunc));

			// Switch ownership of Persistent-Function to workReq
			workReq->apiCallbackFunctions.push_back(persistentApiCallbackFunc);

			String::Utf8Value keyStr(key);
			workReq->vm.registerCallback(std::string(*keyStr), duktapeToNodeBridge);
		}
	}

	uv_work_t* req = new uv_work_t();
	req->data = workReq;

	uv_queue_work(uv_default_loop(), req, onWork, onWorkDone);

	return scope.Close(Undefined());
}

} // namespace duktape
