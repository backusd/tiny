#pragma once
#include "pch.h"

namespace tiny
{
enum class MOUSE_BUTTON
{
	LBUTTON,
	RBUTTON,
	MBUTTON,
	X1BUTTON,
	X2BUTTON
};
}

template <>
struct std::formatter<tiny::MOUSE_BUTTON> : std::formatter<std::string> {
	auto format(tiny::MOUSE_BUTTON mb, std::format_context& ctx) {
		std::string s = "";
		switch (mb)
		{
		case tiny::MOUSE_BUTTON::LBUTTON: s = "LBUTTON"; break;
		case tiny::MOUSE_BUTTON::MBUTTON: s = "MBUTTON"; break;
		case tiny::MOUSE_BUTTON::RBUTTON: s = "RBUTTON"; break;
		case tiny::MOUSE_BUTTON::X1BUTTON: s = "X1BUTTON"; break;
		case tiny::MOUSE_BUTTON::X2BUTTON: s = "X2BUTTON"; break;
		}

		return formatter<std::string>::format(s, ctx);
	}
};