#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"
#include "tiny/Log.h"
#include "tiny/utils/StringHelper.h"

namespace tiny
{
class Shader
{
public:
	Shader(std::shared_ptr<DeviceResources> deviceResources, const std::string& filename) :
		m_deviceResources(deviceResources),
		m_filename(filename)
	{
		TINY_CORE_ASSERT(m_deviceResources != nullptr, "No device resources");
		TINY_CORE_ASSERT(m_filename.size() > 0, "Filename cannot be empty");

		ReadFileToBlob();
	}
	Shader(const Shader& rhs) :
		m_deviceResources(rhs.m_deviceResources),
		m_filename(rhs.m_filename),
		m_blob(nullptr)
	{
		ReadFileToBlob();
	}
	Shader(Shader&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		m_filename(rhs.m_filename),
		m_blob(rhs.m_blob) // Just make a copy of the ComPtr to the underlying blob because the rhs object will die soon, so no need to worry about multiple objects managing the same blob
	{}
	Shader& operator=(const Shader& rhs)
	{
		m_deviceResources = rhs.m_deviceResources;
		m_filename = rhs.m_filename;
		ReadFileToBlob();
		return *this;
	}
	Shader& operator=(Shader&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		m_filename = rhs.m_filename;
		m_blob = rhs.m_blob;
		return *this;
	}
	~Shader() noexcept {}

	ND inline void* GetBufferPointer() const noexcept { return m_blob->GetBufferPointer(); }
	ND inline SIZE_T GetBufferSize() const noexcept { return m_blob->GetBufferSize(); }
	ND inline D3D12_SHADER_BYTECODE GetShaderByteCode() const noexcept { return { reinterpret_cast<BYTE*>(m_blob->GetBufferPointer()), m_blob->GetBufferSize() }; }

protected:
	void ReadFileToBlob()
	{
		GFX_THROW_INFO(
			D3DReadFileToBlob(utility::ToWString(m_filename).c_str(), m_blob.ReleaseAndGetAddressOf())
		);
	}

	std::shared_ptr<DeviceResources> m_deviceResources;
	std::string						 m_filename;
	Microsoft::WRL::ComPtr<ID3DBlob> m_blob;
};
}