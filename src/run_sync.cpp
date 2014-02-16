#include "run_sync.h"

#include "callback.h"
#include "duktapevm.h"

namespace duktape {

using namespace v8;

Handle<Value> runSync(const Arguments& args) 
{
	HandleScope scope;
	if(args.Length() < 3) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) 
	{
		ThrowException(Exception::TypeError(String::New("Wrong arguments")));
		return scope.Close(Undefined());
	}

	duktape::DuktapeVM vm;

	if(args[3]->IsObject())
	{
		auto object = Handle<Object>::Cast(args[3]);
		auto properties = object->GetPropertyNames();

		const auto len = properties->Length();
		for(unsigned int i = 0; i < len; ++i)
		{
			const Local<Value> key = properties->Get(i);
			const Local<Value> value = object->Get(key);
			if(!key->IsString() || !value->IsFunction())
			{
				ThrowException(Exception::Error(String::New("Error in API-definition")));
				return scope.Close(Undefined());
			}

			Local<Function> apiCallbackFunc = Local<Function>::Cast(value);

			auto duktapeToNodeBridge = duktape::Callback([apiCallbackFunc] (const std::string& paramString) 
			{
				Handle<Value> argv[1];
				argv[0] = String::New(paramString.c_str());

				auto retVal = apiCallbackFunc->Call(Context::GetCurrent()->Global(), 1, argv);

				String::Utf8Value retString(retVal);

				return std::string(*retString);
			});

			String::Utf8Value keyStr(key);
			vm.registerCallback(std::string(*keyStr), duktapeToNodeBridge);
		}		
	}

 	String::Utf8Value functionName(args[0]->ToString());
	String::Utf8Value parameters(args[1]->ToString());
	String::Utf8Value script(args[2]->ToString());

	auto ret = vm.run(std::string(*functionName), std::string(*parameters), std::string(*script));

	if(ret.errorCode != 0)
	{
		ThrowException(Exception::Error(String::New(ret.value.c_str())));
		return scope.Close(Undefined());
	}

	return scope.Close(String::New(ret.value.c_str()));
}

} // namespace duktape

