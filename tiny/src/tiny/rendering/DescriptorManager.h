#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"
#include "tiny/rendering/DescriptorVector.h"



namespace tiny
{
class DescriptorManager
{
public:
	static inline void Init(std::shared_ptr<DeviceResources> deviceResources) noexcept { Get().InitImpl(deviceResources); }

	static ND inline unsigned int Count() noexcept { return Get().CountImpl(); }
	static ND inline unsigned int Capacity() noexcept { return Get().CapacityImpl(); }
	static ND inline D3D12_DESCRIPTOR_HEAP_TYPE Type() noexcept { return Get().TypeImpl(); }

	static ND D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAt(UINT index) noexcept { return Get().GetCPUHandleAtImpl(index); }
	static ND D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAt(UINT index) noexcept { return Get().GetGPUHandleAtImpl(index); }
	static ND inline ID3D12DescriptorHeap* GetRawHeapPointer() noexcept { return Get().GetRawHeapPointerImpl(); }

	static unsigned int EmplaceBackShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc) { return Get().EmplaceBackShaderResourceViewImpl(pResource, desc); }
	static unsigned int EmplaceBackConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc) { return Get().EmplaceBackConstantBufferViewImpl(desc); }
	static unsigned int EmplaceBackUnorderedAccessView(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc) { return Get().EmplaceBackUnorderedAccessViewImpl(pResource, desc); }

	static void ReleaseAt(unsigned int index) noexcept { Get().ReleaseAtImpl(index); }

private:
	DescriptorManager() noexcept = default;
	DescriptorManager(const DescriptorManager&) = delete;
	DescriptorManager(DescriptorManager&&) = delete;
	DescriptorManager& operator=(const DescriptorManager&) = delete;
	DescriptorManager& operator=(DescriptorManager&&) = delete;

	static DescriptorManager& Get() noexcept { static DescriptorManager dm; return dm; }

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources) noexcept
	{
		TINY_CORE_ASSERT(!m_initialized, "DescriptorManager has already been initialized"); 
		m_deviceResources = deviceResources; 
		m_descriptorVector = std::make_unique<DescriptorVector>(m_deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); 
		m_initialized = true;
	}

	ND inline unsigned int CountImpl() const noexcept { return m_descriptorVector->Count(); }
	ND inline unsigned int CapacityImpl() const noexcept { return m_descriptorVector->Capacity(); }
	ND inline D3D12_DESCRIPTOR_HEAP_TYPE TypeImpl() const noexcept { return m_descriptorVector->Type(); }

	ND D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAtImpl(UINT index) const noexcept { return m_descriptorVector->GetCPUHandleAt(index); }
	ND D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAtImpl(UINT index) const noexcept { return m_descriptorVector->GetGPUHandleAt(index); }
	ND inline ID3D12DescriptorHeap* GetRawHeapPointerImpl() const noexcept { return m_descriptorVector->GetRawHeapPointer(); }

	unsigned int EmplaceBackShaderResourceViewImpl(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc) { return m_descriptorVector->EmplaceBackShaderResourceView(pResource, desc); }
	unsigned int EmplaceBackConstantBufferViewImpl(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc) { return m_descriptorVector->EmplaceBackConstantBufferView(desc); }
	unsigned int EmplaceBackUnorderedAccessViewImpl(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc) { return m_descriptorVector->EmplaceBackUnorderedAccessView(pResource, desc); }

	void ReleaseAtImpl(unsigned int index) noexcept { m_descriptorVector->ReleaseAt(index); }


	bool m_initialized = false;
	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;
	std::unique_ptr<DescriptorVector> m_descriptorVector = nullptr;
};
}