#pragma once
#include <tiny.h>


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
	TREE_ARRAY_2,
	Count
};

namespace sandbox
{
class GameObject
{
public:
	struct Material
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float			  Roughness = 0.25f;

		// Used in texture mapping.
		DirectX::XMFLOAT4X4 MatTransform = tiny::MathHelper::Identity4x4();
	};

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 World = tiny::MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = tiny::MathHelper::Identity4x4();
	};

	GameObject(std::shared_ptr<tiny::DeviceResources> deviceResources);
	~GameObject();

	tiny::RenderItem* CreateRenderItem(tiny::RenderPassLayer* layer);

	void SetMaterialDiffuseAlbedo(const DirectX::XMFLOAT4& albedo) noexcept;
	void SetMaterialFresnelR0(const DirectX::XMFLOAT3& fresnel) noexcept;
	void SetMaterialRoughness(float roughness) noexcept;
	void SetMaterialTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetMaterialTransform(const DirectX::XMMATRIX& transform) noexcept;
	void SetWorldTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetWorldTransform(const DirectX::XMMATRIX& transform) noexcept;
	void SetTextureTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetTextureTransform(const DirectX::XMMATRIX& transform) noexcept;

	ND inline DirectX::XMFLOAT4X4& GetMaterialTransform() noexcept { m_materialNumFramesDirty = tiny::gNumFrameResources; return m_material.MatTransform; }
	ND inline const DirectX::XMFLOAT4X4& GetMaterialTransformConst() noexcept { return m_material.MatTransform; }

private:
	std::shared_ptr<tiny::DeviceResources> m_deviceResources;

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each frame in flight, we have to apply the
	// update to each frame in flight. Thus, when we modify obect data we should set 
	// m_numFramesDirty = gNumFrameResources so that each frame resource gets the update.
	ObjectConstants m_objectConstants;
	int m_numFramesDirty = tiny::gNumFrameResources;

	Material m_material;
	int m_materialNumFramesDirty = tiny::gNumFrameResources; // <-- Set this as dirty so the constant buffer gets updated immediately

	std::unique_ptr<tiny::ConstantBufferT<ObjectConstants>> m_objectConstantsCB = nullptr;
	std::unique_ptr<tiny::ConstantBufferT<Material>> m_materialCB = nullptr;

	std::vector<std::tuple<tiny::RenderPassLayer*, tiny::RenderItem*>> m_allRenderItems;
};
}