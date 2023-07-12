#pragma once
#include "../facade/facade.h" // NOTE: When including facade, it MUST be included first because of include conflicts between boost and Windows.h
#include <tiny.h>
#include "../SharedStuff.h"

namespace sandbox
{
	namespace stencilexample
	{
		struct Vertex
		{
			Vertex() :
				Pos(0.0f, 0.0f, 0.0f),
				Normal(0.0f, 0.0f, 0.0f),
				TexC(0.0f, 0.0f)
			{}
			Vertex(float px, float py, float pz, float nx, float ny, float nz, float tu, float tv) :
				Pos(px, py, pz),
				Normal(nx, ny, nz),
				TexC(tu, tv)
			{}
			DirectX::XMFLOAT3 Pos;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT2 TexC;
		};

		static constexpr int MaxLights = 16;

		struct Light
		{
			DirectX::XMFLOAT3   Strength = { 0.5f, 0.5f, 0.5f };
			float               FalloffStart = 1.0f;                // point/spot light only
			DirectX::XMFLOAT3   Direction = { 0.0f, -1.0f, 0.0f };  // directional/spot light only
			float               FalloffEnd = 10.0f;                 // point/spot light only
			DirectX::XMFLOAT3   Position = { 0.0f, 0.0f, 0.0f };    // point/spot light only
			float               SpotPower = 64.0f;                  // spot light only
		};

		struct PassConstants
		{
			DirectX::XMFLOAT4X4 View = tiny::MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4 InvView = tiny::MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4 Proj = tiny::MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4 InvProj = tiny::MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4 ViewProj = tiny::MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4 InvViewProj = tiny::MathHelper::Identity4x4();
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
	}

class StencilExample
{
public:
	StencilExample(std::shared_ptr<tiny::DeviceResources> deviceResources);

	void Update(const tiny::Timer& timer);
	void Render() { tiny::Engine::Render(); }
	void Present() { tiny::Engine::Present(); }

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
	void CreateSharedPassResources();
	void BuildStencilExample();
	void BuildMainRenderPass();
	void BuildReflectedRenderPass();
	void BuildMirrorAndShadowRenderPass();
	void LoadSkullGeometry(std::vector<stencilexample::Vertex>& vertices, std::vector<uint16_t>& indices);
	void UpdateCamera(const tiny::Timer& timer);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	std::shared_ptr<tiny::DeviceResources> m_deviceResources;

	// Camera
	tiny::Camera m_camera;
	DirectX::XMFLOAT2 m_lastMousePos;
	bool m_lButtonDown = false;
	bool m_keyWIsDown = false;
	bool m_keyAIsDown = false;
	bool m_keySIsDown = false;
	bool m_keyDIsDown = false;

	// Textures
	std::array<std::unique_ptr<tiny::Texture>, (int)TEXTURE::Count> m_textures;

	// Stencil Scene ------------------------------------------------------------------
	// 

	// Shared Resources
	std::unique_ptr<tiny::Shader> m_standardVS = nullptr;
	std::unique_ptr<tiny::Shader> m_opaquePS = nullptr; 
	std::unique_ptr<tiny::Shader> m_alphaTestedPS = nullptr; 
	std::unique_ptr<tiny::InputLayout> m_inputLayout = nullptr; 

	// Main Render Pass
	tiny::RenderPass m_mainRenderPass;
	stencilexample::PassConstants m_passConstants;
	std::unique_ptr<tiny::ConstantBufferT<stencilexample::PassConstants>> m_mainRenderPassConstantsCB = nullptr;
	
	std::unique_ptr<tiny::RasterizerState> m_opaqueRasterizerState = nullptr;
	std::unique_ptr<tiny::BlendState> m_opaqueBlendState = nullptr;
	std::unique_ptr<tiny::DepthStencilState> m_opaqueDepthStencilState = nullptr;

	std::unique_ptr<tiny::RasterizerState> m_mirrorRasterizerState = nullptr;
	std::unique_ptr<tiny::BlendState> m_mirrorBlendState = nullptr;
	std::unique_ptr<tiny::DepthStencilState> m_mirrorDepthStencilState = nullptr;

	// Reflected Render Pass
	tiny::RenderPass m_reflectedRenderPass;
	std::unique_ptr<tiny::ConstantBufferT<stencilexample::PassConstants>> m_reflectedRenderPassConstantsCB = nullptr;

	std::unique_ptr<tiny::RasterizerState> m_reflectedRasterizerState = nullptr;
	std::unique_ptr<tiny::BlendState> m_reflectedBlendState = nullptr;
	std::unique_ptr<tiny::DepthStencilState> m_reflectedDepthStencilState = nullptr;

	// Draw Mirror and Shadows Pass (Note: this pass will re-use the mainRenderPassConstantsCB)
//	tiny::RenderPass m_mirrorAndShadowsRenderPass;



	// Scene objects
	std::unique_ptr<GameObject> m_floorObject = nullptr;
	std::unique_ptr<GameObject> m_wallObject = nullptr;
	std::unique_ptr<GameObject> m_skullObject = nullptr;
	std::unique_ptr<GameObject> m_reflectedSkullObject = nullptr;

	std::unique_ptr<GameObject> m_mirrorObject = nullptr;

	// Box
//	std::unique_ptr<GameObject> m_boxObject = nullptr;
//
//	// Waves
//	std::unique_ptr<GameObject> m_wavesObject = nullptr;
//	std::unique_ptr<Waves> m_waves;
//	tiny::DynamicMeshGroupT<stencilexample::Vertex>* m_dynamicWaveMesh = nullptr;


};
}