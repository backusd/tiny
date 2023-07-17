#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"
#include "tiny/rendering/DescriptorVector.h"

// External methods so that the client can define which textures are included in the app
extern std::wstring GetTextureFilename(unsigned int index);
extern std::size_t GetTotalTextureCount();

namespace tiny
{
// Texture =======================================================================================================
class TextureManager; // Forward declare so we can set it as a friend
//class TextureVector;
class Texture
{
public:	
	~Texture() noexcept;

	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const noexcept 
	{
		return m_descriptorVector->GetGPUHandleAt(m_srvDescriptorIndex); 
	}
	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetUAVHandle() const noexcept { return m_descriptorVector->GetGPUHandleAt(m_uavDescriptorIndex); }

private:
	Texture() noexcept = default;
	Texture(std::shared_ptr<DeviceResources> deviceResources,
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			DescriptorVector* descriptorVector,
			unsigned int srvIndex,
			unsigned int uavIndex = 0,
			unsigned int indexIntoAllManagedTextures = 0,
			bool isManagedTexture = false) noexcept;
	// Note, we need to delete all copy/move constructors. You might think it would be okay to implement move operations, however, if this was
	// allowed, then we could in theory copy the data over to the new texture object, but when the destructor is called on the rhs object, it would
	// potentially release the texture resource from the resource manager's data
	Texture(const Texture&) = delete;	
	Texture(Texture&&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture& operator=(Texture&&) = delete;

	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// Texture data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = nullptr;

	// Data for accessing the descriptors for the texture
	DescriptorVector* m_descriptorVector = nullptr;
	unsigned int	  m_srvDescriptorIndex = 0;
	unsigned int	  m_uavDescriptorIndex = 0;

	// Data for managed textures
	unsigned int	  m_indexIntoAllManagedTextures = 0;
	bool			  m_isManagedTexture = false;

	// Declare TextureManager a friend so it can construct a Texture
	friend TextureManager;
	//friend TextureVector;
};

// TextureVector ================================================================================================
//class TextureVector
//{
//public:
//	TextureVector(std::shared_ptr<DeviceResources> deviceResources);
//
//	ND const Texture& EmplaceBack(const D3D12_RESOURCE_DESC& desc);
//
//private:
//	std::shared_ptr<DeviceResources> m_deviceResources;
//
//	std::vector<Texture> m_textures;
//	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textureResources;
//
//	std::unique_ptr<DescriptorVector> m_descriptorVector;
//
//};

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
	ND static inline ID3D12DescriptorHeap* GetHeapPointer() noexcept { return Get().GetHeapPointerImpl(); }

private:
	TextureManager() noexcept = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	static TextureManager& Get() noexcept { static TextureManager tm; return tm; }

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept;
	ND Texture* GetTextureImpl(unsigned int indexIntoAllTextures);
	ND inline ID3D12DescriptorHeap* GetHeapPointerImpl() const noexcept { return m_descriptorVector->GetRawHeapPointer(); }

private:
	static void ReleaseTexture(unsigned int indexIntoAllTextures) noexcept { Get().ReleaseTextureImpl(indexIntoAllTextures); }
	void ReleaseTextureImpl(unsigned int indexIntoAllTextures) noexcept;

	bool m_initialized = false;
	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// all textures, their reference count, and their index into the DescriptorVector
	std::vector<TextureInstanceData> m_allTextures;

	// Descriptor Vector to hold all the shader resource views for the Textures
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;

	// Make Texture a friend so its destructor can call ReleaseTexture()
	friend Texture;
};
}