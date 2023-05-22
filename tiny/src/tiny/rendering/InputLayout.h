#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"

namespace tiny
{
class InputLayout
{
public:
	InputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputs)
	{
		for (const auto& elem : inputs)
			m_inputLayout.push_back(elem);
	}
	InputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>&& inputs) :
		m_inputLayout(std::move(inputs))
	{}
	InputLayout(const InputLayout&) noexcept = delete;
	InputLayout& operator=(const InputLayout&) noexcept = delete;
	~InputLayout() noexcept {}

	ND inline D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc() const noexcept { return { m_inputLayout.data(), (UINT)m_inputLayout.size() }; }


private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
};
}