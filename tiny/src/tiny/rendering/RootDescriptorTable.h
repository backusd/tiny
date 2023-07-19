#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"



namespace tiny
{
class RootDescriptorTable
{
public:
	RootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle) noexcept :
		RootParameterIndex(rootParameterIndex),
		DescriptorHandle(descriptorHandle)
	{}
	RootDescriptorTable(const RootDescriptorTable&) noexcept = default;
	RootDescriptorTable(RootDescriptorTable&&) noexcept = default;
	RootDescriptorTable& operator=(const RootDescriptorTable&) noexcept = default;
	RootDescriptorTable& operator=(RootDescriptorTable&&) noexcept = default;
	~RootDescriptorTable() noexcept {}

	std::function<void(const Timer&, int)> Update = [](const Timer&, int) {};
	UINT RootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle;
};
}