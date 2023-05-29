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
struct TextureResources
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

class TextureManager; // Forward declare so we can set it as a friend
class Texture
{
public:	
	~Texture();
	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_descriptorVector->GetGPUHandleAt(m_indexIntoDescriptorVector); }

private:
	Texture(DescriptorVector* descriptorVector, unsigned int indexIntoDescriptorVector, unsigned int indexIntoAllTextures);
	Texture(const Texture& rhs) = delete;
	Texture& operator=(const Texture& rhs) = delete;


	DescriptorVector* m_descriptorVector;
	unsigned int m_indexIntoDescriptorVector;
	unsigned int m_indexIntoAllTextures;

	// Declare TextureManager a friend so it can construct a Texture
	friend TextureManager;
};


class TextureManager
{
public:
	static void Init(std::shared_ptr<DeviceResources> deviceResources) noexcept { Get().InitImpl(deviceResources); }
	ND static std::unique_ptr<Texture> GetTexture(unsigned int indexIntoAllTextures) { return std::move(Get().GetTextureImpl(indexIntoAllTextures)); }
	ND static ID3D12DescriptorHeap* GetHeapPointer() noexcept { return Get().GetHeapPointerImpl(); }

private:
	TextureManager() noexcept = default;
	TextureManager(const TextureManager& rhs) = delete;
	TextureManager& operator=(const TextureManager& rhs) = delete;

	static TextureManager& Get() noexcept
	{
		static TextureManager tm;
		return tm;
	}

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept;
	ND std::unique_ptr<Texture> GetTextureImpl(unsigned int indexIntoAllTextures);
	ND ID3D12DescriptorHeap* GetHeapPointerImpl() noexcept { return m_descriptorVector->GetRawHeapPointer(); }




private:
	static void ReleaseTexture(unsigned int indexIntoAllTextures) noexcept { Get().ReleaseTextureImpl(indexIntoAllTextures); }
	void ReleaseTextureImpl(unsigned int indexIntoAllTextures) noexcept;

	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// all textures, their reference count, and their index into the DescriptorVector
	std::vector<std::tuple<TextureResources, unsigned int, unsigned int>> m_allTextures;

	// Descriptor Vector to hold all the shader resource views for the Textures
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;

	// Make Texture a friend so its destructor can call ReleaseTexture()
	friend Texture;
};
}