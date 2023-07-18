#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "ConstantBuffer.h"
#include "RootConstantBufferView.h"
#include "RootDescriptorTable.h"
#include "tiny/utils/Timer.h"

namespace tiny
{
class ComputeItem
{
public:
	ComputeItem() noexcept
	{
		// Register the ComputeItem with the Engine (used by the Engine to call ComputeItem::Update)
		Engine::AddComputeItem(this);
	}
	// Because we are storing Compute Items in a vector, it is possible that even when using emplace_back, the vector
	// may need to grow, and therefore, need to move the contents of all of the compute items. So we must implement
	// move operations. However, moving the object causes the 'this' pointer to change, so we must update the Engine
	ComputeItem(ComputeItem&& rhs) noexcept :
		ConstantBufferViews(std::move(rhs.ConstantBufferViews)),
		DescriptorTables(std::move(rhs.DescriptorTables)),
		ThreadGroupCountX(rhs.ThreadGroupCountX),
		ThreadGroupCountY(rhs.ThreadGroupCountY),
		ThreadGroupCountZ(rhs.ThreadGroupCountZ)
	{
		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddComputeItem(this);
	}
	ComputeItem& operator=(ComputeItem&& rhs) noexcept
	{
		LOG_CORE_WARN("{}", "ComputeItem Move Assignment operator called, but this method has not been tested. Make sure updates to the Engine are correct. NOTE: I have tested the Move Constructor, so I believe the Move Assignment operator should be fine...");

		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddComputeItem(this);

		ConstantBufferViews = std::move(rhs.ConstantBufferViews);
		DescriptorTables = std::move(rhs.DescriptorTables);
		ThreadGroupCountX = rhs.ThreadGroupCountX;
		ThreadGroupCountY = rhs.ThreadGroupCountY;
		ThreadGroupCountZ = rhs.ThreadGroupCountZ;

		return *this;
	}
	~ComputeItem() noexcept
	{
		// Unregister the ComputeItem with the Engine
		Engine::RemoveComputeItem(this);
	}

	// 0+ constant buffer views for per-item constants
	std::vector<RootConstantBufferView> ConstantBufferViews;

	// 0+ descriptor tables for per-item resources
	std::vector<RootDescriptorTable> DescriptorTables;

	// Thread group counts to use when call Dispatch
	unsigned int ThreadGroupCountX = 1;
	unsigned int ThreadGroupCountY = 1;
	unsigned int ThreadGroupCountZ = 1;

private:
	ComputeItem(const ComputeItem&) = delete;
	ComputeItem& operator=(const ComputeItem&) = delete;
};
}