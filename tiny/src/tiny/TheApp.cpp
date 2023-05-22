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

		BuildDescriptorHeaps();
		BuildConstantBuffers();
		BuildRootSignature();
		BuildShadersAndInputLayout();
		BuildBoxGeometry();
		BuildPSO();

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

	void TheApp::Update()
	{
		// Convert Spherical to Cartesian coordinates.
		float x = m_radius * sinf(m_phi) * cosf(m_theta);
		float z = m_radius * sinf(m_phi) * sinf(m_theta);
		float y = m_radius * cosf(m_phi);

		// Build the view matrix.
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
		DirectX::XMStoreFloat4x4(&m_view, view);

		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_world);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);
		DirectX::XMMATRIX worldViewProj = world * view * proj;

		// Update the constant buffer with the latest worldViewProj matrix.
		ObjectConstants objConstants;
		DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));
		m_objectCB->CopyData(0, objConstants);
	}
	void TheApp::Render()
	{
		// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
		GFX_THROW_INFO(m_deviceResources->GetCommandAllocator()->Reset());

		auto commandList = m_deviceResources->GetCommandList();

		// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
		// Reusing the command list reuses memory.
		GFX_THROW_INFO(
			commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pso.Get())
		);

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


		auto _vbv = m_boxGeo->VertexBufferView();
		commandList->IASetVertexBuffers(0, 1, &_vbv);
		auto _ibv = m_boxGeo->IndexBufferView();
		commandList->IASetIndexBuffer(&_ibv);
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

		commandList->DrawIndexedInstanced(
			m_boxGeo->DrawArgs["box"].IndexCount,
			1, 0, 0, 0);

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

	void TheApp::BuildDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
		cbvHeapDesc.NumDescriptors = 1;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap))
		);
	}
	void TheApp::BuildConstantBuffers()
	{
		m_objectCB = std::make_unique<tiny::UploadBuffer<ObjectConstants>>(m_deviceResources->GetDevice(), 1, true);

		UINT objCBByteSize = tiny::utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();
		// Offset to the ith object constant buffer in the buffer.
		int boxCBufIndex = 0;
		cbAddress += boxCBufIndex * objCBByteSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = tiny::utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		m_deviceResources->GetDevice()->CreateConstantBufferView(
			&cbvDesc,
			m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	}
	void TheApp::BuildRootSignature()
	{
		// Shader programs typically require resources as input (constant buffers,
		// textures, samplers).  The root signature defines the resources the shader
		// programs expect.  If we think of the shader programs as a function, and
		// the input resources as function parameters, then the root signature can be
		// thought of as defining the function signature.  

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Create a single descriptor table of CBVs.
		CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
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
		m_vertexShader = std::make_unique<Shader>(m_deviceResources, "color_vs.cso");
		m_pixelShader = std::make_unique<Shader>(m_deviceResources, "color_ps.cso");

		m_inputLayout = std::make_unique<InputLayout>(
			std::vector<D3D12_INPUT_ELEMENT_DESC>{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			}
		);
	}
	void TheApp::BuildBoxGeometry()
	{
		std::array<Vertex, 8> vertices =
		{
			Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
			Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
			Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
			Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
			Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
			Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
			Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
			Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
		};

		std::array<std::uint16_t, 36> indices =
		{
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
		};

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		m_boxGeo = std::make_unique<tiny::MeshGeometry>();
		m_boxGeo->Name = "boxGeo";

		GFX_THROW_INFO(D3DCreateBlob(vbByteSize, &m_boxGeo->VertexBufferCPU));
		CopyMemory(m_boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		GFX_THROW_INFO(D3DCreateBlob(ibByteSize, &m_boxGeo->IndexBufferCPU));
		CopyMemory(m_boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		m_boxGeo->VertexBufferGPU = tiny::utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
			m_deviceResources->GetCommandList(), vertices.data(), vbByteSize, m_boxGeo->VertexBufferUploader);

		m_boxGeo->IndexBufferGPU = tiny::utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
			m_deviceResources->GetCommandList(), indices.data(), ibByteSize, m_boxGeo->IndexBufferUploader);

		m_boxGeo->VertexByteStride = sizeof(Vertex);
		m_boxGeo->VertexBufferByteSize = vbByteSize;
		m_boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
		m_boxGeo->IndexBufferByteSize = ibByteSize;

		tiny::SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		m_boxGeo->DrawArgs["box"] = submesh;
	}
	void TheApp::BuildPSO()
	{
		m_rasterizerState = std::make_unique<RasterizerState>();
		m_blendState = std::make_unique<BlendState>();
		m_depthStencilState = std::make_unique<DepthStencilState>();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		psoDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = m_vertexShader->GetShaderByteCode();
		psoDesc.PS = m_pixelShader->GetShaderByteCode();
		psoDesc.RasterizerState = m_rasterizerState->GetRasterizerDesc();
		psoDesc.BlendState = m_blendState->GetBlendDesc();
		psoDesc.DepthStencilState = m_depthStencilState->GetDepthStencilDesc();
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
		psoDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
		psoDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
		psoDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();
		GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
	}
}