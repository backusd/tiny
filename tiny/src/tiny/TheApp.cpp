#include "tiny-pch.h"
#include "TheApp.h"



namespace tiny
{
	TheApp::TheApp(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources)
	{
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&m_proj, P);

		GFX_THROW_INFO(
			m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr)
		);

		BuildRootSignature();
		BuildShadersAndInputLayout();
		BuildShapeGeometry();
		BuildRenderItems();
		BuildFrameResources();
		BuildDescriptorHeaps();
		BuildConstantBufferViews();
		BuildPSOs();

		// Execute the initialization commands.
		GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
		ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
		m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		m_deviceResources->FlushCommandQueue();
	}
	void TheApp::OnResize(int height, int width)
	{
		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&m_proj, P);
	}
	void TheApp::SetViewport(float top, float left, float height, float width) noexcept
	{
		m_viewport.TopLeftX = left;
		m_viewport.TopLeftY = top;
		m_viewport.Height = height;
		m_viewport.Width = width;
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;

		m_scissorRect = { 0, 0, static_cast<int>(width), static_cast<int>(height) };
	}

	void TheApp::Update(const Timer& timer)
	{
		UpdateCamera(timer);

		// Cycle through the circular frame resource array.
		m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % gNumFrameResources;
		m_currFrameResource = m_frameResources[m_currFrameResourceIndex].get();

		// Has the GPU finished processing the commands of the current frame resource?
		// If not, wait until the GPU has completed commands up to this fence point.
		if (m_currFrameResource->Fence != 0 && m_deviceResources->GetFence()->GetCompletedValue() < m_currFrameResource->Fence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
			GFX_THROW_INFO(
				m_deviceResources->GetFence()->SetEventOnCompletion(m_currFrameResource->Fence, eventHandle)
			);
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}

		UpdateObjectCBs(timer);
		UpdateMainPassCB(timer);
	}

	void TheApp::UpdateCamera(const Timer& timer)
	{
		// Convert Spherical to Cartesian coordinates.
		m_eyePos.x = m_radius * sinf(m_phi) * cosf(m_theta);
		m_eyePos.z = m_radius * sinf(m_phi) * sinf(m_theta);
		m_eyePos.y = m_radius * cosf(m_phi);

		// Build the view matrix.
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
		DirectX::XMStoreFloat4x4(&m_view, view);
	}
	void TheApp::UpdateObjectCBs(const Timer& timer)
	{
		auto currObjectCB = m_currFrameResource->ObjectCB.get();
		for (auto& e : m_allRitems)
		{
			// Only update the cbuffer data if the constants have changed.  
			// This needs to be tracked per frame resource.
			if (e->NumFramesDirty > 0)
			{
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);

				ObjectConstants objConstants; 
				DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));

				currObjectCB->CopyData(e->ObjCBIndex, objConstants);

				// Next FrameResource need to be updated too.
				e->NumFramesDirty--;
			}
		}
	}
	void TheApp::UpdateMainPassCB(const Timer& timer)
	{
		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_view);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);

		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

		DirectX::XMVECTOR _det = DirectX::XMMatrixDeterminant(view);
		DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&_det, view);

		_det = DirectX::XMMatrixDeterminant(proj);
		DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&_det, proj);

		_det = DirectX::XMMatrixDeterminant(viewProj);
		DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&_det, viewProj);

		DirectX::XMStoreFloat4x4(&m_mainPassCB.View, DirectX::XMMatrixTranspose(view));
		DirectX::XMStoreFloat4x4(&m_mainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
		DirectX::XMStoreFloat4x4(&m_mainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
		DirectX::XMStoreFloat4x4(&m_mainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
		DirectX::XMStoreFloat4x4(&m_mainPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
		DirectX::XMStoreFloat4x4(&m_mainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
		m_mainPassCB.EyePosW = m_eyePos;

		float height = static_cast<float>(m_deviceResources->GetHeight());
		float width = static_cast<float>(m_deviceResources->GetWidth());

		m_mainPassCB.RenderTargetSize = DirectX::XMFLOAT2(width, height);
		m_mainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
		m_mainPassCB.NearZ = 1.0f;
		m_mainPassCB.FarZ = 1000.0f;
		m_mainPassCB.TotalTime = timer.TotalTime();
		m_mainPassCB.DeltaTime = timer.DeltaTime();

		auto currPassCB = m_currFrameResource->PassCB.get();
		currPassCB->CopyData(0, m_mainPassCB);
	}

	void TheApp::Render()
	{
		auto commandAllocator = m_currFrameResource->CmdListAlloc;

		// Reuse the memory associated with command recording.
		// We can only reset when the associated command lists have finished execution on the GPU.
		GFX_THROW_INFO(commandAllocator->Reset());

		auto commandList = m_deviceResources->GetCommandList();

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		if (m_isWireframe)
		{
			GFX_THROW_INFO(commandList->Reset(commandAllocator.Get(), m_psos["opaque_wireframe"].Get()));
		}
		else
		{
			GFX_THROW_INFO(commandList->Reset(commandAllocator.Get(), m_psos["opaque"].Get()));
		}

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate a state transition on the resource usage.
		auto _b = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, 
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		commandList->ResourceBarrier(1, &_b);

		// Clear the back buffer and depth buffer.
		commandList->ClearRenderTargetView(m_deviceResources->CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
		commandList->ClearDepthStencilView(m_deviceResources->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		auto currentBackBufferView = m_deviceResources->CurrentBackBufferView();
		auto depthStencilView = m_deviceResources->DepthStencilView();
		commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		int passCbvIndex = m_passCbvOffset + m_currFrameResourceIndex;
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCbvIndex, m_deviceResources->GetCBVSRVUAVDescriptorSize());
		commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);


		DrawRenderItems(commandList, m_opaqueRitems);


		// Indicate a state transition on the resource usage.
		auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(
			m_deviceResources->CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_PRESENT
		);
		commandList->ResourceBarrier(1, &_b2);

		// Done recording commands.
		GFX_THROW_INFO(commandList->Close());

		// Add the command list to the queue for execution.
		ID3D12CommandList* cmdsLists[] = { commandList };
		m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	}
	void TheApp::DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& ritems)
	{
		UINT objCBByteSize = utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		auto objectCB = m_currFrameResource->ObjectCB->Resource();

		// For each render item...
		for (size_t i = 0; i < ritems.size(); ++i)
		{
			auto ri = ritems[i];

			D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
			commandList->IASetVertexBuffers(0, 1, &vbv);

			D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
			commandList->IASetIndexBuffer(&ibv);

			commandList->IASetPrimitiveTopology(ri->PrimitiveType);

			// Offset to the CBV in the descriptor heap for this object and for this frame resource.
			UINT cbvIndex = m_currFrameResourceIndex * (UINT)m_opaqueRitems.size() + ri->ObjCBIndex;
			auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
			cbvHandle.Offset(cbvIndex, m_deviceResources->GetCBVSRVUAVDescriptorSize());

			commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

			commandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		}
	}
	void TheApp::Present() 
	{ 
		m_deviceResources->Present();

		m_currFrameResource->Fence = m_deviceResources->GetCurrentFenceValue();

		// Add an instruction to the command queue to set a new fence point. 
		// Because we are on the GPU timeline, the new fence point won't be 
		// set until the GPU finishes processing all the commands prior to this Signal().
		m_deviceResources->GetCommandQueue()->Signal(m_deviceResources->GetFence(), m_currFrameResource->Fence);
	}

	void TheApp::BuildDescriptorHeaps()
	{
		UINT objCount = (UINT)m_opaqueRitems.size();

		// Need a CBV descriptor for each object for each frame resource,
		// +1 for the perPass CBV for each frame resource.
		UINT numDescriptors = (objCount + 1) * gNumFrameResources;

		// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
		m_passCbvOffset = objCount * gNumFrameResources;

		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
		cbvHeapDesc.NumDescriptors = numDescriptors;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap))
		);
	}
	void TheApp::BuildConstantBufferViews()
	{
		UINT objCBByteSize = utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		UINT objCount = (UINT)m_opaqueRitems.size();

		// Need a CBV descriptor for each object for each frame resource.
		for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
		{
			auto objectCB = m_frameResources[frameIndex]->ObjectCB->Resource();
			for (UINT i = 0; i < objCount; ++i)
			{
				D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

				// Offset to the ith object constant buffer in the buffer.
				cbAddress += i * objCBByteSize;

				// Offset to the object cbv in the descriptor heap.
				int heapIndex = frameIndex * objCount + i;
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
				handle.Offset(heapIndex, m_deviceResources->GetCBVSRVUAVDescriptorSize());

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
				cbvDesc.BufferLocation = cbAddress;
				cbvDesc.SizeInBytes = objCBByteSize;

				m_deviceResources->GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
			}
		}

		UINT passCBByteSize = utility::CalcConstantBufferByteSize(sizeof(PassConstants));

		// Last three descriptors are the pass CBVs for each frame resource.
		for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
		{
			auto passCB = m_frameResources[frameIndex]->PassCB->Resource();
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

			// Offset to the pass cbv in the descriptor heap.
			int heapIndex = m_passCbvOffset + frameIndex;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, m_deviceResources->GetCBVSRVUAVDescriptorSize());

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = passCBByteSize;

			m_deviceResources->GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
		}
	}
	void TheApp::BuildRootSignature()
	{
		CD3DX12_DESCRIPTOR_RANGE cbvTable0;
		cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		CD3DX12_DESCRIPTOR_RANGE cbvTable1;
		cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[2];

		// Create root CBVs.
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
		slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
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
	void TheApp::BuildShadersAndInputLayout()
	{
		m_shaders["standardVS"] = std::make_unique<Shader>(m_deviceResources, "color_vs.cso");
		m_shaders["opaquePS"] = std::make_unique<Shader>(m_deviceResources, "color_ps.cso");

		m_inputLayout = std::make_unique<InputLayout>(
			std::vector<D3D12_INPUT_ELEMENT_DESC>{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			}
		);
	}
	void TheApp::BuildShapeGeometry()
	{
		GeometryGenerator geoGen;
		GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
		GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
		GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

		//
		// We are concatenating all the geometry into one big vertex/index buffer.  So
		// define the regions in the buffer each submesh covers.
		//

		// Cache the vertex offsets to each object in the concatenated vertex buffer.
		UINT boxVertexOffset = 0;
		UINT gridVertexOffset = (UINT)box.Vertices.size();
		UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
		UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

		// Cache the starting index for each object in the concatenated index buffer.
		UINT boxIndexOffset = 0;
		UINT gridIndexOffset = (UINT)box.Indices32.size();
		UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
		UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

		// Define the SubmeshGeometry that cover different 
		// regions of the vertex/index buffers.

		SubmeshGeometry boxSubmesh;
		boxSubmesh.IndexCount = (UINT)box.Indices32.size();
		boxSubmesh.StartIndexLocation = boxIndexOffset;
		boxSubmesh.BaseVertexLocation = boxVertexOffset;

		SubmeshGeometry gridSubmesh;
		gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
		gridSubmesh.StartIndexLocation = gridIndexOffset;
		gridSubmesh.BaseVertexLocation = gridVertexOffset;

		SubmeshGeometry sphereSubmesh;
		sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
		sphereSubmesh.StartIndexLocation = sphereIndexOffset;
		sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

		SubmeshGeometry cylinderSubmesh;
		cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
		cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
		cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		auto totalVertexCount =
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size();

		std::vector<Vertex> vertices(totalVertexCount);

		UINT k = 0;
		for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::DarkGreen);
		}

		for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::ForestGreen);
		}

		for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::Crimson);
		}

		for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::SteelBlue);
		}

		std::vector<std::uint16_t> indices;
		indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
		indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
		indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
		indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "shapeGeo";

		GFX_THROW_INFO(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		GFX_THROW_INFO(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
			m_deviceResources->GetCommandList(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
			m_deviceResources->GetCommandList(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["box"] = boxSubmesh;
		geo->DrawArgs["grid"] = gridSubmesh;
		geo->DrawArgs["sphere"] = sphereSubmesh;
		geo->DrawArgs["cylinder"] = cylinderSubmesh;

		m_geometries[geo->Name] = std::move(geo);
	}
	void TheApp::BuildPSOs()
	{
		m_rasterizerState = std::make_unique<RasterizerState>();
		m_blendState = std::make_unique<BlendState>();
		m_depthStencilState = std::make_unique<DepthStencilState>();

		//
		// PSO for opaque objects.
		//
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)); 
		opaquePsoDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
		opaquePsoDesc.pRootSignature = m_rootSignature.Get();
		opaquePsoDesc.VS = m_shaders["standardVS"]->GetShaderByteCode();
		opaquePsoDesc.PS = m_shaders["opaquePS"]->GetShaderByteCode();
		opaquePsoDesc.RasterizerState = m_rasterizerState->GetRasterizerDesc();
		opaquePsoDesc.BlendState = m_blendState->GetBlendDesc();
		opaquePsoDesc.DepthStencilState = m_depthStencilState->GetDepthStencilDesc();
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		opaquePsoDesc.NumRenderTargets = 1;
		opaquePsoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
		opaquePsoDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
		opaquePsoDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
		opaquePsoDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_psos["opaque"]))
		);

		//
		// PSO for opaque wireframe objects.
		//
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
		opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_psos["opaque_wireframe"]))
		);
	}
	void TheApp::BuildFrameResources()
	{
		for (int i = 0; i < gNumFrameResources; ++i)
		{
			m_frameResources.push_back(
				std::make_unique<FrameResource>(
					m_deviceResources->GetDevice(),
					1, 
					(UINT)m_allRitems.size()
				)
			);
		}
	}
	void TheApp::BuildRenderItems()
	{
		auto boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));
		boxRitem->ObjCBIndex = 0;
		boxRitem->Geo = m_geometries["shapeGeo"].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		m_allRitems.push_back(std::move(boxRitem));

		auto gridRitem = std::make_unique<RenderItem>();
		gridRitem->World = MathHelper::Identity4x4();
		gridRitem->ObjCBIndex = 1;
		gridRitem->Geo = m_geometries["shapeGeo"].get();
		gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
		gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
		gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
		m_allRitems.push_back(std::move(gridRitem));

		UINT objCBIndex = 2;
		for (int i = 0; i < 5; ++i)
		{
			auto leftCylRitem = std::make_unique<RenderItem>();
			auto rightCylRitem = std::make_unique<RenderItem>();
			auto leftSphereRitem = std::make_unique<RenderItem>();
			auto rightSphereRitem = std::make_unique<RenderItem>();

			DirectX::XMMATRIX leftCylWorld = DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
			DirectX::XMMATRIX rightCylWorld = DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

			DirectX::XMMATRIX leftSphereWorld = DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
			DirectX::XMMATRIX rightSphereWorld = DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

			XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
			leftCylRitem->ObjCBIndex = objCBIndex++;
			leftCylRitem->Geo = m_geometries["shapeGeo"].get();
			leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
			leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
			leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

			XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
			rightCylRitem->ObjCBIndex = objCBIndex++;
			rightCylRitem->Geo = m_geometries["shapeGeo"].get();
			rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
			rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
			rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

			XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
			leftSphereRitem->ObjCBIndex = objCBIndex++;
			leftSphereRitem->Geo = m_geometries["shapeGeo"].get();
			leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
			leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
			leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

			XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
			rightSphereRitem->ObjCBIndex = objCBIndex++;
			rightSphereRitem->Geo = m_geometries["shapeGeo"].get();
			rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
			rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
			rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

			m_allRitems.push_back(std::move(leftCylRitem));
			m_allRitems.push_back(std::move(rightCylRitem));
			m_allRitems.push_back(std::move(leftSphereRitem));
			m_allRitems.push_back(std::move(rightSphereRitem));
		}

		// All the render items are opaque.
		for (auto& e : m_allRitems)
			m_opaqueRitems.push_back(e.get());
	}
}