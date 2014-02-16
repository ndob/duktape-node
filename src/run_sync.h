#pragma once

#include <v8.h>

namespace duktape {

v8::Handle<v8::Value> runSync(const v8::Arguments& args);

}