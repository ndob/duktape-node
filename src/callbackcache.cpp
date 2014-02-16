#include "callbackcache.h"

namespace duktape
{

CallbackCache::CallbackCache()
{
}

CallbackCache::~CallbackCache()
{
}

void CallbackCache::registerContext(duk_context* ctx)
{
	m_callbackCache[ctx];
}

void CallbackCache::unregisterContext(duk_context* ctx)
{
	auto context = m_callbackCache.find(ctx);
	if(context != m_callbackCache.end())
	{
		m_callbackCache.erase(context);
	}
}

void CallbackCache::addCallback(duk_context* ctx, const std::string& functionName, duktape::Callback callbackFunc)
{
	auto context = m_callbackCache.find(ctx);
	if(context != m_callbackCache.end())
	{
		auto callback = context->second.find(functionName);
		if(callback == context->second.end())
		{
			context->second[functionName] = callbackFunc;
		}
	}
}

std::string CallbackCache::doCallbackToV8(duk_context* ctx, const std::string& callbackName, const std::string& parameter)
{
	auto context = m_callbackCache.find(ctx);
	if(context != m_callbackCache.end())
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
