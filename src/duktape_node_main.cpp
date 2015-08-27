#include "run_sync.h"
#include "run_async.h"

#include <nan.h>

namespace {

// Main entrypoint
NAN_MODULE_INIT(init)
{
	using Nan::GetFunction;
	using Nan::New;
	using Nan::Set;
	using v8::String;
	using v8::FunctionTemplate;

	Set(target, 
		New<String>("runSync").ToLocalChecked(), 
		GetFunction(New<FunctionTemplate>(duktape::runSync)).ToLocalChecked());

	Set(target, 
		New<String>("run").ToLocalChecked(), 
		GetFunction(New<FunctionTemplate>(duktape::run)).ToLocalChecked());

}

} // unnamed namespace

NODE_MODULE(duktape, init)
