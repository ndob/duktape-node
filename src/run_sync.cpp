#include "run_sync.h"

#include "callback.h"
#include "duktapevm.h"
	
using namespace v8;

namespace {

struct CallbackHelper
{
	CallbackHelper(Local<Function> apiCallbackFunc):
	m_apiCallbackFunc(apiCallbackFunc)
	{
	}

	std::string operator()(const std::string& paramString)
	{
		Handle<Value> argv[1];
		argv[0] = Nan::New(paramString).ToLocalChecked();

		auto retVal = m_apiCallbackFunc->Call(Nan::GetCurrentContext()->Global(), 1, argv);

		String::Utf8Value retString(retVal);

		return std::string(*retString);
	}

private:
	Local<Function> m_apiCallbackFunc;
};

}

namespace duktape {

NAN_METHOD(runSync)
{	
	if(info.Length() < 3) 
	{
		Nan::ThrowTypeError(Nan::New("Wrong number of arguments").ToLocalChecked());
		return;
	}

	if (!info[0]->IsString() || !info[1]->IsString() || !info[2]->IsString()) 
	{
		Nan::ThrowTypeError(Nan::New("Wrong arguments").ToLocalChecked());
		return;
	}

	duktape::DuktapeVM vm;

	if(info[3]->IsObject())
	{
		auto object = Handle<Object>::Cast(info[3]);
		auto properties = object->GetPropertyNames();

		const auto len = properties->Length();
		for(unsigned int i = 0; i < len; ++i)
		{
			const Local<Value> key = properties->Get(i);
			const Local<Value> value = object->Get(key);
			if(!key->IsString() || !value->IsFunction())
			{
				Nan::ThrowError(Nan::New("Error in API-definition").ToLocalChecked());
				return;
			}

			String::Utf8Value keyStr(key);
			CallbackHelper duktapeToNodeBridge(Local<Function>::Cast(value));
			vm.registerCallback(std::string(*keyStr), duktapeToNodeBridge);
		}
	}

 	String::Utf8Value functionName(info[0]);
	String::Utf8Value parameters(info[1]);
	String::Utf8Value script(info[2]);

	auto ret = vm.run(std::string(*functionName), std::string(*parameters), std::string(*script));

	if(ret.errorCode != 0)
	{
		Nan::ThrowError(Nan::New(ret.value).ToLocalChecked());
		return;
	}

	info.GetReturnValue().Set(Nan::New(ret.value).ToLocalChecked());
}

} // namespace duktape
