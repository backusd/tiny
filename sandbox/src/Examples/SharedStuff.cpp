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
	m_deviceResources(deviceResources),
	m_material()
{
	m_objectConstantsCB = std::make_unique<ConstantBufferT<ObjectConstants>>(m_deviceResources);
	m_materialCB = std::make_unique<ConstantBufferT<Material>>(m_deviceResources);
}
GameObject::~GameObject()
{
	// Remove all render items associated with this object
	for (auto& tup : m_allRenderItems)
	{
		RenderPassLayer* layer = std::get<0>(tup);
		RenderItem* ri = std::get<1>(tup);
		if (layer != nullptr && ri != nullptr)
			layer->RemoveRenderItem(ri);
	}
}

RenderItem* GameObject::CreateRenderItem(tiny::RenderPassLayer* layer)
{
	TINY_ASSERT(layer != nullptr, "Layer should never be nullptr");

	RenderItem& ri = layer->RenderItems.emplace_back();

	// Keep track of all render items and which layer they belong to (will be used in the destructor)
	m_allRenderItems.emplace_back(layer, &ri);

	// Constant Buffer
	auto& boxConstantsCBV = ri.ConstantBufferViews.emplace_back(1, m_objectConstantsCB.get());
	boxConstantsCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (m_numFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_objectConstants.World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&m_objectConstants.TexTransform);

			ObjectConstants boxConstants;
			DirectX::XMStoreFloat4x4(&boxConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&boxConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			m_objectConstantsCB->CopyData(frameIndex, boxConstants);

			--m_numFramesDirty;
		}
	};

	// Material Buffer
	auto& boxMaterialCBV = ri.ConstantBufferViews.emplace_back(3, m_materialCB.get());
	boxMaterialCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		if (m_materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&m_material.MatTransform);

			Material mat = m_material;
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
	m_numFramesDirty = gNumFrameResources;
}
void GameObject::SetWorldTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.World, transform);
	m_numFramesDirty = gNumFrameResources;
}
void GameObject::SetTextureTransform(const DirectX::XMFLOAT4X4& transform) noexcept
{
	m_objectConstants.TexTransform = transform;
	m_numFramesDirty = gNumFrameResources;
}
void GameObject::SetTextureTransform(const DirectX::XMMATRIX& transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_objectConstants.TexTransform, transform);
	m_numFramesDirty = gNumFrameResources;
}
}