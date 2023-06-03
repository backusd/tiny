#include "tiny-pch.h"
#include "FrameResource.h"
#include "tiny/DeviceResources.h"

namespace tiny
{
FrameResource::FrameResource(ID3D12Device* device, UINT waveVertCount)
{
    GFX_THROW_INFO(
        device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
        )
    );

    WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, waveVertCount, false);
}

}