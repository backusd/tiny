#include "tiny-pch.h"
#include "FrameResource.h"
#include "tiny/DeviceResources.h"

namespace tiny
{
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
    GFX_THROW_INFO(
        device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
        )
    );

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
}

}