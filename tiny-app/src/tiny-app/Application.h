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
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Present() = 0;

	// Application Events
	virtual void OnWindowResize(WindowResizeEvent& e) = 0;
	virtual void OnWindowCreate(WindowCreateEvent& e) = 0;
	virtual void OnWindowClose(WindowCloseEvent& e) = 0;
	virtual void OnAppTick(AppTickEvent& e) = 0;
	virtual void OnAppUpdate(AppUpdateEvent& e) = 0;
	virtual void OnAppRender(AppRenderEvent& e) = 0;

	// Key Events
	virtual void OnChar(CharEvent& e) = 0;
	virtual void OnKeyPressed(KeyPressedEvent& e) = 0;
	virtual void OnKeyReleased(KeyReleasedEvent& e) = 0;

	// Mouse Events
	virtual void OnMouseMove(MouseMoveEvent& e) = 0;
	virtual void OnMouseEnter(MouseEnterEvent& e) = 0;
	virtual void OnMouseLeave(MouseLeaveEvent& e) = 0;
	virtual void OnMouseScrolledVertical(MouseScrolledEvent& e) = 0;
	virtual void OnMouseScrolledHorizontal(MouseScrolledEvent& e) = 0;
	virtual void OnMouseButtonPressed(MouseButtonPressedEvent& e) = 0;
	virtual void OnMouseButtonReleased(MouseButtonReleasedEvent& e) = 0;
	virtual void OnMouseButtonDoubleClick(MouseButtonDoubleClickEvent& e) = 0;

private:
	std::unique_ptr<Window> m_window;
};
#pragma warning( pop )

// To be defined in CLIENT
Application* CreateApplication();
}