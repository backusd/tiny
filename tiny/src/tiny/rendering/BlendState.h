#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"

namespace tiny
{
	class BlendState
	{
	public:
		BlendState()
		{
			m_desc.AlphaToCoverageEnable = FALSE;
			m_desc.IndependentBlendEnable = FALSE;

			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
				FALSE,FALSE,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};

			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				m_desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}
		BlendState(const BlendState&) noexcept = delete;
		BlendState& operator=(const BlendState&) noexcept = delete;
		~BlendState() noexcept {}

		ND inline D3D12_BLEND_DESC GetBlendDesc() const noexcept { return m_desc; }

		// Setters
		inline void SetAlphaToCoverageEnabled(BOOL enabled) noexcept { m_desc.AlphaToCoverageEnable = enabled; }
		inline void SetIndependentBlendEnabled(BOOL enabled) noexcept { m_desc.IndependentBlendEnable = enabled; }

		inline void SetRenderTargetBlendEnabled(BOOL enabled, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].BlendEnable = enabled;
		}
		inline void SetRenderTargetLogicOpEnabled(BOOL enabled, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].LogicOpEnable = enabled;
		}
		inline void SetRenderTargetSrcBlend(D3D12_BLEND blend, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].SrcBlend = blend;
		}
		inline void SetRenderTargetDestBlend(D3D12_BLEND blend, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].DestBlend = blend;
		}
		inline void SetRenderTargetBlendOp(D3D12_BLEND_OP blendOp, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].BlendOp = blendOp;
		}
		inline void SetRenderTargetSrcBlendAlpha(D3D12_BLEND blend, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].SrcBlendAlpha = blend;
		}
		inline void SetRenderTargetDestBlendAlpha(D3D12_BLEND blend, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].DestBlendAlpha = blend;
		}
		inline void SetRenderTargetBlendOpAlpha(D3D12_BLEND_OP blendOp, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].BlendOpAlpha = blendOp;
		}
		inline void SetRenderTargetLogicOp(D3D12_LOGIC_OP logicOp, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].LogicOp = logicOp;
		}
		inline void SetRenderTargetWriteMask(UINT8 mask, unsigned int renderTargetIndex = 0)
		{
			TINY_CORE_ASSERT(renderTargetIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "renderTargetIndex is too large");
			m_desc.RenderTarget[renderTargetIndex].RenderTargetWriteMask = mask;
		}

	private:
		D3D12_BLEND_DESC m_desc;
	};
}