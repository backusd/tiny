#include "tiny-pch.h"
#include "TheApp.h"



namespace tiny
{
	TheApp::TheApp(std::shared_ptr<DeviceResources> deviceResources) :
		m_deviceResources(deviceResources)
		//m_srvDescriptors(deviceResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		TextureManager::Init(m_deviceResources);

		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
		DirectX::XMStoreFloat4x4(&m_proj, P);

		GFX_THROW_INFO(
			m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr)
		);

		m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

		LoadTextures();
		BuildRootSignature();
		BuildDescriptorHeaps();
		BuildShadersAndInputLayout();
		BuildLandGeometry();
		BuildWavesGeometry();
		BuildBoxGeometry();
		BuildMaterials();
		BuildRenderItems();
		BuildFrameResources();
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

		AnimateMaterials(timer);
		UpdateObjectCBs(timer);
		UpdateMaterialCBs(timer);
		UpdateMainPassCB(timer);
		UpdateWaves(timer);
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
				DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e->TexTransform);

				ObjectConstants objConstants; 
				DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
				DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

				currObjectCB->CopyData(e->ObjCBIndex, objConstants);

				// Next FrameResource need to be updated too.
				e->NumFramesDirty--;
			}
		}
	}
	void TheApp::UpdateMaterialCBs(const Timer& timer)
	{
		auto currMaterialCB = m_currFrameResource->MaterialCB.get();
		for (auto& e : m_materials)
		{
			// Only update the cbuffer data if the constants have changed.  If the cbuffer
			// data changes, it needs to be updated for each FrameResource.
			Material* mat = e.second.get();
			if (mat->NumFramesDirty > 0)
			{
				DirectX::XMMATRIX matTransform = DirectX::XMLoadFloat4x4(&mat->MatTransform);

				MaterialConstants matConstants;
				matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
				matConstants.FresnelR0 = mat->FresnelR0;
				matConstants.Roughness = mat->Roughness;
				DirectX::XMStoreFloat4x4(&matConstants.MatTransform, DirectX::XMMatrixTranspose(matTransform));

				currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

				// Next FrameResource need to be updated too.
				mat->NumFramesDirty--;
			}
		}
	}
	void TheApp::UpdateMainPassCB(const Timer& timer)
	{
		using namespace DirectX;

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

		m_mainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

		m_mainPassCB.FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		m_mainPassCB.gFogStart = 5.0f;
		m_mainPassCB.gFogRange = 150.0f;

		m_mainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
		m_mainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
		m_mainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		m_mainPassCB.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
		m_mainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
		m_mainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

		auto currPassCB = m_currFrameResource->PassCB.get();
		currPassCB->CopyData(0, m_mainPassCB);
	}
	void TheApp::UpdateWaves(const Timer& timer)
	{
		// Every quarter second, generate a random wave.
		static float t_base = 0.0f;
		if ((timer.TotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;

			int i = MathHelper::Rand(4, m_waves->RowCount() - 5);
			int j = MathHelper::Rand(4, m_waves->ColumnCount() - 5);

			float r = MathHelper::RandF(0.2f, 0.5f);

			m_waves->Disturb(i, j, r);
		}

		// Update the wave simulation.
		m_waves->Update(timer.DeltaTime());

		// Update the wave vertex buffer with the new solution.
		auto currWavesVB = m_currFrameResource->WavesVB.get();
		for (int i = 0; i < m_waves->VertexCount(); ++i)
		{
			Vertex v;

			v.Pos = m_waves->Position(i);
			v.Normal = m_waves->Normal(i);

			// Derive tex-coords from position by 
			// mapping [-w/2,w/2] --> [0,1]
			v.TexC.x = 0.5f + v.Pos.x / m_waves->Width();
			v.TexC.y = 0.5f - v.Pos.z / m_waves->Depth();

			currWavesVB->CopyData(i, v);
		}

		// Set the dynamic VB of the wave renderitem to the current frame VB.
		m_wavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
	}
	void TheApp::AnimateMaterials(const Timer& timer)
	{
		// Scroll the water material texture coordinates.
		auto waterMat = m_materials["water"].get();

		float& tu = waterMat->MatTransform(3, 0);
		float& tv = waterMat->MatTransform(3, 1);

		tu += 0.1f * timer.DeltaTime();
		tv += 0.02f * timer.DeltaTime();

		if (tu >= 1.0f)
			tu -= 1.0f;

		if (tv >= 1.0f)
			tv -= 1.0f;

		waterMat->MatTransform(3, 0) = tu;
		waterMat->MatTransform(3, 1) = tv;

		// Material has changed, so need to update cbuffer.
		waterMat->NumFramesDirty = gNumFrameResources;
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
		GFX_THROW_INFO(commandList->Reset(commandAllocator.Get(), m_psos["opaque"].Get()));

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
		commandList->ClearRenderTargetView(m_deviceResources->CurrentBackBufferView(), (float*)&m_mainPassCB.FogColor, 0, nullptr);
		commandList->ClearDepthStencilView(m_deviceResources->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Specify the buffers we are going to render to.
		auto currentBackBufferView = m_deviceResources->CurrentBackBufferView();
		auto depthStencilView = m_deviceResources->DepthStencilView();
		commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

		
		//ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescriptorHeap.Get() };
		//ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescriptors.GetRawHeapPointer() };
		ID3D12DescriptorHeap* descriptorHeaps[] = { TextureManager::GetHeapPointer() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		// Bind per-pass constant buffer.  We only need to do this once per-pass.
		auto passCB = m_currFrameResource->PassCB->Resource();
		commandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());


		DrawRenderItems(commandList, m_renderItemLayer[(int)RenderLayer::Opaque]);

		commandList->SetPipelineState(m_psos["alphaTested"].Get());
		DrawRenderItems(commandList, m_renderItemLayer[(int)RenderLayer::AlphaTested]);

		commandList->SetPipelineState(m_psos["transparent"].Get());
		DrawRenderItems(commandList, m_renderItemLayer[(int)RenderLayer::Transparent]);


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
		UINT matCBByteSize = utility::CalcConstantBufferByteSize(sizeof(MaterialConstants));

		auto objectCB = m_currFrameResource->ObjectCB->Resource();
		auto matCB = m_currFrameResource->MaterialCB->Resource();

		// For each render item...
		for (size_t i = 0; i < ritems.size(); ++i)
		{
			auto ri = ritems[i];

			D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
			commandList->IASetVertexBuffers(0, 1, &vbv);
			D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
			commandList->IASetIndexBuffer(&ibv);
			commandList->IASetPrimitiveTopology(ri->PrimitiveType);

			//CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			//tex.Offset(ri->Mat->DiffuseSrvHeapIndex, m_deviceResources->GetCBVSRVUAVDescriptorSize());

			//D3D12_GPU_DESCRIPTOR_HANDLE tex = m_srvDescriptors.GetGPUHandleAt(ri->Mat->DiffuseSrvHeapIndex);

			D3D12_GPU_DESCRIPTOR_HANDLE tex = m_textures[ri->Mat->DiffuseSrvHeapIndex]->GetGPUHandle();

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize; 
			D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize; 

			commandList->SetGraphicsRootDescriptorTable(0, tex);
			commandList->SetGraphicsRootConstantBufferView(1, objCBAddress); 
			commandList->SetGraphicsRootConstantBufferView(3, matCBAddress); 

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

	void TheApp::LoadTextures()
	{
		m_textures[0] = std::move(TextureManager::GetTexture(TEXTURE::GRASS));
		m_textures[1] = std::move(TextureManager::GetTexture(TEXTURE::WATER1));
		m_textures[2] = std::move(TextureManager::GetTexture(TEXTURE::WIRE_FENCE));

		//auto grassTex = std::make_unique<Texture>();
		//grassTex->Name = "grassTex";
		//grassTex->Filename = L"src/textures/grass.dds";
		//GFX_THROW_INFO(
		//	DirectX::CreateDDSTextureFromFile12(
		//		m_deviceResources->GetDevice(),
		//		m_deviceResources->GetCommandList(), 
		//		grassTex->Filename.c_str(),
		//		grassTex->Resource, 
		//		grassTex->UploadHeap
		//	)
		//);
		//
		//auto waterTex = std::make_unique<Texture>();
		//waterTex->Name = "waterTex";
		//waterTex->Filename = L"src/textures/water1.dds";
		//GFX_THROW_INFO(
		//	DirectX::CreateDDSTextureFromFile12(
		//		m_deviceResources->GetDevice(),
		//		m_deviceResources->GetCommandList(),
		//		waterTex->Filename.c_str(),
		//		waterTex->Resource, 
		//		waterTex->UploadHeap
		//	)
		//);
		//
		//auto fenceTex = std::make_unique<Texture>();
		//fenceTex->Name = "fenceTex";
		//fenceTex->Filename = L"src/textures/WireFence.dds";
		//GFX_THROW_INFO(
		//	DirectX::CreateDDSTextureFromFile12(
		//		m_deviceResources->GetDevice(),
		//		m_deviceResources->GetCommandList(),
		//		fenceTex->Filename.c_str(),
		//		fenceTex->Resource, 
		//		fenceTex->UploadHeap
		//	)
		//);
		//
		//m_textures[grassTex->Name] = std::move(grassTex);
		//m_textures[waterTex->Name] = std::move(waterTex);
		//m_textures[fenceTex->Name] = std::move(fenceTex);
	}
	void TheApp::BuildRootSignature()
	{
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[4];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0);
		slotRootParameter[2].InitAsConstantBufferView(1);
		slotRootParameter[3].InitAsConstantBufferView(2);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
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
		m_shaders["standardVS"] = std::make_unique<Shader>(m_deviceResources, "LightingVS.cso");
		m_shaders["opaquePS"] = std::make_unique<Shader>(m_deviceResources, "LightingFogPS.cso");
		m_shaders["alphaTestedPS"] = std::make_unique<Shader>(m_deviceResources, "LightingFogAlphaTestPS.cso");

		m_inputLayout = std::make_unique<InputLayout>(
			std::vector<D3D12_INPUT_ELEMENT_DESC>{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			}
		);
	}
	void TheApp::BuildDescriptorHeaps()
	{
		//
		// Create the SRV heap.
		//
		// D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		// srvHeapDesc.NumDescriptors = 3;
		// srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		// srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		// GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)));


		//
		// Fill out the heap with actual descriptors.
		//
		//CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

//		auto grassTex = m_textures["grassTex"]->Resource;
//		auto waterTex = m_textures["waterTex"]->Resource;
//		auto fenceTex = m_textures["fenceTex"]->Resource;
//
//		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//		srvDesc.Format = grassTex->GetDesc().Format;
//		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//		srvDesc.Texture2D.MostDetailedMip = 0;
//		srvDesc.Texture2D.MipLevels = -1;
//		//m_deviceResources->GetDevice()->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);
//		m_srvDescriptors.EmplaceBackShaderResourceView(grassTex.Get(), &srvDesc);
//
//		// next descriptor
//		//hDescriptor.Offset(1, m_deviceResources->GetCBVSRVUAVDescriptorSize());
//
//		srvDesc.Format = waterTex->GetDesc().Format;
//		//m_deviceResources->GetDevice()->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);
//		m_srvDescriptors.EmplaceBackShaderResourceView(waterTex.Get(), &srvDesc);
//
//
//		// next descriptor
//		//hDescriptor.Offset(1, m_deviceResources->GetCBVSRVUAVDescriptorSize());
//
//		srvDesc.Format = fenceTex->GetDesc().Format;
//		//m_deviceResources->GetDevice()->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);
//		m_srvDescriptors.EmplaceBackShaderResourceView(fenceTex.Get(), &srvDesc);

	}
	void TheApp::BuildLandGeometry()
	{
		GeometryGenerator geoGen;
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

		//
		// Extract the vertex elements we are interested and apply the height function to
		// each vertex.  In addition, color the vertices based on their height so we have
		// sandy looking beaches, grassy low hills, and snow mountain peaks.
		//

		std::vector<Vertex> vertices(grid.Vertices.size());
		for (size_t i = 0; i < grid.Vertices.size(); ++i)
		{ 
			auto& p = grid.Vertices[i].Position; 
			vertices[i].Pos = p; 
			vertices[i].Pos.y = GetHillsHeight(p.x, p.z); 
			vertices[i].Normal = GetHillsNormal(p.x, p.z); 
			vertices[i].TexC = grid.Vertices[i].TexC; 
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

		std::vector<std::uint16_t> indices = grid.GetIndices16();
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "landGeo";

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

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["grid"] = submesh;

		m_geometries["landGeo"] = std::move(geo);
	}
	void TheApp::BuildWavesGeometry()
	{
		std::vector<std::uint16_t> indices(3 * m_waves->TriangleCount()); // 3 indices per face
		TINY_CORE_ASSERT(m_waves->VertexCount() < 0x0000ffff, "Too many vertices");

		// Iterate over each quad.
		int m = m_waves->RowCount();
		int n = m_waves->ColumnCount();
		int k = 0;
		for (int i = 0; i < m - 1; ++i)
		{
			for (int j = 0; j < n - 1; ++j)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;

				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quad
			}
		}

		UINT vbByteSize = m_waves->VertexCount() * sizeof(Vertex);
		UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "waterGeo";

		// Set dynamically.
		geo->VertexBufferCPU = nullptr;
		geo->VertexBufferGPU = nullptr;

		GFX_THROW_INFO(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->IndexBufferGPU = utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
			m_deviceResources->GetCommandList(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["grid"] = submesh;

		m_geometries["waterGeo"] = std::move(geo);
	}
	void TheApp::BuildBoxGeometry()
	{
		GeometryGenerator geoGen;
		GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

		std::vector<Vertex> vertices(box.Vertices.size());
		for (size_t i = 0; i < box.Vertices.size(); ++i)
		{
			auto& p = box.Vertices[i].Position;
			vertices[i].Pos = p;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].TexC = box.Vertices[i].TexC;
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

		std::vector<std::uint16_t> indices = box.GetIndices16();
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "boxGeo";

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

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs["box"] = submesh;

		m_geometries["boxGeo"] = std::move(geo);
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
		// PSO for transparent objects
		//
		D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

		//D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
		//transparencyBlendDesc.BlendEnable = true;
		//transparencyBlendDesc.LogicOpEnable = false;
		//transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		//transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		//transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		//transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		//transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		//transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		//transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		//transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		//
		//transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

		std::unique_ptr<BlendState> transparentBlendState = std::make_unique<BlendState>();
		transparentBlendState->SetRenderTargetBlendEnabled(true);
		transparentBlendState->SetRenderTargetLogicOpEnabled(false);
		transparentBlendState->SetRenderTargetSrcBlend(D3D12_BLEND_SRC_ALPHA);
		transparentBlendState->SetRenderTargetDestBlend(D3D12_BLEND_INV_SRC_ALPHA);
		transparentBlendState->SetRenderTargetBlendOp(D3D12_BLEND_OP_ADD);
		transparentBlendState->SetRenderTargetSrcBlendAlpha(D3D12_BLEND_ONE);
		transparentBlendState->SetRenderTargetDestBlendAlpha(D3D12_BLEND_ZERO);
		transparentBlendState->SetRenderTargetBlendOpAlpha(D3D12_BLEND_OP_ADD);
		transparentBlendState->SetRenderTargetLogicOp(D3D12_LOGIC_OP_NOOP);
		transparentBlendState->SetRenderTargetWriteMask(D3D12_COLOR_WRITE_ENABLE_ALL);

		transparentPsoDesc.BlendState = transparentBlendState->GetBlendDesc();
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&m_psos["transparent"]))
		);
		
		// 
		// PSO for alpha tested objects
		//
		D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
		alphaTestedPsoDesc.PS = m_shaders["alphaTestedPS"]->GetShaderByteCode();
		alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&m_psos["alphaTested"]))
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
					(UINT)m_allRitems.size(),
					(UINT)m_materials.size(),
					m_waves->VertexCount()
				)
			);
		}
	}
	void TheApp::BuildMaterials()
	{
		auto grass = std::make_unique<Material>();
		grass->Name = "grass";
		grass->MatCBIndex = 0;
		grass->DiffuseSrvHeapIndex = 0;
		grass->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		grass->FresnelR0 = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
		grass->Roughness = 0.125f;

		// This is not a good water material definition, but we do not have all the rendering
		// tools we need (transparency, environment reflection), so we fake it for now.
		auto water = std::make_unique<Material>();
		water->Name = "water";
		water->MatCBIndex = 1;
		water->DiffuseSrvHeapIndex = 1;
		water->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		water->FresnelR0 = DirectX::XMFLOAT3(0.2f, 0.2f, 0.2f);
		water->Roughness = 0.0f;

		auto wirefence = std::make_unique<Material>();
		wirefence->Name = "wirefence";
		wirefence->MatCBIndex = 2;
		wirefence->DiffuseSrvHeapIndex = 2;
		wirefence->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		wirefence->FresnelR0 = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
		wirefence->Roughness = 0.25f;

		m_materials["grass"] = std::move(grass);
		m_materials["water"] = std::move(water);
		m_materials["wirefence"] = std::move(wirefence);
	}
	void TheApp::BuildRenderItems()
	{
		auto wavesRitem = std::make_unique<RenderItem>();
		wavesRitem->World = MathHelper::Identity4x4();
		DirectX::XMStoreFloat4x4(&wavesRitem->TexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
		wavesRitem->ObjCBIndex = 0;
		wavesRitem->Mat = m_materials["water"].get();
		wavesRitem->Geo = m_geometries["waterGeo"].get();
		wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
		wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
		wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

		m_wavesRitem = wavesRitem.get();

		m_renderItemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

		auto gridRitem = std::make_unique<RenderItem>();
		gridRitem->World = MathHelper::Identity4x4();
		DirectX::XMStoreFloat4x4(&gridRitem->TexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
		gridRitem->ObjCBIndex = 1;
		gridRitem->Mat = m_materials["grass"].get();
		gridRitem->Geo = m_geometries["landGeo"].get();
		gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
		gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
		gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

		m_renderItemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

		auto boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixTranslation(3.0f, 2.0f, -9.0f));
		boxRitem->ObjCBIndex = 2;
		boxRitem->Mat = m_materials["wirefence"].get();
		boxRitem->Geo = m_geometries["boxGeo"].get();
		boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

		m_renderItemLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());

		m_allRitems.push_back(std::move(wavesRitem));
		m_allRitems.push_back(std::move(gridRitem));
		m_allRitems.push_back(std::move(boxRitem));
	}

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TheApp::GetStaticSamplers()
	{
		// Applications usually only need a handful of samplers.  So just define them all up front
		// and keep them available as part of the root signature.  

		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy

		return {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp };
	}


	float TheApp::GetHillsHeight(float x, float z)const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}

	DirectX::XMFLOAT3 TheApp::GetHillsNormal(float x, float z)const
	{
		// n = (-df/dx, 1, -df/dz)
		DirectX::XMFLOAT3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

		DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&n));
		DirectX::XMStoreFloat3(&n, unitNormal);

		return n;
	}
}