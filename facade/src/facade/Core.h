#pragma once

#define ND [[nodiscard]]

#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define BIT(x) (1 << x)