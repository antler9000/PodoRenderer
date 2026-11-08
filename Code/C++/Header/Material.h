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

	void Create(ID3D12Device* pDevice, const Texture* pTexture)
	{
		m_pTexture = pTexture;

		constexpr UINT			constantBufferSize	= (sizeof(MaterialBuffer) + 255) & ~255;
		D3D12_HEAP_PROPERTIES	heapProperties		= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC		resourceDesc		= CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		ThrowIfFailed
		(
			pDevice->CreateCommittedResource
			(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
			)
		);

		UpdateBuffer();
	}

	//NOTE: XMFLOAT4 이하의 구조체들은 크기가 작으므로, 상수 참조 대신 간단한 값복사 방식을 사용하기로 함
	void SetDiffuseAlbedo(DirectX::XMFLOAT3 diffuseAlbedo)
	{
		m_diffuseAlbedo = diffuseAlbedo;
	}

	void SetFresnelR0(DirectX::XMFLOAT3 fresnelR0)
	{
		m_fresnelR0 = fresnelR0;
	}

	void SetShininess(float shininess)
	{
		m_shininess = shininess;
	}

	void UpdateBuffer()
	{
		MaterialBuffer materialConstantBufferData = {};
		materialConstantBufferData.diffuseAlbedo	= m_diffuseAlbedo;
		materialConstantBufferData.fresnelR0		= m_fresnelR0;
		materialConstantBufferData.shininess		= m_shininess;

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_buffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &materialConstantBufferData, sizeof(materialConstantBufferData));
		m_buffer->Unmap(0, nullptr);
	}

	const Texture* GetTexture() const
	{
		return m_pTexture;

	}
	D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUAddress() const
	{
		return m_buffer->GetGPUVirtualAddress();
	}

	UINT GetBufferWidth() const
	{
		return static_cast<UINT>(m_buffer->GetDesc().Width);
	}

	void SetConstantBufferViewGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_constantBufferViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantBufferViewGPUHandle() const
	{
		return m_constantBufferViewGPUHandle;
	}

private:

	const Texture* m_pTexture = {};

	DirectX::XMFLOAT3	m_diffuseAlbedo	= {};
	DirectX::XMFLOAT3	m_fresnelR0		= {};
	float				m_shininess		= {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferViewGPUHandle = {};
};