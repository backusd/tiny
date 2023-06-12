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
class Texture
{
public:	
	~Texture() noexcept;

	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_descriptorVector->GetGPUHandleAt(m_indexIntoDescriptorVector); }

private:
	Texture(DescriptorVector* descriptorVector, unsigned int indexIntoDescriptorVector, unsigned int indexIntoAllTextures) noexcept;
	// Note, we need to delete all copy/move constructors. You might think it would be okay to implement move operations, however, if this was
	// allowed, then we could in theory copy the data over to the new texture object, but when the destructor is called on the rhs object, it would
	// potentially release the texture resource from the resource manager's data
	Texture(const Texture&) = delete;	
	Texture(Texture&&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture& operator=(Texture&&) = delete;

	DescriptorVector* m_descriptorVector;
	unsigned int	  m_indexIntoDescriptorVector;
	unsigned int	  m_indexIntoAllTextures;

	// Declare TextureManager a friend so it can construct a Texture
	friend TextureManager;
};

// TextureManager ================================================================================================
class TextureManager
{
private:
	struct TextureInstanceData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = nullptr;
		unsigned int refCount = 0;
		unsigned int descriptorVectorIndex = 0;
	};

public:
	static inline void Init(std::shared_ptr<DeviceResources> deviceResources) noexcept { Get().InitImpl(deviceResources); }
	ND static inline std::unique_ptr<Texture> GetTexture(unsigned int indexIntoAllTextures) { return std::move(Get().GetTextureImpl(indexIntoAllTextures)); }
	ND static inline ID3D12DescriptorHeap* GetHeapPointer() noexcept { return Get().GetHeapPointerImpl(); }

private:
	TextureManager() noexcept = default;
	TextureManager(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

	static TextureManager& Get() noexcept { static TextureManager tm; return tm; }

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept;
	ND std::unique_ptr<Texture> GetTextureImpl(unsigned int indexIntoAllTextures);
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