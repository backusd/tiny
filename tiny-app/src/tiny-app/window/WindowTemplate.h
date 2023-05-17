#pragma once
#include "tiny-app-pch.h"
#include "tiny-app/Core.h"
#include "tiny-app/exception/WindowException.h"
#include "WindowProperties.h"

namespace tiny
{
template<typename T>
class WindowTemplate
{
public:
	WindowTemplate(const WindowProperties& props);
	WindowTemplate(const WindowTemplate&) = delete;
	WindowTemplate& operator=(const WindowTemplate&) = delete;
	~WindowTemplate() noexcept;

	ND virtual LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	ND inline HWND GetHWND() const noexcept { return m_hWnd; }


protected:
	ND static LRESULT CALLBACK HandleMsgSetupBase(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	ND static LRESULT CALLBACK HandleMsgBase(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	// Window Class Data
	static constexpr const char* wndBaseClassName = "Tiny Window";
	HINSTANCE m_hInst;

	// Window Data
	unsigned int m_width;
	unsigned int m_height;
	std::string m_title;
	HWND m_hWnd;
};

template<typename T>
WindowTemplate<T>::WindowTemplate(const WindowProperties& props) :
	m_height(props.height),
	m_width(props.width),
	m_title(props.title),
	m_hInst(GetModuleHandle(nullptr)) // I believe GetModuleHandle should not ever throw, even though it is not marked noexcept
{
	// Register the window class
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = HandleMsgSetupBase;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst;
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = wndBaseClassName;
	wc.hIconSm = nullptr;
	RegisterClassEx(&wc);

	// calculate window size based on desired client region size
	RECT rect;
	rect.left = 100;
	rect.right = m_width + rect.left;
	rect.top = 100;
	rect.bottom = m_height + rect.top;

	auto WS_options = WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION | WS_MAXIMIZEBOX | WS_SIZEBOX;

	if (AdjustWindowRect(&rect, WS_options, FALSE) == 0)
	{
		throw WINDOW_LAST_EXCEPT();
	};

	// create window & get hWnd
	m_hWnd = CreateWindow(
		wndBaseClassName,
		m_title.c_str(),
		WS_options,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		m_hInst,
		this
	);

	if (m_hWnd == nullptr)
	{
		throw WINDOW_LAST_EXCEPT();
	}

	// show window
	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
};

template<typename T>
WindowTemplate<T>::~WindowTemplate() noexcept
{
	UnregisterClass(wndBaseClassName, m_hInst);
	DestroyWindow(m_hWnd);
};

template<typename T>
LRESULT CALLBACK WindowTemplate<T>::HandleMsgSetupBase(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
	if (msg == WM_NCCREATE)
	{
		// extract ptr to window class from creation data
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);

		WindowTemplate<T>* const pWnd = static_cast<WindowTemplate<T>*>(pCreate->lpCreateParams);

		// set WinAPI-managed user data to store ptr to window class
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		// set message proc to normal (non-setup) handler now that setup is finished
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowTemplate<T>::HandleMsgBase));
		// forward message to window class handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	// if we get a message before the WM_NCCREATE message, handle with default handler
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

template<typename T>
LRESULT CALLBACK WindowTemplate<T>::HandleMsgBase(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// retrieve ptr to window class
	WindowTemplate<T>* const pWnd = reinterpret_cast<WindowTemplate<T>*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	// forward message to window class handler
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

}