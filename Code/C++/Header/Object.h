#pragma once

//Podo
#include "Asset.h"
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

struct ObjectBuffer
{
	DirectX::XMFLOAT4X4 world			= {};
	DirectX::XMFLOAT4X4 invTransWorld	= {};
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

	void Create(ID3D12Device* pDevice, unsigned int frameResourceNum)
	{
		m_frameResourceNum	= frameResourceNum;
		m_pDirtyFlags		= std::make_unique<bool[]>(frameResourceNum);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}

		unsigned int			resourceSize	= frameResourceNum * frameResourceSize;
		D3D12_HEAP_PROPERTIES	heapProperties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC		resourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(resourceSize);

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

	void SetAssetPtr(Asset* pAsset)
	{
		m_pAsset = pAsset;
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

	//NOTE: lookDirection이나 upDirection이 영벡터에 가깝거나, 서로 평행에 가까우면 회전을 설정하지 않고 false를 리턴함
	bool XM_CALLCONV SetRotation(DirectX::FXMVECTOR lookDirection, DirectX::FXMVECTOR upDirection)
	{
		if (IsNearZero(lookDirection) == true || IsNearZero(upDirection) == true || IsNearParallel(lookDirection, upDirection) == true)
		{
			return false;
		}

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

		DirectX::XMVECTOR rotationQuaternion = DirectX::XMQuaternionRotationMatrix(rotationMatrix);

		DirectX::XMStoreFloat4(&m_rotation, rotationQuaternion);

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}

		return true;
	}

	void SetScale(DirectX::XMFLOAT3 scale)
	{
		m_scale = scale;

		for (unsigned int frameResourceIndex = 0; frameResourceIndex < m_frameResourceNum; frameResourceIndex++)
		{
			m_pDirtyFlags[frameResourceIndex] = true;
		}
	}

	void CheckAndUpdateFrameResource(unsigned int frameResourceIndex)
	{
		if (m_pDirtyFlags[frameResourceIndex] == true)
		{
			DirectX::XMVECTOR direction = DirectX::XMLoadFloat4(&m_rotation);
			DirectX::XMVECTOR scale = DirectX::XMLoadFloat3(&m_scale);
			DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_position);

			DirectX::XMMATRIX worldMatrix =
				DirectX::XMMatrixScalingFromVector(scale) *
				DirectX::XMMatrixRotationQuaternion(direction) *
				DirectX::XMMatrixTranslationFromVector(position);

			DirectX::XMMATRIX worldInvMatrix = XMMatrixInverse(nullptr, worldMatrix);

			ObjectBuffer objectFrameResourceData = {};
			XMStoreFloat4x4(&objectFrameResourceData.world, XMMatrixTranspose(worldMatrix));
			XMStoreFloat4x4(&objectFrameResourceData.invTransWorld, worldInvMatrix);

			unsigned int offset = frameResourceIndex * frameResourceSize;

			void* pMappedData = nullptr;
			D3D12_RANGE readRange = { 0, 0 };
			ThrowIfFailed(m_resource->Map(0, &readRange, &pMappedData));

			std::memcpy(static_cast<std::uint8_t*>(pMappedData) + offset, &objectFrameResourceData, sizeof(ObjectBuffer));

			D3D12_RANGE writtenRange = { offset, offset + frameResourceSize };
			m_resource->Unmap(0, &writtenRange);

			m_pDirtyFlags[frameResourceIndex] = false;
		}
	}

	Asset* GetAssetPtr() const
	{
		return m_pAsset;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUAddress(unsigned int frameResourceIndex) const
	{
		return m_resource->GetGPUVirtualAddress() + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(frameResourceIndex) * frameResourceSize;
	}
	
	unsigned int GetResourceWidth() const
	{
		return frameResourceSize;
	}

	void SetConstantBufferViewGPUHandle(unsigned int frameResourceIndex, D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_pCBVLocationGPUHandles[frameResourceIndex] = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetConstantBufferViewGPUHandle(unsigned int frameResourceIndex) const
	{
		return m_pCBVLocationGPUHandles[frameResourceIndex];
	}

private:

	static constexpr unsigned int frameResourceSize = (sizeof(ObjectBuffer) + 255) & ~255;

private:

	unsigned int m_frameResourceNum			= {};
	std::unique_ptr<bool[]> m_pDirtyFlags	= {};

	Asset*	m_pAsset = {};

	DirectX::XMFLOAT3 m_scale		= { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 m_rotation	= { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 m_position	= { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = {};

	std::unique_ptr<D3D12_GPU_DESCRIPTOR_HANDLE[]> m_pCBVLocationGPUHandles = {};
};