#include <functional>
#include <string>

namespace duktape {

typedef std::function<std::string(const std::string&)> Callback;
	
}