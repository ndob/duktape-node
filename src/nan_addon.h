#pragma once
// Additional compability helpers for functionalites, 
// that nan-package doesn't cover.

#include <nan.h>

namespace nan_addon {

// Copyable traits have to be used, because PersistentCallback 
// is eventually wrapped inside std::function, 
// which needs to be CopyConstructible.
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
typedef Nan::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> PersistentCallback;
#else
typedef Nan::Persistent<v8::Function, Nan::CopyablePersistentTraits<v8::Function>> PersistentCallback;
#endif

// libuv API for uv_async_cb has changed between node 0.10 and 0.12.
#if (NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION)
#define NAN_ADDON_UV_ASYNC_CB(funcName) void funcName(uv_async_t* handle)
#else
#define NAN_ADDON_UV_ASYNC_CB(funcName) void funcName(uv_async_t* handle, int status)
#endif

}