#pragma once
#include "../../facade/facade.h" // NOTE: When including facade, it MUST be included first because of include conflicts between boost and Windows.h
#include <tiny.h>
#include "../SharedStuff.h"

namespace sandbox
{
	namespace landandwaves
	{
		struct Vertex
		{
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
class Waves;

class LandAndWavesScene
{
public:
	LandAndWavesScene(std::shared_ptr<tiny::DeviceResources> deviceResources);

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
	void BuildLandAndWaterScene();

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
	std::array<tiny::Texture*, (int)TEXTURE::Count> m_textures;

	// Land and Water Scene ------------------------------------------------------------------
	// 
	float GetHillsHeight(float x, float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z) const;
	void UpdateWavesVertices(const tiny::Timer&);
	void UpdateWavesMaterials(const tiny::Timer& timer);



	// Render Data
	tiny::RenderPass m_mainRenderPass;
	std::unique_ptr<tiny::ConstantBufferT<landandwaves::PassConstants>> m_mainRenderPassConstantsCB = nullptr;
	std::unique_ptr<tiny::Shader> m_standardVS = nullptr;
	std::unique_ptr<tiny::Shader> m_opaquePS = nullptr;
	std::unique_ptr<tiny::Shader> m_alphaTestedPS = nullptr;
	std::unique_ptr<tiny::InputLayout> m_inputLayout = nullptr;

	// Grid
	std::unique_ptr<GameObject> m_gridObject = nullptr;

	// Box
	std::unique_ptr<GameObject> m_boxObject = nullptr;

	// Waves
	std::unique_ptr<GameObject> m_wavesObject = nullptr;
	std::unique_ptr<Waves> m_waves;
	tiny::DynamicMeshGroupT<landandwaves::Vertex>* m_dynamicWaveMesh = nullptr;

	std::unique_ptr<tiny::RasterizerState> m_rasterizerState = nullptr;
	std::unique_ptr<tiny::BlendState> m_blendState = nullptr;
	std::unique_ptr<tiny::DepthStencilState> m_depthStencilState = nullptr;

	void UpdateCamera(const tiny::Timer& timer);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
};

// ======================================================================================================
// Waves
// ======================================================================================================
class Waves
{
public:
	Waves(int m, int n, float dx, float dt, float speed, float damping);
	Waves(const Waves& rhs) = delete;
	Waves& operator=(const Waves& rhs) = delete;
	~Waves();

	int RowCount()const;
	int ColumnCount()const;
	int VertexCount()const;
	int TriangleCount()const;
	float Width()const;
	float Depth()const;

	// Returns the solution at the ith grid point.
	const DirectX::XMFLOAT3& Position(int i)const { return mCurrSolution[i]; }

	// Returns the solution normal at the ith grid point.
	const DirectX::XMFLOAT3& Normal(int i)const { return mNormals[i]; }

	// Returns the unit tangent vector at the ith grid point in the local x-axis direction.
	const DirectX::XMFLOAT3& TangentX(int i)const { return mTangentX[i]; }

	void Update(float dt);
	void Disturb(int i, int j, float magnitude);

private:
	int mNumRows = 0;
	int mNumCols = 0;

	int mVertexCount = 0;
	int mTriangleCount = 0;

	// Simulation constants we can precompute.
	float mK1 = 0.0f;
	float mK2 = 0.0f;
	float mK3 = 0.0f;

	float mTimeStep = 0.0f;
	float mSpatialStep = 0.0f;

	std::vector<DirectX::XMFLOAT3> mPrevSolution;
	std::vector<DirectX::XMFLOAT3> mCurrSolution;
	std::vector<DirectX::XMFLOAT3> mNormals;
	std::vector<DirectX::XMFLOAT3> mTangentX;
};
}