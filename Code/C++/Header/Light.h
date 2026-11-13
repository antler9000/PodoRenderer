#pragma once

//Podo
#include "Shadow.h"
#include "Vector.h"
#include "Debug.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <d3dx12_core.h>
#include <d3d12.h>
#include <DirectXMath.h>

//C++
#include <memory>
#include <cstring>
#include <cstdint>

//NOTE: C++의 XMFLOAT4X4는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
struct LightBuffer
{
	std::uint32_t		isShadowOn		= {};
	DirectX::XMFLOAT3	strength		= {};
	DirectX::XMFLOAT3	direction		= {};
	std::uint32_t		padding			= {};
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

	void Create(ID3D12Device* pDevice, bool isShadowOn = false, unsigned int shadowWidth = 1024, unsigned int shadowHeight = 1024)
	{
		m_upShadow.reset();

		m_isShadowOn = isShadowOn;
		if (isShadowOn == true)
		{
			m_upShadow = std::make_unique<Shadow>();

			m_upShadow->Create(pDevice, shadowWidth, shadowHeight);
		}

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

	void SetStrength(DirectX::XMFLOAT3 strength)
	{
		m_strength = strength;
	}

	//NOTE: 위치나 방향에 관한 매개변수는 호출부에서 SIMD 연산을 한 뒤 전달하기 편하도록 XMVECTOR 타입으로 둠
	void XM_CALLCONV SetDirection(DirectX::FXMVECTOR lookDirection)
	{
		DirectX::XMStoreFloat3(&m_direction, lookDirection);
	}

	void XM_CALLCONV SetPosition(DirectX::FXMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);
	}

	void SetViewVolume(float width, float height, float nearZ, float farZ)
	{
		m_width		= width;
		m_height	= height;
		m_nearZ		= nearZ;
		m_farZ		= farZ;
	}

	//NOTE: m_direction이 영벡터에 가깝다면 임의로 아래를 바라보게 하고, m_direction이 Z축과 평행하다면 임의로 Y축을 upDirection으로 삼음
	void UpdateBuffer()
	{
		DirectX::XMVECTOR position		= DirectX::XMLoadFloat3(&m_position);
		DirectX::XMVECTOR lookDirection	= DirectX::XMLoadFloat3(&m_direction);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		if (IsNearZero(lookDirection) == true)
		{
			lookDirection = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
		}

		if (IsNearParallel(lookDirection, upDirection) == true)
		{
			upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		}

		DirectX::XMMATRIX viewMatrix		= DirectX::XMMatrixLookToLH(position, lookDirection, upDirection);
		DirectX::XMMATRIX orthoProjMatrix	= DirectX::XMMatrixOrthographicLH(m_width, m_height, m_nearZ, m_farZ);

		DirectX::XMMATRIX orthoViewProjMatrix = viewMatrix * orthoProjMatrix;

		LightBuffer lightConstantBufferData	= {};
		lightConstantBufferData.isShadowOn	= m_isShadowOn;
		lightConstantBufferData.strength	= m_strength;
		lightConstantBufferData.direction	= m_direction;
		DirectX::XMStoreFloat4x4(&lightConstantBufferData.orthoViewProj, XMMatrixTranspose(orthoViewProjMatrix));

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_buffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &lightConstantBufferData, sizeof(lightConstantBufferData));
		m_buffer->Unmap(0, nullptr);
	}

	bool GetIsShadowOn() const
	{
		return m_isShadowOn;
	}

	Shadow* GetShadowPtr() const
	{
		return m_upShadow.get();
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
		m_constantBufferViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantBufferViewGPUHandle() const
	{
		return m_constantBufferViewGPUHandle;
	}

private:

	bool					m_isShadowOn	= {};
	std::unique_ptr<Shadow>	m_upShadow		= {};

	DirectX::XMFLOAT3	m_strength	= {};
	DirectX::XMFLOAT3	m_direction	= { 0.0f, -1.0f, 0.0f };
	DirectX::XMFLOAT3	m_position	= { 0.0f, 10.0f, 0.0f };
	float				m_width		= 100.0f;
	float				m_height	= 100.0f;
	float				m_nearZ		= 0.1f;
	float				m_farZ		= 1000.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_constantBufferViewGPUHandle = {};
};