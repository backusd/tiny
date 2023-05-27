#include "tiny-pch.h"
#include "Texture.h"
#include "tiny/utils/StringHelper.h"

namespace tiny
{
const std::vector<std::string> g_textureFiles{ "src/textures/grass.dds", "src/textures/water1.dds", "src/textures/WireFence.dds" };

// TextureManager ===========================================================================
void TextureManager::InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept
{
	m_deviceResources = deviceResources;

	// Reserve enough space in the vector of all textures to fit one of each Texture
	m_allTextures.reserve((int)TEXTURE::Count);

	// Initialize all tuples to nullptr and a ref count of 0 (and set dummy descriptor heap index of 0)
	for (unsigned int iii = 0; iii < (int)TEXTURE::Count; ++iii)
		m_allTextures.push_back(std::make_tuple<TextureResources, unsigned int, unsigned int>(TextureResources(), 0, 0));
		//m_allTextures.emplace_back(TextureImpl(), 0, 0);

	// Create the descriptor vector
	m_descriptorVector = std::make_unique<DescriptorVector>(m_deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

std::unique_ptr<Texture> TextureManager::GetTextureImpl(TEXTURE texture)
{
	TINY_CORE_ASSERT(texture != TEXTURE::Count, "Cannot load the TEXTURE::Count value");

	TextureResources& textureResources = std::get<0>(m_allTextures[(int)texture]);

	// Only read the texture from file if the ref count is 0
	if (std::get<1>(m_allTextures[(int)texture]) == 0)
	{
		TINY_CORE_ASSERT(textureResources.Resource == nullptr, "Resource was not nullptr, but ref count was 0");
		TINY_CORE_ASSERT(textureResources.UploadHeap == nullptr, "UploadHeap was not nullptr, but ref count was 0");

		// Load the texture from file
		GFX_THROW_INFO(
			DirectX::CreateDDSTextureFromFile12(
				m_deviceResources->GetDevice(),
				m_deviceResources->GetCommandList(),
				utility::ToWString(g_textureFiles[(int)texture]).c_str(),
				textureResources.Resource,
				textureResources.UploadHeap
			)
		);

		// Create a new descriptor for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureResources.Resource->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		unsigned int index = m_descriptorVector->EmplaceBackShaderResourceView(textureResources.Resource.Get(), &srvDesc);

		// Keep track of the index needed to look up this descriptor
		std::get<2>(m_allTextures[(int)texture]) = index;
	}

	TINY_CORE_ASSERT(textureResources.Resource != nullptr, "Texture Resource should not be nullptr here");

	// Increment the ref count
	std::get<1>(m_allTextures[(int)texture]) += 1;

	// Return a Texture object
	return std::unique_ptr<Texture>(
		new Texture(
			m_descriptorVector.get(), 
			std::get<2>(m_allTextures[(int)texture]),
			texture
		)
	);
}

void TextureManager::ReleaseTextureImpl(TEXTURE texture) noexcept
{
	unsigned int& refCount = std::get<1>(m_allTextures[(int)texture]);
	TINY_CORE_ASSERT(refCount > 0, "Should not be calling ReleaseTexture for a Texture that already has a ref count of 0");
	--refCount;

	// If refCount is down to 0, then go ahead and delete the Texture data
	if (refCount == 0)
	{
		TextureResources& textureResources = std::get<0>(m_allTextures[(int)texture]);
		textureResources.Resource = nullptr;
		textureResources.UploadHeap = nullptr;

		// Good practice to just reset the index as well, although this is not necessary
		std::get<2>(m_allTextures[(int)texture]) = 0;
	}
}



// Texture ===============================================================================================================
Texture::Texture(DescriptorVector* descriptorVector, unsigned int index, TEXTURE texture) :
	m_descriptorVector(descriptorVector),
	m_index(index),
	m_texture(texture)
{}
Texture::~Texture()
{
	m_descriptorVector = nullptr;
	TextureManager::ReleaseTexture(m_texture);
}


}