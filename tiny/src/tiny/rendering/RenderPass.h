#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/Engine.h"
#include "tiny/rendering/RootSignature.h"
#include "tiny/rendering/RootConstantBufferView.h"
#include "tiny/rendering/RenderPassLayer.h"
#include "tiny/utils/Timer.h"



namespace tiny
{
class RenderPass
{
public:
	RenderPass() noexcept
	{
		// Register the RenderPass with the Engine
		Engine::AddRenderPass(this);
	}
	~RenderPass() noexcept
	{
		// Unregister the RenderPass with the Engine
		Engine::RemoveRenderPass(this);
	}

	void Update(const Timer& timer, int frameIndex)
	{
		// Loop over the constant buffer views to update them
		for (auto& rcbv : ConstantBufferViews)
			rcbv.Update(timer, nullptr, frameIndex);
	}

	// Function pointers for Pre/Post-Work/Update
	// NOTE: You ARE allowed to assign a lambda to a function pointer as long as the lambda does NOT capture anything
	void (*PreWork)(RenderPass*, ID3D12GraphicsCommandList*) = [](RenderPass*, ID3D12GraphicsCommandList*) {};
	void (*PostWork)(RenderPass*, ID3D12GraphicsCommandList*) = [](RenderPass*, ID3D12GraphicsCommandList*) {};

	// Shared pointer because root signatures may be shared
	std::shared_ptr<RootSignature> RootSignature = nullptr;

	// 0+ constant buffer views for per-pass constants
	std::vector<RootConstantBufferView> ConstantBufferViews;

	// 1+ render layers
	std::vector<RenderPassLayer> RenderPassLayers;

private:
	RenderPass(const RenderPass& rhs) = delete;
	RenderPass& operator=(const RenderPass& rhs) = delete;
	
	// NOTE: We need to make RenderPass immovable because the constructor passes the 'this' pointer
	//       to the engine so that this object can be tracked. However, if the RenderPass is later
	//       moved, the 'this' pointer will no longer be valid. 
	//		 See: https://stackoverflow.com/questions/28492326/c11-does-a-move-operation-change-the-address#:~:text=Short%20answer%3A%20no%2C%20the%20address,not%20be%20a%20useful%20state.
	RenderPass(RenderPass&&) = delete;
	RenderPass& operator=(RenderPass&& rhs) = delete;
};
}