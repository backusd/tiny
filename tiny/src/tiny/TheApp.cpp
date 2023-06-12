#include "tiny-pch.h"
#include "TheApp.h"
#include "Engine.h"


namespace tiny
{
TheApp::TheApp(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources),
	m_mainRenderPass()
{
	Engine::Init(m_deviceResources);
	TextureManager::Init(m_deviceResources);


	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr));

	m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);

	LoadTextures();
	BuildMainRenderPass();

	// Execute the initialization commands.
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
	ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	m_deviceResources->FlushCommandQueue();
}
void TheApp::OnResize(int height, int width)
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
}
void TheApp::SetViewport(float top, float left, float height, float width) noexcept
{
	D3D12_VIEWPORT vp;
	vp.TopLeftX = left;
	vp.TopLeftY = top;
	vp.Height = height;
	vp.Width = width;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	Engine::SetViewport(vp);

	D3D12_RECT scissorRect = { 0, 0, static_cast<int>(width), static_cast<int>(height) };
	Engine::SetScissorRect(scissorRect);
}

void TheApp::LoadTextures()
{
	m_textures[0] = TextureManager::GetTexture(0);
	m_textures[1] = TextureManager::GetTexture(1);
	m_textures[2] = TextureManager::GetTexture(2);
}
void TheApp::BuildMainRenderPass()
{
	// Root Signature --------------------------------------------------------------------------------
	CD3DX12_DESCRIPTOR_RANGE texTable; 
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); 

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4]; 

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); 
	slotRootParameter[1].InitAsConstantBufferView(0); // ObjectCB
	slotRootParameter[2].InitAsConstantBufferView(1); // PassConstants
	slotRootParameter[3].InitAsConstantBufferView(2); // MaterialCB

	auto staticSamplers = GetStaticSamplers(); 

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, 
		(UINT)staticSamplers.size(), staticSamplers.data(), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT); 

	m_mainRenderPass.RootSignature = std::make_shared<RootSignature>(m_deviceResources, rootSigDesc);

		
	// Per-pass constants -----------------------------------------------------------------------------
	m_mainRenderPassConstantsCB = std::make_unique<ConstantBufferT<PassConstants>>(m_deviceResources);

	// Currently using root parameter index #2 for the per-pass constants
	auto& renderPassCBV = m_mainRenderPass.ConstantBufferViews.emplace_back(2, m_mainRenderPassConstantsCB.get());
	renderPassCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		DirectX::XMMATRIX view = m_camera.GetView();
		DirectX::XMMATRIX proj = m_camera.GetProj();

		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj); 

		DirectX::XMVECTOR _det = DirectX::XMMatrixDeterminant(view); 
		DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&_det, view); 

		_det = DirectX::XMMatrixDeterminant(proj); 
		DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&_det, proj); 

		_det = DirectX::XMMatrixDeterminant(viewProj); 
		DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&_det, viewProj); 

		PassConstants passConstants;

		DirectX::XMStoreFloat4x4(&passConstants.View, DirectX::XMMatrixTranspose(view));
		DirectX::XMStoreFloat4x4(&passConstants.InvView, DirectX::XMMatrixTranspose(invView));
		DirectX::XMStoreFloat4x4(&passConstants.Proj, DirectX::XMMatrixTranspose(proj));
		DirectX::XMStoreFloat4x4(&passConstants.InvProj, DirectX::XMMatrixTranspose(invProj));
		DirectX::XMStoreFloat4x4(&passConstants.ViewProj, DirectX::XMMatrixTranspose(viewProj));
		DirectX::XMStoreFloat4x4(&passConstants.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
		
		passConstants.EyePosW = m_camera.GetPosition3f();

		float height = static_cast<float>(m_deviceResources->GetHeight());
		float width = static_cast<float>(m_deviceResources->GetWidth());

		passConstants.RenderTargetSize = DirectX::XMFLOAT2(width, height);
		passConstants.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
		passConstants.NearZ = 1.0f;
		passConstants.FarZ = 1000.0f;
		passConstants.TotalTime = timer.TotalTime();
		passConstants.DeltaTime = timer.DeltaTime();

		passConstants.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

		passConstants.FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		passConstants.gFogStart = 5.0f;
		passConstants.gFogRange = 150.0f;

		passConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
		passConstants.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
		passConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		passConstants.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
		passConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
		passConstants.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

		m_mainRenderPassConstantsCB->CopyData(frameIndex, passConstants);
	};



	// Render Pass Layer: Opaque ----------------------------------------------------------------------
	RenderPassLayer& opaqueLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);

	// PSO
	m_standardVS = std::make_unique<Shader>(m_deviceResources, "C:/dev/tiny/sandbox/LightingVS.cso"); 
	m_opaquePS = std::make_unique<Shader>(m_deviceResources, "C:/dev/tiny/sandbox/LightingFogPS.cso"); 
	m_alphaTestedPS = std::make_unique<Shader>(m_deviceResources, "C:/dev/tiny/sandbox/LightingFogAlphaTestPS.cso"); 

	m_inputLayout = std::make_unique<InputLayout>( 
		std::vector<D3D12_INPUT_ELEMENT_DESC>{ 
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, 
		}
	);

	m_rasterizerState = std::make_unique<RasterizerState>();
	m_blendState = std::make_unique<BlendState>();
	m_depthStencilState = std::make_unique<DepthStencilState>();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = m_inputLayout->GetInputLayoutDesc(); 
	opaqueDesc.pRootSignature = m_mainRenderPass.RootSignature->Get();
	opaqueDesc.VS = m_standardVS->GetShaderByteCode(); 
	opaqueDesc.PS = m_opaquePS->GetShaderByteCode(); 
	opaqueDesc.RasterizerState = m_rasterizerState->GetRasterizerDesc();
	opaqueDesc.BlendState = m_blendState->GetBlendDesc();
	opaqueDesc.DepthStencilState = m_depthStencilState->GetDepthStencilDesc();
	opaqueDesc.SampleMask = UINT_MAX;
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	opaqueDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	opaqueDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	opaqueDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	opaqueLayer.SetPSO(opaqueDesc);

	// Topology
	opaqueLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	GeometryGenerator geoGen; 
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);  

	std::vector<std::uint16_t> indices = grid.GetIndices16(); 
	std::vector<Vertex> vertices(grid.Vertices.size()); 
	for (size_t i = 0; i < grid.Vertices.size(); ++i) 
	{
		auto& p = grid.Vertices[i].Position;			// Extract the vertex elements we are interested and apply the height function to
		vertices[i].Pos = p;							// each vertex. In addition, color the vertices based on their height so we have
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);	// sandy looking beaches, grassy low hills, and snow mountain peaks.
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	std::vector<std::vector<Vertex>> allVertices;
	allVertices.push_back(std::move(vertices));
	std::vector<std::vector<std::uint16_t>> allIndices;
	allIndices.push_back(std::move(indices));

	opaqueLayer.Meshes = std::make_unique<MeshGroupT<Vertex>>(m_deviceResources, allVertices, allIndices);

	// Render Items
	m_gridObjectConstantsCB = std::make_unique<ConstantBufferT<ObjectConstants>>(m_deviceResources);
	m_gridMaterialCB = std::make_unique<ConstantBufferT<Material>>(m_deviceResources);

	RenderItem& gridRI = opaqueLayer.RenderItems.emplace_back();

	gridRI.World = MathHelper::Identity4x4();
	DirectX::XMStoreFloat4x4(&gridRI.TexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));

	gridRI.material = std::make_unique<Material>();
	gridRI.material->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	gridRI.material->FresnelR0 = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
	gridRI.material->Roughness = 0.125f;

	auto& gridConstantsCBV = gridRI.ConstantBufferViews.emplace_back(1, m_gridObjectConstantsCB.get());
	gridConstantsCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (ri->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&ri->World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&ri->TexTransform);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			m_gridObjectConstantsCB->CopyData(frameIndex, objConstants);

			--ri->NumFramesDirty;
		}
	};

	auto& gridMaterialCBV = gridRI.ConstantBufferViews.emplace_back(3, m_gridMaterialCB.get());
	gridMaterialCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		if (ri->materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&ri->material->MatTransform);

			Material mat = *ri->material.get(); 
			DirectX::XMStoreFloat4x4(&mat.MatTransform, DirectX::XMMatrixTranspose(transform)); 

			m_gridMaterialCB->CopyData(frameIndex, mat);

			// Next FrameResource need to be updated too.
			--ri->materialNumFramesDirty;
		}
	};

	gridRI.submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0
	
	int grassTexture = 0; // grass is texture #0
	auto& dt = gridRI.DescriptorTables.emplace_back(0, m_textures[grassTexture]->GetGPUHandle());
	dt.Update = [](const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};







	// Render Pass Layer: Alpha Test ----------------------------------------------------------------------
	RenderPassLayer& alphaTestLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);

	// PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestDesc = opaqueDesc;
	alphaTestDesc.PS = m_alphaTestedPS->GetShaderByteCode();
	alphaTestDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	alphaTestLayer.SetPSO(alphaTestDesc);

	// Topology
	alphaTestLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Meshes
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

	std::vector<std::uint16_t> boxIndices = box.GetIndices16();
	std::vector<Vertex> boxVertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		boxVertices[i].Pos = p;
		boxVertices[i].Normal = box.Vertices[i].Normal;
		boxVertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::vector<Vertex>> allBoxVertices;
	allBoxVertices.push_back(std::move(boxVertices));
	std::vector<std::vector<std::uint16_t>> allBoxIndices;
	allBoxIndices.push_back(std::move(boxIndices));

	alphaTestLayer.Meshes = std::make_unique<MeshGroupT<Vertex>>(m_deviceResources, allBoxVertices, allBoxIndices);

	// Render Items
	m_boxObjectConstantsCB = std::make_unique<ConstantBufferT<ObjectConstants>>(m_deviceResources);
	m_boxMaterialCB = std::make_unique<ConstantBufferT<Material>>(m_deviceResources);

	RenderItem& boxRI = alphaTestLayer.RenderItems.emplace_back();

	DirectX::XMStoreFloat4x4(&boxRI.World, DirectX::XMMatrixTranslation(3.0f, 2.0f, -9.0f));

	boxRI.material = std::make_unique<Material>();
	boxRI.material->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	boxRI.material->FresnelR0 = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
	boxRI.material->Roughness = 0.25f;

	auto& boxConstantsCBV = boxRI.ConstantBufferViews.emplace_back(1, m_boxObjectConstantsCB.get());
	boxConstantsCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (ri->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&ri->World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&ri->TexTransform);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			m_boxObjectConstantsCB->CopyData(frameIndex, objConstants);

			--ri->NumFramesDirty;
		}
	};

	auto& boxMaterialCBV = boxRI.ConstantBufferViews.emplace_back(3, m_boxMaterialCB.get());
	boxMaterialCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		if (ri->materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&ri->material->MatTransform);

			Material mat = *ri->material.get();
			DirectX::XMStoreFloat4x4(&mat.MatTransform, DirectX::XMMatrixTranspose(transform));

			m_boxMaterialCB->CopyData(frameIndex, mat);

			// Next FrameResource need to be updated too.
			--ri->materialNumFramesDirty;
		}
	};

	boxRI.submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0

	int boxTexture = 2; // box is texture #2
	auto& boxDT = boxRI.DescriptorTables.emplace_back(0, m_textures[boxTexture]->GetGPUHandle());
	boxDT.Update = [](const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};







	// Render Pass Layer: Transparent ----------------------------------------------------------------------
	RenderPassLayer& transparentLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);

	// PSO
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentDesc = opaqueDesc; 
	transparentDesc.BlendState = transparentBlendState->GetBlendDesc(); 

	transparentLayer.SetPSO(transparentDesc);

	// Topology
	transparentLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	std::vector<std::uint16_t> waveIndices(3 * m_waves->TriangleCount()); // 3 indices per face
	TINY_CORE_ASSERT(m_waves->VertexCount() < 0x0000ffff, "Too many vertices");

	// Iterate over each quad.
	int m = m_waves->RowCount();
	int n = m_waves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			waveIndices[k] = i * n + j;
			waveIndices[k + 1] = i * n + j + 1;
			waveIndices[k + 2] = (i + 1) * n + j;

			waveIndices[k + 3] = (i + 1) * n + j;
			waveIndices[k + 4] = i * n + j + 1;
			waveIndices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	std::vector<Vertex> waveVertices(m_waves->VertexCount());
	for (int i = 0; i < m_waves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = m_waves->Position(i);
		v.Normal = m_waves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / m_waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / m_waves->Depth();

		waveVertices[i] = v;
	}

	transparentLayer.Meshes = std::make_unique<DynamicMeshGroupT<Vertex>>(m_deviceResources, std::move(waveVertices), std::move(waveIndices));
	m_dynamicWaveMesh = static_cast<DynamicMeshGroupT<Vertex>*>(transparentLayer.Meshes.get());

	// Render Items
	m_wavesObjectConstantsCB = std::make_unique<ConstantBufferT<ObjectConstants>>(m_deviceResources);
	m_wavesMaterialCB = std::make_unique<ConstantBufferT<Material>>(m_deviceResources);

	RenderItem& wavesRI = transparentLayer.RenderItems.emplace_back();

	wavesRI.World = MathHelper::Identity4x4();
	DirectX::XMStoreFloat4x4(&wavesRI.TexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRI.material = std::make_unique<Material>();
	wavesRI.material->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	wavesRI.material->FresnelR0 = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
	wavesRI.material->Roughness = 0.0f;
		
	m_wavesRI = &wavesRI;

	auto& wavesConstantsCBV = wavesRI.ConstantBufferViews.emplace_back(1, m_wavesObjectConstantsCB.get());
	wavesConstantsCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.  
		if (ri->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&ri->World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&ri->TexTransform);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(texTransform));

			m_wavesObjectConstantsCB->CopyData(frameIndex, objConstants);

			--ri->NumFramesDirty;
		}
	};

	auto& wavesMaterialCBV = wavesRI.ConstantBufferViews.emplace_back(3, m_wavesMaterialCB.get());
	wavesMaterialCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex)
	{
		if (ri->materialNumFramesDirty > 0)
		{
			// Must transpose the transform before loading it into the constant buffer
			DirectX::XMMATRIX transform = DirectX::XMLoadFloat4x4(&ri->material->MatTransform);

			Material mat = *ri->material.get();
			DirectX::XMStoreFloat4x4(&mat.MatTransform, DirectX::XMMatrixTranspose(transform));

			m_wavesMaterialCB->CopyData(frameIndex, mat);

			// Next FrameResource need to be updated too.
			--ri->materialNumFramesDirty;
		}
	};

	wavesRI.submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0

	int waterTexture = 1; // water is texture #1
	auto& waveDT = wavesRI.DescriptorTables.emplace_back(0, m_textures[waterTexture]->GetGPUHandle());
	waveDT.Update = [](const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};



}




void TheApp::Update(const Timer& timer)
{
	// IMPORTANT: Do all necessary updates/animation first, but then be sure to call Engine::Update()

	UpdateCamera(timer);

	UpdateWavesVertices(timer);
	UpdateWavesMaterials(timer);


	// IMPORTANT: Must call this last so that the updates made above will take effect for this frame
	Engine::Update(timer);
}

void TheApp::UpdateCamera(const Timer& timer)
{
	const float dt = timer.DeltaTime();

	if (m_keyWIsDown)
		m_camera.Walk(10.0f * dt);

	if (m_keySIsDown)
		m_camera.Walk(-10.0f * dt);

	if (m_keyAIsDown)
		m_camera.Strafe(-10.0f * dt);

	if (m_keyDIsDown)
		m_camera.Strafe(10.0f * dt);

	m_camera.UpdateViewMatrix();
}
void TheApp::UpdateWavesVertices(const Timer& timer)
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
	std::vector<Vertex> waveVertices(m_waves->VertexCount());
	for (int i = 0; i < m_waves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = m_waves->Position(i);
		v.Normal = m_waves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / m_waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / m_waves->Depth();

		waveVertices[i] = v;
	}

	m_dynamicWaveMesh->CopyVertices(Engine::GetCurrentFrameIndex(), std::move(waveVertices));
}
void TheApp::UpdateWavesMaterials(const Timer& timer)
{
	// Scroll the water material texture coordinates.

	Material* waveMaterial = m_wavesRI->material.get();

	float& tu = waveMaterial->MatTransform(3, 0);
	float& tv = waveMaterial->MatTransform(3, 1);

	tu += 0.1f * timer.DeltaTime();
	tv += 0.02f * timer.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waveMaterial->MatTransform(3, 0) = tu;
	waveMaterial->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	m_wavesRI->materialNumFramesDirty = gNumFrameResources;
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







void TheApp::OnMouseMove(float x, float y)
{
	if (m_lButtonDown)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

		m_camera.Pitch(dy);
		m_camera.RotateY(dx);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}



}