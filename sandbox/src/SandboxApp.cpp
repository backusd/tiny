#include <tiny-app.h>
#include <tiny-app/EntryPoint.h>
#include <tiny.h>

class Sandbox : public tiny::Application
{
public:
	Sandbox()
	{
	}
	Sandbox(const Sandbox&) = delete;
	Sandbox& operator=(const Sandbox&) = delete;
	virtual ~Sandbox() noexcept override {}

protected:
	void Update() override {}
	void Render() override {}
	void Present() override {}

	// Application Events
	void OnWindowResize(tiny::WindowResizeEvent& e) override {}
	void OnWindowCreate(tiny::WindowCreateEvent& e) override {}
	void OnWindowClose(tiny::WindowCloseEvent& e) override {}
	void OnAppTick(tiny::AppTickEvent& e) override {}
	void OnAppUpdate(tiny::AppUpdateEvent& e) override {}
	void OnAppRender(tiny::AppRenderEvent& e) override {}

	// Key Events
	void OnChar(tiny::CharEvent& e) override {}
	void OnKeyPressed(tiny::KeyPressedEvent& e) override {}
	void OnKeyReleased(tiny::KeyReleasedEvent& e) override {}

	// Mouse Events
	void OnMouseMove(tiny::MouseMoveEvent& e) override {}
	void OnMouseEnter(tiny::MouseEnterEvent& e) override {}
	void OnMouseLeave(tiny::MouseLeaveEvent& e) override {}
	void OnMouseScrolledVertical(tiny::MouseScrolledEvent& e) override {}
	void OnMouseScrolledHorizontal(tiny::MouseScrolledEvent& e) override {}
	void OnMouseButtonPressed(tiny::MouseButtonPressedEvent& e) override {}
	void OnMouseButtonReleased(tiny::MouseButtonReleasedEvent& e) override {}
	void OnMouseButtonDoubleClick(tiny::MouseButtonDoubleClickEvent& e) override {}

};

tiny::Application* tiny::CreateApplication()
{
	return new Sandbox();
}