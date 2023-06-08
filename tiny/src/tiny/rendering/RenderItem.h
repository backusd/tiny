#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "FrameResource.h"
#include "MeshGeometry.h"
#include "Material.h"
#include "ConstantBuffer.h"
#include "RootConstantBufferView.h"
#include "RootDescriptorTable.h"
#include "tiny/utils/Timer.h"

namespace tiny
{
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

class RenderItem
{
public:
	RenderItem() noexcept
	{
		// Register the RenderItem with the Engine
		Engine::AddRenderItem(this);
	}
	// Because we are storing render items in a vector, it is possible that even when using emplace_back, the vector
	// may need to grow, and therefore, need to move the contents of all of the render items. So we must implement
	// move operations. However, moving the object causes the 'this' pointer to change, so we must update the Engine
	RenderItem(RenderItem&& rhs) noexcept
	{
		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddRenderItem(this);

		ConstantBufferViews = std::move(rhs.ConstantBufferViews);
		DescriptorTables = std::move(rhs.DescriptorTables);
		NumFramesDirty = rhs.NumFramesDirty;
		World = rhs.World;
		TexTransform = rhs.TexTransform;
		material = std::move(rhs.material);
		materialNumFramesDirty = rhs.materialNumFramesDirty;
		texture = rhs.texture;
		submeshIndex = rhs.submeshIndex;
	}
	RenderItem& operator=(RenderItem&& rhs) noexcept
	{
		// I don't think we need to explicitly call RemoveRenderItem on the rhs object because its destructor should do that
		Engine::AddRenderItem(this);

		ConstantBufferViews = std::move(rhs.ConstantBufferViews);
		DescriptorTables = std::move(rhs.DescriptorTables);
		NumFramesDirty = rhs.NumFramesDirty;
		World = rhs.World;
		TexTransform = rhs.TexTransform;
		material = std::move(rhs.material);
		materialNumFramesDirty = rhs.materialNumFramesDirty;
		texture = rhs.texture;
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

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each frame in flight, we have to apply the
	// update to each frame in flight. Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Hold a unique_ptr to the material for this render item. In theory, multiple render items
	// could definitely share materials. However, adding the ability to share significantly increases
	// code complexity and maintenance. And in reality, the majority of objects will be unique and not
	// share materials.
	std::unique_ptr<Material> material = nullptr;
	int materialNumFramesDirty = gNumFrameResources; // <-- Set this as dirty so the constant buffer gets updated immediately

	// Index into the vector of all Textures
	unsigned int texture = 0;

	// The PSO will hold and bind the mesh-group for all of the render items it will render.
	// Here, we just need to keep track of which submesh index the render item references
	unsigned int submeshIndex = 0;


private:
	RenderItem(const RenderItem& rhs) = delete;
	RenderItem& operator=(const RenderItem& rhs) = delete;
};

}