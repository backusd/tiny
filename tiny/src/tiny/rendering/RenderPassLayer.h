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
	std::shared_ptr<DeviceResources> m_deviceResources;
};
}