#include <string>

namespace duktape 
{

struct Result
{
	Result(): errorCode(0) {}
	int errorCode;
	std::string value;
};

Result runInVM(std::string scriptName, std::string parameter, std::string script);

}  // namespace duktape
