#include "run_async.h"

#include "duktapevm.h"
#include "callback.h"

#include <string>
#include <vector>

using namespace v8;
using node::FatalException;

namespace {

typedef Nan::Persistent<Function, CopyablePersistentTraits<Function>> PersistentCallback;

// Forward declaration for APICallbackSignaling destructor.
void cleanupUvAsync(uv_handle_s* handle);

// Forward declaration for CallbackHelper.
void callV8FunctionOnMainThread(uv_async_t* handle); // 0.10 needs this:, int status);

struct WorkRequest
{
	WorkRequest(std::string functionName, std::string parameters, std::string script, Local<Function> callback):
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
		callback.Reset();
	}

	duktape::DuktapeVM vm;

	// in
	std::string functionName;
	std::string parameters;
	std::string script;

	// out
	Nan::Persistent<Function> callback;
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
	APICallbackSignaling(const PersistentCallback& cb, std::string parameter, uv_async_cb cbFunc):
	 callback(cb)
	,parameter(parameter)
	,returnValue("")
	,done(false)
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

	const PersistentCallback& callback;
	std::string parameter;
	std::string returnValue;

	bool done;

	uv_cond_t cv;
	uv_mutex_t mutex;
	uv_async_cb cbFunc;

	// Has to be on heap, because of closing logic.
	uv_async_t* async;
};

struct CallbackHelper
{
	CallbackHelper(Local<Function> apiCallbackFunc):
	m_persistentApiCallbackFunc(apiCallbackFunc)
	{
	}

	~CallbackHelper()
	{
		m_persistentApiCallbackFunc.Reset();
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
		while(!cbsignaling.done)
		{
			uv_cond_wait(&cbsignaling.cv, &cbsignaling.mutex);			
		}
		std::string retStr(cbsignaling.returnValue);

		uv_mutex_unlock(&cbsignaling.mutex);

		return retStr;
	}

private:
	PersistentCallback m_persistentApiCallbackFunc;
};

void cleanupUvAsync(uv_handle_s* handle)
{
	// "handle" is "async"-parameter passed to uv_close
	delete (uv_async_t*) handle;
}

void callV8FunctionOnMainThread(uv_async_t* handle) 
{
	auto signalData = static_cast<APICallbackSignaling*> (handle->data);
	uv_mutex_lock(&signalData->mutex);

	Handle<Value> argv[1];
	argv[0] = Nan::New(signalData->parameter).ToLocalChecked();

	auto callbackHandle = Local<Function>::New(v8::Isolate::GetCurrent(), signalData->callback);
	auto retVal = callbackHandle->Call(Nan::GetCurrentContext()->Global(), 1, argv);
	String::Utf8Value retString(retVal);
	signalData->returnValue = std::string(*retString);

	signalData->done = true;
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

	Handle<Value> argv[2];
	argv[0] = Nan::New(work->hasError);
	argv[1] = Nan::New(work->returnValue).ToLocalChecked();

	TryCatch try_catch;
	auto callbackHandle = Nan::New(work->callback);
	callbackHandle->Call(Nan::GetCurrentContext()->Global(), 2, argv);

	if (try_catch.HasCaught()) 
	{
		FatalException(try_catch);
	}
}

} // unnamed namespace

namespace duktape {

//Handle<Value> run(const Arguments& args) 
NAN_METHOD(run)
{
	if(info.Length() < 5) 
	{
		Nan::ThrowTypeError(Nan::New("Wrong number of arguments").ToLocalChecked());
		return;
	}

	if (!info[0]->IsString() || !info[1]->IsString() || !info[2]->IsString() || !info[4]->IsFunction()) 
	{
		Nan::ThrowTypeError(Nan::New("Wrong arguments").ToLocalChecked());
		return;
	}

	String::Utf8Value functionName(info[0]->ToString());
	String::Utf8Value parameters(info[1]->ToString());
	String::Utf8Value script(info[2]->ToString());


	auto workReq = new WorkRequest(	std::string(*functionName), 
											std::string(*parameters), 
											std::string(*script), 
											Local<Function>::Cast(info[4]));

	// API Handling
	if(info[3]->IsObject())
	{
		auto object = Handle<Object>::Cast(info[3]);
		auto properties = object->GetPropertyNames();

		auto len = properties->Length();
		for(unsigned int i = 0; i < len; ++i)
		{
			Local<Value> key = properties->Get(i);
			Local<Value> value = object->Get(key);
			if(!key->IsString() || !value->IsFunction())
			{
				Nan::ThrowError(Nan::New("Error in API-definition").ToLocalChecked());
				return;
			}
			
			String::Utf8Value keyStr(key);
			CallbackHelper duktapeToNodeBridge(Local<Function>::Cast(value));
			workReq->vm.registerCallback(std::string(*keyStr), duktapeToNodeBridge);
		}
	}

	uv_work_t* req = new uv_work_t();
	req->data = workReq;

	uv_queue_work(uv_default_loop(), req, onWork, onWorkDone);

	return;
}

} // namespace duktape
