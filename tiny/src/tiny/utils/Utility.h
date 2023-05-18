#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

namespace tiny
{
	namespace utility
	{
        UINT TINY_API CalcConstantBufferByteSize(UINT byteSize) noexcept;

        Microsoft::WRL::ComPtr<ID3DBlob> TINY_API LoadBinary(const std::wstring& filename);

        Microsoft::WRL::ComPtr<ID3D12Resource> TINY_API CreateDefaultBuffer(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* cmdList,
            const void* initData,
            UINT64 byteSize,
            Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

        Microsoft::WRL::ComPtr<ID3DBlob> TINY_API CompileShader(
            const std::wstring& filename,
            const D3D_SHADER_MACRO* defines,
            const std::string& entrypoint,
            const std::string& target);
	}
}