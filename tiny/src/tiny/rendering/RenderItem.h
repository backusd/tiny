#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "FrameResource.h"
#include "MeshGeometry.h"
#include "Material.h"

#include "ConstantBuffer.h"

namespace tiny
{
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Constant Buffer to hold World Matrix and Tex transform
	std::unique_ptr<ConstantBuffer<ObjectConstants>> ObjectConstantBuffer = nullptr;

	// Hold a unique_ptr to the material for this render item. In theory, multiple render items
	// could definitely share materials. However, adding the ability to share significantly increases
	// code complexity and maintenance. And in reality, the majority of objects will be unique and not
	// share materials.
	std::unique_ptr<Material> material = nullptr;
	std::unique_ptr<ConstantBuffer<MaterialConstants>> materialConstantBuffer = nullptr;
	int materialNumFramesDirty = gNumFrameResources; // <-- Set this as dirty so the constant buffer gets updated immediately

	MeshGeometry* Geo = nullptr;
	//Material* Mat = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

}