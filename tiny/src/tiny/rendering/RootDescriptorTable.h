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

	inline void Bind(ID3D12GraphicsCommandList* commandList) const noexcept
	{
		commandList->SetGraphicsRootDescriptorTable(RootParameterIndex, DescriptorHandle);
	}

	std::function<void(const Timer&, int)> Update = [](const Timer&, int) {};


	UINT RootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle;
};
}