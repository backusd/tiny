#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

namespace tiny
{
namespace utility
{
    UINT TINY_API CalcConstantBufferByteSize(UINT byteSize) noexcept;

    Microsoft::WRL::ComPtr<ID3D12Resource> TINY_API CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

}
}