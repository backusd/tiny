#include "tiny-pch.h"
#include "DescriptorVector.h"


namespace tiny
{
DescriptorVector::DescriptorVector(std::shared_ptr<DeviceResources> deviceResources,
                                   D3D12_DESCRIPTOR_HEAP_TYPE type,
                                   unsigned int initialCapacity) :
    m_count(0),
    m_deviceResources(deviceResources),
    m_capacity(initialCapacity),
    m_type(type),
    m_handleIncrementSize(deviceResources->GetDevice()->GetDescriptorHandleIncrementSize(type))
{
    TINY_CORE_ASSERT(m_deviceResources != nullptr, "No device resources");
    TINY_CORE_ASSERT(m_capacity > 0, "Capacity must be greater than 0 so that we can safely double the capacity when necessary");
    
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = m_capacity;
    desc.Type = m_type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    // Create the CPU copyable descriptor heap
    GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeapCopyable)));

    // Create the descriptor heap that will actually be use for retrieving descriptors for rendering
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeapShaderVisible)));

    m_cpuHeapStart = m_descriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart();
    m_gpuHeapStart = m_descriptorHeapShaderVisible->GetGPUDescriptorHandleForHeapStart();
}

void DescriptorVector::DoubleTheCapacity()
{
    auto device = m_deviceResources->GetDevice();

    // Create new descriptor heaps with doubled capacity
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newDescriptorHeapCopyable;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newDescriptorHeapShaderVisible;

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = m_capacity * 2;
    desc.Type = m_type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeapCopyable)));

    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    GFX_THROW_INFO(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeapShaderVisible)));

    // Copy previous descriptor heap to the new one
    device->CopyDescriptorsSimple(
        m_capacity,
        newDescriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_type
    );
    device->CopyDescriptorsSimple(
        m_capacity,
        newDescriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart(),
        m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart(),
        m_type
    );

    // Reassign the descriptor heap pointers
    m_descriptorHeapCopyable = nullptr;
    m_descriptorHeapShaderVisible = nullptr;

    m_descriptorHeapCopyable = newDescriptorHeapCopyable;
    m_descriptorHeapShaderVisible = newDescriptorHeapShaderVisible;

    // Update the heap start handles
    m_cpuHeapStart = m_descriptorHeapShaderVisible->GetCPUDescriptorHandleForHeapStart();
    m_gpuHeapStart = m_descriptorHeapShaderVisible->GetGPUDescriptorHandleForHeapStart();

    // Update the capacity
    m_capacity *= 2;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorVector::GetCPUHandleAt(UINT index) const noexcept
{
    TINY_CORE_ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_cpuHeapStart);
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorVector::GetGPUHandleAt(UINT index) const noexcept
{
    TINY_CORE_ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_gpuHeapStart);
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorVector::GetCPUCopyableHandleAt(UINT index) const noexcept
{
    TINY_CORE_ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_descriptorHeapCopyable->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}
D3D12_GPU_DESCRIPTOR_HANDLE DescriptorVector::GetGPUCopyableHandleAt(UINT index) const noexcept
{
    TINY_CORE_ASSERT(index < m_capacity, "Index is too large");
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_descriptorHeapCopyable->GetGPUDescriptorHandleForHeapStart());
    handle.Offset(index, m_handleIncrementSize);
    return handle;
}

unsigned int DescriptorVector::EmplaceBackShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
    TINY_CORE_ASSERT(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Invalid to create a Shader Resource View if the type is not D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV");

    // Increment the count
    ++m_count;

    // If we are over capacity, resize the descriptor heaps
    if (m_count > m_capacity)
        DoubleTheCapacity();

    // Get the handles to the two descriptor heaps
    D3D12_CPU_DESCRIPTOR_HANDLE handleToCopyable = GetCPUCopyableHandleAt(m_count - 1);
    D3D12_CPU_DESCRIPTOR_HANDLE handleToShaderVisible = GetCPUHandleAt(m_count - 1);

    // Create the Shader Resource Views in both heaps
    m_deviceResources->GetDevice()->CreateShaderResourceView(pResource, desc, handleToCopyable);
    m_deviceResources->GetDevice()->CreateShaderResourceView(pResource, desc, handleToShaderVisible);
    
    // Return the index to the shader resource view
    return m_count - 1;
}



}