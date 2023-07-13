#include "StencilExample.h"

using namespace tiny;
using namespace sandbox::stencilexample;


using DirectX::XMFLOAT2;
using DirectX::XMFLOAT3;


namespace sandbox
{
StencilExample::StencilExample(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources),
	m_mainRenderPass(),
	m_reflectedRenderPass(),
	m_mirrorAndShadowsRenderPass()
{
	PROFILE_SCOPE("StencilExample()");

	Engine::Init(m_deviceResources);
	TextureManager::Init(m_deviceResources);


	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr));

	m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);

	LoadTextures();
	BuildStencilExample();

	// Execute the initialization commands.
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
	ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	m_deviceResources->FlushCommandQueue();
}
void StencilExample::OnResize(int height, int width)
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
}
void StencilExample::SetViewport(float top, float left, float height, float width) noexcept
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
void StencilExample::LoadTextures()
{
	PROFILE_FUNCTION();

	for (int iii = 0; iii < (int)TEXTURE::Count; ++iii)
		m_textures[iii] = TextureManager::GetTexture(iii);
}
void StencilExample::BuildStencilExample()
{
	PROFILE_FUNCTION();

	// Main Render Pass:
	//		1. Opaque Layer - Draw floor, walls, skull
	//		2. Mirror Layer - Draw mirror setting the stencil value to 1
	// Reflected Render Pass:
	//		1. Reflected Layer - Draw all opaque objects, but only in the mirror
	// Mirror and Shadow Pass:
	//		1. Transparent Layer - Draw the mirror with transparency to get blending
	//		2. Shadow Layer - Draw shadows
	
	CreateSharedPassResources();
	BuildMainRenderPass();
	BuildReflectedRenderPass();
	BuildMirrorAndShadowRenderPass();
}
void StencilExample::CreateSharedPassResources()
{
	// Shaders
	m_standardVS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingVS.cso");
	m_opaquePS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingFogPS.cso");
	m_alphaTestedPS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/LightingFogAlphaTestPS.cso");

	// Input Layout
	m_inputLayout = std::make_unique<InputLayout>( 
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		}
	);
}
void StencilExample::BuildMainRenderPass()
{
	PROFILE_FUNCTION();

	// Add name for debug/profiling purposes
	m_mainRenderPass.Name = "Main Render Pass";
	m_mainRenderPass.RenderPassLayers.reserve(2);

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

		DirectX::XMStoreFloat4x4(&m_passConstants.View, DirectX::XMMatrixTranspose(view));
		DirectX::XMStoreFloat4x4(&m_passConstants.InvView, DirectX::XMMatrixTranspose(invView));
		DirectX::XMStoreFloat4x4(&m_passConstants.Proj, DirectX::XMMatrixTranspose(proj));
		DirectX::XMStoreFloat4x4(&m_passConstants.InvProj, DirectX::XMMatrixTranspose(invProj));
		DirectX::XMStoreFloat4x4(&m_passConstants.ViewProj, DirectX::XMMatrixTranspose(viewProj));
		DirectX::XMStoreFloat4x4(&m_passConstants.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));

		m_passConstants.EyePosW = m_camera.GetPosition3f();

		float height = static_cast<float>(m_deviceResources->GetHeight());
		float width = static_cast<float>(m_deviceResources->GetWidth());

		m_passConstants.RenderTargetSize = DirectX::XMFLOAT2(width, height);
		m_passConstants.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
		m_passConstants.NearZ = 1.0f;
		m_passConstants.FarZ = 1000.0f;
		m_passConstants.TotalTime = timer.TotalTime();
		m_passConstants.DeltaTime = timer.DeltaTime();

		m_passConstants.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

		m_passConstants.FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		m_passConstants.gFogStart = 5.0f;
		m_passConstants.gFogRange = 150.0f;

		//m_passConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
		//m_passConstants.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };
		//m_passConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		//m_passConstants.Lights[1].Strength = { 0.5f, 0.5f, 0.5f };
		//m_passConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
		//m_passConstants.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

		m_passConstants.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
		m_passConstants.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
		m_passConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		m_passConstants.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
		m_passConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
		m_passConstants.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

		m_mainRenderPassConstantsCB->CopyData(frameIndex, m_passConstants);
	};

	// Render Pass Layer: Opaque ----------------------------------------------------------------------
	RenderPassLayer& opaqueLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	opaqueLayer.Name = "Opaque Layer";
	opaqueLayer.RenderItems.reserve(3);

	// PSO
	m_opaqueRasterizerState = std::make_unique<RasterizerState>();
	m_opaqueBlendState = std::make_unique<BlendState>();
	m_opaqueDepthStencilState = std::make_unique<DepthStencilState>();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	opaqueDesc.pRootSignature = m_mainRenderPass.RootSignature->Get();
	opaqueDesc.VS = m_standardVS->GetShaderByteCode();
	opaqueDesc.PS = m_opaquePS->GetShaderByteCode();
	opaqueDesc.RasterizerState = m_opaqueRasterizerState->GetRasterizerDesc();
	opaqueDesc.BlendState = m_opaqueBlendState->GetBlendDesc();
	opaqueDesc.DepthStencilState = m_opaqueDepthStencilState->GetDepthStencilDesc();
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
	std::vector<std::vector<Vertex>> allOpaqueVertices;
	std::vector<std::vector<std::uint16_t>> allOpaqueIndices;

	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//   |--------------|
	//   |              |
	//   |----|----|----|
	//   |Wall|Mirr|Wall|
	//   |    | or |    |
	//   /--------------/
	//  /   Floor      /
	// /--------------/
	std::vector<Vertex> floorVertices =
	{
		// Floor: Observe we tile texture coordinates.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f),
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f)
	};
	std::vector<std::uint16_t> floorIndices =
	{
		// Floor
		0, 1, 2,
		0, 2, 3
	};

	allOpaqueVertices.push_back(std::move(floorVertices));
	allOpaqueIndices.push_back(std::move(floorIndices));

	std::vector<Vertex> wallVertices =
	{
		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f),
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};
	std::vector<std::uint16_t> wallIndices =
	{
		// Walls
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11
	};

	allOpaqueVertices.push_back(std::move(wallVertices));
	allOpaqueIndices.push_back(std::move(wallIndices));

	std::vector<Vertex> skullVertices;
	skullVertices.reserve(31067);
	std::vector<uint16_t> skullIndices;
	skullIndices.reserve(181017);
	LoadSkullGeometry(skullVertices, skullIndices);
	allOpaqueVertices.push_back(std::move(skullVertices));
	allOpaqueIndices.push_back(std::move(skullIndices));

	opaqueLayer.Meshes = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, allOpaqueVertices, allOpaqueIndices);


	// Render Items ---------------------
	// 
	// Floor
	m_floorObject = std::make_unique<GameObject>(m_deviceResources); // Create the floor (NOTE: This does NOT create a RenderItem)
	m_floorObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_floorObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.07f, 0.07f, 0.07f));
	m_floorObject->SetMaterialRoughness(0.3f);
	RenderItem* floorRI = m_floorObject->CreateRenderItem(&opaqueLayer);
	floorRI->submeshIndex = 0;

	auto& floorDT = floorRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::CHECKBOARD]->GetGPUHandle());
	floorDT.Update = [](const Timer& timer, int frameIndex) { }; // No update here because the texture is static

	// Wall
	m_wallObject = std::make_unique<GameObject>(m_deviceResources); // Create the wall (NOTE: This does NOT create a RenderItem)
	m_wallObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_wallObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f));
	m_wallObject->SetMaterialRoughness(0.25f);
	RenderItem* wallRI = m_wallObject->CreateRenderItem(&opaqueLayer);
	wallRI->submeshIndex = 1;

	auto& wallDT = wallRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::BRICKS3]->GetGPUHandle());
	wallDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static

	// Skull
	m_skullObject = std::make_unique<GameObject>(m_deviceResources); // Create the skull (NOTE: This does NOT create a RenderItem)
	m_skullObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_skullObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f));
	m_skullObject->SetMaterialRoughness(0.3f);
	m_skullObject->SetWorldTransform(DirectX::XMMatrixRotationY(0.5f * MathHelper::Pi) * DirectX::XMMatrixScaling(0.45f, 0.45f, 0.45f) * DirectX::XMMatrixTranslation(0.0f, 1.0f, -5.0f));
	RenderItem* skullRI = m_skullObject->CreateRenderItem(&opaqueLayer);
	skullRI->submeshIndex = 2;

	auto& skullDT = skullRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::WHITE1X1]->GetGPUHandle());
	skullDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static


	// Render Pass Layer: Mirror (Stencil) ----------------------------------------------------------------------
	RenderPassLayer& mirrorLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	mirrorLayer.Name = "Mirror (Stencil) Layer";
	
	// For this layer, we set the stencil value = 1
	mirrorLayer.PreWork = [](const RenderPassLayer& layer, ID3D12GraphicsCommandList* commandList)
	{
		commandList->OMSetStencilRef(1);
	};

	// PSO
	m_mirrorStencilRasterizerState = std::make_unique<RasterizerState>();

	m_mirrorStencilBlendState = std::make_unique<BlendState>();
	m_mirrorStencilBlendState->SetRenderTargetWriteMask(0);

	m_mirrorStencilDepthStencilState = std::make_unique<DepthStencilState>();
	m_mirrorStencilDepthStencilState->SetDepthEnabled(true);
	m_mirrorStencilDepthStencilState->SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
	m_mirrorStencilDepthStencilState->SetDepthFunc(D3D12_COMPARISON_FUNC_LESS);
	m_mirrorStencilDepthStencilState->SetStencilEnabled(true);
	m_mirrorStencilDepthStencilState->SetStencilReadMask(0xff);
	m_mirrorStencilDepthStencilState->SetStencilWriteMask(0xff);

	m_mirrorStencilDepthStencilState->SetFrontFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_mirrorStencilDepthStencilState->SetFrontFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_mirrorStencilDepthStencilState->SetFrontFaceStencilPassOp(D3D12_STENCIL_OP_REPLACE);
	m_mirrorStencilDepthStencilState->SetFrontFaceStencilFunc(D3D12_COMPARISON_FUNC_ALWAYS);
	// We are not rendering backfacing polygons, so these settings do not matter.
	m_mirrorStencilDepthStencilState->SetBackFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_mirrorStencilDepthStencilState->SetBackFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_mirrorStencilDepthStencilState->SetBackFaceStencilPassOp(D3D12_STENCIL_OP_REPLACE);
	m_mirrorStencilDepthStencilState->SetBackFaceStencilFunc(D3D12_COMPARISON_FUNC_ALWAYS);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mirrorDesc;
	ZeroMemory(&mirrorDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	mirrorDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	mirrorDesc.pRootSignature = m_mainRenderPass.RootSignature->Get();
	mirrorDesc.VS = m_standardVS->GetShaderByteCode();
	mirrorDesc.PS = m_opaquePS->GetShaderByteCode();
	mirrorDesc.RasterizerState = m_mirrorStencilRasterizerState->GetRasterizerDesc();
	mirrorDesc.BlendState = m_mirrorStencilBlendState->GetBlendDesc();
	mirrorDesc.DepthStencilState = m_mirrorStencilDepthStencilState->GetDepthStencilDesc();
	mirrorDesc.SampleMask = UINT_MAX;
	mirrorDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	mirrorDesc.NumRenderTargets = 1;
	mirrorDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	mirrorDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	mirrorDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	mirrorDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	mirrorLayer.SetPSO(mirrorDesc);

	// Topology
	mirrorLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	std::vector<std::vector<Vertex>> allMirrorVertices;
	std::vector<std::vector<std::uint16_t>> allMirrorIndices;

	std::vector<Vertex> mirrorVertices =
	{
		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};
	std::vector<std::uint16_t> mirrorIndices =
	{
		// Mirror
		0, 1, 2,
		0, 2, 3
	};

	allMirrorVertices.push_back(std::move(mirrorVertices));
	allMirrorIndices.push_back(std::move(mirrorIndices));

	mirrorLayer.Meshes = std::make_shared<MeshGroupT<Vertex>>(m_deviceResources, allMirrorVertices, allMirrorIndices);

	// Render Items ---------------------
	// 
	// Mirror
	m_mirrorStencilObject = std::make_unique<GameObject>(m_deviceResources); // Create the mirror (NOTE: This does NOT create a RenderItem)
	m_mirrorStencilObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_mirrorStencilObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));
	m_mirrorStencilObject->SetMaterialRoughness(0.5f);
	RenderItem* mirrorRI = m_mirrorStencilObject->CreateRenderItem(&mirrorLayer);
	mirrorRI->submeshIndex = 0;

	auto& mirrorDT = mirrorRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::ICE]->GetGPUHandle());
	mirrorDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static

}
void StencilExample::BuildReflectedRenderPass()
{
	PROFILE_FUNCTION();

	// Add name for debug/profiling purposes
	m_reflectedRenderPass.Name = "Reflected Render Pass";
	m_reflectedRenderPass.RenderPassLayers.reserve(1);

	// Root Signature --------------------------------------------------------------------------------
	// Use the same one as the main render pass
	m_reflectedRenderPass.RootSignature = m_mainRenderPass.RootSignature;

	// Per-pass constants -----------------------------------------------------------------------------
	m_reflectedRenderPassConstantsCB = std::make_unique<ConstantBufferT<PassConstants>>(m_deviceResources);

	// Currently using root parameter index #2 for the per-pass constants
	auto& renderPassCBV = m_reflectedRenderPass.ConstantBufferViews.emplace_back(2, m_reflectedRenderPassConstantsCB.get());
	renderPassCBV.Update = [this](const Timer& timer, RenderItem* ri, int frameIndex) 
	{
		// Reflect the lighting but keep all other constants the same

		PassConstants reflectedPassConstants = m_passConstants;

		DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
		DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);

		// Reflect the lighting.
		for (int i = 0; i < 3; ++i)
		{
			DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_passConstants.Lights[i].Direction);
			DirectX::XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
			DirectX::XMStoreFloat3(&reflectedPassConstants.Lights[i].Direction, reflectedLightDir);
		}

		m_reflectedRenderPassConstantsCB->CopyData(frameIndex, reflectedPassConstants);
	};

	// Render Pass Layer: Reflected Layer ----------------------------------------------------------------------
	RenderPassLayer& reflectedLayer = m_reflectedRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	reflectedLayer.Name = "Reflected Layer";

	// PSO
	m_reflectedRasterizerState = std::make_unique<RasterizerState>();
	m_reflectedRasterizerState->SetCullMode(D3D12_CULL_MODE_BACK);
	m_reflectedRasterizerState->SetFrontCounterClockwise(true);

	m_reflectedBlendState = std::make_unique<BlendState>();

	m_reflectedDepthStencilState = std::make_unique<DepthStencilState>();
	m_reflectedDepthStencilState->SetDepthEnabled(true);
	m_reflectedDepthStencilState->SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL);
	m_reflectedDepthStencilState->SetDepthFunc(D3D12_COMPARISON_FUNC_LESS);
	m_reflectedDepthStencilState->SetStencilEnabled(true);
	m_reflectedDepthStencilState->SetStencilReadMask(0xff);
	m_reflectedDepthStencilState->SetStencilWriteMask(0xff);

	m_reflectedDepthStencilState->SetFrontFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetFrontFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetFrontFaceStencilPassOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetFrontFaceStencilFunc(D3D12_COMPARISON_FUNC_EQUAL);
	// We are not rendering backfacing polygons, so these settings do not matter.
	m_reflectedDepthStencilState->SetBackFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetBackFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetBackFaceStencilPassOp(D3D12_STENCIL_OP_KEEP);
	m_reflectedDepthStencilState->SetBackFaceStencilFunc(D3D12_COMPARISON_FUNC_EQUAL);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectedDesc;
	ZeroMemory(&reflectedDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	reflectedDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	reflectedDesc.pRootSignature = m_reflectedRenderPass.RootSignature->Get();
	reflectedDesc.VS = m_standardVS->GetShaderByteCode();
	reflectedDesc.PS = m_opaquePS->GetShaderByteCode();
	reflectedDesc.RasterizerState = m_reflectedRasterizerState->GetRasterizerDesc();
	reflectedDesc.BlendState = m_reflectedBlendState->GetBlendDesc();
	reflectedDesc.DepthStencilState = m_reflectedDepthStencilState->GetDepthStencilDesc();
	reflectedDesc.SampleMask = UINT_MAX;
	reflectedDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	reflectedDesc.NumRenderTargets = 1;
	reflectedDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	reflectedDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	reflectedDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	reflectedDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	reflectedLayer.SetPSO(reflectedDesc);

	// Topology
	reflectedLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	// Don't reload/recreate the skull mesh. Just have this layer reference the opaque layer's MeshGroup, which contains the skull mesh
	reflectedLayer.Meshes = m_mainRenderPass.RenderPassLayers[0].Meshes;

	// Render Items ---------------------
	// 
	// Skull
	m_reflectedSkullObject = std::make_unique<GameObject>(m_deviceResources); // Create the skull (NOTE: This does NOT create a RenderItem)
	m_reflectedSkullObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_reflectedSkullObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f));
	m_reflectedSkullObject->SetMaterialRoughness(0.3f);
	
	DirectX::XMMATRIX world = DirectX::XMMatrixRotationY(0.5f * MathHelper::Pi) * DirectX::XMMatrixScaling(0.45f, 0.45f, 0.45f) * DirectX::XMMatrixTranslation(0.0f, 1.0f, -5.0f);
	DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);
	DirectX::XMMATRIX reflectedWorld = world * R;
	m_reflectedSkullObject->SetWorldTransform(reflectedWorld);

	RenderItem* skullRI = m_reflectedSkullObject->CreateRenderItem(&reflectedLayer);
	skullRI->submeshIndex = 2;

	auto& skullDT = skullRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::WHITE1X1]->GetGPUHandle());
	skullDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static

}
void StencilExample::BuildMirrorAndShadowRenderPass()
{
	using namespace DirectX;

	PROFILE_FUNCTION();

	// Add name for debug/profiling purposes
	m_mirrorAndShadowsRenderPass.Name = "Mirror and Shadows Render Pass";
	m_mirrorAndShadowsRenderPass.RenderPassLayers.reserve(2);

	// Root Signature --------------------------------------------------------------------------------
	// Use the same one as the main render pass
	m_mirrorAndShadowsRenderPass.RootSignature = m_mainRenderPass.RootSignature;

	// Render Pass Layer: Mirror Layer ----------------------------------------------------------------------
	RenderPassLayer& mirrorLayer = m_mirrorAndShadowsRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	mirrorLayer.Name = "Mirror Layer";

	mirrorLayer.PreWork = [](const RenderPassLayer& layer, ID3D12GraphicsCommandList* commandList)
	{
		commandList->OMSetStencilRef(0);
	};

	// PSO
	m_mirrorRasterizerState = std::make_unique<RasterizerState>();
	m_mirrorDepthStencilState = std::make_unique<DepthStencilState>();

	m_mirrorBlendState = std::make_unique<BlendState>();
	m_mirrorBlendState->SetRenderTargetBlendEnabled(true);
	m_mirrorBlendState->SetRenderTargetLogicOpEnabled(false);
	m_mirrorBlendState->SetRenderTargetSrcBlend(D3D12_BLEND_SRC_ALPHA);
	m_mirrorBlendState->SetRenderTargetDestBlend(D3D12_BLEND_INV_SRC_ALPHA);
	m_mirrorBlendState->SetRenderTargetBlendOp(D3D12_BLEND_OP_ADD);
	m_mirrorBlendState->SetRenderTargetSrcBlendAlpha(D3D12_BLEND_ONE);
	m_mirrorBlendState->SetRenderTargetDestBlendAlpha(D3D12_BLEND_ZERO);
	m_mirrorBlendState->SetRenderTargetBlendOpAlpha(D3D12_BLEND_OP_ADD);
	m_mirrorBlendState->SetRenderTargetLogicOp(D3D12_LOGIC_OP_NOOP);
	m_mirrorBlendState->SetRenderTargetWriteMask(D3D12_COLOR_WRITE_ENABLE_ALL);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mirrorDesc;
	ZeroMemory(&mirrorDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	mirrorDesc.InputLayout = m_inputLayout->GetInputLayoutDesc(); 
	mirrorDesc.pRootSignature = m_mirrorAndShadowsRenderPass.RootSignature->Get();
	mirrorDesc.VS = m_standardVS->GetShaderByteCode(); 
	mirrorDesc.PS = m_opaquePS->GetShaderByteCode(); 
	mirrorDesc.RasterizerState = m_mirrorRasterizerState->GetRasterizerDesc();
	mirrorDesc.BlendState = m_mirrorBlendState->GetBlendDesc();
	mirrorDesc.DepthStencilState = m_mirrorDepthStencilState->GetDepthStencilDesc();
	mirrorDesc.SampleMask = UINT_MAX; 
	mirrorDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	mirrorDesc.NumRenderTargets = 1; 
	mirrorDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat(); 
	mirrorDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1; 
	mirrorDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0; 
	mirrorDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat(); 

	mirrorLayer.SetPSO(mirrorDesc);

	// Topology
	mirrorLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	// Don't reload/recreate the mirror mesh. Just have this layer reference the mirror layer's MeshGroup, which contains the mirror mesh
	mirrorLayer.Meshes = m_mainRenderPass.RenderPassLayers[1].Meshes;

	// Render Items ---------------------
	// 
	// Mirror
	m_mirrorObject = std::make_unique<GameObject>(m_deviceResources); // Create the skull (NOTE: This does NOT create a RenderItem)
	m_mirrorObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f));
	m_mirrorObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));
	m_mirrorObject->SetMaterialRoughness(0.5f);

	RenderItem* mirrorRI = m_mirrorObject->CreateRenderItem(&mirrorLayer);
	mirrorRI->submeshIndex = 0;

	auto& mirrorDT = mirrorRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::ICE]->GetGPUHandle());
	mirrorDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static



	// Render Pass Layer: Shadow Layer ----------------------------------------------------------------------
	RenderPassLayer& shadowLayer = m_mirrorAndShadowsRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	shadowLayer.Name = "Shadow Layer";

	// PSO
	m_shadowRasterizerState = std::make_unique<RasterizerState>();

	m_shadowBlendState = std::make_unique<BlendState>();
	m_shadowBlendState->SetRenderTargetBlendEnabled(true);
	m_shadowBlendState->SetRenderTargetLogicOpEnabled(false);
	m_shadowBlendState->SetRenderTargetSrcBlend(D3D12_BLEND_SRC_ALPHA);
	m_shadowBlendState->SetRenderTargetDestBlend(D3D12_BLEND_INV_SRC_ALPHA);
	m_shadowBlendState->SetRenderTargetBlendOp(D3D12_BLEND_OP_ADD);
	m_shadowBlendState->SetRenderTargetSrcBlendAlpha(D3D12_BLEND_ONE);
	m_shadowBlendState->SetRenderTargetDestBlendAlpha(D3D12_BLEND_ZERO);
	m_shadowBlendState->SetRenderTargetBlendOpAlpha(D3D12_BLEND_OP_ADD);
	m_shadowBlendState->SetRenderTargetLogicOp(D3D12_LOGIC_OP_NOOP);
	m_shadowBlendState->SetRenderTargetWriteMask(D3D12_COLOR_WRITE_ENABLE_ALL);
	
	m_shadowDepthStencilState = std::make_unique<DepthStencilState>();
	m_shadowDepthStencilState->SetDepthEnabled(true);
	m_shadowDepthStencilState->SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL);
	m_shadowDepthStencilState->SetDepthFunc(D3D12_COMPARISON_FUNC_LESS);
	m_shadowDepthStencilState->SetStencilEnabled(true);
	m_shadowDepthStencilState->SetStencilReadMask(0xff);
	m_shadowDepthStencilState->SetStencilWriteMask(0xff);

	m_shadowDepthStencilState->SetFrontFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_shadowDepthStencilState->SetFrontFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_shadowDepthStencilState->SetFrontFaceStencilPassOp(D3D12_STENCIL_OP_INCR);
	m_shadowDepthStencilState->SetFrontFaceStencilFunc(D3D12_COMPARISON_FUNC_EQUAL);
	// We are not rendering backfacing polygons, so these settings do not matter.
	m_shadowDepthStencilState->SetBackFaceStencilFailOp(D3D12_STENCIL_OP_KEEP);
	m_shadowDepthStencilState->SetBackFaceStencilDepthFailOp(D3D12_STENCIL_OP_KEEP);
	m_shadowDepthStencilState->SetBackFaceStencilPassOp(D3D12_STENCIL_OP_INCR);
	m_shadowDepthStencilState->SetBackFaceStencilFunc(D3D12_COMPARISON_FUNC_EQUAL);


	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDesc;
	ZeroMemory(&shadowDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	shadowDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	shadowDesc.pRootSignature = m_mirrorAndShadowsRenderPass.RootSignature->Get();
	shadowDesc.VS = m_standardVS->GetShaderByteCode();
	shadowDesc.PS = m_opaquePS->GetShaderByteCode();
	shadowDesc.RasterizerState = m_shadowRasterizerState->GetRasterizerDesc();
	shadowDesc.BlendState = m_shadowBlendState->GetBlendDesc();
	shadowDesc.DepthStencilState = m_shadowDepthStencilState->GetDepthStencilDesc();
	shadowDesc.SampleMask = UINT_MAX;
	shadowDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	shadowDesc.NumRenderTargets = 1;
	shadowDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	shadowDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	shadowDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	shadowDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	shadowLayer.SetPSO(shadowDesc);

	// Topology
	shadowLayer.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// MeshGroup
	// Don't reload/recreate the skull mesh. Just have this layer reference the opaque layer's MeshGroup, which contains the skull mesh
	shadowLayer.Meshes = m_mainRenderPass.RenderPassLayers[0].Meshes;

	// Render Items ---------------------
	// 
	// Shadow
	m_shadowObject = std::make_unique<GameObject>(m_deviceResources); // Create the shadow (NOTE: This does NOT create a RenderItem)
	m_shadowObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
	m_shadowObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f));
	m_shadowObject->SetMaterialRoughness(0.0f);

	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	XMVECTOR toMainLight = -XMLoadFloat3(&m_passConstants.Lights[0].Direction);
	XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
	XMMATRIX world = XMMatrixRotationY(0.5f * MathHelper::Pi) * XMMatrixScaling(0.45f, 0.45f, 0.45f) * XMMatrixTranslation(0.0f, 1.0f, -5.0f);
	XMMATRIX shadowWorld = world * S * shadowOffsetY;
	m_shadowObject->SetWorldTransform(shadowWorld);

	RenderItem* shadowRI = m_shadowObject->CreateRenderItem(&shadowLayer);
	shadowRI->submeshIndex = 2;

	auto& shadowDT = shadowRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::ICE]->GetGPUHandle());
	shadowDT.Update = [](const Timer& timer, int frameIndex) {}; // No update here because the texture is static

}

void StencilExample::LoadSkullGeometry(std::vector<Vertex>& outVertices, std::vector<uint16_t>& outIndices)
{
	std::ifstream fin("src/models/skull.txt"); 

	if (!fin)
	{
		LOG_ERROR("{}", "Could not find file: src/models/skull.txt");
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		// Model does not have texture coordinates, so just zero them out.
		vertices[i].TexC = { 0.0f, 0.0f };
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::uint16_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	outVertices = std::move(vertices);
	outIndices = std::move(indices);
}

void StencilExample::Update(const Timer& timer)
{
	PROFILE_FUNCTION();

	// IMPORTANT: Do all necessary updates/animation first, but then be sure to call Engine::Update()

	UpdateCamera(timer);


	// IMPORTANT: Must call this last so that the updates made above will take effect for this frame
	Engine::Update(timer);
}
void StencilExample::UpdateCamera(const Timer& timer)
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
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> StencilExample::GetStaticSamplers()
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
void StencilExample::OnMouseMove(float x, float y)
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