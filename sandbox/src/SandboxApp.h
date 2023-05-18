#pragma once
#include <tiny-app.h>
#include <tiny.h>

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = tiny::MathHelper::Identity4x4();
};




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

	// ------------------------------------------------------
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

	std::unique_ptr<tiny::UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

	std::unique_ptr<tiny::MeshGeometry> m_boxGeo = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;

	DirectX::XMFLOAT4X4 m_world = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();

	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 5.0f;

	POINT m_lastMousePos;
};
}