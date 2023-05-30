#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/DeviceResources.h"

namespace tiny
{
class DescriptorVector
{
public:
    DescriptorVector(std::shared_ptr<DeviceResources> deviceResources, 
                     D3D12_DESCRIPTOR_HEAP_TYPE type, 
                     unsigned int initialCapacity = 8);

    ND inline unsigned int Count() const noexcept { return m_count; }
    ND inline unsigned int Capacity() const noexcept { return m_capacity; }
    ND inline D3D12_DESCRIPTOR_HEAP_TYPE Type() const noexcept { return m_type; }

    ND D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAt(UINT index) const noexcept;
    ND D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAt(UINT index) const noexcept;
    ND inline ID3D12DescriptorHeap* GetRawHeapPointer() const noexcept { return m_descriptorHeapShaderVisible.Get(); }

    unsigned int EmplaceBackShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);



private:
    DescriptorVector(const DescriptorVector& rhs) = delete;
    DescriptorVector& operator=(const DescriptorVector& rhs) = delete;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCopyableHandleAt(UINT index) const noexcept;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCopyableHandleAt(UINT index) const noexcept;
    void DoubleTheCapacity();

    std::shared_ptr<DeviceResources> m_deviceResources;
    unsigned int m_count;
    unsigned int m_capacity;
    const UINT m_handleIncrementSize;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeapCopyable;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeapShaderVisible;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHeapStart;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHeapStart;

    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
};
}
