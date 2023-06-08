#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"
#include "tiny/Engine.h"
#include "tiny/utils/Timer.h"



namespace tiny
{
// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
struct SubmeshGeometry
{
	UINT IndexCount			= 0;
	UINT StartIndexLocation = 0;
	INT  BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	DirectX::BoundingBox Bounds;
};

class MeshGroup
{
public:
	MeshGroup(std::shared_ptr<DeviceResources> deviceResources) noexcept : m_deviceResources(deviceResources) {}
	virtual ~MeshGroup() {}

	inline void Bind(ID3D12GraphicsCommandList* commandList) const
	{
		commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		commandList->IASetIndexBuffer(&m_indexBufferView);
	}

	ND inline SubmeshGeometry GetSubmesh(unsigned int index) const noexcept
	{
		return m_submeshes[index];
	}

protected:
	ND Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = nullptr;

		// Create the actual default buffer resource.
		auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT); 
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(defaultBuffer.GetAddressOf())
			)
		);

		// In order to copy CPU memory data into our default buffer, we need to create an intermediate upload heap. 
		props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); 
		desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(uploadBuffer.GetAddressOf())
			)
		);

		// Describe the data we want to copy into the default buffer.
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = initData;
		subResourceData.RowPitch = byteSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		// Schedule to copy the data to the default buffer resource. At a high level, the helper function UpdateSubresources
		// will copy the CPU memory into the intermediate upload heap. Then, using ID3D12CommandList::CopySubresourceRegion,
		// the intermediate upload heap data will be copied to mBuffer.
		auto commandList = m_deviceResources->GetCommandList();
		auto _b = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &_b);
		UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		auto _b2 = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		commandList->ResourceBarrier(1, &_b2);

		// MUST delete the upload buffer AFTER it is done being referenced by the GPU
		Engine::DelayedDelete(uploadBuffer);

		// Note: uploadBuffer has to be kept alive after the above function calls because
		// the command list has not been executed yet that performs the actual copy.
		// The caller can Release the uploadBuffer after it knows the copy has been executed.
		return defaultBuffer;
	}
	void CleanUp()
	{
		Engine::DelayedDelete(m_vertexBufferGPU);
		Engine::DelayedDelete(m_indexBufferGPU);
	}

	std::shared_ptr<DeviceResources> m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;

	// Keep track of views for the two buffers
	// NOTE: All values are dummy values except the DXGI_FORMAT value for the index buffer view
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = { 0, 0, 0 };
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = { 0, 0, DXGI_FORMAT_R16_UINT };

	std::vector<SubmeshGeometry> m_submeshes;


};


//
// MeshGroupT ======================================================================================================
//
template<typename T>
class MeshGroupT : public MeshGroup
{
public:
	MeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
		const std::vector<std::vector<T>>& vertices,
		const std::vector<std::vector<std::uint16_t>>& indices);
	virtual ~MeshGroupT() override { CleanUp(); }

private:
	// System memory copies. 
	std::vector<T> m_vertices;
	std::vector<std::uint16_t> m_indices;
};

template<typename T>
MeshGroupT<T>::MeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
						  const std::vector<std::vector<T>>& vertices,
						  const std::vector<std::vector<std::uint16_t>>& indices) :
	MeshGroup(deviceResources)
{
	TINY_CORE_ASSERT(vertices.size() > 0, "No vertices to add");
	TINY_CORE_ASSERT(vertices.size() == indices.size(), "There must be a 1:1 correspondence between the number of vertex lists and index lists");

	// Compute the total number of vertices and indices
	size_t totalVertices = 0;
	size_t totalIndices = 0;
	for (const std::vector<T>& vec : vertices)
		totalVertices += vec.size();
	for (const std::vector<std::uint16_t>& vec : indices)
		totalIndices += vec.size();

	// reserve space for all vertices & indices
	m_vertices.reserve(totalVertices);
	m_indices.reserve(totalIndices);

	// Loop over the list of vertex lists creating a submesh for each one
	for (unsigned int iii = 0; iii < vertices.size(); ++iii)
	{
		// Create the new submesh structure for the mesh we are about to add
		SubmeshGeometry submesh; 
		submesh.IndexCount = (UINT)indices[iii].size();
		submesh.StartIndexLocation = (UINT)m_indices.size(); 
		submesh.BaseVertexLocation = (INT)m_vertices.size();
		m_submeshes.push_back(submesh);

		// Add the vertices and indices
		for (const T& v : vertices[iii]) 
			m_vertices.push_back(v); 
		for (const std::uint16_t& i : indices[iii])
			m_indices.push_back(i);
	}

	// Compute the vertex/index buffer view data
	m_vertexBufferView.StrideInBytes = sizeof(T);
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size()) * sizeof(T);
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size()) * sizeof(std::uint16_t);

	// Create the vertex and index buffers with the initial data
	m_vertexBufferGPU = CreateDefaultBuffer(vertices.data(), m_vertexBufferView.SizeInBytes);
	m_indexBufferGPU = CreateDefaultBuffer(indices.data(), m_indexBufferView.SizeInBytes);

	// Get the buffer locations
	m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
	m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
}

//
// DynamicMesh ======================================================================================================
//
// NOTE: For dynamic meshes, we make the simplification that the underlying MeshGroup will ONLY hold a single mesh.
//       The reason for this is that we have to change the vertex/index buffer view each frame as well as copy data
//		 from the CPU to GPU each frame which is all made easier by forcing the class to only manage a single Mesh
class DynamicMeshGroup : public MeshGroup
{
public:
	DynamicMeshGroup(std::shared_ptr<DeviceResources> deviceResources) noexcept : MeshGroup(deviceResources) {}
	virtual ~DynamicMeshGroup() override {}
	inline void Update(int frameIndex) noexcept
	{
		// For dynamic meshes, we keep gNumFrameResources copies of the vertex/index buffer in a single, continuous buffer
		// All we need to do every time Update() is called, is to update the vertex/index buffer views to point at the correct
		// starting location for the next buffer we want to use
		m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress() + frameIndex * m_vertexBufferView.SizeInBytes;
		m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress() + frameIndex * m_indexBufferView.SizeInBytes;
	}

protected:
	ND Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(UINT64 totalBufferSize)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

		// Need to create the buffer in an upload heap so the CPU can regularly send new data to it
		auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		// Create a buffer that will hold an entire buffer per frame resource
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(totalBufferSize * gNumFrameResources);

		GFX_THROW_INFO(
			m_deviceResources->GetDevice()->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)
			)
		);

		return uploadBuffer;
	}

private:
	// NOTE: We need to make DynamicMeshGroup immovable because the DynamicMeshGroupT passes the 'this' pointer
	//       to the engine so that this object can be tracked. However, if the DynamicMeshGroup is later
	//       moved, the 'this' pointer will no longer be valid. 
	//		 See: https://stackoverflow.com/questions/28492326/c11-does-a-move-operation-change-the-address#:~:text=Short%20answer%3A%20no%2C%20the%20address,not%20be%20a%20useful%20state.
	DynamicMeshGroup(DynamicMeshGroup&&) = delete;
	DynamicMeshGroup& operator=(DynamicMeshGroup&& rhs) = delete;
};


//
// MeshGroupDynamicT ======================================================================================================
//
template<typename T>
class DynamicMeshGroupT : public DynamicMeshGroup
{
public:
	// NOTE: For Dynamic meshes, we only allow there to be a single mesh - see Note above the DynamicMeshGroup class
	DynamicMeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
					  std::vector<T>&& vertices,
					  std::vector<std::uint16_t>&& indices) noexcept :
		DynamicMeshGroup(deviceResources),
		m_vertices(std::move(vertices)),
		m_indices(std::move(indices))
	{
		TINY_CORE_ASSERT(m_vertices.size() > 0, "No vertices");
		TINY_CORE_ASSERT(m_indices.size() > 0, "No indices");

		Engine::AddDynamicMeshGroup(this);

		// Create the submesh structure for the single mesh
		SubmeshGeometry submesh; 
		submesh.IndexCount = (UINT)m_indices.size();  
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;
		m_submeshes.push_back(submesh);

		// Compute the vertex/index buffer view data
		m_vertexBufferView.StrideInBytes = sizeof(T); 
		m_vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size()) * sizeof(T); 
		m_indexBufferView.Format = DXGI_FORMAT_R16_UINT; 
		m_indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size()) * sizeof(std::uint16_t); 

		// Create the vertex and index buffers as UPLOAD buffers (so there will be gNumFrameResources copies of the vertex/index buffers)
		m_vertexBufferGPU = CreateUploadBuffer(m_vertexBufferView.SizeInBytes); 
		m_indexBufferGPU = CreateUploadBuffer(m_indexBufferView.SizeInBytes);

		// Map the vertex and index buffers
		GFX_THROW_INFO(m_vertexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVertexData)));
		GFX_THROW_INFO(m_indexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedIndexData)));

		// Copy the data into all slots of the upload buffers (We do all slots because creation of the dynamic buffer
		// may occur at any point, not necessarily just at program start up, so we can't just assume we are on frame index 0)
		for (unsigned int iii = 0; iii < gNumFrameResources; ++iii)
		{
			memcpy(&m_mappedVertexData[iii * m_vertexBufferView.SizeInBytes], &m_vertices.data(), m_vertexBufferView.SizeInBytes);
			memcpy(&m_mappedIndexData[iii * m_indexBufferView.SizeInBytes], &m_indices.data(), m_indexBufferView.SizeInBytes);
		}

		// Set the buffer locations as the start of the Upload buffers. This will later be changed each frame when Update() is called
		m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
		m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	}
	virtual ~DynamicMeshGroupT() noexcept override 
	{ 
		Engine::RemoveDynamicMeshGroup(this);

		if (m_vertexBufferGPU != nullptr)
			m_vertexBufferGPU->Unmap(0, nullptr);

		if (m_indexBufferGPU != nullptr)
			m_indexBufferGPU->Unmap(0, nullptr);

		CleanUp();
	}

	inline void CopyVertices(unsigned int frameIndex, std::vector<T>&& newVertices) noexcept
	{
		TINY_CORE_ASSERT(newVertices.size() == m_vertices.size(), "The new set of vertices must have the same total number as the original set");
		TINY_CORE_ASSERT(frameIndex < gNumFrameResources, "Frame index is larger than expected");
		m_vertices = std::move(newVertices);
		memcpy(&m_mappedVertexData[frameIndex * m_vertexBufferView.SizeInBytes], &m_vertices.data(), m_vertexBufferView.SizeInBytes);
	}
	inline void CopyIndices(unsigned int frameIndex, std::vector<std::uint16_t>&& newIndices) noexcept
	{
		TINY_CORE_ASSERT(newIndices.size() == m_indices.size(), "The new set of indices must have the same total number as the original set");
		TINY_CORE_ASSERT(frameIndex < gNumFrameResources, "Frame index is larger than expected"); 
		m_indices = std::move(newIndices);
		memcpy(&m_mappedIndexData[frameIndex * m_indexBufferView.SizeInBytes], &m_indices.data(), m_indexBufferView.SizeInBytes);
	}

private:
	// NOTE: We need to make DynamicMeshGroupT immovable because the constructor passes the 'this' pointer
	//       to the engine so that this object can be tracked. However, if the DynamicMeshGroupT is later
	//       moved, the 'this' pointer will no longer be valid. 
	//		 See: https://stackoverflow.com/questions/28492326/c11-does-a-move-operation-change-the-address#:~:text=Short%20answer%3A%20no%2C%20the%20address,not%20be%20a%20useful%20state.
	DynamicMeshGroupT(DynamicMeshGroupT&&) = delete;
	DynamicMeshGroupT& operator=(DynamicMeshGroupT&& rhs) = delete;

	// System memory copies. 
	std::vector<T> m_vertices;
	std::vector<std::uint16_t> m_indices;

	BYTE* m_mappedVertexData = nullptr;
	BYTE* m_mappedIndexData = nullptr;
};

}