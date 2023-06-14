#pragma once
#include <tiny-app.h>
#include <tiny.h>


namespace sandbox
{
class Sandbox : public tiny::Application<Sandbox>
{
public:
	Sandbox();
	Sandbox(const Sandbox&) = delete;
	Sandbox& operator=(const Sandbox&) = delete;
	virtual ~Sandbox() noexcept override {}

	// REQUIRED FOR CRTP - DoFrame is called in the Application::Run loop
	bool DoFrame() noexcept;

	// REQUIRED FOR CRTP - Application Events
	void OnWindowResize(tiny::WindowResizeEvent& e);
	void OnWindowCreate(tiny::WindowCreateEvent& e);
	void OnWindowClose(tiny::WindowCloseEvent& e);
	void OnAppTick(tiny::AppTickEvent& e);
	void OnAppUpdate(tiny::AppUpdateEvent& e);
	void OnAppRender(tiny::AppRenderEvent& e);

	// REQUIRED FOR CRTP - Key Events
	void OnChar(tiny::CharEvent& e);
	void OnKeyPressed(tiny::KeyPressedEvent& e);
	void OnKeyReleased(tiny::KeyReleasedEvent& e);

	// REQUIRED FOR CRTP - Mouse Events
	void OnMouseMove(tiny::MouseMoveEvent& e);
	void OnMouseEnter(tiny::MouseEnterEvent& e);
	void OnMouseLeave(tiny::MouseLeaveEvent& e);
	void OnMouseScrolledVertical(tiny::MouseScrolledEvent& e);
	void OnMouseScrolledHorizontal(tiny::MouseScrolledEvent& e);
	void OnMouseButtonPressed(tiny::MouseButtonPressedEvent& e);
	void OnMouseButtonReleased(tiny::MouseButtonReleasedEvent& e);
	void OnMouseButtonDoubleClick(tiny::MouseButtonDoubleClickEvent& e);

private:
	void Update(const tiny::Timer& timer);
	void Render();
	void Present();

	void CalculateFrameStats();


private:
	tiny::Timer m_timer;
	std::shared_ptr<tiny::DeviceResources> m_deviceResources;

	// TEMPORARY: We use TheApp class to implement app-specific stuff in the library itself.
	//            Eventually this should be removed, but it serves as an easy way to run the code on Win32 and UWP right now
	std::unique_ptr<tiny::TheApp> m_app;
};
}