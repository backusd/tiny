#include "LandAndWavesSceneCS.h"

using namespace tiny;
using namespace sandbox::landandwavescs;

namespace sandbox
{
LandAndWavesSceneCS::LandAndWavesSceneCS(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources),
	m_mainRenderPass()
{
	PROFILE_SCOPE("LandAndWavesSceneCS()");

	Engine::Init(m_deviceResources);
	DescriptorManager::Init(m_deviceResources);
	TextureManager::Init(m_deviceResources);


	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr));

	m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);

	m_gpuWaves = std::make_unique<GPUWaves>(m_deviceResources, 256, 256, 0.25f, 0.03f, 2.0f, 0.2f);

	LoadTextures();
	BuildLandAndWaterScene();

	// Execute the initialization commands.
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
	ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	m_deviceResources->FlushCommandQueue();
}
void LandAndWavesSceneCS::OnResize(int height, int width)
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
}
void LandAndWavesSceneCS::SetViewport(float top, float left, float height, float width) noexcept
{
	D3D12_VIEWPORT vp{};
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

void LandAndWavesSceneCS::LoadTextures()
{
	PROFILE_FUNCTION();

	for (int iii = 0; iii < (int)TEXTURE::Count; ++iii)
		m_textures[iii] = TextureManager::GetTexture(iii);
}
void LandAndWavesSceneCS::BuildLandAndWaterScene()
{
	PROFILE_FUNCTION();

	// Add name for debug/profiling purposes
	m_mainRenderPass.Name = "Main Render Pass";
	m_mainRenderPass.RenderPassLayers.reserve(3);

	// Compute Layer: Update ---------------------------------------------------------------------------------
	m_mainRenderPass.ComputeLayers.reserve(1);
	ComputeLayer& computeLayer = m_mainRenderPass.ComputeLayers.emplace_back(m_deviceResources);
	computeLayer.Name = "Compute Layer: Update";

	// Root Signature
	CD3DX12_DESCRIPTOR_RANGE uavTable0, uavTable1, uavTable2; 
	uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); 
	uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); 
	uavTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2); 
	
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER csSlotRootParameter[4]; 

	// Perfomance TIP: Order from most frequent to least frequent.
	csSlotRootParameter[0].InitAsConstantBufferView(0);
	csSlotRootParameter[1].InitAsDescriptorTable(1, &uavTable0); 
	csSlotRootParameter[2].InitAsDescriptorTable(1, &uavTable1); 
	csSlotRootParameter[3].InitAsDescriptorTable(1, &uavTable2); 

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC computeRootSigDesc(4, csSlotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	computeLayer.RootSignature = std::make_shared<RootSignature>(m_deviceResources, computeRootSigDesc);

	// PSO
	m_wavesUpdateCS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/WavesUpdateCS.cso");

	D3D12_COMPUTE_PIPELINE_STATE_DESC wavesUpdatePSO;
	ZeroMemory(&wavesUpdatePSO, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	wavesUpdatePSO.pRootSignature = computeLayer.RootSignature->Get();
	wavesUpdatePSO.CS = m_wavesUpdateCS->GetShaderByteCode();
	wavesUpdatePSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE; 

	computeLayer.SetPSO(wavesUpdatePSO);

	// Compute Item
	ComputeItem& computeItem = computeLayer.ComputeItems.emplace_back();
	computeItem.ThreadGroupCountX = m_gpuWaves->NumColumns() / 16;
	computeItem.ThreadGroupCountY = m_gpuWaves->NumRows() / 16;
	computeItem.ThreadGroupCountZ = 1;

	m_waveUpdateSettingsCB = std::make_unique<ConstantBufferT<WavesUpdateSettings>>(m_deviceResources); 
	auto& waveUpdateCBV = computeItem.ConstantBufferViews.emplace_back(0, m_waveUpdateSettingsCB.get());
	waveUpdateCBV.Update = [this](const Timer& timer, int frameIndex)
	{
		// Only update the cbuffer data if the constants have changed.
		if (m_waveUpdateNumFramesDirty > 0)
		{
			WavesUpdateSettings settings;
			settings.WaveConstant0 = m_gpuWaves->WaveConstant(0);
			settings.WaveConstant1 = m_gpuWaves->WaveConstant(1);
			settings.WaveConstant2 = m_gpuWaves->WaveConstant(2);
			settings.DisturbIndex = DirectX::XMINT2(0, 0);
			settings.DisturbMag = 1.0f;
		
			m_waveUpdateSettingsCB->CopyData(frameIndex, settings);
		
			--m_waveUpdateNumFramesDirty;
		}
	};
	computeItem.DescriptorTables.reserve(4);
	auto& prevSolDT = computeItem.DescriptorTables.emplace_back(1, m_gpuWaves->PrevSol()->GetUAVHandle());
	prevSolDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->PrevSol()->GetUAVHandle();
	};
	
	auto& currSolDT = computeItem.DescriptorTables.emplace_back(2, m_gpuWaves->CurrSol()->GetUAVHandle());
	currSolDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->CurrSol()->GetUAVHandle();
	}; 
	
	auto& nextSolDT = computeItem.DescriptorTables.emplace_back(3, m_gpuWaves->NextSol()->GetUAVHandle());
	nextSolDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->NextSol()->GetUAVHandle();
	};

	computeLayer.PreWork = [this](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) -> bool
	{
		m_gpuWaves->PreUpdate();
		return true;
	};
	computeLayer.PostWork = [this](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int) 
	{
		m_gpuWaves->PostUpdate();
	};


	// Compute Layer: Disturb ---------------------------------------------------------------------------------
	m_wavesComputeLayerDisturb = std::make_unique<ComputeLayer>(m_deviceResources);
	m_wavesComputeLayerDisturb->Name = "Compute Layer: Disturb";
	Engine::AddComputeUpdateLayer(m_wavesComputeLayerDisturb.get());

	// Root Signature
	m_wavesComputeLayerDisturb->RootSignature = computeLayer.RootSignature;

	// PSO
	m_wavesDisturbCS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/WavesDisturbCS.cso");

	D3D12_COMPUTE_PIPELINE_STATE_DESC wavesDisturbPSO = wavesUpdatePSO;
	wavesDisturbPSO.CS = m_wavesDisturbCS->GetShaderByteCode();

	m_wavesComputeLayerDisturb->SetPSO(wavesDisturbPSO);

	// Compute Item
	ComputeItem& computeItemDisturb = m_wavesComputeLayerDisturb->ComputeItems.emplace_back();
	computeItemDisturb.ThreadGroupCountX = 1;
	computeItemDisturb.ThreadGroupCountY = 1;
	computeItemDisturb.ThreadGroupCountZ = 1;

	auto& waveDisturbCBV = computeItemDisturb.ConstantBufferViews.emplace_back(0, m_waveUpdateSettingsCB.get());
	waveDisturbCBV.Update = [this](const Timer& timer, int frameIndex) 
	{
	};

	computeItemDisturb.DescriptorTables.reserve(4);

	auto& prevSolDisturbDT = computeItemDisturb.DescriptorTables.emplace_back(1, m_gpuWaves->PrevSol()->GetUAVHandle());
	prevSolDisturbDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->PrevSol()->GetUAVHandle();
	};

	auto& currSolDisturbDT = computeItemDisturb.DescriptorTables.emplace_back(2, m_gpuWaves->CurrSol()->GetUAVHandle());
	currSolDisturbDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->CurrSol()->GetUAVHandle();
	};

	auto& nextSolDisturbDT = computeItemDisturb.DescriptorTables.emplace_back(3, m_gpuWaves->NextSol()->GetUAVHandle());
	nextSolDisturbDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// The textures get shuffled every frame, so this needs to get updated every frame as well
		dt->DescriptorHandle = m_gpuWaves->NextSol()->GetUAVHandle();
	};

	m_wavesComputeLayerDisturb->PreWork = [this](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer* timer, int frameIndex) -> bool
	{
		// Every quarter second, generate a random wave.
		static float t_base = 0.0f;

		// Check if we are beyond the quarter second mark
		if (timer->TotalTime() - t_base > 0.25f)
		{
			t_base += 0.25f;

			m_gpuWaves->PreUpdate(); // Call PreUpdate to tansition the texture to UAV

			WavesUpdateSettings settings;
			settings.WaveConstant0 = m_gpuWaves->WaveConstant(0);
			settings.WaveConstant1 = m_gpuWaves->WaveConstant(1);
			settings.WaveConstant2 = m_gpuWaves->WaveConstant(2);

			int i = MathHelper::Rand(4, m_gpuWaves->NumRows() - 5);
			int j = MathHelper::Rand(4, m_gpuWaves->NumColumns() - 5);
			float r = MathHelper::RandF(1.0f, 2.0f);

			settings.DisturbIndex = DirectX::XMINT2(j, i);
			settings.DisturbMag = r;

			m_waveUpdateSettingsCB->CopyData(frameIndex, settings); 
			return true;
		}

		// return false because we only want to apply an update every quarter second
		return false;
	};
	m_wavesComputeLayerDisturb->PostWork = [this](const ComputeLayer&, ID3D12GraphicsCommandList*, const Timer*, int)
	{
		//m_gpuWaves->PostUpdate();
	};
















	// Root Signature --------------------------------------------------------------------------------
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE displacementMapTable; 
	displacementMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); 

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // ObjectCB
	slotRootParameter[2].InitAsConstantBufferView(1); // PassConstants
	slotRootParameter[3].InitAsConstantBufferView(2); // MaterialCB
	slotRootParameter[4].InitAsDescriptorTable(1, &displacementMapTable, D3D12_SHADER_VISIBILITY_ALL); 


	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_mainRenderPass.RootSignature = std::make_shared<RootSignature>(m_deviceResources, rootSigDesc);


	// Per-pass constants -----------------------------------------------------------------------------
	m_mainRenderPassConstantsCB = std::make_unique<ConstantBufferT<PassConstants>>(m_deviceResources);

	// Currently using root parameter index #2 for the per-pass constants
	auto& renderPassCBV = m_mainRenderPass.ConstantBufferViews.emplace_back(2, m_mainRenderPassConstantsCB.get());
	renderPassCBV.Update = [this](const Timer& timer, int frameIndex)
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
	opaqueLayer.Name = "Opaque Layer";

	// PSO
	m_standardVS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingVS.cso");
	m_opaquePS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingFogPS.cso");
	m_alphaTestedPS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingFogAlphaTestPS.cso");


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

	opaqueLayer.Meshes = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, allVertices, allIndices);

	// Render Items
	m_gridObject = std::make_unique<GameObject>(m_deviceResources); // Create the grid (NOTE: This does NOT create a RenderItem)
	m_gridObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_gridObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f));
	m_gridObject->SetMaterialRoughness(0.125f);
	m_gridObject->SetTextureTransform(DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
	RenderItem* gridRI = m_gridObject->CreateRenderItem(&opaqueLayer);

	gridRI->submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0

	auto& gridDT = gridRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::GRASS]->GetSRVHandle());
	gridDT.Update = [](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};





	// Render Pass Layer: Alpha Test ----------------------------------------------------------------------
	RenderPassLayer& alphaTestLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	alphaTestLayer.Name = "Alpha Test Layer";

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

	alphaTestLayer.Meshes = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, allBoxVertices, allBoxIndices);

	// Render Items
	m_boxObject = std::make_unique<GameObject>(m_deviceResources); // Create the box (NOTE: This does NOT create a RenderItem)
	m_boxObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_boxObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));
	m_boxObject->SetMaterialRoughness(0.25f);
	m_boxObject->SetWorldTransform(DirectX::XMMatrixTranslation(3.0f, 2.0f, -9.0f));
	RenderItem* boxRI = m_boxObject->CreateRenderItem(&alphaTestLayer);

	boxRI->submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0

	auto& boxDT = boxRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::WIRE_FENCE]->GetSRVHandle());
	boxDT.Update = [](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};




	// Render Pass Layer: Transparent ----------------------------------------------------------------------
	RenderPassLayer& gpuWavesLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	gpuWavesLayer.Name = "GPU Waves Layer";
	
	// PSO
	m_wavesVS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/WavesVS.cso");

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
	transparentDesc.VS = m_wavesVS->GetShaderByteCode();

	gpuWavesLayer.SetPSO(transparentDesc);  

	// Topology
	gpuWavesLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	GeometryGenerator::MeshData waterGrid = geoGen.CreateGrid(160.0f, 160.0f, m_gpuWaves->NumRows(), m_gpuWaves->NumColumns());

	std::vector<std::uint16_t> waterIndices = waterGrid.GetIndices16();
	std::vector<Vertex> waterVertices(waterGrid.Vertices.size());
	for (size_t i = 0; i < waterGrid.Vertices.size(); ++i)
	{
		waterVertices[i].Pos    = waterGrid.Vertices[i].Position;
		waterVertices[i].Normal = waterGrid.Vertices[i].Normal;
		waterVertices[i].TexC   = waterGrid.Vertices[i].TexC;
	}

	std::vector<std::vector<Vertex>> allWaterVertices;
	allWaterVertices.push_back(std::move(waterVertices));
	std::vector<std::vector<std::uint16_t>> allWaterIndices;
	allWaterIndices.push_back(std::move(waterIndices));

	gpuWavesLayer.Meshes = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, allWaterVertices, allWaterIndices);

	// Render Items
	m_wavesObject = std::make_unique<GridGameObject>(m_deviceResources);  // Create the waves (NOTE: This does NOT create a RenderItem)
	m_wavesObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f));
	m_wavesObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));
	m_wavesObject->SetMaterialRoughness(0.0f);
	m_wavesObject->SetTextureTransform(DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
	m_wavesObject->SetDisplacementMapTexelSize(DirectX::XMFLOAT2(1.0f / m_gpuWaves->NumColumns(), 1.0f / m_gpuWaves->NumRows()));
	m_wavesObject->SetGridSpatialStep(m_gpuWaves->SpatialStep());
	RenderItem* wavesRI = m_wavesObject->CreateRenderItem(&gpuWavesLayer);

	wavesRI->submeshIndex = 0;

	auto& wavesDT = wavesRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::WATER1]->GetSRVHandle());
	wavesDT.Update = [](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};
	auto& wavesDisplacementMapDT = wavesRI->DescriptorTables.emplace_back(4, m_gpuWaves->CurrSol()->GetSRVHandle());
	wavesDisplacementMapDT.Update = [this](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// Need to update the descriptor handle every frame because the "current solution" will get shuffled
		// every frame
		dt->DescriptorHandle = m_gpuWaves->CurrSol()->GetSRVHandle();
	};


}



void LandAndWavesSceneCS::Update(const Timer& timer)
{
	PROFILE_FUNCTION();

	// IMPORTANT: Do all necessary updates/animation first, but then be sure to call Engine::Update()

	UpdateCamera(timer);

	// Land And Water Scene Update --------------------------------------------------------------------
	//UpdateWavesVertices(timer);
	UpdateWavesMaterials(timer);


	// IMPORTANT: Must call this last so that the updates made above will take effect for this frame
	Engine::Update(timer);
}

void LandAndWavesSceneCS::UpdateCamera(const Timer& timer)
{
	PROFILE_FUNCTION();

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

void LandAndWavesSceneCS::UpdateWavesMaterials(const Timer& timer)
{
	PROFILE_FUNCTION();

	// Scroll the water material texture coordinates.
	DirectX::XMFLOAT4X4& matTransform = m_wavesObject->GetMaterialTransform();

	float& tu = matTransform(3, 0);
	float& tv = matTransform(3, 1);

	tu += 0.1f * timer.DeltaTime();
	tv += 0.02f * timer.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	matTransform(3, 0) = tu;
	matTransform(3, 1) = tv;
}


std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> LandAndWavesSceneCS::GetStaticSamplers()
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

float LandAndWavesSceneCS::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}
DirectX::XMFLOAT3 LandAndWavesSceneCS::GetHillsNormal(float x, float z)const
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

void LandAndWavesSceneCS::OnMouseMove(float x, float y)
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


// ======================================================================================================
// Waves
// ======================================================================================================
namespace landandwavescs
{
	GPUWaves::GPUWaves(std::shared_ptr<tiny::DeviceResources> deviceResources,
		UINT m, UINT n,
		float dx, float dt,
		float speed, float damping) :
		m_deviceResources(deviceResources),
		m_textureVector(deviceResources),
		m_numRows(m),
		m_numColumns(n),
		m_vertexCount(m * n),
		m_TriangleCount((m - 1) * (n - 1) * 2),
		m_timeStep(dt),
		m_spatialStep(dx)
	{
		TINY_ASSERT((m * n) % 256 == 0, "Must be a multiple of 256");

		float d = damping * dt + 2.0f; 
		float e = (speed * speed) * (dt * dt) / (dx * dx);
		m_k[0] = (damping * dt - 2.0f) / d;
		m_k[1] = (4.0f - 8.0f * e) / d;
		m_k[2] = (2.0f * e) / d;

		D3D12_RESOURCE_DESC texDesc; 
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC)); 
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; 
		texDesc.Alignment = 0; 
		texDesc.Width = m_numColumns; 
		texDesc.Height = m_numRows; 
		texDesc.DepthOrArraySize = 1; 
		texDesc.MipLevels = 1; 
		texDesc.Format = DXGI_FORMAT_R32_FLOAT; 
		texDesc.SampleDesc.Count = 1; 
		texDesc.SampleDesc.Quality = 0; 
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; 

		m_prevSol = m_textureVector.EmplaceBack(texDesc, true, true);
		m_currSol = m_textureVector.EmplaceBack(texDesc, true, true);
		m_nextSol = m_textureVector.EmplaceBack(texDesc, true, true);

		TINY_ASSERT(m_prevSol != nullptr, "Something went wrong");
		TINY_ASSERT(m_currSol != nullptr, "Something went wrong");
		TINY_ASSERT(m_nextSol != nullptr, "Something went wrong");

		std::vector<float> initData(m_numRows * m_numColumns, 0.0f);
		for (int i = 0; i < initData.size(); ++i) 
			initData[i] = 0.0f; 

		m_prevSol->TransitionToState(D3D12_RESOURCE_STATE_COPY_DEST);
		m_prevSol->CopyData(initData);
		m_prevSol->TransitionToState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		m_currSol->TransitionToState(D3D12_RESOURCE_STATE_COPY_DEST);
		m_currSol->CopyData(initData);
		m_currSol->TransitionToState(D3D12_RESOURCE_STATE_GENERIC_READ);

		m_nextSol->TransitionToState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	void GPUWaves::PreUpdate()
	{
		// The current solution needs to be transitioned to have unordered access by the compute shader
		m_currSol->TransitionToState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	void GPUWaves::PostUpdate()
	{
		Texture* tmp = m_prevSol;
		m_prevSol = m_currSol;
		m_currSol = m_nextSol;
		m_nextSol = tmp;

		// The current solution needs to be able to be read by the vertex shader, so change its state to GENERIC_READ.
		m_currSol->TransitionToState(D3D12_RESOURCE_STATE_GENERIC_READ);
	}

}
}