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

typedef std::function<std::string(std::string)> Callback;

class DuktapeVM
{
public:
	DuktapeVM();
	~DuktapeVM();

	Result run(std::string scriptName, std::string parameter, std::string script);
	void registerCallback(std::string functionName, Callback callback);

private:
	duk_context* m_ctx;
};

}  // namespace duktape
