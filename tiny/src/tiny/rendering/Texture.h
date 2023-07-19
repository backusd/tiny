#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"
#include "tiny/rendering/DescriptorManager.h"

// External methods so that the client can define which textures are included in the app
extern std::wstring GetTextureFilename(unsigned int index);
extern std::size_t GetTotalTextureCount();

namespace tiny
{
// Texture =======================================================================================================
class TextureManager; // Forward declare so we can set it as a friend
class TextureVector;
class Texture
{
public:	
	~Texture() noexcept;

	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const noexcept { return DescriptorManager::GetGPUHandleAt(m_srvDescriptorIndex); }
	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetUAVHandle() const noexcept { return DescriptorManager::GetGPUHandleAt(m_uavDescriptorIndex); }

	void CopyData(const std::vector<float>& data);
	void TransitionToState(D3D12_RESOURCE_STATES newState);

protected:
	Texture(std::shared_ptr<DeviceResources> deviceResources,
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			unsigned int srvIndex,
			unsigned int uavIndex = 0,
			unsigned int indexIntoAllManagedTextures = 0,
			bool isManagedTexture = false) noexcept;

	Texture(Texture&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		m_resource(rhs.m_resource),
		m_currentResourceState(rhs.m_currentResourceState),
		m_srvDescriptorIndex(rhs.m_srvDescriptorIndex),
		m_uavDescriptorIndex(rhs.m_uavDescriptorIndex),
		m_indexIntoAllManagedTextures(rhs.m_indexIntoAllManagedTextures),
		m_isManagedTexture(rhs.m_isManagedTexture),
		m_movedFrom(false)
	{
		rhs.m_movedFrom = true;
	}
	Texture& operator=(Texture&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		m_resource = rhs.m_resource;
		m_currentResourceState = rhs.m_currentResourceState;
		m_srvDescriptorIndex = rhs.m_srvDescriptorIndex;
		m_uavDescriptorIndex = rhs.m_uavDescriptorIndex;
		m_indexIntoAllManagedTextures = rhs.m_indexIntoAllManagedTextures;
		m_isManagedTexture = rhs.m_isManagedTexture;
		m_movedFrom = false;

		rhs.m_movedFrom = true;
	}

	// Safest to just delete copy constructors. Need to protect against the case where a Texture's destructor attempts to clean up 
	// its resources but another Texture still references the resource
	Texture(const Texture&) = delete;	
	Texture& operator=(const Texture&) = delete;

	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// Texture data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = nullptr;
	D3D12_RESOURCE_STATES m_currentResourceState = D3D12_RESOURCE_STATE_COMMON;

	// Data for accessing the descriptors for the texture
	unsigned int	  m_srvDescriptorIndex = 0;
	unsigned int	  m_uavDescriptorIndex = 0;

	// Data for managed textures
	unsigned int	  m_indexIntoAllManagedTextures = 0;
	bool			  m_isManagedTexture = false;

	bool m_movedFrom = false;

	// Declare TextureManager a friend so it can construct a Texture
	friend TextureManager;
	friend TextureVector;
};

// TextureVector ================================================================================================
class TextureVector
{
public:
	TextureVector(std::shared_ptr<DeviceResources> deviceResources);
	~TextureVector() noexcept
	{
		// See the note in ~TextureManager() for why this is necessary
		m_textures.clear();
	}

	ND Texture* EmplaceBack(const D3D12_RESOURCE_DESC& desc, bool createSRV = true, bool createUAV = false);
	ND inline Texture* operator[](size_t index) { return m_textures[index].get(); }

private:
	std::shared_ptr<DeviceResources>		m_deviceResources;
	std::vector<std::unique_ptr<Texture>>	m_textures;
};

// TextureManager ================================================================================================
// TextureManager is used for managing textures the are read from file. Texture Manager is a singleton, therefore,
// all code can lookup any texture at any time. There is also automatic reference counting - textures are only 
// loaded into GPU memory when being referenced and are released when the reference counts drops to 0.
class TextureManager
{
private:
	struct TextureInstanceData
	{
		unsigned int refCount = 0;
		std::unique_ptr<Texture> texture = nullptr;
	};

public:
	~TextureManager() noexcept
	{
		// For some reason, if we don't explicitly clear the textures first, then the destructor for the
		// DescriptorVector gets called first. This is an issue because the Texture destructor attempts to
		// call TextureManager::ReleaseTexture() which calls DescriptorVector::ReleaseAt(). So we manually
		// clear all the Textures to ensure their destructors run first.
		m_allTextures.clear();
	}

	static inline void Init(std::shared_ptr<DeviceResources> deviceResources) noexcept { Get().InitImpl(deviceResources); }
	ND static inline Texture* GetTexture(unsigned int indexIntoAllTextures) { return std::move(Get().GetTextureImpl(indexIntoAllTextures)); }

private:
	TextureManager() noexcept = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	static TextureManager& Get() noexcept { static TextureManager tm; return tm; }

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept;
	ND Texture* GetTextureImpl(unsigned int indexIntoAllTextures);

private:
	static void ReleaseTexture(unsigned int indexIntoAllTextures) noexcept { Get().ReleaseTextureImpl(indexIntoAllTextures); }
	void ReleaseTextureImpl(unsigned int indexIntoAllTextures) noexcept;

	bool m_initialized = false;
	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// all textures, their reference count, and their index into the DescriptorVector
	std::vector<TextureInstanceData> m_allTextures;

	// Make Texture a friend so its destructor can call ReleaseTexture()
	friend Texture;
};
}