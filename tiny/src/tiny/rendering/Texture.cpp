#include "tiny-pch.h"
#include "Texture.h"
#include "tiny/utils/StringHelper.h"
#include "tiny/utils/ConstexprMap.h"
#include "tiny/Engine.h"

namespace tiny
{
// Texture ===============================================================================================================
Texture::Texture(DescriptorVector* descriptorVector, unsigned int indexIntoDescriptorVector, unsigned int indexIntoAllTextures) noexcept :
	m_descriptorVector(descriptorVector),
	m_indexIntoDescriptorVector(indexIntoDescriptorVector),
	m_indexIntoAllTextures(indexIntoAllTextures)
{}
Texture::~Texture() noexcept
{
	m_descriptorVector = nullptr;
	TextureManager::ReleaseTexture(m_indexIntoAllTextures);
}





// TextureManager ===========================================================================
void TextureManager::InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept
{
	TINY_CORE_ASSERT(!m_initialized, "TextureManager has already been initialized");

	m_deviceResources = deviceResources;

	// Reserve enough space in the vector of all textures to fit one of each Texture
	std::size_t count = GetTotalTextureCount();
	m_allTextures.reserve(count);

	// Initialize all tuples to nullptr and a ref count of 0 (and set dummy descriptor heap index of 0)
	for (unsigned int iii = 0; iii < count; ++iii)
		m_allTextures.emplace_back();

	// Create the descriptor vector
	m_descriptorVector = std::make_unique<DescriptorVector>(m_deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Set Initialized flag
	m_initialized = true;
}

std::unique_ptr<Texture> TextureManager::GetTextureImpl(unsigned int indexIntoAllTextures)
{
	TINY_CORE_ASSERT(m_initialized, "Cannot call TextureManager::GetTextureImpl before calling TextureManager::Init");
	TINY_CORE_ASSERT(indexIntoAllTextures < GetTotalTextureCount(), "Texture does not exist for this index");

	Microsoft::WRL::ComPtr<ID3D12Resource>& textureResource = m_allTextures[indexIntoAllTextures].textureResource;

	// Only read the texture from file if the ref count is 0
	if (m_allTextures[indexIntoAllTextures].refCount == 0)
	{
		TINY_CORE_ASSERT(textureResource == nullptr, "Resource was not nullptr, but ref count was 0");

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;

		// Load the texture from file
		GFX_THROW_INFO(
			DirectX::CreateDDSTextureFromFile12(
				m_deviceResources->GetDevice(),
				m_deviceResources->GetCommandList(),
				GetTextureFilename(indexIntoAllTextures).c_str(),
				textureResource,
				uploadHeap
			)
		);

		// Do a delayed delete on the upload heap
		Engine::DelayedDelete(uploadHeap);

		// Create the SRV descriptor for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureResource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;

		// Create a SRV and keep track of the index needed to look up this descriptor
		m_allTextures[indexIntoAllTextures].descriptorVectorIndex = m_descriptorVector->EmplaceBackShaderResourceView(textureResource.Get(), &srvDesc);
	}

	TINY_CORE_ASSERT(textureResource != nullptr, "Texture Resource should not be nullptr here");

	// Increment the ref count
	++m_allTextures[indexIntoAllTextures].refCount;

	// Return a Texture object
	return std::unique_ptr<Texture>(
		new Texture(
			m_descriptorVector.get(),
			m_allTextures[indexIntoAllTextures].descriptorVectorIndex,
			indexIntoAllTextures
		)
	);
}

void TextureManager::ReleaseTextureImpl(unsigned int indexIntoAllTextures) noexcept
{
	TINY_CORE_ASSERT(m_initialized, "Cannot call TextureManager::ReleaseTextureImpl before calling TextureManager::Init");
	TINY_CORE_ASSERT(m_allTextures[indexIntoAllTextures].refCount > 0, "Should not be calling ReleaseTexture for a Texture that already has a ref count of 0");
	
	--m_allTextures[indexIntoAllTextures].refCount;

	// If refCount is down to 0, then go ahead and delete the Texture data
	if (m_allTextures[indexIntoAllTextures].refCount == 0)
	{
		// Do a delayed delete of the resource because it might still be in use by the GPU
		Engine::DelayedDelete(m_allTextures[indexIntoAllTextures].textureResource);

		// Inform the descriptor vector that the view for this texture can be removed
		m_descriptorVector->ReleaseAt(m_allTextures[indexIntoAllTextures].descriptorVectorIndex);
	}
}






}