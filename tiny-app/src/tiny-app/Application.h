#pragma once
#include "pch.h"
#include "Core.h"
#include "tiny-app/window/Window.h"
#include "tiny-app/event/MouseEvent.h"
#include "tiny-app/event/KeyEvent.h"
#include "tiny-app/event/ApplicationEvent.h"


namespace tiny
{
// Drop this warning because the private members are not accessible by the client application, but 
// the compiler will complain that they don't have a DLL interface
// See: https://stackoverflow.com/questions/767579/exporting-classes-containing-std-objects-vector-map-etc-from-a-dll
#pragma warning( push )
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class
class TINY_APP_API Application
{
public:
	Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	virtual ~Application() {};

	int Run();

protected:
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
	std::unique_ptr<Window> m_window;
};
#pragma warning( pop )

// To be defined in CLIENT
Application* CreateApplication();
}