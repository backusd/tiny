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
	RootConstantBufferView(const RootConstantBufferView&) noexcept = default;
	RootConstantBufferView(RootConstantBufferView&&) noexcept = default;
	RootConstantBufferView& operator=(const RootConstantBufferView&) noexcept = default;
	RootConstantBufferView& operator=(RootConstantBufferView&&) noexcept = default;
	~RootConstantBufferView() noexcept {}


	UINT RootParameterIndex;
	ConstantBuffer* ConstantBuffer;
	std::function<void(const Timer&, int)> Update = [](const Timer&, int) {};
};
}