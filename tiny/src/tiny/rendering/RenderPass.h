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
	RenderPass(RenderPass&& rhs) noexcept :
		PreWork(rhs.PreWork),
		PostWork(rhs.PostWork),
		RootSignature(rhs.RootSignature),
		ConstantBufferViews(std::move(rhs.ConstantBufferViews)),
		RenderPassLayers(std::move(RenderPassLayers)),
		Name(std::move(rhs.Name))
	{
		LOG_CORE_WARN("{}", "RenderPass Move Constructor called, but this method has not been tested. Make sure updates to the Engine are correct");

		// Register the RenderPass with the Engine (don't explicitly remove the rhs object because its destructor should handle that)
		Engine::AddRenderPass(this);
	}
	RenderPass& operator=(RenderPass&& rhs) noexcept
	{
		LOG_CORE_WARN("{}", "RenderPass Move Assignment operator called, but this method has not been tested. Make sure updates to the Engine are correct");

		// Register the RenderPass with the Engine (don't explicitly remove the rhs object because its destructor should handle that)
		Engine::AddRenderPass(this);

		PreWork = rhs.PreWork;
		PostWork = rhs.PostWork;
		RootSignature = rhs.RootSignature;
		ConstantBufferViews = std::move(rhs.ConstantBufferViews);
		RenderPassLayers = std::move(rhs.RenderPassLayers);
		Name = std::move(rhs.Name);

		return *this;
	}
	~RenderPass() noexcept
	{
		// Unregister the RenderPass with the Engine
		Engine::RemoveRenderPass(this);
	}

	void Update(const Timer& timer, int frameIndex)
	{
		// Loop over the constant buffer views to update per-pass constants
		for (auto& rcbv : ConstantBufferViews)
			rcbv.Update(timer, nullptr, frameIndex);
	}

	// Function pointers for Pre/Post-Work 
	std::function<void(RenderPass*, ID3D12GraphicsCommandList*)> PreWork = [](RenderPass*, ID3D12GraphicsCommandList*) {};
	std::function<void(RenderPass*, ID3D12GraphicsCommandList*)> PostWork = [](RenderPass*, ID3D12GraphicsCommandList*) {};

	// Shared pointer because root signatures may be shared
	std::shared_ptr<RootSignature> RootSignature = nullptr;

	// 0+ constant buffer views for per-pass constants
	std::vector<RootConstantBufferView> ConstantBufferViews;

	// 1+ render layers
	std::vector<RenderPassLayer> RenderPassLayers;

	// Name (for debug/profiling purposes)
	std::string Name = "Unnamed RenderPass";

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
};
}