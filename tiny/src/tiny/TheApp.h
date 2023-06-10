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
#include "tiny/rendering/Texture.h"
#include "tiny/rendering/Light.h"
#include "tiny/rendering/Material.h"
#include "tiny/rendering/DescriptorVector.h"
#include "tiny/rendering/ConstantBuffer.h"
#include "tiny/rendering/MeshGroup.h"
#include "tiny/rendering/RenderPass.h"

#include "tiny/other/Waves.h"

namespace tiny
{
enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

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
	void LoadTextures();
	void BuildMainRenderPass();

	float GetHillsHeight(float x, float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z) const;

	void UpdateWavesVertices(const Timer&);
	void UpdateWavesMaterials(const Timer& timer);

	std::shared_ptr<DeviceResources> m_deviceResources;

	// Textures: grass, water, fence
	std::array<std::unique_ptr<Texture>, 3> m_textures;

	// Render Data
	RenderPass m_mainRenderPass;
	std::unique_ptr<ConstantBufferT<PassConstants>> m_mainRenderPassConstantsCB = nullptr;
	std::unique_ptr<Shader> m_standardVS = nullptr;
	std::unique_ptr<Shader> m_opaquePS = nullptr;
	std::unique_ptr<Shader> m_alphaTestedPS = nullptr;
	std::unique_ptr<InputLayout> m_inputLayout = nullptr;

	// Grid
	std::unique_ptr<ConstantBufferT<ObjectConstants>> m_gridObjectConstantsCB = nullptr;
	std::unique_ptr<ConstantBufferT<Material>> m_gridMaterialCB = nullptr;
	// Box
	std::unique_ptr<ConstantBufferT<ObjectConstants>> m_boxObjectConstantsCB = nullptr;
	std::unique_ptr<ConstantBufferT<Material>> m_boxMaterialCB = nullptr;
	// Waves
	std::unique_ptr<Waves> m_waves;
	std::unique_ptr<ConstantBufferT<ObjectConstants>> m_wavesObjectConstantsCB = nullptr;
	std::unique_ptr<ConstantBufferT<Material>> m_wavesMaterialCB = nullptr;
	DynamicMeshGroupT<Vertex>* m_dynamicWaveMesh = nullptr;
	RenderItem* m_wavesRI = nullptr;


	DirectX::XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();

	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 55.0f;

	std::unique_ptr<RasterizerState> m_rasterizerState = nullptr;
	std::unique_ptr<BlendState> m_blendState = nullptr;
	std::unique_ptr<DepthStencilState> m_depthStencilState = nullptr;




//	void BuildRootSignature();
//	void BuildShadersAndInputLayout();
//	void BuildLandGeometry();
//	void BuildWavesGeometry();
//	void BuildBoxGeometry();
//	void BuildPSOs();
//	void BuildFrameResources();
//	void BuildRenderItems();
//	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& ritems, MeshGroup* meshGroup);

	void UpdateCamera(const Timer& timer);
//	void UpdateObjectCBs(const Timer& timer);
//	void UpdateMaterialCBs(const Timer& timer);
//	void UpdateMainPassCB(const Timer& timer);
//	void UpdateWaves(const Timer& timer);


	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();




//	std::vector<std::unique_ptr<FrameResource>> m_frameResources;
//	FrameResource* m_currFrameResource = nullptr;
//	int m_currFrameResourceIndex = 0;
//
//	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
//
//	// Geometry
//	std::unique_ptr<MeshGroupT<Vertex>> m_landMesh = nullptr;
//	std::unique_ptr<MeshGroupT<Vertex>> m_waterMesh = nullptr;
//	std::unique_ptr<MeshGroupT<Vertex>> m_boxMesh = nullptr;
//
//	//std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
//	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_psos;
//	std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
//
//
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> m_allRitems;
//	RenderItem* m_wavesRitem = nullptr;
//
//	// Render items and mesh groups divided by PSO.
//	std::vector<RenderItem*> m_renderItemLayer[(int)RenderLayer::Count];
//	std::array<MeshGroup*, (int)RenderLayer::Count> m_meshGroups;
//	std::unique_ptr<Waves> m_waves;
//
//	// States
//	std::unique_ptr<RasterizerState> m_rasterizerState = nullptr;
//	std::unique_ptr<BlendState> m_blendState = nullptr;
//	std::unique_ptr<DepthStencilState> m_depthStencilState = nullptr;
//
//	// Pass constants and the constant buffer the data is uploaded to
//	PassConstants m_mainPassCB;
//	std::unique_ptr<ConstantBuffer<PassConstants>> m_passConstantsConstantBuffer = nullptr;
//
//	UINT m_passCbvOffset = 0;
//
//	DirectX::XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
//	DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
//	DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();
//
//	float m_theta = 1.5f * DirectX::XM_PI;
//	float m_phi = DirectX::XM_PIDIV4;
//	float m_radius = 55.0f;
//
//	D3D12_VIEWPORT m_viewport;
//	D3D12_RECT m_scissorRect;
};
}