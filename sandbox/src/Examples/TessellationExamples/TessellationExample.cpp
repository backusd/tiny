#include "TessellationExample.h"

using namespace tiny;
using namespace sandbox::tessellationexample;

namespace sandbox
{
TessellationExample::TessellationExample(std::shared_ptr<DeviceResources> deviceResources) :
	m_deviceResources(deviceResources),
	m_mainRenderPass()
{
	PROFILE_SCOPE("TessellationExample()");

	Engine::Init(m_deviceResources);
	DescriptorManager::Init(m_deviceResources);
	TextureManager::Init(m_deviceResources);

	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr));

	m_camera.SetPosition(0.0f, 2.0f, -15.0f);
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);

	LoadTextures();
	BuildScene();

	// Execute the initialization commands.
	GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
	ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	m_deviceResources->FlushCommandQueue();
}
void TessellationExample::OnResize(int height, int width)
{
	m_camera.SetLens(0.25f * MathHelper::Pi, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
}
void TessellationExample::SetViewport(float top, float left, float height, float width) noexcept
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

void TessellationExample::LoadTextures()
{
	PROFILE_FUNCTION();

	for (int iii = 0; iii < (int)TEXTURE::Count; ++iii)
		m_textures[iii] = TextureManager::GetTexture(iii);
}
void TessellationExample::BuildScene()
{
	PROFILE_FUNCTION();

	// Add name for debug/profiling purposes
	m_mainRenderPass.Name = "Main Render Pass";
	m_mainRenderPass.RenderPassLayers.reserve(1);

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
		passConstants.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
		passConstants.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		passConstants.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
		passConstants.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
		passConstants.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

		m_mainRenderPassConstantsCB->CopyData(frameIndex, passConstants);
	};



	// Render Pass Layer: Opaque ----------------------------------------------------------------------
	RenderPassLayer& opaqueLayer = m_mainRenderPass.RenderPassLayers.emplace_back(m_deviceResources);
	opaqueLayer.Name = "Opaque Layer";

	// PSO
	m_tessellationVS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/TessellationBasicVS.cso");
	m_tessellationHS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/TessellationBasicHS.cso");
	m_tessellationDS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/TessellationBasicDS.cso");
	m_tessellationPS = std::make_unique<Shader>(m_deviceResources, "src/shaders/output/TessellationBasicPS.cso");

	m_inputLayout = std::make_unique<InputLayout>(
		std::vector<D3D12_INPUT_ELEMENT_DESC>{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		}
	);

	m_blendState = std::make_unique<BlendState>();
	m_depthStencilState = std::make_unique<DepthStencilState>();

	m_rasterizerState = std::make_unique<RasterizerState>();
	m_rasterizerState->SetFillMode(D3D12_FILL_MODE_WIREFRAME);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueDesc.InputLayout = m_inputLayout->GetInputLayoutDesc();
	opaqueDesc.pRootSignature = m_mainRenderPass.RootSignature->Get();
	opaqueDesc.VS = m_tessellationVS->GetShaderByteCode();
	opaqueDesc.HS = m_tessellationHS->GetShaderByteCode();
	opaqueDesc.DS = m_tessellationDS->GetShaderByteCode();
	opaqueDesc.PS = m_tessellationPS->GetShaderByteCode();
	opaqueDesc.RasterizerState = m_rasterizerState->GetRasterizerDesc();
	opaqueDesc.BlendState = m_blendState->GetBlendDesc();
	opaqueDesc.DepthStencilState = m_depthStencilState->GetDepthStencilDesc();
	opaqueDesc.SampleMask = UINT_MAX;
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
	opaqueDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
	opaqueDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
	opaqueDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();

	opaqueLayer.SetPSO(opaqueDesc);

	// Topology
	opaqueLayer.Topology = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;

	// MeshGroup
	std::vector<std::uint16_t> indices = { 0, 1, 2, 3 };
	std::vector<DirectX::XMFLOAT3> vertices = {
		DirectX::XMFLOAT3(-10.0f, 0.0f, +10.0f),
		DirectX::XMFLOAT3(+10.0f, 0.0f, +10.0f),
		DirectX::XMFLOAT3(-10.0f, 0.0f, -10.0f),
		DirectX::XMFLOAT3(+10.0f, 0.0f, -10.0f)
	};

	std::vector<std::vector<DirectX::XMFLOAT3>> allVertices;
	allVertices.push_back(std::move(vertices));
	std::vector<std::vector<std::uint16_t>> allIndices;
	allIndices.push_back(std::move(indices));

	opaqueLayer.Meshes = std::make_shared<MeshGroupT<DirectX::XMFLOAT3>>(m_deviceResources, allVertices, allIndices);





	// Render Items
	m_gridObject = std::make_unique<GameObject>(m_deviceResources); // Create the grid (NOTE: This does NOT create a RenderItem)
	m_gridObject->SetMaterialDiffuseAlbedo(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	m_gridObject->SetMaterialFresnelR0(DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f));
	m_gridObject->SetMaterialRoughness(0.5f);

	RenderItem* gridRI = m_gridObject->CreateRenderItem(&opaqueLayer);

	gridRI->submeshIndex = 0; // Only using a single mesh, so automatically it is at index 0

	auto& gridDT = gridRI->DescriptorTables.emplace_back(0, m_textures[(int)TEXTURE::WHITE1X1]->GetSRVHandle());
	gridDT.Update = [](RootDescriptorTable* dt, const Timer& timer, int frameIndex)
	{
		// No update here because the texture is static
	};


}



void TessellationExample::Update(const Timer& timer)
{
	PROFILE_FUNCTION();

	// IMPORTANT: Do all necessary updates/animation first, but then be sure to call Engine::Update()

	UpdateCamera(timer);

	// IMPORTANT: Must call this last so that the updates made above will take effect for this frame
	Engine::Update(timer);
}
void TessellationExample::UpdateCamera(const Timer& timer)
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TessellationExample::GetStaticSamplers()
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

void TessellationExample::OnMouseMove(float x, float y)
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