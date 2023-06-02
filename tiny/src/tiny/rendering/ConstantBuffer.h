#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/utils/Constants.h"

namespace tiny
{
template<typename T>
class ConstantBuffer
{
public:
	ConstantBuffer(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources)
	{
		TINY_CORE_ASSERT(m_deviceResources != nullptr, "No device resources");

		// Need to create the buffer in an upload heap so the CPU can regularly send new data to it
		auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		// Create a buffer that will hold one element for each frame resource
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * gNumFrameResources);		

		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_uploadBuffer)
			)
		);

		GFX_THROW_INFO(
			m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData))
		);

		// We do not need to unmap until we are done with the resource. However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}
	~ConstantBuffer()
	{
		if (m_uploadBuffer != nullptr)
			m_uploadBuffer->Unmap(0, nullptr);

		m_mappedData = nullptr;
	}

	inline void CopyData(unsigned int frameIndex, const T& data)
	{
		memcpy(&m_mappedData[frameIndex * m_elementByteSize], &data, sizeof(T));
	}

	ND inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(unsigned int frameIndex)
	{
		return m_uploadBuffer->GetGPUVirtualAddress() + frameIndex * m_elementByteSize;
	}


private:
	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;


	std::shared_ptr<DeviceResources> m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData = nullptr;

	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	static constexpr UINT m_elementByteSize = (sizeof(T) + 255) & ~255;
};

}