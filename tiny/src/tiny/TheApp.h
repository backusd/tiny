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
#include "tiny/rendering/RasterizerState.h"
#include "tiny/rendering/RenderItem.h"
#include "tiny/rendering/GeometryGenerator.h"
#include "tiny/rendering/Texture.h"
#include "tiny/rendering/Light.h"
#include "tiny/rendering/Material.h"
#include "tiny/rendering/DescriptorVector.h"
#include "tiny/rendering/ConstantBuffer.h"
#include "tiny/rendering/MeshGroup.h"
#include "tiny/rendering/RenderPass.h"

#include "tiny/scene/Camera.h"

#include "tiny/other/Waves.h"

// These methods are required because they are listed as "extern" in tiny Texture.h
std::wstring GetTextureFilename(unsigned int index);
std::size_t GetTotalTextureCount();

// This enum class is not required, but serves as a good helper so that we can easily reference textures by int
enum class TEXTURE : int
{
	GRASS = 0,
	WATER1,
	WIRE_FENCE,
	BRICKS,
	BRICKS2,
	BRICKS3,
	CHECKBOARD,
	ICE,
	STONE,
	TILE,
	WHITE1X1,
	Count
};

namespace tiny
{
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	DirectX::XMFLOAT2 cbPerObjectPad2 = { 0.0f, 0.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

class TheApp
{
public:
	TheApp(std::shared_ptr<DeviceResources> deviceResources);

	void Update(const Timer& timer);
	void Render() { Engine::Render(); }
	void Present() { Engine::Present(); }

	void OnResize(int height, int width);
	void SetViewport(float top, float left, float height, float width) noexcept;
	
	// Mouse/Keyboard Events
	void OnMouseMove(float x, float y);
	void OnLButtonUpDown(bool isDown) { m_lButtonDown = isDown; }
	void OnWKeyUpDown(bool isDown) noexcept { m_keyWIsDown = isDown; }
	void OnAKeyUpDown(bool isDown) noexcept { m_keyAIsDown = isDown; }
	void OnSKeyUpDown(bool isDown) noexcept { m_keySIsDown = isDown; }
	void OnDKeyUpDown(bool isDown) noexcept { m_keyDIsDown = isDown; }

private:
	void LoadTextures();
	void BuildLandAndWaterScene();
	void BuildSkullAndMirrorScene();

	std::shared_ptr<DeviceResources> m_deviceResources;

	// Camera
	Camera m_camera;
	DirectX::XMFLOAT2 m_lastMousePos;
	bool m_lButtonDown = false;
	bool m_keyWIsDown = false;
	bool m_keyAIsDown = false;
	bool m_keySIsDown = false;
	bool m_keyDIsDown = false;

	// Textures
	std::array<std::unique_ptr<Texture>, (int)TEXTURE::Count> m_textures;

	// Land and Water Scene ------------------------------------------------------------------
	// 
	float GetHillsHeight(float x, float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z) const;
	void UpdateWavesVertices(const Timer&);
	void UpdateWavesMaterials(const Timer& timer);



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

	std::unique_ptr<RasterizerState> m_rasterizerState = nullptr;
	std::unique_ptr<BlendState> m_blendState = nullptr;
	std::unique_ptr<DepthStencilState> m_depthStencilState = nullptr;


	// Skull and Mirror Scene ------------------------------------------------------------------
	// 
	// Render Data
//	RenderPass m_mainRenderPass;





	void UpdateCamera(const Timer& timer);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
};
}