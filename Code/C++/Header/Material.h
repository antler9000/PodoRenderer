#pragma once

//Podo
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
struct MaterialConstantBuffer
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

	void Create(ID3D12Device* pDevice)
	{
		constexpr UINT			constantBufferSize	= (sizeof(MaterialConstantBuffer) + 255) & ~255;
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
				IID_PPV_ARGS(m_materialConstantBuffer.ReleaseAndGetAddressOf())
			)
		);

		UpdateMaterialConstantBuffer();
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

	void UpdateMaterialConstantBuffer()
	{
		MaterialConstantBuffer materialConstantBufferData = {};
		materialConstantBufferData.diffuseAlbedo	= m_diffuseAlbedo;
		materialConstantBufferData.fresnelR0		= m_fresnelR0;
		materialConstantBufferData.shininess		= m_shininess;

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_materialConstantBuffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &materialConstantBufferData, sizeof(materialConstantBufferData));
		m_materialConstantBuffer->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialConstantBufferGPUAddress() const
	{
		return m_materialConstantBuffer->GetGPUVirtualAddress();
	}

	UINT GetMaterialConstantBufferWidth() const
	{
		return static_cast<UINT>(m_materialConstantBuffer->GetDesc().Width);
	}

	void SetMaterialConstantBufferViewGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_materialConstantBufferViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetMaterialConstantBufferViewGPUHandle() const
	{
		return m_materialConstantBufferViewGPUHandle;
	}

private:

	DirectX::XMFLOAT3	m_diffuseAlbedo	= {};
	DirectX::XMFLOAT3	m_fresnelR0		= {};
	float				m_shininess		= {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_materialConstantBuffer = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_materialConstantBufferViewGPUHandle = {};
};