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
struct BasicMaterial
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float			  Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = tiny::MathHelper::Identity4x4();
};

struct BasicObjectConstants
{
	DirectX::XMFLOAT4X4 World = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = tiny::MathHelper::Identity4x4();
};
struct GridObjectConstants
{
	DirectX::XMFLOAT4X4 World = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = tiny::MathHelper::Identity4x4();
	DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f, 1.0f };
	float GridSpatialStep = 1.0f;
	float _padding = 0.0f;
};

template<class _Constants, class _Material>
class GameObjectBase
{
public:
	GameObjectBase(std::shared_ptr<tiny::DeviceResources> deviceResources) :
		m_deviceResources(deviceResources)
	{}
	virtual ~GameObjectBase() noexcept { Cleanup(); }

	virtual tiny::RenderItem* CreateRenderItem(tiny::RenderPassLayer* layer) = 0;

protected:
	virtual void Cleanup() noexcept
	{
		// Remove all render items associated with this object
		for (auto& tup : m_allRenderItems)
		{
			tiny::RenderPassLayer* layer = std::get<0>(tup);
			tiny::RenderItem* ri = std::get<1>(tup);
			if (layer != nullptr && ri != nullptr)
				layer->RemoveRenderItem(ri);
		}
	}

	std::shared_ptr<tiny::DeviceResources> m_deviceResources;

	_Constants m_objectConstants;
	int m_constantsNumFramesDirty = tiny::gNumFrameResources;

	_Material m_material;
	int m_materialNumFramesDirty = tiny::gNumFrameResources;

	std::unique_ptr<tiny::ConstantBufferT<_Constants>> m_constantsCB = nullptr;
	std::unique_ptr<tiny::ConstantBufferT<_Material>> m_materialCB = nullptr;

	std::vector<std::tuple<tiny::RenderPassLayer*, tiny::RenderItem*>> m_allRenderItems;
};



class GameObject : public GameObjectBase<BasicObjectConstants, BasicMaterial>
{
public:
	GameObject(std::shared_ptr<tiny::DeviceResources> deviceResources);
	virtual ~GameObject() noexcept override { Cleanup(); }

	tiny::RenderItem* CreateRenderItem(tiny::RenderPassLayer* layer) override;

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

};

class GridGameObject : public GameObjectBase<GridObjectConstants, BasicMaterial>
{
public:
	GridGameObject(std::shared_ptr<tiny::DeviceResources> deviceResources);
	virtual ~GridGameObject() noexcept override { Cleanup(); }

	tiny::RenderItem* CreateRenderItem(tiny::RenderPassLayer* layer) override;

	void SetMaterialDiffuseAlbedo(const DirectX::XMFLOAT4& albedo) noexcept;
	void SetMaterialFresnelR0(const DirectX::XMFLOAT3& fresnel) noexcept;
	void SetMaterialRoughness(float roughness) noexcept;
	void SetMaterialTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetMaterialTransform(const DirectX::XMMATRIX& transform) noexcept;
	void SetWorldTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetWorldTransform(const DirectX::XMMATRIX& transform) noexcept;
	void SetTextureTransform(const DirectX::XMFLOAT4X4& transform) noexcept;
	void SetTextureTransform(const DirectX::XMMATRIX& transform) noexcept;
	void SetDisplacementMapTexelSize(const DirectX::XMFLOAT2& size) noexcept;
	void SetGridSpatialStep(float step) noexcept;

	ND inline DirectX::XMFLOAT4X4& GetMaterialTransform() noexcept { m_materialNumFramesDirty = tiny::gNumFrameResources; return m_material.MatTransform; }
	ND inline const DirectX::XMFLOAT4X4& GetMaterialTransformConst() noexcept { return m_material.MatTransform; }

};
}