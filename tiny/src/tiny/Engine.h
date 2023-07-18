#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/utils/Timer.h"



namespace tiny
{
class ConstantBuffer;
class RenderPass;
class RenderItem;
class ComputeItem;
class MeshGroup;
class DynamicMeshGroup;
class Texture;
class TextureManager;

class Engine
{
public:
	// NOTE: ONLY these methods should be public because these are the only ones that the client 
	//       needs to actually call. Every other method can be private because we make all of the
	//       render classes friends of Engine
	static inline void Init(std::shared_ptr<DeviceResources> deviceResources) { Get().InitImpl(deviceResources); }
	static inline void Update(const Timer& timer) { Get().UpdateImpl(timer); }
	static inline void Render() { Get().RenderImpl(); }
	static inline void Present() { Get().PresentImpl(); }
	static inline void SetViewport(const D3D12_VIEWPORT& vp) noexcept { Get().SetViewportImpl(vp); }
	static inline void SetScissorRect(const D3D12_RECT& rect) noexcept { Get().SetScissorRectImpl(rect); }
	ND static inline int GetCurrentFrameIndex() noexcept { return Get().GetCurrentFrameIndexImpl(); }

private:
	Engine() noexcept = default;
	Engine(const Engine& rhs) = delete;
	Engine& operator=(const Engine& rhs) = delete;

	static Engine& Get() noexcept
	{
		static Engine e;
		return e;
	}

	void InitImpl(std::shared_ptr<DeviceResources> deviceResources);
	void UpdateImpl(const Timer& timer);
	void RenderImpl();
	void PresentImpl();
	inline void SetViewportImpl(const D3D12_VIEWPORT& vp) noexcept { m_viewport = vp; }
	inline void SetScissorRectImpl(const D3D12_RECT& rect) noexcept { m_scissorRect = rect; }
	ND inline int GetCurrentFrameIndexImpl() const noexcept { return m_currentFrameIndex; }


	void CleanupResources() noexcept;
	static inline void DelayedDelete(Microsoft::WRL::ComPtr<ID3D12Resource> resource) noexcept { Get().DelayedDeleteImpl(resource); }
	void DelayedDeleteImpl(Microsoft::WRL::ComPtr<ID3D12Resource> resource) noexcept;


	static inline void AddRenderPass(RenderPass* pass) noexcept { Get().AddRenderPassImpl(pass); }
	static inline void RemoveRenderPass(RenderPass* pass) noexcept { Get().RemoveRenderPassImpl(pass); }
	static inline void AddRenderItem(RenderItem* item) noexcept { Get().AddRenderItemImpl(item); }
	static inline void RemoveRenderItem(RenderItem* item) noexcept { Get().RemoveRenderItemImpl(item); }
	static inline void AddComputeItem(ComputeItem* item) noexcept { Get().AddComputeItemImpl(item); }
	static inline void RemoveComputeItem(ComputeItem* item) noexcept { Get().RemoveComputeItemImpl(item); }
	static inline void AddDynamicMeshGroup(DynamicMeshGroup* mesh) noexcept { Get().AddDynamicMeshGroupImpl(mesh); }
	static inline void RemoveDynamicMeshGroup(DynamicMeshGroup* mesh) noexcept { Get().RemoveDynamicMeshGroupImpl(mesh); }
	
	inline void AddRenderPassImpl(RenderPass* pass) noexcept { m_renderPasses.push_back(pass); }
	void RemoveRenderPassImpl(RenderPass* pass) noexcept;
	void AddRenderItemImpl(RenderItem* item) noexcept;
	void RemoveRenderItemImpl(RenderItem* item) noexcept;
	void AddComputeItemImpl(ComputeItem* item) noexcept;
	void RemoveComputeItemImpl(ComputeItem* item) noexcept;
	void AddDynamicMeshGroupImpl(DynamicMeshGroup* mesh) noexcept;
	void RemoveDynamicMeshGroupImpl(DynamicMeshGroup* mesh) noexcept;

	// Update methods
	void UpdateRenderItems(const Timer& timer);
	void UpdateRenderPasses(const Timer& timer);
	void UpdateDynamicMeshes(const Timer& timer);

private:
	std::shared_ptr<DeviceResources> m_deviceResources = nullptr;
	bool m_initialized = false;

	// Rendering resources
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, gNumFrameResources> m_allocators;
	int m_currentFrameIndex = 0;
	D3D12_VIEWPORT m_viewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f }; // Dummy values
	D3D12_RECT m_scissorRect = { 0, 0, 1, 1 }; // Dummy values
	std::array<UINT64, gNumFrameResources> m_fences = {};

	// Resources that can be deleted once they are no longer referenced by the GPU
	std::vector<std::tuple<UINT64, Microsoft::WRL::ComPtr<ID3D12Resource>>> m_resourcesToDelete;

	// Render passes that will be looped over during rendering
	std::vector<RenderPass*> m_renderPasses;

	// Data that will be looped over during Update
	std::vector<RenderItem*> m_allRenderItems;
	std::vector<ComputeItem*> m_allComputeItems;
	std::vector<DynamicMeshGroup*> m_dynamicMeshes;


	// Declare all render classes friends of the Engine
	friend ConstantBuffer;
	friend RenderPass;
	friend RenderItem;
	friend ComputeItem;
	friend MeshGroup;
	friend DynamicMeshGroup;
	template<typename> friend class DynamicMeshGroupT;
	friend Texture;
	friend TextureManager;
};
}