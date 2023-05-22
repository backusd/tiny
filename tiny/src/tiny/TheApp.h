#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/rendering/Shader.h"
#include "tiny/rendering/UploadBuffer.h"
#include "tiny/rendering/MeshGeometry.h"


namespace tiny
{
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT4 Color;
	};

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 WorldViewProj = tiny::MathHelper::Identity4x4();
	};

	class TheApp
	{
	public:
		TheApp(std::shared_ptr<DeviceResources> deviceResources);

		void Update();
		void Render();
		void Present() { m_deviceResources->Present(); }

		void OnResize(int height, int width);
		void SetViewport(float top, float left, float height, float width) noexcept;
			

	private:
		void BuildDescriptorHeaps();
		void BuildConstantBuffers();
		void BuildRootSignature();
		void BuildShadersAndInputLayout();
		void BuildBoxGeometry();
		void BuildPSO();

		std::shared_ptr<DeviceResources> m_deviceResources;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

		std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

		std::unique_ptr<tiny::MeshGeometry> m_boxGeo = nullptr;

		std::unique_ptr<Shader> m_vertexShader = nullptr;
		std::unique_ptr<Shader> m_pixelShader = nullptr;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;

		DirectX::XMFLOAT4X4 m_world = tiny::MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();

		float m_theta = 1.5f * DirectX::XM_PI;
		float m_phi = DirectX::XM_PIDIV4;
		float m_radius = 5.0f;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;
	};
}