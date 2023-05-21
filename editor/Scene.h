#pragma once
#include "pch.h"
#include "../tiny/src/tiny.h"
#include "ISceneUIControl.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = tiny::MathHelper::Identity4x4();
};



class Scene
{
public:
    Scene(std::shared_ptr<tiny::DeviceResources> deviceResources, ISceneUIControl* uiControl);
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    ~Scene() {}

    void CreateWindowSizeDependentResources();
    void StartRenderLoop();
    void StopRenderLoop();
    void Suspend();
    void Resume();

    concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

    void WindowActivationChanged(winrt::Windows::UI::Core::CoreWindowActivationState activationState);


    // Rendering Stuff
    inline void SetViewport(float top, float left, float height, float width) 
	{ 
		//m_viewport.TopLeftX = left;
		//m_viewport.TopLeftY = top;
		//m_viewport.Height = height;
		//m_viewport.Width = width;
	}


private:
    bool m_haveFocus;

    std::shared_ptr<tiny::DeviceResources> m_deviceResources;
    ISceneUIControl* m_uiControl;

    concurrency::critical_section            m_criticalSection;
    winrt::Windows::Foundation::IAsyncAction m_renderLoopWorker;

    //==============================================================
//	void BuildDescriptorHeaps();
//	void BuildConstantBuffers();
//	void BuildRootSignature();
//	void BuildShadersAndInputLayout();
//	void BuildBoxGeometry();
//	void BuildPSO();
//
//	winrt::com_ptr<ID3DBlob> CompileShader(
//		const std::wstring& filename,
//		const D3D_SHADER_MACRO* defines,
//		const std::string& entrypoint,
//		const std::string& target);
//
//
//	winrt::com_ptr<ID3D12RootSignature> m_rootSignature = nullptr;
//	winrt::com_ptr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;
//
//	std::unique_ptr<tiny::UploadBuffer<ObjectConstants>> m_objectCB = nullptr;
//
//	std::unique_ptr<tiny::MeshGeometry> m_boxGeo = nullptr;
//
//	winrt::com_ptr<ID3DBlob> m_vsByteCode = nullptr;
//	winrt::com_ptr<ID3DBlob> m_psByteCode = nullptr;
//
//	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
//
//	winrt::com_ptr<ID3D12PipelineState> m_pso = nullptr;
//
//	DirectX::XMFLOAT4X4 m_world = tiny::MathHelper::Identity4x4();
//	DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
//	DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();
//
//	float m_theta = 1.5f * DirectX::XM_PI;
//	float m_phi = DirectX::XM_PIDIV4;
//	float m_radius = 5.0f;
//
//	POINT m_lastMousePos;
//
//	D3D12_VIEWPORT m_viewport;
};