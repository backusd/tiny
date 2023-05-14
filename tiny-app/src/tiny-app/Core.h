#pragma once

#ifdef TINY_APP_DLL
#define TINY_APP_API __declspec(dllexport)
#else
#define TINY_APP_API __declspec(dllimport)
#endif 

#define ND [[nodiscard]]

#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define BIT(x) (1 << x)

#ifdef _DEBUG
#define TINY_ASSERT(x, ...) { if (!(x)) { LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define TINY_CORE_ASSERT(x, ...) { if (!(x)) { LOG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define TINY_ASSERT(x, ...) 
#define TINY_CORE_ASSERT(x, ...) 
#endif