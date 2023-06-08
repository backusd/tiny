#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "ConstantBuffer.h"
#include "tiny/utils/Timer.h"


namespace tiny
{
class RenderItem;

class RootConstantBufferView
{
public:
	RootConstantBufferView(UINT rootParameterIndex, ConstantBuffer* cb) noexcept :
		RootParameterIndex(rootParameterIndex),
		ConstantBuffer(cb)
	{
		TINY_CORE_ASSERT(ConstantBuffer != nullptr, "ConstantBuffer should not be nullptr");
	}
	inline void Bind(ID3D12GraphicsCommandList* commandList, int frameIndex) const noexcept
	{
		commandList->SetGraphicsRootConstantBufferView(RootParameterIndex, ConstantBuffer->GetGPUVirtualAddress(frameIndex));
	}

	UINT RootParameterIndex;
	ConstantBuffer* ConstantBuffer;

	std::function<void(const Timer&, RenderItem*, int)> Update = [](const Timer&, RenderItem*, int) {};
};
}