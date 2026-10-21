#pragma once

//Podo
#include "Debug.h"

//Win32
#include <windows.h>
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
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 color;
};

class Asset
{
public:

	Asset() = default;
	Asset(Asset&& sourceAsset) noexcept = default;
	Asset& operator=(Asset&& sourceAsset) noexcept = default;
	~Asset() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Asset(const Asset& sourceAsset) = delete;
	Asset& operator=(const Asset& sourceAsset) = delete;

	void Create
	(
		ID3D12Device* device,
		DirectX::ResourceUploadBatch& resourceUpload,
		std::vector<Vertex>&& vertices,
		std::vector<uint32_t>&& indices
	)
	{
		m_vertexCount	= static_cast<UINT>(vertices.size());
		m_indexCount	= static_cast<UINT>(indices.size());

		ThrowIfFailed
		(
			DirectX::CreateStaticBuffer
			(
				device,
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
				device,
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

	UINT GetVertexCount() const
	{
		return m_vertexCount;
	}

	UINT GetVertexBufferSize() const
	{
		return m_vertexCount * sizeof(Vertex);
	}

	UINT GetIndexCount() const
	{
		return m_indexCount;
	}

	UINT GetIndexBufferSize() const
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

	UINT m_vertexCount	= 0;
	UINT m_indexCount	= 0;

	Microsoft::WRL::ComPtr<ID3D12Resource>	m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>	m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView	= {};
	D3D12_INDEX_BUFFER_VIEW		m_indexBufferView	= {};
};