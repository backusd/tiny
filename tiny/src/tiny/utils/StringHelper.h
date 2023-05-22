#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

namespace tiny
{
    namespace utility
    {
        // String conversions
        ND std::string TINY_API ToString(const std::wstring& str);
        ND std::wstring TINY_API ToWString(const std::string& str);
    }
}