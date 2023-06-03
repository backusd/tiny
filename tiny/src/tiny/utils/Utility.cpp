#include "tiny-pch.h"
#include "Utility.h"
#include "tiny/DeviceResources.h"
#include "tiny/Log.h"

#include <fstream>

using Microsoft::WRL::ComPtr;

namespace tiny
{
namespace utility
{
    UINT CalcConstantBufferByteSize(UINT byteSize) noexcept
    {
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
        return (byteSize + 255) & ~255;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
    {
        ComPtr<ID3D12Resource> defaultBuffer;

        // Create the actual default buffer resource.
        auto _p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto _d = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
        GFX_THROW_INFO(device->CreateCommittedResource(
            &_p,
            D3D12_HEAP_FLAG_NONE,
            &_d,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

        // In order to copy CPU memory data into our default buffer, we need to create
        // an intermediate upload heap. 
        auto _p2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto _d2 = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
        GFX_THROW_INFO(device->CreateCommittedResource(
            &_p2,
            D3D12_HEAP_FLAG_NONE,
            &_d2,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


        // Describe the data we want to copy into the default buffer.
        D3D12_SUBRESOURCE_DATA subResourceData = {};
        subResourceData.pData = initData;
        subResourceData.RowPitch = byteSize;
        subResourceData.SlicePitch = subResourceData.RowPitch;

        // Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
        // will copy the CPU memory into the intermediate upload heap. Then, using ID3D12CommandList::CopySubresourceRegion,
        // the intermediate upload heap data will be copied to mBuffer.
        auto _b = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(1, &_b);
        UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
        auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        cmdList->ResourceBarrier(1, &_b2);

        // Note: uploadBuffer has to be kept alive after the above function calls because
        // the command list has not been executed yet that performs the actual copy.
        // The caller can Release the uploadBuffer after it knows the copy has been executed.

        return defaultBuffer;
    }

}
}