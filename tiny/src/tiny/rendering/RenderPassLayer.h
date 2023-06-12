#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "RenderItem.h"
#include "MeshGroup.h"


namespace tiny
{
class RenderPassLayer
{
public:
	RenderPassLayer(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources),
		PipelineState(nullptr),
		Topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
		Meshes(nullptr)
	{}
	RenderPassLayer(RenderPassLayer&& rhs) noexcept :
		m_deviceResources(rhs.m_deviceResources),
		RenderItems(std::move(rhs.RenderItems)),
		PipelineState(rhs.PipelineState),
		Topology(rhs.Topology),
		Meshes(std::move(rhs.Meshes))
	{}
	RenderPassLayer& operator=(RenderPassLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		RenderItems = std::move(rhs.RenderItems);
		PipelineState = rhs.PipelineState;
		Topology = rhs.Topology;
		Meshes = std::move(rhs.Meshes);
		return *this;
	}
	~RenderPassLayer() noexcept {}


	inline void SetPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&PipelineState))
		);
	}

	std::vector<RenderItem> RenderItems;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY Topology;
	std::unique_ptr<MeshGroup> Meshes;

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	RenderPassLayer(const RenderPassLayer&) noexcept = delete;
	RenderPassLayer& operator=(const RenderPassLayer&) noexcept = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;
};
}