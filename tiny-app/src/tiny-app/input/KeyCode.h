#pragma once
#include "pch.h"

namespace tiny
{
	enum class KEY_CODE
	{
		INVALID,
		LBUTTON,
		MBUTTON,
		RBUTTON,
		CANCEL,
		X1BUTTON,
		X2BUTTON,
		BACKSPACE,
		TAB,
		CLEAR,
		ENTER,
		SHIFT,
		CTRL,
		ALT,
		PAUSE,
		CAPS_LOCK,
		ESC,
		SPACEBAR,
		PAGE_UP,
		PAGE_DOWN,
		END,
		HOME,
		LEFT_ARROW,
		RIGHT_ARROW,
		UP_ARROW,
		DOWN_ARROW,
		SELECT,
		PRINT,
		EXECUTE,
		PRINT_SCREEN,
		INSERT,
		HELP,
		_0,
		_1,
		_2,
		_3,
		_4,
		_5,
		_6,
		_7,
		_8,
		_9,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LEFT_WINDOWS,
		RIGHT_WINDOWS,
		APPLICATIONS,
		SLEEP,
		MULTIPLY,
		ADD,
		SEPARATOR,
		SUBTRACT,
		DECIMAL,
		DIVIDE,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		NUM_LOCK,
		SCROLL_LOCK
	};
}

template <>
struct std::formatter<tiny::KEY_CODE> : std::formatter<std::string> {
	auto format(tiny::KEY_CODE kc, std::format_context& ctx) {
		std::string s = "";
		switch (kc)
		{
		case tiny::KEY_CODE::LBUTTON: s = "LBUTTON"; break;
		case tiny::KEY_CODE::MBUTTON: s = "MBUTTON"; break;
		case tiny::KEY_CODE::RBUTTON: s = "RBUTTON"; break;
		case tiny::KEY_CODE::CANCEL: s = "CANCEL"; break;
		case tiny::KEY_CODE::X1BUTTON: s = "X1BUTTON"; break;
		case tiny::KEY_CODE::X2BUTTON: s = "X2BUTTON"; break;
		case tiny::KEY_CODE::BACKSPACE: s = "BACKSPACE"; break;
		case tiny::KEY_CODE::TAB: s = "TAB"; break;
		case tiny::KEY_CODE::CLEAR: s = "CLEAR"; break;
		case tiny::KEY_CODE::ENTER: s = "ENTER"; break;
		case tiny::KEY_CODE::SHIFT: s = "SHIFT"; break;
		case tiny::KEY_CODE::CTRL: s = "CTRL"; break;
		case tiny::KEY_CODE::ALT: s = "ALT"; break;
		case tiny::KEY_CODE::PAUSE: s = "PAUSE"; break;
		case tiny::KEY_CODE::CAPS_LOCK: s = "CAPS_LOCK"; break;
		case tiny::KEY_CODE::ESC: s = "ESC"; break;
		case tiny::KEY_CODE::SPACEBAR: s = "SPACEBAR"; break;
		case tiny::KEY_CODE::PAGE_UP: s = "PAGE_UP"; break;
		case tiny::KEY_CODE::PAGE_DOWN: s = "PAGE_DOWN"; break;
		case tiny::KEY_CODE::END: s = "END"; break;
		case tiny::KEY_CODE::HOME: s = "HOME"; break;
		case tiny::KEY_CODE::LEFT_ARROW: s = "LEFT_ARROW"; break;
		case tiny::KEY_CODE::RIGHT_ARROW: s = "RIGHT_ARROW"; break;
		case tiny::KEY_CODE::UP_ARROW: s = "UP_ARROW"; break;
		case tiny::KEY_CODE::DOWN_ARROW: s = "DOWN_ARROW"; break;
		case tiny::KEY_CODE::SELECT: s = "SELECT"; break;
		case tiny::KEY_CODE::PRINT: s = "PRINT"; break;
		case tiny::KEY_CODE::EXECUTE: s = "EXECUTE"; break;
		case tiny::KEY_CODE::PRINT_SCREEN: s = "PRINT_SCREEN"; break;
		case tiny::KEY_CODE::INSERT: s = "INSERT"; break;
		case tiny::KEY_CODE::HELP: s = "HELP"; break;
		case tiny::KEY_CODE::_0: s = "0"; break;
		case tiny::KEY_CODE::_1: s = "1"; break;
		case tiny::KEY_CODE::_2: s = "2"; break;
		case tiny::KEY_CODE::_3: s = "3"; break;
		case tiny::KEY_CODE::_4: s = "4"; break;
		case tiny::KEY_CODE::_5: s = "5"; break;
		case tiny::KEY_CODE::_6: s = "6"; break;
		case tiny::KEY_CODE::_7: s = "7"; break;
		case tiny::KEY_CODE::_8: s = "8"; break;
		case tiny::KEY_CODE::_9: s = "9"; break;
		case tiny::KEY_CODE::A: s = "A"; break;
		case tiny::KEY_CODE::B: s = "B"; break;
		case tiny::KEY_CODE::C: s = "C"; break;
		case tiny::KEY_CODE::D: s = "D"; break;
		case tiny::KEY_CODE::E: s = "E"; break;
		case tiny::KEY_CODE::F: s = "F"; break;
		case tiny::KEY_CODE::G: s = "G"; break;
		case tiny::KEY_CODE::H: s = "H"; break;
		case tiny::KEY_CODE::I: s = "I"; break;
		case tiny::KEY_CODE::J: s = "J"; break;
		case tiny::KEY_CODE::K: s = "K"; break;
		case tiny::KEY_CODE::L: s = "L"; break;
		case tiny::KEY_CODE::M: s = "M"; break;
		case tiny::KEY_CODE::N: s = "N"; break;
		case tiny::KEY_CODE::O: s = "O"; break;
		case tiny::KEY_CODE::P: s = "P"; break;
		case tiny::KEY_CODE::Q: s = "Q"; break;
		case tiny::KEY_CODE::R: s = "R"; break;
		case tiny::KEY_CODE::S: s = "S"; break;
		case tiny::KEY_CODE::T: s = "T"; break;
		case tiny::KEY_CODE::U: s = "U"; break;
		case tiny::KEY_CODE::V: s = "V"; break;
		case tiny::KEY_CODE::W: s = "W"; break;
		case tiny::KEY_CODE::X: s = "X"; break;
		case tiny::KEY_CODE::Y: s = "Y"; break;
		case tiny::KEY_CODE::Z: s = "Z"; break;
		case tiny::KEY_CODE::LEFT_WINDOWS: s = "LEFT_WINDOWS"; break;
		case tiny::KEY_CODE::RIGHT_WINDOWS: s = "RIGHT_WINDOWS"; break;
		case tiny::KEY_CODE::APPLICATIONS: s = "APPLICATIONS"; break;
		case tiny::KEY_CODE::SLEEP: s = "SLEEP"; break;
		case tiny::KEY_CODE::MULTIPLY: s = "MULTIPLY"; break;
		case tiny::KEY_CODE::ADD: s = "ADD"; break;
		case tiny::KEY_CODE::SEPARATOR: s = "SEPARATOR"; break;
		case tiny::KEY_CODE::SUBTRACT: s = "SUBTRACT"; break;
		case tiny::KEY_CODE::DECIMAL: s = "DECIMAL"; break;
		case tiny::KEY_CODE::DIVIDE: s = "DIVIDE"; break;
		case tiny::KEY_CODE::F1: s = "F1"; break;
		case tiny::KEY_CODE::F2: s = "F2"; break;
		case tiny::KEY_CODE::F3: s = "F3"; break;
		case tiny::KEY_CODE::F4: s = "F4"; break;
		case tiny::KEY_CODE::F5: s = "F5"; break;
		case tiny::KEY_CODE::F6: s = "F6"; break;
		case tiny::KEY_CODE::F7: s = "F7"; break;
		case tiny::KEY_CODE::F8: s = "F8"; break;
		case tiny::KEY_CODE::F9: s = "F9"; break;
		case tiny::KEY_CODE::F10: s = "F10"; break;
		case tiny::KEY_CODE::F11: s = "F11"; break;
		case tiny::KEY_CODE::F12: s = "F12"; break;
		case tiny::KEY_CODE::F13: s = "F13"; break;
		case tiny::KEY_CODE::F14: s = "F14"; break;
		case tiny::KEY_CODE::F15: s = "F15"; break;
		case tiny::KEY_CODE::F16: s = "F16"; break;
		case tiny::KEY_CODE::F17: s = "F17"; break;
		case tiny::KEY_CODE::F18: s = "F18"; break;
		case tiny::KEY_CODE::F19: s = "F19"; break;
		case tiny::KEY_CODE::F20: s = "F20"; break;
		case tiny::KEY_CODE::F21: s = "F21"; break;
		case tiny::KEY_CODE::F22: s = "F22"; break;
		case tiny::KEY_CODE::F23: s = "F23"; break;
		case tiny::KEY_CODE::F24: s = "F24"; break;
		case tiny::KEY_CODE::NUM_LOCK: s = "NUM_LOCK"; break;
		case tiny::KEY_CODE::SCROLL_LOCK: s = "SCROLL_LOCK"; break;
		case tiny::KEY_CODE::INVALID: s = "Invalid"; break;
		}

		return formatter<std::string>::format(s, ctx);
	}
};