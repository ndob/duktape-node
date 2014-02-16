#include "run_sync.h"
#include "run_async.h"

#include <node.h>
#include <v8.h>

namespace {

using namespace v8;

// Main entrypoint
void init(Handle<Object> exports) 
{
	exports->Set(String::NewSymbol("runSync"), FunctionTemplate::New(duktape::runSync)->GetFunction());
	exports->Set(String::NewSymbol("run"), FunctionTemplate::New(duktape::run)->GetFunction());
}

} // unnamed namespace

NODE_MODULE(duktape, init)
