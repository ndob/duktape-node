#include "duktapevm.h"

#include <node.h>
#include <v8.h>

#include <iostream>
#include <string>

using namespace v8;
using node::FatalException;

namespace {

struct WorkRequest
{
	WorkRequest(std::string functionName, std::string parameters, std::string script, Persistent<Function> callback):
	 functionName(std::move(functionName))
	,parameters(std::move(parameters))
	,script(std::move(script))
	,callback(callback)
	,hasError(false)
	{		
	};

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
		m_workRequest->callback.Dispose();
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

void onWork(uv_work_t* req)
{
	// Do not use scoped-wrapper as req is still needed in onWorkDone.
	WorkRequest* work = static_cast<WorkRequest*> (req->data);
	auto ret = duktape::runInVM(work->functionName, work->parameters, work->script);
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
}

Handle<Value> run(const Arguments& args) 
{
	HandleScope scope;
	if(args.Length() < 4) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString() || !args[3]->IsFunction()) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong arguments")));
		return scope.Close(Undefined());
	}

 	String::Utf8Value functionName(args[0]->ToString());
	String::Utf8Value parameters(args[1]->ToString());
	String::Utf8Value script(args[2]->ToString());
	Local<Function> callback = Local<Function>::Cast(args[3]);

	WorkRequest* workReq = new WorkRequest(	std::string(*functionName), 
											std::string(*parameters), 
											std::string(*script), 
											Persistent<Function>::New(callback));

    uv_work_t* req = new uv_work_t();
    req->data = workReq;

	uv_queue_work(uv_default_loop(), req, onWork, onWorkDone);

	return scope.Close(Undefined());
}


Handle<Value> runSync(const Arguments& args) 
{
	HandleScope scope;
	if(args.Length() < 3) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong arguments")));
		return scope.Close(Undefined());
	}

 	String::Utf8Value functionName(args[0]->ToString());
	String::Utf8Value parameters(args[1]->ToString());
	String::Utf8Value script(args[2]->ToString());

	auto ret = duktape::runInVM(std::string(*functionName), std::string(*parameters), std::string(*script));

	if(ret.errorCode != 0)
	{
		ThrowException(Exception::Error(String::New(ret.value.c_str())));
		return scope.Close(Undefined());
	}

	return scope.Close(String::New(ret.value.c_str()));
}

void init(Handle<Object> exports) 
{
	exports->Set(String::NewSymbol("runSync"), FunctionTemplate::New(runSync)->GetFunction());
	exports->Set(String::NewSymbol("run"), FunctionTemplate::New(run)->GetFunction());
}

} // unnamed namespace

NODE_MODULE(duktapejs, init)
