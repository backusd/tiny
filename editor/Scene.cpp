#include "pch.h"
#include "Scene.h"


using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::UI::Core;


Scene::Scene(std::shared_ptr<tiny::DeviceResources> deviceResources, ISceneUIControl* uiControl) :
    m_deviceResources(deviceResources),
    m_uiControl(uiControl),
    m_haveFocus(false)
{
//    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
//    DirectX::XMStoreFloat4x4(&m_proj, P);
//
//    GFX_THROW_INFO(
//        m_deviceResources->GetCommandList()->Reset(m_deviceResources->GetCommandAllocator(), nullptr)
//    );
//
//    BuildDescriptorHeaps();
//    BuildConstantBuffers();
//    BuildRootSignature();
//    BuildShadersAndInputLayout();
//    BuildBoxGeometry();
//    BuildPSO();
//
//    // Execute the initialization commands.
//    GFX_THROW_INFO(m_deviceResources->GetCommandList()->Close());
//    ID3D12CommandList* cmdsLists[] = { m_deviceResources->GetCommandList() };
//    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Wait until initialization is complete.
//    m_deviceResources->FlushCommandQueue();
}

void Scene::CreateWindowSizeDependentResources()
{
    // Make any necessary updates here, then start the render loop if necessary
    // ...
    //DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, m_deviceResources->AspectRatio(), 1.0f, 1000.0f);
    //DirectX::XMStoreFloat4x4(&m_proj, P);



    if (m_renderLoopWorker == nullptr || m_renderLoopWorker.Status() != AsyncStatus::Started)
    {
        StartRenderLoop();
    }
}
void Scene::StartRenderLoop()
{
    if (m_renderLoopWorker != nullptr && m_renderLoopWorker.Status() == AsyncStatus::Started)
    {
        return;
    }

    // Create a task that will be run on a background thread.
    auto workItemHandler = WorkItemHandler([this](IAsyncAction action)
        {
            tiny::Timer timer;
            timer.Reset();
            timer.Start();

            // Calculate the updated frame and render once per vertical blanking interval.
            while (action.Status() == AsyncStatus::Started)
            {
                concurrency::critical_section::scoped_lock lock(m_criticalSection);

                timer.Tick();

                // Update =========================================================================
                m_deviceResources->Update();
                

                // Render =========================================================================
                //
                m_deviceResources->Render();

                // Present ========================================================================
                m_deviceResources->Present();

                if (!m_haveFocus)
                {
                    // The app is in an inactive state so stop rendering
                    // This optimizes for power and allows the framework to become more queiecent
                    break;
                }
            }
        });

    // Run task on a dedicated high priority background thread.
    m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}
void Scene::StopRenderLoop()
{
    m_renderLoopWorker.Cancel();
}
void Scene::Suspend()
{

}
void Scene::Resume()
{

}

void Scene::WindowActivationChanged(CoreWindowActivationState activationState)
{
    if (activationState == CoreWindowActivationState::Deactivated)
    {
        m_haveFocus = false;
    }
    else if (activationState == CoreWindowActivationState::CodeActivated ||
             activationState == CoreWindowActivationState::PointerActivated)
    {
        m_haveFocus = true;

        if (m_renderLoopWorker == nullptr || m_renderLoopWorker.Status() != AsyncStatus::Started)
        {
            StartRenderLoop();
        }
    }
}

// ===================================================================================================

//void Scene::BuildDescriptorHeaps()
//{
//    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
//    cbvHeapDesc.NumDescriptors = 1;
//    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//    cbvHeapDesc.NodeMask = 0;
//    GFX_THROW_INFO(
//        m_deviceResources->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(m_cbvHeap.put()))
//    );
//}
//void Scene::BuildConstantBuffers()
//{
//    m_objectCB = std::make_unique<tiny::UploadBuffer<ObjectConstants>>(m_deviceResources->GetDevice(), 1, true);
//
//    UINT objCBByteSize = tiny::utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();
//    // Offset to the ith object constant buffer in the buffer.
//    int boxCBufIndex = 0;
//    cbAddress += boxCBufIndex * objCBByteSize;
//
//    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//    cbvDesc.BufferLocation = cbAddress;
//    cbvDesc.SizeInBytes = tiny::utility::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    m_deviceResources->GetDevice()->CreateConstantBufferView(
//        &cbvDesc,
//        m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
//}
//void Scene::BuildRootSignature()
//{
//    // Shader programs typically require resources as input (constant buffers,
//// textures, samplers).  The root signature defines the resources the shader
//// programs expect.  If we think of the shader programs as a function, and
//// the input resources as function parameters, then the root signature can be
//// thought of as defining the function signature.  
//
//// Root parameter can be a table, root descriptor or root constants.
//    CD3DX12_ROOT_PARAMETER slotRootParameter[1];
//
//    // Create a single descriptor table of CBVs.
//    CD3DX12_DESCRIPTOR_RANGE cbvTable;
//    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
//    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
//
//    // A root signature is an array of root parameters.
//    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
//    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
//    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//    if (errorBlob != nullptr)
//    {
//        LOG_ERROR("D3DCompileFromFile() failed with message: {}", (char*)errorBlob->GetBufferPointer());
//    }
//    if (FAILED(hr))
//        throw tiny::DeviceResourcesException(__LINE__, __FILE__, hr);
//
//    GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateRootSignature(
//        0,
//        serializedRootSig->GetBufferPointer(),
//        serializedRootSig->GetBufferSize(),
//        IID_PPV_ARGS(m_rootSignature.put()))
//    );
//}
//void Scene::BuildShadersAndInputLayout()
//{
//    m_vsByteCode = CompileShader(L"src\\shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
//    m_psByteCode = CompileShader(L"src\\shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
//
//    m_inputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//    };
//}
//void Scene::BuildBoxGeometry()
//{
//    std::array<Vertex, 8> vertices =
//    {
//        Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
//        Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
//        Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
//        Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
//        Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
//        Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
//        Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
//        Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
//    };
//
//    std::array<std::uint16_t, 36> indices =
//    {
//        // front face
//        0, 1, 2,
//        0, 2, 3,
//
//        // back face
//        4, 6, 5,
//        4, 7, 6,
//
//        // left face
//        4, 5, 1,
//        4, 1, 0,
//
//        // right face
//        3, 2, 6,
//        3, 6, 7,
//
//        // top face
//        1, 5, 6,
//        1, 6, 2,
//
//        // bottom face
//        4, 0, 3,
//        4, 3, 7
//    };
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
//
//    m_boxGeo = std::make_unique<tiny::MeshGeometry>();
//    m_boxGeo->Name = "boxGeo";
//
//    GFX_THROW_INFO(D3DCreateBlob(vbByteSize, &m_boxGeo->VertexBufferCPU));
//    CopyMemory(m_boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//    GFX_THROW_INFO(D3DCreateBlob(ibByteSize, &m_boxGeo->IndexBufferCPU));
//    CopyMemory(m_boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//    m_boxGeo->VertexBufferGPU = tiny::utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
//        m_deviceResources->GetCommandList(), vertices.data(), vbByteSize, m_boxGeo->VertexBufferUploader);
//
//    m_boxGeo->IndexBufferGPU = tiny::utility::CreateDefaultBuffer(m_deviceResources->GetDevice(),
//        m_deviceResources->GetCommandList(), indices.data(), ibByteSize, m_boxGeo->IndexBufferUploader);
//
//    m_boxGeo->VertexByteStride = sizeof(Vertex);
//    m_boxGeo->VertexBufferByteSize = vbByteSize;
//    m_boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
//    m_boxGeo->IndexBufferByteSize = ibByteSize;
//
//    tiny::SubmeshGeometry submesh;
//    submesh.IndexCount = (UINT)indices.size();
//    submesh.StartIndexLocation = 0;
//    submesh.BaseVertexLocation = 0;
//
//    m_boxGeo->DrawArgs["box"] = submesh;
//}
//void Scene::BuildPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
//    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//    psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
//    psoDesc.pRootSignature = m_rootSignature.get();
//    psoDesc.VS =
//    {
//        reinterpret_cast<BYTE*>(m_vsByteCode->GetBufferPointer()),
//        m_vsByteCode->GetBufferSize()
//    };
//    psoDesc.PS =
//    {
//        reinterpret_cast<BYTE*>(m_psByteCode->GetBufferPointer()),
//        m_psByteCode->GetBufferSize()
//    };
//    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//    psoDesc.SampleMask = UINT_MAX;
//    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    psoDesc.NumRenderTargets = 1;
//    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
//    psoDesc.SampleDesc.Count = m_deviceResources->MsaaEnabled() ? 4 : 1;
//    psoDesc.SampleDesc.Quality = m_deviceResources->MsaaEnabled() ? (m_deviceResources->MsaaQuality() - 1) : 0;
//    psoDesc.DSVFormat = m_deviceResources->GetDepthStencilFormat();
//    GFX_THROW_INFO(m_deviceResources->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pso.put())));
//}
//
//winrt::com_ptr<ID3DBlob> Scene::CompileShader(
//    const std::wstring& filename,
//    const D3D_SHADER_MACRO* defines,
//    const std::string& entrypoint,
//    const std::string& target)
//{
//    UINT compileFlags = 0;
//#if defined(DEBUG) || defined(_DEBUG)  
//    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#endif
//
//    HRESULT hr = S_OK;
//
//    winrt::com_ptr<ID3DBlob> byteCode = nullptr;
//    winrt::com_ptr<ID3DBlob> errors = nullptr;
//    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
//        entrypoint.c_str(), target.c_str(), compileFlags, 0, byteCode.put(), errors.put());
//
//    if (errors != nullptr)
//    {
//        LOG_CORE_ERROR("D3DCompileFromFile() failed with message: {}", (char*)errors->GetBufferPointer());
//    }
//
//    if (FAILED(hr))
//        throw tiny::DeviceResourcesException(__LINE__, __FILE__, hr);
//
//    return byteCode;
//}