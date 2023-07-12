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
		Meshes(std::move(rhs.Meshes)),
		Name(std::move(rhs.Name))
	{}
	RenderPassLayer& operator=(RenderPassLayer&& rhs) noexcept
	{
		m_deviceResources = rhs.m_deviceResources;
		RenderItems = std::move(rhs.RenderItems);
		PipelineState = rhs.PipelineState;
		Topology = rhs.Topology;
		Meshes = std::move(rhs.Meshes);
		Name = std::move(rhs.Name);
		return *this;
	}
	~RenderPassLayer() noexcept {}


	inline void SetPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&PipelineState))
		);
	}

	void RemoveRenderItem(RenderItem* ri)
	{
		if (ri != nullptr) LIKELY
		{
			for (unsigned int iii = 0; iii < RenderItems.size(); ++iii)
			{
				if (&RenderItems[iii] == ri) UNLIKELY
				{
					RenderItems.erase(RenderItems.begin() + iii);
					break;
				}
			}
		}
	}

	std::function<void(const RenderPassLayer&, ID3D12GraphicsCommandList*)> PreWork = [](const RenderPassLayer&, ID3D12GraphicsCommandList*) {};

	std::vector<RenderItem> RenderItems;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY Topology;
	std::shared_ptr<MeshGroup> Meshes; // shared_ptr because it is possible (if not likely) that different layers will want to reference the same mesh

	// Name (for debug/profiling purposes)
	std::string Name = "Unnamed RenderPassLayer";

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	RenderPassLayer(const RenderPassLayer&) noexcept = delete;
	RenderPassLayer& operator=(const RenderPassLayer&) noexcept = delete;

	std::shared_ptr<DeviceResources> m_deviceResources;
};
}