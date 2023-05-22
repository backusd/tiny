#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"

namespace tiny
{
	class DepthStencilState
	{
	public:
		DepthStencilState()
		{
			m_desc.DepthEnable = TRUE;
			m_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			m_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			m_desc.StencilEnable = FALSE;
			m_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			m_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			m_desc.FrontFace = defaultStencilOp;
			m_desc.BackFace = defaultStencilOp;
		}
		DepthStencilState(const DepthStencilState&) noexcept = delete;
		DepthStencilState& operator=(const DepthStencilState&) noexcept = delete;
		~DepthStencilState() noexcept {}

		ND inline D3D12_DEPTH_STENCIL_DESC GetDepthStencilDesc() const noexcept { return m_desc; }

		// Setters
		inline void SetDepthEnabled(BOOL enabled) noexcept { m_desc.DepthEnable = enabled; }
		inline void SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask) noexcept { m_desc.DepthWriteMask = mask; }
		inline void SetDepthFunc(D3D12_COMPARISON_FUNC func) noexcept { m_desc.DepthFunc = func; }
		inline void SetStencilEnabled(BOOL enabled) noexcept { m_desc.StencilEnable = enabled; }
		inline void SetStencilReadMask(UINT8 mask) noexcept { m_desc.StencilReadMask = mask; }
		inline void SetStencilWriteMask(UINT8 mask) noexcept { m_desc.StencilWriteMask = mask; }

		inline void SetFrontFaceStencilFailOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.FrontFace.StencilFailOp = stencilOp; }
		inline void SetFrontFaceStencilDepthFailOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.FrontFace.StencilDepthFailOp = stencilOp; }
		inline void SetFrontFaceStencilPassOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.FrontFace.StencilPassOp = stencilOp; }
		inline void SetFrontFaceStencilFunc(D3D12_COMPARISON_FUNC func) noexcept { m_desc.FrontFace.StencilFunc = func; }

		inline void SetBackFaceStencilFailOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.BackFace.StencilFailOp = stencilOp; }
		inline void SetBackFaceStencilDepthFailOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.BackFace.StencilDepthFailOp = stencilOp; }
		inline void SetBackFaceStencilPassOp(D3D12_STENCIL_OP stencilOp) noexcept { m_desc.BackFace.StencilPassOp = stencilOp; }
		inline void SetBackFaceStencilFunc(D3D12_COMPARISON_FUNC func) noexcept { m_desc.BackFace.StencilFunc = func; }

	private:
		D3D12_DEPTH_STENCIL_DESC m_desc;
	};
}