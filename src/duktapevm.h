#pragma once

#include <duktape.h>

#include <string>
#include <functional>

namespace duktape 
{

struct Result
{
	Result(): errorCode(0) {}
	int errorCode;
	std::string value;
};

typedef std::function<std::string(const std::string&)> Callback;

class DuktapeVM
{
public:
	DuktapeVM();
	~DuktapeVM();

	Result run(const std::string& scriptName, const std::string& parameter, const std::string& script);
	void registerCallback(const std::string& functionName, Callback callback);

private:
	duk_context* m_ctx;
};

}  // namespace duktape
