#pragma once

#include <v8.h>

namespace duktape {

v8::Handle<v8::Value> run(const v8::Arguments& args);

} // namespace duktape
