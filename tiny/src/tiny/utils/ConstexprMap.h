#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"


namespace tiny
{
namespace utility
{

template <typename Key, typename Value, std::size_t Size>
struct ConstexprMap
{
	std::array<std::pair<Key, Value>, Size> m_data;

	ND constexpr Value at(const Key& key) const
	{
		const auto itr = std::find_if(std::begin(m_data), std::end(m_data), [&key](const auto& v) { return v.first == key; });
		if (itr != std::end(m_data))
		{
			return itr->second;
		}
		else
		{
			throw std::range_error("Not found");
		}
	}
};

} // namespace utility
} // namespace tiny