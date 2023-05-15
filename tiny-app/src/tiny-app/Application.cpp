#include "pch.h"
#include "Application.h"


namespace tiny
{
Application::Application()
{
	m_window = std::make_unique<Window>();
	TINY_ASSERT(m_window != nullptr, "Failed to create Window");

	// Application Events
	m_window->m_OnWindowResizeFn = [this](WindowResizeEvent& e) { this->m_OnWindowResizeFn(e); };
	m_window->m_OnWindowCreateFn = [this](WindowCreateEvent& e) { this->m_OnWindowCreateFn(e); };
	m_window->m_OnWindowCloseFn = [this](WindowCloseEvent& e) { this->m_OnWindowCloseFn(e); };
	m_window->m_OnAppTickFn = [this](AppTickEvent& e) { this->m_OnAppTickFn(e); };
	m_window->m_OnAppUpdateFn = [this](AppUpdateEvent& e) { this->m_OnAppUpdateFn(e); };
	m_window->m_OnAppRenderFn = [this](AppRenderEvent& e) { this->m_OnAppRenderFn(e); };

	// Key Events
	m_window->m_OnCharFn = [this](CharEvent& e) { this->m_OnCharFn(e); };
	m_window->m_OnKeyPressedFn = [this](KeyPressedEvent& e) { this->m_OnKeyPressedFn(e); };
	m_window->m_OnKeyReleasedFn = [this](KeyReleasedEvent& e) { this->m_OnKeyReleasedFn(e); };

	// Mouse Events
	m_window->m_OnMouseMoveFn = [this](MouseMoveEvent& e) { this->m_OnMouseMoveFn(e); };
	m_window->m_OnMouseEnterFn = [this](MouseEnterEvent& e) { this->m_OnMouseEnterFn(e); };
	m_window->m_OnMouseLeaveFn = [this](MouseLeaveEvent& e) { this->m_OnMouseLeaveFn(e); };
	m_window->m_OnMouseScrolledVerticalFn = [this](MouseScrolledEvent& e) { this->m_OnMouseScrolledVerticalFn(e); };
	m_window->m_OnMouseScrolledHorizontalFn = [this](MouseScrolledEvent& e) { this->m_OnMouseScrolledHorizontalFn(e); };
	m_window->m_OnMouseButtonPressedFn = [this](MouseButtonPressedEvent& e) { this->m_OnMouseButtonPressedFn(e); };
	m_window->m_OnMouseButtonReleasedFn = [this](MouseButtonReleasedEvent& e) { this->m_OnMouseButtonReleasedFn(e); };
	m_window->m_OnMouseButtonDoubleClickFn = [this](MouseButtonDoubleClickEvent& e) { this->m_OnMouseButtonDoubleClickFn(e); };
}

int Application::Run()
{
	LOG_CORE_ERROR("{}", "error");
	LOG_CORE_WARN("{}", "warn");
	LOG_CORE_INFO("{}", "info");
	LOG_CORE_TRACE("{}", "trace");


	while (true)
	{
		// process all messages pending, but to not block for new messages
		if (const auto ecode = m_window->ProcessMessages())
		{
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}
	}
}
}