#pragma once
#include "tiny-pch.h"
#include "tiny/Core.h"
#include "tiny/Log.h"
#include "tiny/DeviceResources.h"


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
	MeshGroup(std::shared_ptr<DeviceResources> deviceResources) noexcept :
		m_deviceResources(deviceResources)
	{}

	ND inline D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const noexcept
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = m_vertexByteStride;
		vbv.SizeInBytes = m_vertexBufferByteSize;
		return vbv;
	}

	ND inline D3D12_INDEX_BUFFER_VIEW IndexBufferView() const noexcept
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = m_indexFormat;
		ibv.SizeInBytes = m_indexBufferByteSize;
		return ibv;
	}

	inline void Bind(ID3D12GraphicsCommandList* commandList) const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv = VertexBufferView();
		commandList->IASetVertexBuffers(0, 1, &vbv);

		D3D12_INDEX_BUFFER_VIEW ibv = IndexBufferView();
		commandList->IASetIndexBuffer(&ibv);
	}

	ND inline SubmeshGeometry GetSubmesh(unsigned int index) const noexcept
	{
		return m_submeshes[index];
	}

protected:
	ND Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

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

		// In order to copy CPU memory data into our default buffer, we need to create
		// an intermediate upload heap. 
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

		// Note: uploadBuffer has to be kept alive after the above function calls because
		// the command list has not been executed yet that performs the actual copy.
		// The caller can Release the uploadBuffer after it knows the copy has been executed.
		return defaultBuffer;
	}

	std::shared_ptr<DeviceResources> m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;

	// Data about the buffers.
	UINT m_vertexByteStride = 0;
	DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;
	UINT m_vertexBufferByteSize = 0;
	UINT m_indexBufferByteSize = 0;

	std::vector<SubmeshGeometry> m_submeshes;
};


//
// MeshGroupT ======================================================================================================
//
template<typename T>
class MeshGroupT : public MeshGroup
{
public:
	MeshGroupT(std::shared_ptr<DeviceResources> deviceResources);
	unsigned int AddMesh(const std::vector<T>& vertices, const std::vector<std::uint16_t>& indices);

private:
	// System memory copies. 
	std::vector<T> m_vertices;
	std::vector<std::uint16_t> m_indices;
};

template<typename T>
MeshGroupT<T>::MeshGroupT(std::shared_ptr<DeviceResources> deviceResources) :
	MeshGroup(deviceResources)
{
	m_vertexByteStride = sizeof(T);
}

template<typename T>
unsigned int MeshGroupT<T>::AddMesh(const std::vector<T>& vertices, const std::vector<std::uint16_t>& indices)
{
	TINY_CORE_ASSERT(vertices.size() > 0, "No vertices to add");
	TINY_CORE_ASSERT(indices.size() > 0, "No indices to add");

	// Create the new submesh structure for the mesh we are about to add
	SubmeshGeometry submesh; 
	submesh.IndexCount = (UINT)indices.size(); 
	submesh.StartIndexLocation = (UINT)m_indices.size(); 
	submesh.BaseVertexLocation = (UINT)m_vertices.size();
	m_submeshes.push_back(submesh);

	// Add the vertices and indices to the CPU-side 
	// TODO: These should be a one-liners
	m_vertices.reserve(m_vertices.size() + vertices.size());
	for (auto& v : vertices)
		m_vertices.push_back(v);

	m_indices.reserve(m_indices.size() + indices.size());
	for (auto& i : indices)
		m_indices.push_back(i);

	// Re-compute the total byte size of the buffers
	m_vertexBufferByteSize = static_cast<UINT>(m_vertices.size()) * m_vertexByteStride;
	m_indexBufferByteSize = static_cast<UINT>(m_indices.size()) * sizeof(std::uint16_t);

	// If the buffer has not previously been allocated, just create it here
	if (m_vertexBufferGPU == nullptr)
	{
		m_vertexBufferGPU = CreateDefaultBuffer(vertices.data(), m_vertexBufferByteSize, m_vertexBufferUploader);
		m_indexBufferGPU = CreateDefaultBuffer(indices.data(), m_indexBufferByteSize, m_indexBufferUploader);
	}
	else
	{
		// TODO: NOT YET IMPLEMENTED
		int iii = 0;
	}

	// Return the index into the vector of submeshes
	return static_cast<unsigned int>(m_submeshes.size()) - 1;
}

}