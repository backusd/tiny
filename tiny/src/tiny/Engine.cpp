#include "tiny-pch.h"
#include "Engine.h"
#include "rendering/DescriptorManager.h"
#include "rendering/RenderPass.h"
#include "rendering/RenderPassLayer.h"
#include "rendering/RenderItem.h"
#include "rendering/MeshGroup.h"
#include "rendering/Texture.h"
#include "tiny/utils/Profile.h"


namespace tiny
{
void Engine::InitImpl(std::shared_ptr<DeviceResources> deviceResources)
{
	TINY_CORE_ASSERT(!m_initialized, "Engine has already been initialized");
	TINY_CORE_ASSERT(deviceResources != nullptr, "No device resources");
	m_deviceResources = deviceResources;

	// Initialize all fence values to 0
	std::fill(std::begin(m_fences), std::end(m_fences), 0);

	// Initialize allocators
	auto device = m_deviceResources->GetDevice();
	for (unsigned int iii = 0; iii < gNumFrameResources; ++iii)
	{
		GFX_THROW_INFO(
			device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_allocators[iii].GetAddressOf())
			)
		);
	}

	m_initialized = true;
}
void Engine::UpdateImpl(const Timer& timer)
{
	PROFILE_FUNCTION();
	TINY_CORE_ASSERT(m_initialized, "Engine has not been initialized");

	// Cycle through the circular frame resource array.
	m_currentFrameIndex = (m_currentFrameIndex + 1) % gNumFrameResources;

	// Has the GPU finished processing the commands of the current frame?
	// If not, wait until the GPU has completed commands up to this fence point.
	UINT64 currentFence = m_fences[m_currentFrameIndex];
	{
		PROFILE_SCOPE("Waiting for GPU to update fence");

		if (currentFence != 0 && m_deviceResources->GetFence()->GetCompletedValue() < currentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
			GFX_THROW_INFO(
				m_deviceResources->GetFence()->SetEventOnCompletion(currentFence, eventHandle)
			);

			TINY_CORE_ASSERT(eventHandle != NULL, "Handle should not be null");
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	// Cleanup resources that were passed to DelayedDelete()
	CleanupResources();

	// Update dynamic data
	UpdateRenderItems(timer);
	UpdateComputeItems(timer);
	UpdateRenderPasses(timer);
	UpdateDynamicMeshes(timer);
}
void Engine::RenderImpl()
{
	PROFILE_FUNCTION();
	TINY_CORE_ASSERT(m_initialized, "Engine has not been initialized");
	TINY_CORE_ASSERT(m_renderPasses.size() > 0, "No render passes");

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = m_allocators[m_currentFrameIndex];
	{
		PROFILE_SCOPE("commandAllocator->Reset()");
		GFX_THROW_INFO(commandAllocator->Reset());
	}

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	// NOTE: When resetting the commandlist, we are allowed to specify the PSO we want the command list to have.
	//       However, this is slightly inconvenient given the way we have structured the loop below. According to
	//		 the documentation for resetting using nullptr: "If NULL, the runtime sets a dummy initial pipeline 
	//		 state so that drivers don't have to deal with undefined state. The overhead for this is low, 
	//		 particularly for a command list, for which the overall cost of recording the command list likely 
	//		 dwarfs the cost of one initial state setting."
	auto commandList = m_deviceResources->GetCommandList();
	{
		PROFILE_SCOPE("commandList->Reset()");
		GFX_THROW_INFO(commandList->Reset(commandAllocator.Get(), nullptr));
	}

	{
		PROFILE_SCOPE("SetViewport/ScissorRects");
		GFX_THROW_INFO_ONLY(commandList->RSSetViewports(1, &m_viewport));
		GFX_THROW_INFO_ONLY(commandList->RSSetScissorRects(1, &m_scissorRect));
	}

	// Indicate a state transition on the resource usage.
	{
		PROFILE_SCOPE("Back Buffer Transition: PRESENT -> RENDER_TARGET");
		auto _b = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b));
	}

	// Clear the back buffer and depth buffer.
	{
		PROFILE_SCOPE("Clear RTV & DSV");
		FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		GFX_THROW_INFO_ONLY(commandList->ClearRenderTargetView(m_deviceResources->CurrentBackBufferView(), color, 0, nullptr));
		GFX_THROW_INFO_ONLY(commandList->ClearDepthStencilView(m_deviceResources->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr));
	}
	// Specify the buffers we are going to render to.
	auto currentBackBufferView = m_deviceResources->CurrentBackBufferView();
	auto depthStencilView = m_deviceResources->DepthStencilView();
	{
		PROFILE_SCOPE("OMSetRenderTargets");
		GFX_THROW_INFO_ONLY(
			commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView)
		);
	}

	{
		PROFILE_SCOPE("SetDescriptorHeaps");
		ID3D12DescriptorHeap* descriptorHeaps[] = { DescriptorManager::GetRawHeapPointer() };
		GFX_THROW_INFO_ONLY(
			commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps)
		);
	}

	for (RenderPass* pass : m_renderPasses)
	{
		PROFILE_SCOPE(pass->Name.c_str());

		TINY_CORE_ASSERT(pass != nullptr, "Pass should never be nullptr");
		TINY_CORE_ASSERT(pass->RootSignature != nullptr, "Pass has no root signature");
		TINY_CORE_ASSERT(pass->RenderPassLayers.size() > 0 || pass->ComputeLayers.size() > 0, "Pass has no render layers nor compute layers. Must have at least 1 type of layer to be valid.");

		// Before attempting to perform any rendering, first perform all compute operations
		for (const ComputeLayer& layer : pass->ComputeLayers)
		{
			PROFILE_SCOPE(layer.Name.c_str());

			TINY_CORE_ASSERT(layer.ComputeItems.size() > 0, "Compute layer has no compute items");
			TINY_CORE_ASSERT(layer.PipelineState != nullptr, "Compute layer has no pipeline state");
			TINY_CORE_ASSERT(layer.RootSignature != nullptr, "Compute layer has no root signature");

			// Pre-Work method - possibly for transitioning resources
			layer.PreWork(layer, commandList);

			// Root Signature / PSO
			GFX_THROW_INFO_ONLY(commandList->SetComputeRootSignature(layer.RootSignature->Get()));
			GFX_THROW_INFO_ONLY(commandList->SetPipelineState(layer.PipelineState.Get()));

			// Iterate over each compute item and call dispatch to submit compute work to the GPU
			for (const ComputeItem& item : layer.ComputeItems)
			{
				// Tables and CBV's ARE allowed to be empty
				for (const RootDescriptorTable& table : item.DescriptorTables)
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetComputeRootDescriptorTable(table.RootParameterIndex, table.DescriptorHandle)
					);
				}
				for (const RootConstantBufferView& cbv : item.ConstantBufferViews)
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetComputeRootConstantBufferView(cbv.RootParameterIndex, cbv.ConstantBuffer->GetGPUVirtualAddress(m_currentFrameIndex))
					);
				}

				GFX_THROW_INFO_ONLY(commandList->Dispatch(item.ThreadGroupCountX, item.ThreadGroupCountY, item.ThreadGroupCountZ));
			}

			// Post-Work method - possibly for transitioning resources
			layer.PostWork(layer, commandList);
		}



		// Pre-Work method - possibly for transitioning resources or anything necessary
		pass->PreWork(pass, commandList);

		// Set only a single root signature per RenderPass
		GFX_THROW_INFO_ONLY(commandList->SetGraphicsRootSignature(pass->RootSignature->Get()));

		// Bind any per-pass constant buffer views
		for (const RootConstantBufferView& cbv : pass->ConstantBufferViews)
		{
			GFX_THROW_INFO_ONLY(
				commandList->SetGraphicsRootConstantBufferView(cbv.RootParameterIndex, cbv.ConstantBuffer->GetGPUVirtualAddress(m_currentFrameIndex))
			);
		}

		// Render the render layers for the pass
		for (const RenderPassLayer& layer : pass->RenderPassLayers)
		{
			PROFILE_SCOPE(layer.Name.c_str());

			TINY_CORE_ASSERT(layer.RenderItems.size() > 0, "Layer has no render items");
			TINY_CORE_ASSERT(layer.PipelineState != nullptr, "Layer has no pipeline state");
			TINY_CORE_ASSERT(layer.Meshes != nullptr, "Layer has no mesh group");

			// PSO / Pre-Work / MeshGroup / Primitive Topology
			GFX_THROW_INFO_ONLY(commandList->SetPipelineState(layer.PipelineState.Get()));
			layer.PreWork(layer, commandList);		// Pre-Work method (example usage: setting stencil value)
			layer.Meshes->Bind(commandList);
			GFX_THROW_INFO_ONLY(commandList->IASetPrimitiveTopology(layer.Topology));

			MeshGroup* meshGroup = layer.Meshes.get();

			for (const RenderItem& item : layer.RenderItems)
			{
				// Tables and CBV's ARE allowed to be empty
				for (const RootDescriptorTable& table : item.DescriptorTables)
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetGraphicsRootDescriptorTable(table.RootParameterIndex, table.DescriptorHandle)
					);
				}

				for (const RootConstantBufferView& cbv : item.ConstantBufferViews)
				{
					GFX_THROW_INFO_ONLY(
						commandList->SetGraphicsRootConstantBufferView(cbv.RootParameterIndex, cbv.ConstantBuffer->GetGPUVirtualAddress(m_currentFrameIndex))
					);
				}

				SubmeshGeometry mesh = meshGroup->GetSubmesh(item.submeshIndex);
				GFX_THROW_INFO_ONLY(
					commandList->DrawIndexedInstanced(mesh.IndexCount, 1, mesh.StartIndexLocation, mesh.BaseVertexLocation, 0)
				);
			}
		}

		pass->PostWork(pass, commandList);
	}

	{
		PROFILE_SCOPE("Back Buffer Transition: RENDER_TARGET -> PRESENT");
		// Indicate a state transition on the resource usage.
		auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		GFX_THROW_INFO_ONLY(commandList->ResourceBarrier(1, &_b2));
	}

	// Done recording commands.
	{
		PROFILE_SCOPE("commandList->Close()");
		GFX_THROW_INFO(commandList->Close());
	}

	// Add the command list to the queue for execution.
	{
		PROFILE_SCOPE("commandQueue->ExecuteCommandLists()");
		ID3D12CommandList* cmdsLists[] = { commandList };
		GFX_THROW_INFO_ONLY(
			m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists)
		);
	}
}
void Engine::PresentImpl()
{
	PROFILE_FUNCTION();
	TINY_CORE_ASSERT(m_initialized, "Engine has not been initialized");
	
	m_deviceResources->Present();

	m_fences[m_currentFrameIndex] = m_deviceResources->GetCurrentFenceValue();

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	m_deviceResources->GetCommandQueue()->Signal(m_deviceResources->GetFence(), m_fences[m_currentFrameIndex]);
}
void Engine::CleanupResources() noexcept
{
	PROFILE_FUNCTION();

	if (m_resourcesToDelete.size() > 0)
	{
		UINT64 completedFence = m_deviceResources->GetFence()->GetCompletedValue();

		// Erase all elements for which the completed fence is at or beyond the fence value in the tuple
		std::erase_if(m_resourcesToDelete, [&completedFence](const std::tuple<UINT64, Microsoft::WRL::ComPtr<ID3D12Resource>>& tup)
			{
				return completedFence >= std::get<0>(tup);
			}
		);
	}
}
void Engine::UpdateRenderItems(const Timer& timer)
{
	PROFILE_FUNCTION();

	for (RenderItem* item : m_allRenderItems)
	{
		TINY_CORE_ASSERT(item != nullptr, "RenderItem should never be nullptr");
		item->Update(timer, m_currentFrameIndex);
	}
}
void Engine::UpdateComputeItems(const Timer& timer)
{
	PROFILE_FUNCTION();

	for (ComputeItem* item : m_allComputeItems)
	{
		TINY_CORE_ASSERT(item != nullptr, "RenderItem should never be nullptr");
		item->Update(timer, m_currentFrameIndex);
	}
}
void Engine::UpdateRenderPasses(const Timer& timer)
{
	PROFILE_FUNCTION();

	for (RenderPass* pass : m_renderPasses)
	{
		TINY_CORE_ASSERT(pass != nullptr, "RenderPass should never be nullptr");
		pass->Update(timer, m_currentFrameIndex);
	}
}
void Engine::UpdateDynamicMeshes(const Timer& timer)
{
	PROFILE_FUNCTION();

	for (DynamicMeshGroup* mesh : m_dynamicMeshes)
	{
		TINY_CORE_ASSERT(mesh != nullptr, "DynamicMeshGroup should never be nullptr");
		mesh->Update(m_currentFrameIndex);
	}
}

void Engine::DelayedDeleteImpl(Microsoft::WRL::ComPtr<ID3D12Resource> resource) noexcept
{
	// store a ComPtr to the resource as well as the maximum fence value for the GPU where
	// the resource might still be referenced
	m_resourcesToDelete.emplace_back(m_fences[m_currentFrameIndex], resource);
}



void Engine::RemoveRenderPassImpl(RenderPass* pass) noexcept
{
	std::vector<RenderPass*>::iterator position = std::find(m_renderPasses.begin(), m_renderPasses.end(), pass);
	if (position != m_renderPasses.end())
		m_renderPasses.erase(position);
}
void Engine::AddRenderItemImpl(RenderItem* item) noexcept
{
	// The reason for holding a list of render items is strictly so we can call RenderItem::Update()
	// Therefore, we do NOT want duplicate render items, which is possible if a render item is shared
	// across render passes
	std::vector<RenderItem*>::iterator position = std::find(m_allRenderItems.begin(), m_allRenderItems.end(), item);
	if (position == m_allRenderItems.end())
		m_allRenderItems.push_back(item);
}
void Engine::RemoveRenderItemImpl(RenderItem* item) noexcept
{
	std::vector<RenderItem*>::iterator position = std::find(m_allRenderItems.begin(), m_allRenderItems.end(), item);
	if (position != m_allRenderItems.end())
		m_allRenderItems.erase(position);
}
void Engine::AddComputeItemImpl(ComputeItem* item) noexcept
{
	// The reason for holding a list of compute items is strictly so we can call ComputeItem::Update()
	// Therefore, we do NOT want duplicate compute items, which is possible if a compute item is shared
	// across render passes
	std::vector<ComputeItem*>::iterator position = std::find(m_allComputeItems.begin(), m_allComputeItems.end(), item);
	if (position == m_allComputeItems.end())
		m_allComputeItems.push_back(item);
}
void Engine::RemoveComputeItemImpl(ComputeItem* item) noexcept
{
	std::vector<ComputeItem*>::iterator position = std::find(m_allComputeItems.begin(), m_allComputeItems.end(), item);
	if (position != m_allComputeItems.end())
		m_allComputeItems.erase(position);
}
void Engine::AddDynamicMeshGroupImpl(DynamicMeshGroup* mesh) noexcept
{
	// The reason for holding a list of dynamic meshes is strictly so we can call DynamicMeshGroup::Update()
	// Therefore, we do NOT want duplicate dynamic meshes, which is possible if the mesh is shared across
	// render passes
	std::vector<DynamicMeshGroup*>::iterator position = std::find(m_dynamicMeshes.begin(), m_dynamicMeshes.end(), mesh);
	if (position == m_dynamicMeshes.end())
		m_dynamicMeshes.push_back(mesh);
}
void Engine::RemoveDynamicMeshGroupImpl(DynamicMeshGroup* mesh) noexcept
{
	std::vector<DynamicMeshGroup*>::iterator position = std::find(m_dynamicMeshes.begin(), m_dynamicMeshes.end(), mesh); 
	if (position != m_dynamicMeshes.end())
		m_dynamicMeshes.erase(position);
}

}