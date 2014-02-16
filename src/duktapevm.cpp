#include "duktapevm.h"

#include "callbackcache.h"

#include <iostream>

namespace {	

using duktape::Result;

static duktape::CallbackCache cbCache;

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

Result serializeData(duk_context* ctx, int stackPosition)
{
	Result res;

	switch(duk_get_type(ctx, stackPosition))
	{
		case DUK_TYPE_BOOLEAN:
		case DUK_TYPE_NUMBER:
		case DUK_TYPE_STRING:
			res.value = duk_to_string(ctx, stackPosition);
			break;
		case DUK_TYPE_OBJECT:
			if(!duk_is_function(ctx, stackPosition) && !duk_is_null_or_undefined(ctx, stackPosition))
			{
				res.errorCode = duk_safe_call(ctx, safeToJSON, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
				res.value = duk_to_string(ctx, stackPosition);
			}
			break;
		default:
			res.value = "";
			break;
	}
	return res;
}

int callbackHandler(duk_context* ctx) 
{
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "__callbackName");
	
	std::string callbackName = duk_to_string(ctx, -1);
	std::string parameter = "";
	Result res = serializeData(ctx, 0);

	std::string retVal = cbCache.doCallbackToV8(ctx, callbackName, res.value);
	duk_push_string(ctx, retVal.c_str());

	return 1;
}

Result getError(duk_context* ctx, int errorCode)
{
	Result res;
	// Get error string.
	res.value = duk_to_string(ctx, -1);
	res.errorCode = errorCode;
	return res;
}

} // unnamed namespace

namespace duktape {

DuktapeVM::DuktapeVM():
m_ctx(duk_create_heap_default())
{
	cbCache.registerContext(m_ctx);
}

DuktapeVM::~DuktapeVM()
{
	cbCache.unregisterContext(m_ctx);
	duk_destroy_heap(m_ctx);
}

Result DuktapeVM::run(const std::string& scriptName, 
	const std::string& parameter, 
	const std::string& script) 
{
	int rc = 0;

	// 1.Eval script
	duk_push_string(m_ctx, script.c_str());
	rc = duk_safe_call(m_ctx, safeEval, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if (rc != DUK_EXEC_SUCCESS) 
		return getError(m_ctx, rc);

	// 2.Prepare and call function.
	duk_push_global_object(m_ctx);
	duk_get_prop_string(m_ctx, -1, scriptName.c_str());
	duk_push_string(m_ctx, parameter.c_str());	
	rc = duk_safe_call(m_ctx, safeCall, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if(rc != DUK_EXEC_SUCCESS)
		return getError(m_ctx, rc);

	// 3. Serialize output
	Result res = serializeData(m_ctx, -1);

	duk_pop(m_ctx);
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

	cbCache.addCallback(m_ctx, functionName, callback);
}


} // namespace duktape
