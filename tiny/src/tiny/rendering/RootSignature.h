#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/Engine.h"



namespace tiny
{
class RootSignature
{
public:
	RootSignature(std::shared_ptr<DeviceResources> deviceResources, const D3D12_ROOT_SIGNATURE_DESC& desc) :
		m_deviceResources(deviceResources),
		m_rootSignature(nullptr)
	{
		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			LOG_ERROR("D3D12SerializeRootSignature() failed with message: {}", (char*)errorBlob->GetBufferPointer());
		}
		if (FAILED(hr))
			throw tiny::DeviceResourcesException(__LINE__, __FILE__, hr);

		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateRootSignature(
				0,
				serializedRootSig->GetBufferPointer(),
				serializedRootSig->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature)
			)
		);
	}

	ND inline ID3D12RootSignature* Get() const noexcept { return m_rootSignature.Get(); }


private:
	std::shared_ptr<DeviceResources> m_deviceResources;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

};
}