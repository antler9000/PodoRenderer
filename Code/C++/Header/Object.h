#pragma once

//Podo
#include "Material.h"
#include "Mesh.h"
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

	void Create(ID3D12Device* pDevice)
	{
		constexpr unsigned int	constantBufferSize	= (sizeof(ObjectBuffer) + 255) & ~255;
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

	//NOTE: 메쉬는 업데이트가 이뤄지지 않으므로 const로 받아 저장함
	void SetMeshPtr(const Mesh* pMesh)
	{
		m_pMesh = pMesh;
	}

	void SetMaterialPtr(Material* pMaterial)
	{
		m_pMaterial = pMaterial;
	}

	//NOTE: 위치나 방향에 관한 매개변수는 호출부에서 SIMD 연산을 한 뒤 전달하기 편하도록 XMVECTOR 타입으로 둠
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

		DirectX::XMVECTOR rotationQuaternion = DirectX::XMQuaternionRotationMatrix(rotationMatrix);

		DirectX::XMStoreFloat4(&m_rotation, rotationQuaternion);
	}

	void SetScale(DirectX::XMFLOAT3 scale)
	{
		m_scale = scale;
	}

	void UpdateBuffer()
	{
		DirectX::XMVECTOR direction	= DirectX::XMLoadFloat4(&m_rotation);
		DirectX::XMVECTOR scale		= DirectX::XMLoadFloat3(&m_scale);
		DirectX::XMVECTOR position	= DirectX::XMLoadFloat3(&m_position);

		DirectX::XMMATRIX worldMatrix =
			DirectX::XMMatrixScalingFromVector(scale) *
			DirectX::XMMatrixRotationQuaternion(direction) *
			DirectX::XMMatrixTranslationFromVector(position);

		DirectX::XMMATRIX worldInvMatrix = XMMatrixInverse(nullptr, worldMatrix);

		ObjectBuffer objectConstantBufferData = {};
		XMStoreFloat4x4(&objectConstantBufferData.world, XMMatrixTranspose(worldMatrix));
		XMStoreFloat4x4(&objectConstantBufferData.invTransWorld, worldInvMatrix);

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_buffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &objectConstantBufferData, sizeof(objectConstantBufferData));
		m_buffer->Unmap(0, nullptr);
	}

	const Mesh* GetMesh() const
	{
		return m_pMesh;
	}

	Material* GetMaterial() const
	{
		return m_pMaterial;
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

	const Mesh*	m_pMesh		= {};
	Material*	m_pMaterial	= {};

	DirectX::XMFLOAT3 m_scale		= { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 m_rotation	= { 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 m_position	= { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_objectConstantBufferViewGPUHandle = {};
};