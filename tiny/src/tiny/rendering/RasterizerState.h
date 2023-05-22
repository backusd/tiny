#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

namespace tiny
{
	class RasterizerState
	{
	public:
		RasterizerState()
		{
			m_desc.FillMode = D3D12_FILL_MODE_SOLID;
			m_desc.CullMode = D3D12_CULL_MODE_BACK;
			m_desc.FrontCounterClockwise = FALSE;
			m_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			m_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			m_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			m_desc.DepthClipEnable = TRUE;
			m_desc.MultisampleEnable = FALSE;
			m_desc.AntialiasedLineEnable = FALSE;
			m_desc.ForcedSampleCount = 0;
			m_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}
		RasterizerState(const RasterizerState&) noexcept = delete;
		RasterizerState& operator=(const RasterizerState&) noexcept = delete;
		~RasterizerState() noexcept {}

		ND inline D3D12_RASTERIZER_DESC GetRasterizerDesc() const noexcept { return m_desc;	}

		// Setters
		inline void SetFillMode(D3D12_FILL_MODE mode) noexcept { m_desc.FillMode = mode; }
		inline void SetCullMode(D3D12_CULL_MODE mode) noexcept { m_desc.CullMode = mode; }
		inline void SetFrontCounterClockwise(BOOL fcc) noexcept { m_desc.FrontCounterClockwise = fcc; }
		inline void SetDepthBias(INT bias) noexcept { m_desc.DepthBias = bias; }
		inline void SetDepthBiasClamp(FLOAT clamp) noexcept { m_desc.DepthBiasClamp = clamp; }
		inline void SetSlopeScaledDepthBias(FLOAT bias) noexcept { m_desc.SlopeScaledDepthBias = bias; }
		inline void SetDepthClipEnabled(BOOL enabled) noexcept { m_desc.DepthClipEnable = enabled; }
		inline void SetMultisampleEnabled(BOOL enabled) noexcept { m_desc.MultisampleEnable = enabled; }
		inline void SetAntialiasedLineEnabled(BOOL enabled) noexcept { m_desc.AntialiasedLineEnable = enabled; }
		inline void SetForcedSampleCount(UINT count) noexcept { m_desc.ForcedSampleCount = count; }
		inline void SetConservativeRaster(D3D12_CONSERVATIVE_RASTERIZATION_MODE mode) noexcept { m_desc.ConservativeRaster = mode; }


	private:
		D3D12_RASTERIZER_DESC m_desc;
	};
}