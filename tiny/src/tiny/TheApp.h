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
	std::shared_ptr<DeviceResources> m_deviceResources;

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& ritems);

	void UpdateCamera(const Timer& timer);
	void UpdateObjectCBs(const Timer& timer);
	void UpdateMaterialCBs(const Timer& timer);
	void UpdateMainPassCB(const Timer& timer);
	void UpdateWaves(const Timer& timer);
	void AnimateMaterials(const Timer& timer);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	float GetHillsHeight(float x, float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z) const;


	std::vector<std::unique_ptr<FrameResource>> m_frameResources;
	FrameResource* m_currFrameResource = nullptr;
	int m_currFrameResourceIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

	//DescriptorVector m_srvDescriptors;
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;
	//std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_psos;
	std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;

	// grass, water, fence
	std::array<std::unique_ptr<Texture>, 3> m_textures;


	std::unique_ptr<InputLayout> m_inputLayout = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> m_allRitems;
	RenderItem* m_wavesRitem = nullptr;

	// Render items divided by PSO.
	std::vector<RenderItem*> m_renderItemLayer[(int)RenderLayer::Count];
	std::unique_ptr<Waves> m_waves;

	// States
	std::unique_ptr<RasterizerState> m_rasterizerState = nullptr;
	std::unique_ptr<BlendState> m_blendState = nullptr;
	std::unique_ptr<DepthStencilState> m_depthStencilState = nullptr;

	PassConstants m_mainPassCB;

	UINT m_passCbvOffset = 0;

	DirectX::XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 m_view = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = tiny::MathHelper::Identity4x4();

	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 55.0f;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};
}