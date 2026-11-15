#pragma once

//Podo
#include "Texture.h"
#include "Debug.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <d3dx12_core.h>
#include <d3d12.h>
#include <DirectXMath.h>

//C++
#include <cstring>
#include <cstdint>
#include <memory>

//NOTE: C++의 XMFLOAT3는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
struct MaterialBuffer
{
	DirectX::XMFLOAT3	diffuseAlbedo	= {};
	std::uint32_t		padding			= {};
	DirectX::XMFLOAT3	fresnelR0		= {};
	float				shininess		= {};
};

class Material
{
public:

	Material() = default;
	Material(Material&& sourceMaterial) noexcept = default;
	Material& operator=(Material&& sourceMaterial) noexcept = default;
	~Material() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Material(const Material& sourceMaterial) = delete;
	Material& operator=(const Material& sourceMaterial) = delete;

	void Create(ID3D12Device* pDevice, unsigned int frameResourceNum)
	{
		m_frameResourceNum	= frameResourceNum;
		m_pDirtyFlags		= std::make_unique<bool[]>(frameResourceNum);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}

		unsigned int			resourceSize		= frameResourceNum * frameResourceSize;
		D3D12_HEAP_PROPERTIES	heapProperties		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC		resourceDesc		= CD3DX12_RESOURCE_DESC::Buffer(resourceSize);

		ThrowIfFailed
		(
			pDevice->CreateCommittedResource
			(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf())
			)
		);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < frameResourceNum; frameResourceIndex++)
		{
			CheckAndUpdateFrameResource(frameResourceIndex);
		}

		m_pCBVLocationGPUHandles = std::make_unique<D3D12_GPU_DESCRIPTOR_HANDLE[]>(frameResourceNum);
	}

	//NOTE: 텍스처는 업데이트가 이뤄지지 않으므로 const로 받아 저장함
	void SetTexturePtr(const Texture* pTexture)
	{
		m_pTexture = pTexture;
	}

	void SetDiffuseAlbedo(DirectX::XMFLOAT3 diffuseAlbedo)
	{
		m_diffuseAlbedo = diffuseAlbedo;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void SetFresnelR0(DirectX::XMFLOAT3 fresnelR0)
	{
		m_fresnelR0 = fresnelR0;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void SetShininess(float shininess)
	{
		m_shininess = shininess;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void CheckAndUpdateFrameResource(unsigned int frameResourceIndex)
	{
		if (m_pDirtyFlags[frameResourceIndex] == true)
		{
			MaterialBuffer materialFrameResourceData = {};
			materialFrameResourceData.diffuseAlbedo = m_diffuseAlbedo;
			materialFrameResourceData.fresnelR0 = m_fresnelR0;
			materialFrameResourceData.shininess = m_shininess;

			unsigned int offset = frameResourceIndex * frameResourceSize;

			void* pMappedData = nullptr;
			D3D12_RANGE readRange = { 0, 0 };
			ThrowIfFailed(m_resource->Map(0, &readRange, &pMappedData));

			std::memcpy(static_cast<std::uint8_t*>(pMappedData) + offset, &materialFrameResourceData, sizeof(MaterialBuffer));

			D3D12_RANGE writtenRange = { offset, offset + frameResourceSize };
			m_resource->Unmap(0, &writtenRange);

			m_pDirtyFlags[frameResourceIndex] = false;
		}
	}

	const Texture* GetTexturePtr() const
	{
		return m_pTexture;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUAddress(unsigned int frameResourceIndex) const
	{
		return m_resource->GetGPUVirtualAddress() + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(frameResourceIndex) * frameResourceSize;
	}

	unsigned int GetResourceWidth() const
	{
		return frameResourceSize;
	}

	void SetCBVLocationGPUHandle(unsigned int frameResourceIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_pCBVLocationGPUHandles[frameResourceIndex] = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetCBVLocationGPUHandle(unsigned int frameResourceIndex) const
	{
		return m_pCBVLocationGPUHandles[frameResourceIndex];
	}

private:

	static constexpr unsigned int frameResourceSize = (sizeof(MaterialBuffer) + 255) & ~255;

private:

	unsigned int m_frameResourceNum			= {};
	std::unique_ptr<bool[]> m_pDirtyFlags	= {};

	const Texture* m_pTexture = {};

	DirectX::XMFLOAT3	m_diffuseAlbedo	= {};
	DirectX::XMFLOAT3	m_fresnelR0		= {};
	float				m_shininess		= {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = {};

	std::unique_ptr<D3D12_GPU_DESCRIPTOR_HANDLE[]> m_pCBVLocationGPUHandles = {};
};