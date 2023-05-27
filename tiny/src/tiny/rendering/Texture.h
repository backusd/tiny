#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"
#include "tiny/rendering/DescriptorVector.h"

namespace tiny
{
// struct _Texture
// {
// 	// Unique material name for lookup.
// 	std::string Name;
// 
// 	std::wstring Filename;
// 
// 	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
// 	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
// };

//const std::vector<std::string> g_textureFiles;

enum class TEXTURE : int
{
	GRASS = 0,
	WATER1,
	WIRE_FENCE,
	Count
};

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
	ND inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_descriptorVector->GetGPUHandleAt(m_index); }

private:
	Texture(DescriptorVector* descriptorVector, unsigned int index, TEXTURE texture);
	Texture(const Texture& rhs) = delete;
	Texture& operator=(const Texture& rhs) = delete;


	DescriptorVector* m_descriptorVector;
	unsigned int m_index;
	TEXTURE m_texture;

	// Declare TextureManager a friend so it can construct a Texture
	friend TextureManager;
};


class TextureManager
{
public:
	static void Init(std::shared_ptr<DeviceResources> deviceResources) noexcept { Get().InitImpl(deviceResources); }
	ND static std::unique_ptr<Texture> GetTexture(TEXTURE texture) { return std::move(Get().GetTextureImpl(texture)); }
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
	ND std::unique_ptr<Texture> GetTextureImpl(TEXTURE texture);
	ND ID3D12DescriptorHeap* GetHeapPointerImpl() noexcept { return m_descriptorVector->GetRawHeapPointer(); }




private:
	static void ReleaseTexture(TEXTURE texture) noexcept { Get().ReleaseTextureImpl(texture); }
	void ReleaseTextureImpl(TEXTURE texture) noexcept;

	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;

	// all textures, their refernce count, and their index into the DescriptorVector
	std::vector<std::tuple<TextureResources, unsigned int, unsigned int>> m_allTextures;

	// Descriptor Vector to hold all the shader resource views for the Textures
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;

	// Make Texture a friend so its destructor can call ReleaseTexture()
	friend Texture;
};
}