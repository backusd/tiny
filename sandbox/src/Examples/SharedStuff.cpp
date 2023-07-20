#include "SharedStuff.h"

static constexpr std::array g_textureFiles{
	L"src/textures/grass.dds",
	L"src/textures/water1.dds",
	L"src/textures/WireFence.dds",
	L"src/textures/bricks.dds",
	L"src/textures/bricks2.dds",
	L"src/textures/bricks3.dds",
	L"src/textures/checkboard.dds",
	L"src/textures/ice.dds",
	L"src/textures/stone.dds",
	L"src/textures/tile.dds",
	L"src/textures/white1x1.dds",
	L"src/textures/treeArray2.dds"
};

std::wstring GetTextureFilename(unsigned int index)
{
	return g_textureFiles[index];
}
std::size_t GetTotalTextureCount()
{
	return static_cast<std::size_t>(TEXTURE::Count);
}

using namespace tiny;

namespace sandbox
{
GameObject::GameObject(std::shared_ptr<tiny::DeviceResources> deviceResources) :
	GameObjectBase<BasicObjectConstants, BasicMaterial>(deviceResources)
{
	m_constantsCB = std::make_unique<ConstantBufferT<BasicObjectConstants>>(m_deviceResources);
	m_materialCB = std::make_unique<ConstantBufferT<BasicMaterial>>(m_deviceResources);
}

RenderItem* GameObject::CreateRenderItem(tiny::RenderPassLayer* layer)
{
	TINY_ASSERT(layer != nullptr, "Layer should never be nullptr");

	RenderItem& ri = layer->RenderItems.emplace_back();

	// Keep track of all render items and which layer they belong to (will be used in the destructor)
	m_allRenderItems.emplace_back(layer, &ri);

	// Constant Buffer
	auto& boxConstantsCBV = ri.ConstantBufferViews.emplace_back(1, m_constantsCB.get());
	boxConstantsCBV.Update = [this](const Timer& timer, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (m_constantsNumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_objectConstants.World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&m_objectConstants.TexTransform);

			BasicObjectConstants boxConstants;
			DirectX::XMStoreFloat4x4(&boxConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&boxConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			m_constantsCB->CopyData(frameIndex, boxConstants);

			--m_constantsNumFramesDirty;
		}
	};

	// Material Buffer
	auto& boxMaterialCBV = ri.ConstantBufferViews.emplace_back(3, m_materialCB.get());
	boxMaterialCBV.Update = [this](const Timer& timer, int frameIndex)
	{
		if (m_materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&m_material.MatTransform);

			BasicMaterial mat = m_material;
			DirectX::XMStoreFloat4x4(&mat.MatTransform, DirectX::XMMatrixTranspose(transform));

			m_materialCB->CopyData(frameIndex, mat);

			// Next FrameResource need to be updated too.
			--m_materialNumFramesDirty;
		}
	};

	return &ri;
}

void GameObject::SetMaterialDiffuseAlbedo(const DirectX::XMFLOAT4& albedo) noexcept
{
	m_material.DiffuseAlbedo = albedo;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GameObject::SetMaterialFresnelR0(const DirectX::XMFLOAT3& fresnel) noexcept
{
	m_material.FresnelR0 = fresnel;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GameObject::SetMaterialRoughness(float roughness) noexcept
{
	m_material.Roughness = roughness;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GameObject::SetMaterialTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_material.MatTransform = transform;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GameObject::SetMaterialTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_material.MatTransform, transform);
	m_materialNumFramesDirty = gNumFrameResources;
}
void GameObject::SetWorldTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_objectConstants.World = transform;
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GameObject::SetWorldTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.World, transform);
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GameObject::SetTextureTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_objectConstants.TexTransform = transform;
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GameObject::SetTextureTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.TexTransform, transform);
	m_constantsNumFramesDirty = gNumFrameResources;
}


// =========================================================================================
// GridGameObject

GridGameObject::GridGameObject(std::shared_ptr<tiny::DeviceResources> deviceResources) :
	GameObjectBase<GridObjectConstants, BasicMaterial>(deviceResources)
{
	m_constantsCB = std::make_unique<ConstantBufferT<GridObjectConstants>>(m_deviceResources);
	m_materialCB = std::make_unique<ConstantBufferT<BasicMaterial>>(m_deviceResources);
}

RenderItem* GridGameObject::CreateRenderItem(tiny::RenderPassLayer* layer)
{
	TINY_ASSERT(layer != nullptr, "Layer should never be nullptr");

	RenderItem& ri = layer->RenderItems.emplace_back();

	// Keep track of all render items and which layer they belong to (will be used in the destructor)
	m_allRenderItems.emplace_back(layer, &ri);

	// Constant Buffer
	auto& gridConstantsCBV = ri.ConstantBufferViews.emplace_back(1, m_constantsCB.get());
	gridConstantsCBV.Update = [this](const Timer& timer, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (m_constantsNumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_objectConstants.World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&m_objectConstants.TexTransform);

			GridObjectConstants gridConstants;
			DirectX::XMStoreFloat4x4(&gridConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&gridConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			gridConstants.DisplacementMapTexelSize = m_objectConstants.DisplacementMapTexelSize;
			gridConstants.GridSpatialStep = m_objectConstants.GridSpatialStep;

			m_constantsCB->CopyData(frameIndex, gridConstants);

			--m_constantsNumFramesDirty;
		}
	};

	// Material Buffer
	auto& boxMaterialCBV = ri.ConstantBufferViews.emplace_back(3, m_materialCB.get());
	boxMaterialCBV.Update = [this](const Timer& timer, int frameIndex)
	{
		if (m_materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&m_material.MatTransform);

			BasicMaterial mat = m_material;
			DirectX::XMStoreFloat4x4(&mat.MatTransform, DirectX::XMMatrixTranspose(transform));

			m_materialCB->CopyData(frameIndex, mat);

			// Next FrameResource need to be updated too.
			--m_materialNumFramesDirty;
		}
	};

	return &ri;
}

void GridGameObject::SetMaterialDiffuseAlbedo(const DirectX::XMFLOAT4& albedo) noexcept
{
	m_material.DiffuseAlbedo = albedo;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetMaterialFresnelR0(const DirectX::XMFLOAT3& fresnel) noexcept
{
	m_material.FresnelR0 = fresnel;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetMaterialRoughness(float roughness) noexcept
{
	m_material.Roughness = roughness;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetMaterialTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_material.MatTransform = transform;
	m_materialNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetMaterialTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_material.MatTransform, transform);
	m_materialNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetWorldTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_objectConstants.World = transform;
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetWorldTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.World, transform);
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetTextureTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_objectConstants.TexTransform = transform;
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetTextureTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.TexTransform, transform);
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetDisplacementMapTexelSize(const DirectX::XMFLOAT2& size) noexcept
{
	m_objectConstants.DisplacementMapTexelSize = size;
	m_constantsNumFramesDirty = gNumFrameResources;
}
void GridGameObject::SetGridSpatialStep(float step) noexcept
{
	m_objectConstants.GridSpatialStep = step;
	m_constantsNumFramesDirty = gNumFrameResources;
}

}