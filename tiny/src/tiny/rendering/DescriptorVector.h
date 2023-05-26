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

    ND D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAt(UINT index) const;
    ND D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAt(UINT index) const;
    ND inline ID3D12DescriptorHeap* GetRawHeapPointer() const noexcept { return m_descriptorHeapShaderVisible.Get(); }

    unsigned int PushBackShaderResourceView(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);



private:
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCopyableHandleAt(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCopyableHandleAt(UINT index) const;
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



/*
Example wrapper from: https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-descriptor-heaps

class CDescriptorHeapWrapper
{
public:
    CDescriptorHeapWrapper() { memset(this, 0, sizeof(*this)); }

    HRESULT Create(
        ID3D12Device* pDevice,
        D3D12_DESCRIPTOR_HEAP_TYPE Type,
        UINT NumDescriptors,
        bool bShaderVisible = false)
    {
        D3D12_DESCRIPTOR_HEAP_DESC Desc;
        Desc.Type = Type;
        Desc.NumDescriptors = NumDescriptors;
        Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        HRESULT hr = pDevice->CreateDescriptorHeap(&Desc,
                               __uuidof(ID3D12DescriptorHeap),
                               (void**)&pDH);
        if (FAILED(hr)) return hr;

        hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
        hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();

        HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
        return hr;
    }
    operator ID3D12DescriptorHeap*() { return pDH; }

    D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index)
    {
        return hCPUHeapStart.MakeOffsetted(index,HandleIncrementSize);
    }
    D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index)
    {
        assert(Desc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        return hGPUHeapStart.MakeOffsetted(index,HandleIncrementSize);
    }
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    CComPtr<ID3D12DescriptorHeap> pDH;
    D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
    D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
    UINT HandleIncrementSize;
};
*/