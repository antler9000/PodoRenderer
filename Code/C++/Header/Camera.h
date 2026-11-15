#pragma once

//Podo
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

	void Create(ID3D12Device* pDevice, unsigned int frameResourceNum)
	{
		m_frameResourceNum	= frameResourceNum;
		m_pDirtyFlags		= std::make_unique<bool[]>(frameResourceNum);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}

		unsigned int			constantBufferSize	= frameResourceNum * frameResourceSize;
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
				IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf())
			)
		);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < frameResourceNum; frameResourceIndex++)
		{
			CheckAndUpdateFrameResource(frameResourceIndex);
		}
	}

	void SetDirectionalLightsNum(unsigned int directionalLightsNum)
	{
		m_directionalLightsNum = directionalLightsNum;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void SetAmbientLightStrength(DirectX::XMFLOAT3 ambientLightStrength)
	{
		m_ambientLightStrength = ambientLightStrength;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	//NOTE: 위치나 방향에 관한 매개변수는 호출부에서 SIMD 연산을 한 뒤 전달하기 편하도록 XMVECTOR 타입으로 둠
	void XM_CALLCONV SetPosition(DirectX::FXMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void XM_CALLCONV SetTarget(DirectX::FXMVECTOR target)
	{
		DirectX::XMStoreFloat3(&m_target, target);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void XM_CALLCONV SetUpDirection(DirectX::FXMVECTOR upDirection)
	{
		DirectX::XMStoreFloat3(&m_upDirection, upDirection);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void SetViewVolume(float verticalFOV, float aspectRatio, float nearZ, float farZ)
	{
		m_verticalFOV	= verticalFOV;
		m_aspectRatio	= aspectRatio;
		m_nearZ			= nearZ;
		m_farZ			= farZ;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	//NOTE: (m_target - m_position)이나 m_upDirection이 영벡터에 가까운 경우, 그리고 두 벡터가 평행에 가까운 경우 임의로 방향을 설정함
	void CheckAndUpdateFrameResource(unsigned int frameResourceIndex)
	{
		if (m_pDirtyFlags[frameResourceIndex] == true)
		{
			DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_position);
			DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&m_target);
			DirectX::XMVECTOR upDirection = DirectX::XMLoadFloat3(&m_upDirection);

			if (IsNearZero(DirectX::XMVectorSubtract(target, position)) == true)
			{
				target = DirectX::XMVectorAdd(position, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
			}

			if (IsNearZero(upDirection) == true)
			{
				upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			}

			if (IsNearParallel(DirectX::XMVectorSubtract(target, position), upDirection))
			{
				if (IsNearParallel(upDirection, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) == false)
				{
					upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				}
				else
				{
					upDirection = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				}
			}

			DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(position, target, upDirection);
			DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(m_verticalFOV, m_aspectRatio, m_nearZ, m_farZ);

			DirectX::XMMATRIX viewProjMatrix = viewMatrix * projectionMatrix;

			CameraBuffer cameraFrameResourceData = {};
			cameraFrameResourceData.directionalLightsNum = m_directionalLightsNum;
			cameraFrameResourceData.ambientLightStrength = m_ambientLightStrength;
			DirectX::XMStoreFloat3(&cameraFrameResourceData.eyePos, position);
			DirectX::XMStoreFloat4x4(&cameraFrameResourceData.viewProj, DirectX::XMMatrixTranspose(viewProjMatrix));

			unsigned int offset = frameResourceIndex * frameResourceSize;

			void* pMappedData = nullptr;
			D3D12_RANGE readRange = { 0, 0 };
			ThrowIfFailed(m_resource->Map(0, &readRange, &pMappedData));

			std::memcpy(static_cast<std::uint8_t*>(pMappedData) + offset, &cameraFrameResourceData, sizeof(CameraBuffer));

			D3D12_RANGE writtenRange = { offset, offset + frameResourceSize };
			m_resource->Unmap(0, &writtenRange);

			m_pDirtyFlags[frameResourceIndex] = false;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUAddress(unsigned int frameResourceIndex) const
	{
		return m_resource->GetGPUVirtualAddress() + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(frameResourceIndex) * frameResourceSize;
	}

private:

	static constexpr unsigned int frameResourceSize = (sizeof(CameraBuffer) + 255) & ~255;

private:

	unsigned int m_frameResourceNum			= {};
	std::unique_ptr<bool[]> m_pDirtyFlags	= {};

	unsigned int		m_directionalLightsNum	= {};
	DirectX::XMFLOAT3	m_ambientLightStrength	= {};

	DirectX::XMFLOAT3	m_position		= { 0.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3	m_target		= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3	m_upDirection	= { 0.0f, 1.0f, 0.0f };
	float				m_verticalFOV	= DirectX::XM_PIDIV4;
	float				m_aspectRatio	= 1.0f;
	float				m_nearZ			= 0.1f;
	float				m_farZ			= 1000.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = {};
};