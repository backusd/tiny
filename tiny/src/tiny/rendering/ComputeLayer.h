#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "ComputeItem.h"
#include "RootSignature.h"
#include "tiny/utils/Timer.h"
#include "tiny/Engine.h"

namespace tiny
{
class ComputeLayer
{
public:
	ComputeLayer(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources),
		RootSignature(nullptr),
		PipelineState(nullptr)
	{}
	ComputeLayer(ComputeLayer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		PreWork(rhs.PreWork),
		ComputeItems(std::move(rhs.ComputeItems)),
		PipelineState(rhs.PipelineState),
		Name(std::move(rhs.Name))
	{}
	ComputeLayer& operator=(ComputeLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		PreWork = rhs.PreWork;
		ComputeItems = std::move(rhs.ComputeItems);
		PipelineState = rhs.PipelineState;
		Name = std::move(rhs.Name);
		return *this;
	}
	~ComputeLayer() noexcept
	{
		// Attempt to remove the compute layer from the engine (Note, this will only remove "compute update" layers,
		// NOT compute layers that are part of a render pass)
		Engine::RemoveComputeUpdateLayer(this);
	}

	inline void SetPSO(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PipelineState))
		);
	}

	// PreWork needs to return a bool: false -> signals early exit (i.e. do not call Dispatch for this RenderLayer)
	// Also, because a ComputeLayer can be executed during the Update phase, it can get access to the Timer. However, 
	// if the ComputeLayer is execute during a RenderPass, then it will NOT have access to the timer and the timer 
	// parameter will be nullptr
	std::function<bool(const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int)> PreWork = [](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) { return true; };
	std::function<void(const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int)> PostWork = [](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) { };

	// Shared pointer because root signatures may be shared
	std::shared_ptr<RootSignature> RootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;

	std::vector<ComputeItem> ComputeItems;

	// Name (for debug/profiling purposes)
	std::string Name = "Unnamed ComputeLayer";

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	ComputeLayer(const ComputeLayer&) noexcept = delete;
	ComputeLayer& operator=(const ComputeLayer&) noexcept = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;
};
}