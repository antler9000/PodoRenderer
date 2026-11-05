#pragma once

//Podo
#include "Common.h"
#include "Debug.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <d3dx12_core.h>
#include <d3d12.h>
#include <DirectXMath.h>

//C++
#include <unordered_map>
#include <string>
#include <cstring>
#include <cstdint>

//NOTE: C++의 XMFLOAT3는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
struct DirectionalLight
{
	DirectX::XMFLOAT3	strength	= {};
	std::uint32_t		padding0	= {};
	DirectX::XMFLOAT3	direction	= { 0.0f, -1.0f, 0.0f };
	std::uint32_t		padding1	= {};
};

//NOTE: C++의 DirectionalLight는 4바이트 경계에 맞춰지므로, 셰이더 상수 버퍼의 16바이트 경계에 맞추기 위해 패딩을 추가함
//		또한 어레이로 사용할 경우에 16바이트 경계로 맞춰질 수 있도록 끝에 패딩을 추가함
struct LightConstantBuffer
{
	std::uint32_t		directionalLightsNum						= {};
	std::uint32_t		padding1[3]									= {};
	DirectionalLight	directionalLights[MAX_DIRECTIONAL_LIGHTS]	= {};
	DirectX::XMFLOAT3	ambientLight								= {};
	std::uint32_t		padding2									= {};
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
		constexpr UINT			constantBufferSize	= (sizeof(LightConstantBuffer) + 255) & ~255;
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
				IID_PPV_ARGS(m_lightConstantBuffer.ReleaseAndGetAddressOf())
			)
		);

		UpdateLightConstantBuffer();
	}

	//NOTE: 셰이더의 라이트 상수 버퍼에 담길 수 있는 최대 방향성 광원의 수를 초과하는 경우 false를 반환함
	bool SetDirectionalLight(std::string lightName, DirectionalLight light)
	{
		if (m_directionalLights.find(lightName) == m_directionalLights.end())
		{
			if (m_directionalLights.size() >= MAX_DIRECTIONAL_LIGHTS)
			{
				return false;
			}
		}

		m_directionalLights[lightName] = light;

		return true;
	}
	
	void UnsetDirectionalLight(std::string lightName)
	{
		m_directionalLights.erase(lightName);
	}

	void SetAmbientLight(DirectX::XMFLOAT3 ambientLight)
	{
		m_ambientLight = ambientLight;
	}

	void UpdateLightConstantBuffer()
	{
		LightConstantBuffer lightConstantBufferData = {};
		lightConstantBufferData.directionalLightsNum = static_cast<uint32_t>(m_directionalLights.size());
		int i = 0;
		for (const auto& [name, light] : m_directionalLights)
		{
			lightConstantBufferData.directionalLights[i] = light;

			i++;
		}
		lightConstantBufferData.ambientLight = m_ambientLight;

		void* pMappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };

		ThrowIfFailed(m_lightConstantBuffer->Map(0, &readRange, &pMappedData));
		std::memcpy(pMappedData, &lightConstantBufferData, sizeof(lightConstantBufferData));
		m_lightConstantBuffer->Unmap(0, nullptr);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetLightConstantBufferGPUAddress() const
	{
		return m_lightConstantBuffer->GetGPUVirtualAddress();
	}

private:

	std::unordered_map<std::string, DirectionalLight>	m_directionalLights	= {};
	DirectX::XMFLOAT3									m_ambientLight		= {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_lightConstantBuffer = {};
};