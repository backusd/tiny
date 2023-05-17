#pragma once
#include "tiny-app-pch.h"
#include "tiny-app/Core.h"
#include "tiny-app/input/KeyCode.h"
#include "tiny-app/event/ApplicationEvent.h"
#include "tiny-app/event/KeyEvent.h"
#include "tiny-app/event/MouseEvent.h"
#include "WindowTemplate.h"
#include "WindowProperties.h"

namespace tiny
{
enum class Cursor
{
	ARROW = 0,
	ARROW_AND_HOURGLASS = 1,
	ARROW_AND_QUESTION_MARK = 2,
	CROSS = 3,
	DOUBLE_ARROW_EW = 4,
	DOUBLE_ARROW_NS = 5,
	DOUBLE_ARROW_NESW = 6,
	DOUBLE_ARROW_NWSE = 7,
	HAND = 8,
	HOURGLASS = 9,
	I_BEAM = 10,
	QUAD_ARROW = 11,
	SLASHED_CIRCLE = 12,
	UP_ARROW = 13
};

#pragma warning( push )
#pragma warning( disable : 4251 )
class TINY_APP_API Window : public WindowTemplate<Window>
{
public:
	Window(const WindowProperties& props = WindowProperties()) noexcept;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	virtual ~Window();

	ND std::optional<int> ProcessMessages() const noexcept;
	ND LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept override;

	ND inline unsigned int GetWidth() const noexcept { return m_width; }
	ND inline unsigned int GetHeight() const noexcept { return m_height; }

	// Application Events
	std::function<void(WindowResizeEvent& e)> m_OnWindowResizeFn = [](WindowResizeEvent&) {};
	std::function<void(WindowCreateEvent& e)> m_OnWindowCreateFn = [](WindowCreateEvent&) {};
	std::function<void(WindowCloseEvent& e)> m_OnWindowCloseFn = [](WindowCloseEvent&) {};
	std::function<void(AppTickEvent& e)> m_OnAppTickFn = [](AppTickEvent&) {};
	std::function<void(AppUpdateEvent& e)> m_OnAppUpdateFn = [](AppUpdateEvent&) {};
	std::function<void(AppRenderEvent& e)> m_OnAppRenderFn = [](AppRenderEvent&) {};

	// Key Events
	std::function<void(CharEvent& e)> m_OnCharFn = [](CharEvent&) {};
	std::function<void(KeyPressedEvent& e)> m_OnKeyPressedFn = [](KeyPressedEvent&) {};
	std::function<void(KeyReleasedEvent& e)> m_OnKeyReleasedFn = [](KeyReleasedEvent&) {};

	// Mouse Events
	std::function<void(MouseMoveEvent& e)> m_OnMouseMoveFn = [](MouseMoveEvent&) {};
	std::function<void(MouseEnterEvent& e)> m_OnMouseEnterFn = [](MouseEnterEvent&) {};
	std::function<void(MouseLeaveEvent& e)> m_OnMouseLeaveFn = [](MouseLeaveEvent&) {};
	std::function<void(MouseScrolledEvent& e)> m_OnMouseScrolledVerticalFn = [](MouseScrolledEvent&) {};
	std::function<void(MouseScrolledEvent& e)> m_OnMouseScrolledHorizontalFn = [](MouseScrolledEvent&) {};
	std::function<void(MouseButtonPressedEvent& e)> m_OnMouseButtonPressedFn = [](MouseButtonPressedEvent&) {};
	std::function<void(MouseButtonReleasedEvent& e)> m_OnMouseButtonReleasedFn = [](MouseButtonReleasedEvent&) {};
	std::function<void(MouseButtonDoubleClickEvent& e)> m_OnMouseButtonDoubleClickFn = [](MouseButtonDoubleClickEvent&) {};


private:
	bool m_mouseIsInWindow;
	float m_mouseX;
	float m_mouseY;

	virtual void Init(const WindowProperties& props) noexcept;
	virtual void Shutdown() noexcept;

	inline void BringToForeground() const;

	ND KEY_CODE WParamToKeyCode(WPARAM wParam) const noexcept;

	ND LRESULT OnCreate(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnClose(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnLButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonDoubleClick(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnRButtonUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnResize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ND LRESULT OnMouseMove(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // cannot be const because it modifies m_mouseIsInWindow
	ND LRESULT OnMouseLeave(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMouseWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnMouseHWheel(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnChar(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnSysKeyUp(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnSysKeyDown(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
	ND LRESULT OnKillFocus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) const;

public:
	void CheckCursors() noexcept;
	static void SetCursor(Cursor cursor) noexcept;
private:
	static const std::array<HCURSOR, 14> m_cursors;
};
#pragma warning( pop )
}

template <>
struct std::formatter<tiny::Cursor> : std::formatter<std::string> {
auto format(tiny::Cursor cursor, std::format_context& ctx) {
	std::string s = "";
	switch (cursor)
	{
	case tiny::Cursor::ARROW:					s = "Cursor::ARROW"; break;
	case tiny::Cursor::ARROW_AND_HOURGLASS:		s = "Cursor::ARROW_AND_HOURGLASS"; break;
	case tiny::Cursor::ARROW_AND_QUESTION_MARK:	s = "Cursor::ARROW_AND_QUESTION_MARK"; break;
	case tiny::Cursor::CROSS:					s = "Cursor::CROSS"; break;
	case tiny::Cursor::DOUBLE_ARROW_EW:			s = "Cursor::DOUBLE_ARROW_EW"; break;
	case tiny::Cursor::DOUBLE_ARROW_NS:			s = "Cursor::DOUBLE_ARROW_NS"; break;
	case tiny::Cursor::DOUBLE_ARROW_NESW:		s = "Cursor::DOUBLE_ARROW_NESW"; break;
	case tiny::Cursor::DOUBLE_ARROW_NWSE:		s = "Cursor::DOUBLE_ARROW_NWSE"; break;
	case tiny::Cursor::HAND:					s = "Cursor::HAND"; break;
	case tiny::Cursor::HOURGLASS:				s = "Cursor::HOURGLASS"; break;
	case tiny::Cursor::I_BEAM:					s = "Cursor::I_BEAM"; break;
	case tiny::Cursor::QUAD_ARROW:				s = "Cursor::QUAD_ARROW"; break;
	case tiny::Cursor::SLASHED_CIRCLE:			s = "Cursor::SLASHED_CIRCLE"; break;
	case tiny::Cursor::UP_ARROW:				s = "Cursor::UP_ARROW"; break;
	default:
		s = "Unrecognized Cursor Type";
		break;
	}
	return formatter<std::string>::format(s, ctx);
}
};