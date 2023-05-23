#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/utils/Timer.h"

#include "tiny/rendering/BlendState.h"
#include "tiny/rendering/DepthStencilState.h"
#include "tiny/rendering/InputLayout.h"
#include "tiny/rendering/Shader.h"
#include "tiny/rendering/UploadBuffer.h"
#include "tiny/rendering/MeshGeometry.h"
#include "tiny/rendering/RasterizerState.h"
#include "tiny/rendering/FrameResource.h"
#include "tiny/rendering/RenderItem.h"
#include "tiny/rendering/GeometryGenerator.h"

namespace tiny
{
	class TheApp
	{
	public:
		TheApp(std::shared_ptr<DeviceResources> deviceResources);

		void Update(const Timer& timer);
		void Render();
		void Present();

		void OnResize(int height, int width);
		void SetViewport(float top, float left, float height, float width) noexcept;
			

	private:
		std::shared_ptr<DeviceResources> m_deviceResources;

		void BuildDescriptorHeaps();
		void BuildConstantBufferViews();
		void BuildRootSignature();
		void BuildShadersAndInputLayout();
		void BuildShapeGeometry();
		void BuildPSOs();
		void BuildFrameResources();
		void BuildRenderItems();
		void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& ritems);

		void UpdateCamera(const Timer& timer);
		void UpdateObjectCBs(const Timer& timer);
		void UpdateMainPassCB(const Timer& timer);

		std::vector<std::unique_ptr<FrameResource>> m_frameResources;
		FrameResource* m_currFrameResource = nullptr;
		int m_currFrameResourceIndex = 0;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap = nullptr;

		std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_psos;

		//std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;
		std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;

		std::unique_ptr<InputLayout> m_inputLayout = nullptr;

		// List of all the render items.
		std::vector<std::unique_ptr<RenderItem>> m_allRitems;

		// Render items divided by PSO.
		std::vector<RenderItem*> m_opaqueRitems;

		std::unique_ptr<RasterizerState> m_rasterizerState = nullptr;
		std::unique_ptr<BlendState> m_blendState = nullptr;
		std::unique_ptr<DepthStencilState> m_depthStencilState = nullptr;

		PassConstants m_mainPassCB;

		UINT m_passCbvOffset = 0;

		bool m_isWireframe = true;

		DirectX::XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();


		float m_theta = 1.5f * DirectX::XM_PI;
		float m_phi = DirectX::XM_PIDIV4;
		float m_radius = 15.0f;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;

		// --------------------------------------------------------------------



		//std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

		//std::unique_ptr<tiny::MeshGeometry> m_boxGeo = nullptr;

		//std::unique_ptr<Shader> m_vertexShader = nullptr;
		//std::unique_ptr<Shader> m_pixelShader = nullptr;

		//std::unique_ptr<InputLayout> m_inputLayout = nullptr;

		//icrosoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;



		//DirectX::XMFLOAT4X4 m_world = tiny::MathHelper::Identity4x4();



	};
}