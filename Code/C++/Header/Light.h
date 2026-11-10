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

//NOTE: C++의 XMFLOAT3과 XMFLOAT4X4는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
struct LightBuffer
{
	DirectX::XMFLOAT3	strength		= {};
	std::uint32_t		padding0		= {};
	DirectX::XMFLOAT3	direction		= {};
	std::uint32_t		padding1		= {};
	DirectX::XMFLOAT4X4	orthoViewProj	= {};
};

class Light
{
public:

	Light() = default;
	Light(Light&& sourceLight) noexcept = default;
	Light& operator=(Light&& sourceLight) noexcept = default;
	~Light() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Light(const Light& sourceLight) = delete;
	Light& operator=(const Light& sourceLight) = delete;

	void Create(ID3D12Device* pDevice)
	{
		constexpr unsigned int	constantBufferSize	= (sizeof(LightBuffer) + 255) & ~255;
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

	//NOTE: 위치나 방향에 관한 매개변수는 호출부에서 SIMD 연산을 한 뒤 전달하기 편하도록 XMVECTOR 타입으로 둠
	void XM_CALLCONV SetStrength(DirectX::XMFLOAT3 strength)
	{
		m_strength = strength;
	}

	void XM_CALLCONV SetDirection(DirectX::FXMVECTOR direction)
	{
		DirectX::XMStoreFloat3(&m_direction, direction);
	}

	void XM_CALLCONV SetPosition(DirectX::FXMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);
	}

	void SetWidth(float width)
	{
		m_width = width;
	}

	void SetHeight(float height)
	{
		m_height = height;
	}

	void SetNearZ(float nearZ)
	{
		m_nearZ = nearZ;
	}

	void SetFarZ(float farZ)
	{
		m_farZ = farZ;
	}
	
	void UpdateBuffer()
	{
		DirectX::XMVECTOR position		= DirectX::XMLoadFloat3(&m_position);
		DirectX::XMVECTOR toDirection	= DirectX::XMLoadFloat3(&m_direction);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		DirectX::XMMATRIX viewMatrix		= DirectX::XMMatrixLookToLH(position, toDirection, upDirection);
		DirectX::XMMATRIX orthoProjMatrix	= DirectX::XMMatrixOrthographicLH(m_width, m_height, m_nearZ, m_farZ);

		DirectX::XMMATRIX orthoViewProjMatrix = viewMatrix * orthoProjMatrix;

		LightBuffer lightConstantBufferData = {};
		lightConstantBufferData.strength = m_strength;
		lightConstantBufferData.direction = m_direction;
		DirectX::XMStoreFloat4x4(&lightConstantBufferData.orthoViewProj, orthoViewProjMatrix);

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_buffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &lightConstantBufferData, sizeof(lightConstantBufferData));
		m_buffer->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUAddress() const
	{
		return m_buffer->GetGPUVirtualAddress();
	}

	unsigned int GetBufferWidth() const
	{
		return static_cast<unsigned int>(m_buffer->GetDesc().Width);
	}

	void SetConstantBufferViewGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_objectConstantBufferViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantBufferViewGPUHandle() const
	{
		return m_objectConstantBufferViewGPUHandle;
	}

private:

	DirectX::XMFLOAT3	m_strength	= {};
	DirectX::XMFLOAT3	m_direction	= { 0.0f, -1.0f, 0.0f };
	DirectX::XMFLOAT3	m_position	= { 0.0f, 100.0f, 0.0f };
	float				m_width		= 1000.0f;
	float				m_height	= 1000.0f;
	float				m_nearZ		= 0.1f;
	float				m_farZ		= 1000.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_objectConstantBufferViewGPUHandle = {};
};