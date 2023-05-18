#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/utils/DxgiInfoManager.h"
#include "exception/DeviceResourcesException.h"

namespace tiny
{
#pragma warning( push )
#pragma warning( disable : 4251 )
class TINY_API DeviceResources
{
public:
	DeviceResources(HWND hWnd);
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources& operator=(const DeviceResources&) = delete;

	void OnResize();

	// Getters
	ND inline bool Get4xMsaaState() const noexcept { return m_4xMsaaState; }
	ND inline float AspectRatio() const noexcept { return static_cast<float>(m_width) / m_height; }


	// Setters
	void Set4xMsaaState(bool value);

private:
	void InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	ND std::tuple<int, int> GetWindowHeightWidth() const;



	HWND m_hWnd;
	int m_height;
	int m_width;

	// Set true to use 4X MSAA (§4.1.8).  The default is false.
	bool      m_4xMsaaState = false;    // 4X MSAA enabled
	UINT      m_4xMsaaQuality = 0;      // quality level of 4X MSAA

	Microsoft::WRL::ComPtr<IDXGIFactory4>  m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device>   m_d3dDevice;

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

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

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