#include "duktapevm.h"

#include <duktape.h>

namespace {	

struct ScopedDuktapeContext
{
	ScopedDuktapeContext():
	m_ctx(duk_create_heap_default())
	{
	}

	~ScopedDuktapeContext()
	{
		duk_destroy_heap(m_ctx);
	}

	duk_context* get()
	{
		return m_ctx;
	}

private:
	duk_context* m_ctx;
};

int wrappedEval(duk_context* ctx)
{
	duk_eval(ctx);
	return 0;
}

int wrappedCall(duk_context* ctx)
{
	duk_call(ctx, 2 /* number of params */);
	return 1;
}

} // unnamed namespace


namespace duktape {

Result runInVM(std::string scriptName, std::string parameter, std::string script) 
{
	Result res;
	int rc = 0;
	ScopedDuktapeContext context;

	auto ctx = context.get();

	script += "\nfunction __wrap(functionName, parameter) { return JSON.stringify(this[functionName](parameter)); }";

	// Eval script
	duk_push_string(ctx, script.c_str());
	rc = duk_safe_call(ctx, wrappedEval, 1 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if (rc != DUK_EXEC_SUCCESS) 
	{
		res.value = duk_to_string(ctx, -1);
		res.errorCode = rc;
		return res;
	}		

	// Prepare and call wrapper-function.
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "__wrap");
	duk_push_string(ctx, scriptName.c_str());
	duk_push_string(ctx, parameter.c_str());	
	rc = duk_safe_call(ctx, wrappedCall, 2 /* number of params */, 1 /* number of return values */, DUK_INVALID_INDEX);
	if(rc != 0)
	{
		res.errorCode = rc;
	}

	res.value = duk_to_string(ctx, -1);
	duk_pop(ctx);
	return res;
}

} // namespace duktape
