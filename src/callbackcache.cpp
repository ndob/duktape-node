#include "callbackcache.h"

#include <iostream>

namespace duktape
{

CallbackCache::CallbackCache()
{
	std::cout << "init" << std::endl;
}

CallbackCache::~CallbackCache()
{
	std::cout << "deinit" << std::endl;
}

void CallbackCache::registerContext(duk_context* ctx)
{
	s_callbackCache[ctx];
}

void CallbackCache::unregisterContext(duk_context* ctx)
{
	auto context = s_callbackCache.find(ctx);
	if(context != s_callbackCache.end())
	{
		s_callbackCache.erase(context);
	}
}

void CallbackCache::addCallback(duk_context* ctx, const std::string& functionName, duktape::Callback callbackFunc)
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

std::string CallbackCache::doCallback(duk_context* ctx, const std::string& callbackName, const std::string& parameter)
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

} // namespace duktape
