#pragma once

#ifdef TINY_DLL
#define TINY_API __declspec(dllexport)
#else
#define TINY_API __declspec(dllimport)
#endif 

#define ND [[nodiscard]]

#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

#ifdef _DEBUG
#define TINY_CORE_ASSERT(x, ...) { if (!(x)) { LOG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define TINY_CORE_ASSERT(x, ...) 
#endif