#pragma once

#include "callback.h"

#include <duktape.h>

#include <string>
#include <map>

namespace duktape {

class CallbackCache
{
public:
	CallbackCache();
	~CallbackCache();
	void registerContext(duk_context* ctx);
	void unregisterContext(duk_context* ctx);
	void addCallback(duk_context* ctx, const std::string& functionName, duktape::Callback callbackFunc);
	std::string doCallbackToV8(duk_context* ctx, const std::string& callbackName, const std::string& parameter);
private:
	/* 	
	Key: pointer to context
	Value: Map of function name and callback			
	*/
	typedef std::map<void*, std::map<std::string, duktape::Callback> > CallbackContainer;
	CallbackContainer m_callbackCache;
};

} // namespace duktape

