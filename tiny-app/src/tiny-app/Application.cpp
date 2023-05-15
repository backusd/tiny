#include "pch.h"
#include "Application.h"


namespace tiny
{
Application::Application()
{
	m_window = std::make_unique<Window>();
	TINY_ASSERT(m_window != nullptr, "Failed to create Window");

	// Application Events
	m_window->m_OnWindowResizeFn = [this](WindowResizeEvent& e) { this->OnWindowResize(e); };
	m_window->m_OnWindowCreateFn = [this](WindowCreateEvent& e) { this->OnWindowCreate(e); };
	m_window->m_OnWindowCloseFn = [this](WindowCloseEvent& e) { this->OnWindowClose(e); };
	m_window->m_OnAppTickFn = [this](AppTickEvent& e) { this->OnAppTick(e); };
	m_window->m_OnAppUpdateFn = [this](AppUpdateEvent& e) { this->OnAppUpdate(e); };
	m_window->m_OnAppRenderFn = [this](AppRenderEvent& e) { this->OnAppRender(e); };

	// Key Events
	m_window->m_OnCharFn = [this](CharEvent& e) { this->OnChar(e); };
	m_window->m_OnKeyPressedFn = [this](KeyPressedEvent& e) { this->OnKeyPressed(e); };
	m_window->m_OnKeyReleasedFn = [this](KeyReleasedEvent& e) { this->OnKeyReleased(e); };

	// Mouse Events
	m_window->m_OnMouseMoveFn = [this](MouseMoveEvent& e) { this->OnMouseMove(e); };
	m_window->m_OnMouseEnterFn = [this](MouseEnterEvent& e) { this->OnMouseEnter(e); };
	m_window->m_OnMouseLeaveFn = [this](MouseLeaveEvent& e) { this->OnMouseLeave(e); };
	m_window->m_OnMouseScrolledVerticalFn = [this](MouseScrolledEvent& e) { this->OnMouseScrolledVertical(e); };
	m_window->m_OnMouseScrolledHorizontalFn = [this](MouseScrolledEvent& e) { this->OnMouseScrolledHorizontal(e); };
	m_window->m_OnMouseButtonPressedFn = [this](MouseButtonPressedEvent& e) { this->OnMouseButtonPressed(e); };
	m_window->m_OnMouseButtonReleasedFn = [this](MouseButtonReleasedEvent& e) { this->OnMouseButtonReleased(e); };
	m_window->m_OnMouseButtonDoubleClickFn = [this](MouseButtonDoubleClickEvent& e) { this->OnMouseButtonDoubleClick(e); };
}

int Application::Run()
{
	while (true)
	{
		// process all messages pending, but to not block for new messages
		if (const auto ecode = m_window->ProcessMessages())
		{
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}

		Update();
		Render();
		Present();
	}
}
}