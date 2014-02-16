#include "duktapevm.h"

#include <map>
#include <iostream>

namespace {	

/* 	
Key: context
Value: Map of function name and callback			
*/
typedef std::map<void*, std::map<std::string, duktape::Callback> > CallbackContainer;

static CallbackContainer s_callbackCache;

void registerContext(duk_context* ctx)
{
	s_callbackCache[ctx];
}

void unregisterContext(duk_context* ctx)
{
	auto context = s_callbackCache.find(ctx);
	if(context != s_callbackCache.end())
	{
		s_callbackCache.erase(context);
	}
}

void addCallback(duk_context* ctx, const std::string& functionName, duktape::Callback callbackFunc)
{
	auto context = s_callbackCache.find(ctx);
	if(context != s_callbackCache.end())
	{
		auto callback = context->second.find(functionName);
		if(callback == context->second.end())
		{
			context->second[functionName] = callbackFunc;
		}
	}
}

std::string doCallback(duk_context* ctx, const std::string& callbackName, const std::string& parameter)
{
	auto context = s_callbackCache.find(ctx);
	if(context != s_callbackCache.end())
	{
		auto callback = context->second.find(callbackName);
		if(callback != context->second.end())
		{
			return callback->second(parameter);
		}
	}
	return "";
}

int callbackHandler(duk_context* ctx) 
{
	// TODO: error handling
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "__callbackName");
		
	std::string callbackName = duk_to_string(ctx, -1);
	std::string parameter = duk_to_string(ctx, 0);

	std::string retVal = doCallback(ctx, callbackName, parameter);
	duk_push_string(ctx, retVal.c_str());

	return 1;
}

int safeEval(duk_context* ctx)
{
	duk_eval(ctx);
	return 0;
}

int safeCall(duk_context* ctx)
{
	duk_call(ctx, 1 /* number of params */);
	return 1;
}

int safeToJSON(duk_context* ctx)
{
	duk_json_encode(ctx, -1);
	return 1;
}

} // unnamed namespace

namespace duktape {

DuktapeVM::DuktapeVM():
m_ctx(duk_create_heap_default())
{
	registerContext(m_ctx);
}

DuktapeVM::~DuktapeVM()
{
	unregisterContext(m_ctx);
	duk_destroy_heap(m_ctx);
}

Result DuktapeVM::run(const std::string& scriptName, 
	const std::string& parameter, 
	const std::string& script) 
{
	Result res;
	int rc = 0;

	// Eval script
	duk_push_string(m_ctx, script.c_str());
	rc = duk_safe_call(m_ctx, safeEval, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if (rc != DUK_EXEC_SUCCESS) 
		goto error;	

	// Prepare and call function.
	duk_push_global_object(m_ctx);
	duk_get_prop_string(m_ctx, -1, scriptName.c_str());
	duk_push_string(m_ctx, parameter.c_str());	
	rc = duk_safe_call(m_ctx, safeCall, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if(rc != DUK_EXEC_SUCCESS)
		goto error;

	// Serialize output
	switch(duk_get_type(m_ctx, -1))
	{
		case DUK_TYPE_BOOLEAN:
		case DUK_TYPE_NUMBER:
		case DUK_TYPE_STRING:
			res.value = duk_to_string(m_ctx, -1);
			break;
		case DUK_TYPE_OBJECT:
			if(!duk_is_function(m_ctx, -1) && !duk_is_null_or_undefined(m_ctx, -1))
			{
				rc = duk_safe_call(m_ctx, safeToJSON, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
				if(rc != DUK_EXEC_SUCCESS)
					goto error;

				res.value = duk_to_string(m_ctx, -1);
			}
			break;
		default:
			res.value = "";
			break;
	}

	duk_pop(m_ctx);
	return res;

error:
	// Get error string.
	res.value = duk_to_string(m_ctx, -1);
	res.errorCode = rc;
	return res;
}

void DuktapeVM::registerCallback(const std::string& functionName, Callback callback) 
{
	duk_push_global_object(m_ctx);
	duk_push_c_function(m_ctx, callbackHandler, 1 /* number of parameters */);

	// Callback name as a property for function object.
	duk_push_string(m_ctx, functionName.c_str());
	duk_put_prop_string(m_ctx, -2, "__callbackName");

	duk_put_prop_string(m_ctx, -2, functionName.c_str());
	duk_pop(m_ctx);

	addCallback(m_ctx, functionName, callback);
}


} // namespace duktape
