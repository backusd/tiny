#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/utils/DxgiInfoManager.h"
#include "exception/DeviceResourcesException.h"

namespace tiny
{
#pragma warning( push )
#pragma warning( disable : 4251 )
class TINY_API DeviceResources
{
public:
	DeviceResources(); // Constructor to use in UWP when we don't have an HWND
	DeviceResources(HWND hWnd, int height, int width); // Constructor to use when building for Win32 and we do have an HWND
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources& operator=(const DeviceResources&) = delete;


	void OnResize(int height, int width);
	void FlushCommandQueue();

	// Getters
	ND inline bool Get4xMsaaState() const noexcept { return m_4xMsaaState; }
	ND inline float AspectRatio() const noexcept { return static_cast<float>(m_width) / m_height; }
	ND inline ID3D12GraphicsCommandList* GetCommandList() const noexcept { return m_commandList.Get(); }
	ND inline ID3D12CommandAllocator* GetCommandAllocator() const noexcept { return m_directCmdListAlloc.Get(); }
	ND inline ID3D12CommandQueue* GetCommandQueue() const noexcept { return m_commandQueue.Get(); }
	ND inline ID3D12Device* GetDevice() const noexcept { return m_d3dDevice.Get(); }
	ND inline DXGI_FORMAT GetBackBufferFormat() const noexcept { return m_backBufferFormat; }
	ND inline DXGI_FORMAT GetDepthStencilFormat() const noexcept { return m_depthStencilFormat; }
	ND inline bool MsaaEnabled() const noexcept { return m_4xMsaaState; }
	ND inline UINT MsaaQuality() const noexcept { return m_4xMsaaQuality; }
	ND inline IDXGISwapChain1* GetSwapChain() const noexcept { return m_swapChain.Get(); }

	ND ID3D12Resource* CurrentBackBuffer() const noexcept;
	ND D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	ND D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	ND inline int GetHeight() const noexcept { return m_height; }
	ND inline int GetWidth() const noexcept { return m_width; }

	ND inline ID3D12Fence* GetFence() const noexcept { return m_fence.Get(); }
	ND inline UINT64 GetCurrentFenceValue() const noexcept { return m_currentFence; }

	ND inline UINT GetRTVDescriptorSize() const noexcept { return m_rtvDescriptorSize; }
	ND inline UINT GetDSVDescriptorSize() const noexcept { return m_dsvDescriptorSize; }
	ND inline UINT GetCBVSRVUAVDescriptorSize() const noexcept { return m_cbvSrvUavDescriptorSize; }

	// Setters
	void Set4xMsaaState(bool value);

	void Present();

private:
	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();



	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	HWND m_hWnd;

	int m_height;
	int m_width;

	// Set true to use 4X MSAA (�4.1.8).  The default is false.
	bool      m_4xMsaaState = false;    // 4X MSAA enabled
	UINT      m_4xMsaaQuality = 0;      // quality level of 4X MSAA

	Microsoft::WRL::ComPtr<IDXGIFactory4>	m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device>	m_d3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_currentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_directCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;

	static const int SwapChainBufferCount = 2;
	int m_currBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE m_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


#if defined(_DEBUG)
public:
	ND inline static DxgiInfoManager& GetInfoManager() noexcept { return m_infoManager; }
private:
	static DxgiInfoManager m_infoManager;
#endif
};
#pragma warning( pop )
}