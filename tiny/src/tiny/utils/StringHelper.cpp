#include "tiny-pch.h"
#include "StringHelper.h"

namespace tiny
{
    namespace utility
    {
        // String conversions
        std::string ToString(const std::wstring& w_str)
        {
            std::string str;
            std::transform(w_str.begin(), w_str.end(), std::back_inserter(str), [](wchar_t c) {
                return (char)c;
                });
            return str;
        }
        std::wstring ToWString(const std::string& str)
        {
            std::wstring w_str(str.begin(), str.end());
            return w_str;
        }
    }
}