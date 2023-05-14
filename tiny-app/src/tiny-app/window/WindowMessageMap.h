#pragma once
#include "pch.h"
#include "tiny-app/Core.h"

namespace tiny
{
// Drop this warning because the private members are not accessible by the client application, but 
// the compiler will complain that they don't have a DLL interface
// See: https://stackoverflow.com/questions/767579/exporting-classes-containing-std-objects-vector-map-etc-from-a-dll
#pragma warning( push )
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class
class TINY_APP_API WindowMessageMap
{
public:
	WindowMessageMap() noexcept;
	WindowMessageMap(const WindowMessageMap&) = delete;
	WindowMessageMap& operator=(const WindowMessageMap&) = delete;

	ND inline std::string operator()(DWORD msg, LPARAM lParam, WPARAM wParam) const noexcept;

private:
	std::unordered_map<DWORD, std::string> map;
};
#pragma warning( pop )
}