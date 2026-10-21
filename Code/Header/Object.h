#pragma once

//Podo
#include "Asset.h"
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

struct ObjectConstantBuffer
{
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInvTransMatrix;
};

class Object
{
public:

	Object() = default;
	Object(Object&& sourceObject) noexcept = default;
	Object& operator=(Object&& sourceObject) noexcept = default;
	~Object() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Object(const Object& sourceObject) = delete;
	Object& operator=(const Object& sourceObject) = delete;

	void Create(ID3D12Device* device, const Asset* asset)
	{
		m_asset = asset;

		constexpr UINT			constantBufferSize = (sizeof(ObjectConstantBuffer) + 255) & ~255;
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
				IID_PPV_ARGS(m_objectConstantBuffer.ReleaseAndGetAddressOf())
			)
		);

		UpdateObjectConstantBuffer();
	}

	void SetPosition(DirectX::XMVECTOR position)
	{
		DirectX::XMStoreFloat3(&m_position, position);
	}

	//NOTE: lookDirection과 upDirection가 영벡터에 가깝지 않고, 서로 평행에 가깝지 않음을 호출부에서 책임져야 함
	void XM_CALLCONV SetRotation(DirectX::FXMVECTOR lookDirection, DirectX::FXMVECTOR upDirection)
	{
		DirectX::XMVECTOR zAxis = DirectX::XMVector3Normalize(lookDirection);
		DirectX::XMVECTOR xAxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upDirection, zAxis));
		DirectX::XMVECTOR yAxis = DirectX::XMVector3Cross(zAxis, xAxis);

		DirectX::XMMATRIX rotationMatrix
		(
			DirectX::XMVectorSetW(xAxis, 0.0f),
			DirectX::XMVectorSetW(yAxis, 0.0f),
			DirectX::XMVectorSetW(zAxis, 0.0f),
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
		);

		DirectX::XMVECTOR direction = DirectX::XMQuaternionRotationMatrix(rotationMatrix);

		DirectX::XMStoreFloat4(&m_rotation, direction);
	}

	void SetScale(DirectX::XMVECTOR scale)
	{
		DirectX::XMStoreFloat3(&m_scale, scale);
	}

	void UpdateObjectConstantBuffer()
	{
		DirectX::XMVECTOR direction	= DirectX::XMLoadFloat4(&m_rotation);
		DirectX::XMVECTOR scale		= DirectX::XMLoadFloat3(&m_scale);
		DirectX::XMVECTOR position	= DirectX::XMLoadFloat3(&m_position);

		DirectX::XMMATRIX worldMatrix =
			DirectX::XMMatrixScalingFromVector(scale) *
			DirectX::XMMatrixRotationQuaternion(direction) *
			DirectX::XMMatrixTranslationFromVector(position);

		DirectX::XMMATRIX worldInvMatrix = XMMatrixInverse(nullptr, worldMatrix);

		ObjectConstantBuffer objectConstantBufferData;
		XMStoreFloat4x4(&objectConstantBufferData.worldMatrix, XMMatrixTranspose(worldMatrix));
		XMStoreFloat4x4(&objectConstantBufferData.worldInvTransMatrix, worldInvMatrix);

		void* mappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_objectConstantBuffer->Map(0, &readRange, &mappedData));
		std::memcpy(mappedData, &objectConstantBufferData, sizeof(objectConstantBufferData));
		m_objectConstantBuffer->Unmap(0, nullptr);
	}

	const Asset* GetAsset() const
	{
		return m_asset;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetObjectConstantBufferGPUAddress() const
	{
		return m_objectConstantBuffer->GetGPUVirtualAddress();
	}
	
	UINT GetObjectConstantBufferWidth() const
	{
		return static_cast<UINT>(m_objectConstantBuffer->GetDesc().Width);
	}

	void SetObjectConstantBufferViewGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_objectConstantBufferViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetObjectConstantBufferViewGPUHandle() const
	{
		return m_objectConstantBufferViewGPUHandle;
	}

private:

	const Asset* m_asset = nullptr;

	DirectX::XMFLOAT3 m_scale		= { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 m_rotation	= { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 m_position	= { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_objectConstantBuffer;

	D3D12_GPU_DESCRIPTOR_HANDLE m_objectConstantBufferViewGPUHandle = {};
};