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

//NOTE: C++의 XMFLOAT4X4는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
struct CameraBuffer
{
	std::uint32_t		directionalLightsNum	= {};
	DirectX::XMFLOAT3	ambientLightStrength	= {};
	DirectX::XMFLOAT3	eyePos					= {};
	std::uint32_t		padding					= {};
	DirectX::XMFLOAT4X4 viewProj				= {};
};

class Camera
{
public:

	Camera() = default;
	Camera(Camera&& sourceCamera) noexcept = default;
	Camera& operator=(Camera&& sourceCamera) noexcept = default;
	~Camera() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Camera(const Camera& sourceCamera) = delete;
	Camera& operator=(const Camera& sourceCamera) = delete;

	void Create(ID3D12Device* pDevice)
	{
		constexpr unsigned int	constantBufferSize	= (sizeof(CameraBuffer) + 255) & ~255;
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

	void SetDirectionalLightsNum(unsigned int directionalLightsNum)
	{
		m_directionalLightsNum = directionalLightsNum;
	}

	void SetAmbientLightStrength(DirectX::XMFLOAT3 ambientLightStrength)
	{
		m_ambientLightStrength = ambientLightStrength;
	}

	//NOTE: 위치나 방향에 관한 매개변수는 호출부에서 SIMD 연산을 한 뒤 전달하기 편하도록 XMVECTOR 타입으로 둠
	void XM_CALLCONV SetPosition(DirectX::FXMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);
	}

	void XM_CALLCONV SetTarget(DirectX::FXMVECTOR target)
	{
		DirectX::XMStoreFloat3(&m_target, target);
	}

	void XM_CALLCONV SetUpDirection(DirectX::FXMVECTOR upDirection)
	{
		DirectX::XMStoreFloat3(&m_upDirection, upDirection);
	}

	void SetHorizontalFOV(float horizontalFOV)
	{
		m_horizontalFOV = horizontalFOV;
	}

	void SetAspectRatio(float aspectRatio)
	{
		m_aspectRatio = aspectRatio;
	}

	void SetNearZ(float nearZ)
	{
		m_nearZ = nearZ;
	}

	void SetFarZ(float farZ)
	{
		m_farZ = farZ;
	}

	//NOTE: (m_target - m_position)와 m_upDirection가 영벡터에 가깝지 않고, 서로 평행에 가깝지 않음을 호출부에서 책임져야 함
	void UpdateBuffer()
	{
		DirectX::XMVECTOR position		= DirectX::XMLoadFloat3(&m_position);
		DirectX::XMVECTOR target		= DirectX::XMLoadFloat3(&m_target);
		DirectX::XMVECTOR upDirection	= DirectX::XMLoadFloat3(&m_upDirection);

		DirectX::XMMATRIX viewMatrix		= DirectX::XMMatrixLookAtLH(position, target, upDirection);
		DirectX::XMMATRIX projectionMatrix	= DirectX::XMMatrixPerspectiveFovLH(m_horizontalFOV, m_aspectRatio, m_nearZ, m_farZ);
		
		DirectX::XMMATRIX viewProjMatrix = viewMatrix * projectionMatrix;

		CameraBuffer cameraConstantBufferData = {};
		cameraConstantBufferData.directionalLightsNum	= m_directionalLightsNum;
		cameraConstantBufferData.ambientLightStrength	= m_ambientLightStrength;
		DirectX::XMStoreFloat3(&cameraConstantBufferData.eyePos, position);
		DirectX::XMStoreFloat4x4(&cameraConstantBufferData.viewProj, DirectX::XMMatrixTranspose(viewProjMatrix));

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_buffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &cameraConstantBufferData, sizeof(cameraConstantBufferData));
		m_buffer->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUAddress() const
	{
		return m_buffer->GetGPUVirtualAddress();
	}

private:

	unsigned int		m_directionalLightsNum	= {};
	DirectX::XMFLOAT3	m_ambientLightStrength	= {};

	DirectX::XMFLOAT3	m_position		= { 0.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3	m_target		= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3	m_upDirection	= { 0.0f, 1.0f, 0.0f };
	float				m_horizontalFOV	= DirectX::XM_PIDIV4;
	float				m_aspectRatio	= 1.0f;
	float				m_nearZ			= 0.1f;
	float				m_farZ			= 1000.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = {};
};