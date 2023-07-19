#include "tiny-pch.h"
#include "Texture.h"
#include "tiny/utils/StringHelper.h"
#include "tiny/utils/ConstexprMap.h"
#include "tiny/Engine.h"

namespace tiny
{
// Texture ===============================================================================================================
Texture::Texture(std::shared_ptr<DeviceResources> deviceResources,
				 Microsoft::WRL::ComPtr<ID3D12Resource> resource,
				 unsigned int srvIndex,
				 unsigned int uavIndex,
				 unsigned int indexIntoAllManagedTextures,
				 bool isManagedTexture) noexcept :
	m_deviceResources(deviceResources),
	m_resource(resource),
	m_srvDescriptorIndex(srvIndex),
	m_uavDescriptorIndex(uavIndex),
	m_indexIntoAllManagedTextures(indexIntoAllManagedTextures),
	m_isManagedTexture(isManagedTexture),
	m_currentResourceState(D3D12_RESOURCE_STATE_COMMON)
{
	TINY_CORE_ASSERT(m_deviceResources != nullptr, "No device resources");
}
Texture::~Texture() noexcept
{
	// If the texture has been moved from, then we don't want to clean up what it was referencing
	if (!m_movedFrom)
	{
		// NOTE: Only do a delayed delete if the texture is NOT managed. If it is managed
		//       then the texture manager will be responsible for clean up
		if (m_isManagedTexture)
		{
			TextureManager::ReleaseTexture(m_indexIntoAllManagedTextures);
		}
		else
		{
			Engine::DelayedDelete(m_resource);
		}
	}
}

void Texture::CopyData(const std::vector<float>& data)
{
	TINY_CORE_ASSERT(m_currentResourceState == D3D12_RESOURCE_STATE_COPY_DEST, "Texure must be placed in state 'D3D12_RESOURCE_STATE_COPY_DEST' before you can copy data to it");

	D3D12_RESOURCE_DESC desc = m_resource->GetDesc();
	const UINT num2DSubresources = desc.DepthOrArraySize * desc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_resource.Get(), 0, num2DSubresources);

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr;

	CD3DX12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		)
	);

	D3D12_SUBRESOURCE_DATA subResourceData = {}; 
	subResourceData.pData = data.data(); 
	subResourceData.RowPitch = desc.Width * sizeof(float); 
	subResourceData.SlicePitch = subResourceData.RowPitch * desc.Height; 


	auto* commandList = m_deviceResources->GetCommandList();
	GFX_THROW_INFO_ONLY(
		UpdateSubresources(commandList, m_resource.Get(), uploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData)
	);
	Engine::DelayedDelete(uploadBuffer);
}
void Texture::TransitionToState(D3D12_RESOURCE_STATES newState)
{
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), m_currentResourceState, newState);
	GFX_THROW_INFO_ONLY(m_deviceResources->GetCommandList()->ResourceBarrier(1, &transition));
	m_currentResourceState = newState;
}


// TextureVector ================================================================================================
TextureVector::TextureVector(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources)
{
	TINY_CORE_ASSERT(m_deviceResources != nullptr, "No device resources");
}
Texture* TextureVector::EmplaceBack(const D3D12_RESOURCE_DESC& desc, bool createSRV, bool createUAV)
{
	TINY_CORE_ASSERT(createSRV || createUAV, "It is an error not to create at least one view for the Texture");

	CD3DX12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

	GFX_THROW_INFO(
		m_deviceResources->GetDevice()->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&resource)
		)
	);

	unsigned int srvIndex = 0;
	unsigned int uavIndex = 0;

	if (createSRV)
	{
		// Create the SRV descriptor for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; 
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; 
		srvDesc.Format = desc.Format; 
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; 
		srvDesc.Texture2D.MostDetailedMip = 0; 
		srvDesc.Texture2D.MipLevels = 1; 

		srvIndex = DescriptorManager::EmplaceBackShaderResourceView(resource.Get(), &srvDesc);
	}

	if (createUAV)
	{
		// Create the UAV descriptor for the texture
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D; 
		uavDesc.Texture2D.MipSlice = 0; 

		uavIndex = DescriptorManager::EmplaceBackUnorderedAccessView(resource.Get(), &uavDesc);
	}

	std::unique_ptr<Texture> t = std::unique_ptr<Texture>(new Texture(m_deviceResources, resource, srvIndex, uavIndex));
	m_textures.push_back(std::move(t));
	return m_textures.back().get();
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

	// Set Initialized flag
	m_initialized = true;
}

Texture* TextureManager::GetTextureImpl(unsigned int indexIntoAllTextures)
{
	TINY_CORE_ASSERT(m_initialized, "Cannot call TextureManager::GetTextureImpl before calling TextureManager::Init");
	TINY_CORE_ASSERT(indexIntoAllTextures < GetTotalTextureCount(), "Texture does not exist for this index");

	// Only read the texture from file if the ref count is 0
	if (m_allTextures[indexIntoAllTextures].refCount == 0)
	{
		TINY_CORE_ASSERT(m_allTextures[indexIntoAllTextures].texture == nullptr, "Texture was not nullptr, but ref count was 0");

		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = nullptr;
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
		unsigned int srvIndex = DescriptorManager::EmplaceBackShaderResourceView(textureResource.Get(), &srvDesc);

		m_allTextures[indexIntoAllTextures].refCount = 1;
		m_allTextures[indexIntoAllTextures].texture = std::unique_ptr<Texture>(new Texture(m_deviceResources, textureResource, srvIndex, 0, indexIntoAllTextures, true));
	}
	else
	{
		TINY_CORE_ASSERT(m_allTextures[indexIntoAllTextures].texture != nullptr, "Texture should have already been created");
		++m_allTextures[indexIntoAllTextures].refCount;
	}

	return m_allTextures[indexIntoAllTextures].texture.get();
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
		Engine::DelayedDelete(m_allTextures[indexIntoAllTextures].texture->m_resource);

		// Inform the descriptor vector that the view for this texture can be removed
		DescriptorManager::ReleaseAt(m_allTextures[indexIntoAllTextures].texture->m_srvDescriptorIndex);
	}
}

}