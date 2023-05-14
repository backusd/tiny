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