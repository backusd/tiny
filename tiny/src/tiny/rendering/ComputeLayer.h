#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "ComputeItem.h"
#include "RootSignature.h"


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
	~ComputeLayer() noexcept = default;

	inline void SetPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&PipelineState))
		);
	}

	std::function<void(const ComputeLayer&, ID3D12GraphicsCommandList*)> PreWork = [](const ComputeLayer&, ID3D12GraphicsCommandList*) {};

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