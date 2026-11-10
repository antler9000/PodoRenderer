#pragma once

//Podo
#include "Debug.h"

//Win32
#include <wrl/client.h>

//D3D12
#include <ResourceUploadBatch.h>
#include <BufferHelpers.h>
#include <d3d12.h>
#include <DirectXMath.h>

//DXGI
#include <dxgiformat.h>

//C++
#include <vector>
#include <utility>
#include <cstdint>

struct Vertex
{
	DirectX::XMFLOAT3 pos		= {};
	DirectX::XMFLOAT3 normal	= {};
	DirectX::XMFLOAT2 texCoord	= {};
};

class Mesh
{
public:

	Mesh() = default;
	Mesh(Mesh&& sourceMesh) noexcept = default;
	Mesh& operator=(Mesh&& sourceMesh) noexcept = default;
	~Mesh() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Mesh(const Mesh& sourceMesh) = delete;
	Mesh& operator=(const Mesh& sourceMesh) = delete;

	void Create
	(
		ID3D12Device* pDevice,
		DirectX::ResourceUploadBatch& resourceUpload,
		std::vector<Vertex>&& vertices,
		std::vector<uint32_t>&& indices
	)
	{
		m_vertexCount	= static_cast<unsigned int>(vertices.size());
		m_indexCount	= static_cast<unsigned int>(indices.size());

		ThrowIfFailed
		(
			DirectX::CreateStaticBuffer
			(
				pDevice,
				resourceUpload,
				std::move(vertices),
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				m_vertexBuffer.ReleaseAndGetAddressOf()
			)
		);
		ThrowIfFailed
		(
			DirectX::CreateStaticBuffer
			(
				pDevice,
				resourceUpload,
				std::move(indices),
				D3D12_RESOURCE_STATE_INDEX_BUFFER,
				m_indexBuffer.ReleaseAndGetAddressOf()
			)
		);

		m_vertexBufferView = 
		{
			m_vertexBuffer->GetGPUVirtualAddress(),
			GetVertexBufferSize(),
			sizeof(Vertex)
		};
		m_indexBufferView =
		{
			m_indexBuffer->GetGPUVirtualAddress(),
			GetIndexBufferSize(),
			DXGI_FORMAT_R32_UINT
		};
	}

	unsigned int GetVertexCount() const
	{
		return m_vertexCount;
	}

	unsigned int GetVertexBufferSize() const
	{
		return m_vertexCount * sizeof(Vertex);
	}

	unsigned int GetIndexCount() const
	{
		return m_indexCount;
	}

	unsigned int GetIndexBufferSize() const
	{
		return m_indexCount * sizeof(uint32_t);
	}

	const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const
	{
		return &m_vertexBufferView;
	}

	const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const
	{
		return &m_indexBufferView;
	}

private:

	unsigned int m_vertexCount	= {};
	unsigned int m_indexCount	= {};

	Microsoft::WRL::ComPtr<ID3D12Resource>	m_vertexBuffer	= {};
	Microsoft::WRL::ComPtr<ID3D12Resource>	m_indexBuffer	= {};

	D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView	= {};
	D3D12_INDEX_BUFFER_VIEW		m_indexBufferView	= {};
};