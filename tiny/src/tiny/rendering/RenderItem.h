#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "ConstantBuffer.h"
#include "RootConstantBufferView.h"
#include "RootDescriptorTable.h"
#include "tiny/utils/Timer.h"

namespace tiny
{
class RenderItem
{
public:
	RenderItem() noexcept
	{
		// Register the RenderItem with the Engine (used by the Engine to call RenderItem::Update)
		Engine::AddRenderItem(this);
	}
	// Because we are storing render items in a vector, it is possible that even when using emplace_back, the vector
	// may need to grow, and therefore, need to move the contents of all of the render items. So we must implement
	// move operations. However, moving the object causes the 'this' pointer to change, so we must update the Engine
	RenderItem(RenderItem&& rhs) noexcept :
		ConstantBufferViews(std::move(rhs.ConstantBufferViews)),
		DescriptorTables(std::move(rhs.DescriptorTables)),
		submeshIndex(rhs.submeshIndex)
	{
		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddRenderItem(this);
	}
	RenderItem& operator=(RenderItem&& rhs) noexcept
	{
		LOG_CORE_WARN("{}", "RenderItem Move Assignment operator called, but this method has not been tested. Make sure updates to the Engine are correct. NOTE: I have tested the Move Constructor, so I believe the Move Assignment operator should be fine...");

		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddRenderItem(this);

		ConstantBufferViews = std::move(rhs.ConstantBufferViews);
		DescriptorTables = std::move(rhs.DescriptorTables);
		submeshIndex = rhs.submeshIndex;

		return *this;
	}
	~RenderItem() noexcept
	{
		// Unregister the RenderItem with the Engine
		Engine::RemoveRenderItem(this);
	}

	void Update(const Timer& timer, int frameIndex)
	{
		// Loop over the constant buffer views and descriptor tables to update them
		for (auto& rcbv : ConstantBufferViews)
			rcbv.Update(timer, this, frameIndex);

		for (auto& dt : DescriptorTables)
			dt.Update(timer, frameIndex);
	}

	// 0+ constant buffer views for per-item constants
	std::vector<RootConstantBufferView> ConstantBufferViews;

	// 0+ descriptor tables for per-item resources
	std::vector<RootDescriptorTable> DescriptorTables;

	// The PSO will hold and bind the mesh-group for all of the render items it will render.
	// Here, we just need to keep track of which submesh index the render item references
	unsigned int submeshIndex = 0;

private:
	RenderItem(const RenderItem&) = delete;
	RenderItem& operator=(const RenderItem&) = delete;
};

}