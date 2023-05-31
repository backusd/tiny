#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/utils/Utility.h"
#include "tiny/DeviceResources.h"

namespace tiny
{
template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        m_isConstantBuffer(isConstantBuffer)
    {
        m_elementByteSize = sizeof(T);

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
        if (isConstantBuffer)
            m_elementByteSize = utility::CalcConstantBufferByteSize(sizeof(T));

        auto _p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto _d = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount);

        GFX_THROW_INFO(
            device->CreateCommittedResource(
                &_p,
                D3D12_HEAP_FLAG_NONE,
                &_d,
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

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (m_uploadBuffer != nullptr)
            m_uploadBuffer->Unmap(0, nullptr);

        m_mappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return m_uploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
    BYTE* m_mappedData = nullptr;

    UINT m_elementByteSize = 0;
    bool m_isConstantBuffer = false;
};
}