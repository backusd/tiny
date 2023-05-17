#pragma once
#include "pch.h"
#include "Core.h"
#include "Log.h"
#include "tiny-app/window/Window.h"
#include "tiny-app/event/MouseEvent.h"
#include "tiny-app/event/KeyEvent.h"
#include "tiny-app/event/ApplicationEvent.h"

namespace tiny
{
class IApplication
{
public:
	IApplication() = default;
	IApplication(const IApplication&) = delete;
	IApplication& operator=(const IApplication&) = delete;
	virtual ~IApplication() {};

	virtual int Run() = 0;
};

template<class Derived>
class Application : public IApplication
{
public:
	Application() noexcept
	{
		m_window = std::make_unique<Window>();
		TINY_ASSERT(m_window != nullptr, "Failed to create Window");

		// Application Events
		m_window->m_OnWindowResizeFn = [this](WindowResizeEvent& e) { static_cast<Derived*>(this)->OnWindowResize(e); };
		m_window->m_OnWindowCreateFn = [this](WindowCreateEvent& e) { static_cast<Derived*>(this)->OnWindowCreate(e); };
		m_window->m_OnWindowCloseFn = [this](WindowCloseEvent& e) { static_cast<Derived*>(this)->OnWindowClose(e); };
		m_window->m_OnAppTickFn = [this](AppTickEvent& e) { static_cast<Derived*>(this)->OnAppTick(e); };
		m_window->m_OnAppUpdateFn = [this](AppUpdateEvent& e) { static_cast<Derived*>(this)->OnAppUpdate(e); };
		m_window->m_OnAppRenderFn = [this](AppRenderEvent& e) { static_cast<Derived*>(this)->OnAppRender(e); };
		
		// Key Events
		m_window->m_OnCharFn = [this](CharEvent& e) { static_cast<Derived*>(this)->OnChar(e); };
		m_window->m_OnKeyPressedFn = [this](KeyPressedEvent& e) { static_cast<Derived*>(this)->OnKeyPressed(e); };
		m_window->m_OnKeyReleasedFn = [this](KeyReleasedEvent& e) { static_cast<Derived*>(this)->OnKeyReleased(e); };
		
		// Mouse Events
		m_window->m_OnMouseMoveFn = [this](MouseMoveEvent& e) { static_cast<Derived*>(this)->OnMouseMove(e); };
		m_window->m_OnMouseEnterFn = [this](MouseEnterEvent& e) { static_cast<Derived*>(this)->OnMouseEnter(e); };
		m_window->m_OnMouseLeaveFn = [this](MouseLeaveEvent& e) { static_cast<Derived*>(this)->OnMouseLeave(e); };
		m_window->m_OnMouseScrolledVerticalFn = [this](MouseScrolledEvent& e) { static_cast<Derived*>(this)->OnMouseScrolledVertical(e); };
		m_window->m_OnMouseScrolledHorizontalFn = [this](MouseScrolledEvent& e) { static_cast<Derived*>(this)->OnMouseScrolledHorizontal(e); };
		m_window->m_OnMouseButtonPressedFn = [this](MouseButtonPressedEvent& e) { static_cast<Derived*>(this)->OnMouseButtonPressed(e); };
		m_window->m_OnMouseButtonReleasedFn = [this](MouseButtonReleasedEvent& e) { static_cast<Derived*>(this)->OnMouseButtonReleased(e); };
		m_window->m_OnMouseButtonDoubleClickFn = [this](MouseButtonDoubleClickEvent& e) { static_cast<Derived*>(this)->OnMouseButtonDoubleClick(e); };
	}
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	virtual ~Application() override {};

	virtual int Run() override
	{
		Derived* derived = static_cast<Derived*>(this);

		while (true)
		{
			// process all messages pending, but to not block for new messages
			if (const auto ecode = m_window->ProcessMessages())
			{
				// if return optional has value, means we're quitting so return exit code
				return *ecode;
			}

			derived->DoFrame();
		}
	}

private:
	std::unique_ptr<Window> m_window;
};

// To be defined in CLIENT
IApplication* CreateApplication();
}