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

struct CameraConstantBuffer
{
	DirectX::XMFLOAT4X4 viewProjMatrix;
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

	void Create(ID3D12Device* device)
	{
		constexpr UINT			constantBufferSize = (sizeof(CameraConstantBuffer) + 255) & ~255;
		D3D12_HEAP_PROPERTIES	heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC		resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		ThrowIfFailed
		(
			device->CreateCommittedResource
			(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_cameraConstantBuffer.ReleaseAndGetAddressOf())
			)
		);

		UpdateCameraConstantBuffer(1.0f);
	}

	void SetPosition(DirectX::XMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);
	}

	void SetTarget(DirectX::XMVECTOR target)
	{
		DirectX::XMStoreFloat3(&m_target, target);
	}

	void SetUpDirection(DirectX::XMVECTOR upDirection)
	{
		DirectX::XMStoreFloat3(&m_upDirection, upDirection);
	}

	//NOTE: (m_target - m_position)와 m_upDirection가 영벡터에 가깝지 않고, 서로 평행에 가깝지 않음을 호출부에서 책임져야 함
	void UpdateCameraConstantBuffer(float aspectRatio)
	{
		DirectX::XMVECTOR position		= DirectX::XMLoadFloat3(&m_position);
		DirectX::XMVECTOR target		= DirectX::XMLoadFloat3(&m_target);
		DirectX::XMVECTOR upDirection	= DirectX::XMLoadFloat3(&m_upDirection);

		DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(position, target, upDirection);

		DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);

		DirectX::XMMATRIX viewProjMatrix = viewMatrix * projectionMatrix;

		CameraConstantBuffer cameraConstantBufferData;
		DirectX::XMStoreFloat4x4(&cameraConstantBufferData.viewProjMatrix, DirectX::XMMatrixTranspose(viewProjMatrix));

		void* mappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_cameraConstantBuffer->Map(0, &readRange, &mappedData));
		std::memcpy(mappedData, &cameraConstantBufferData, sizeof(cameraConstantBufferData));
		m_cameraConstantBuffer->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetCameraConstantBufferGPUAddress() const
	{
		return m_cameraConstantBuffer->GetGPUVirtualAddress();
	}

private:

	DirectX::XMFLOAT3 m_position	= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_target		= { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 m_upDirection	= { 0.0f, 1.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraConstantBuffer;
};