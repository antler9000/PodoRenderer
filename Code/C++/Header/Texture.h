#pragma once

//Win32
#include <wrl/client.h>

//D3D12
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <d3d12.h>

//C++
#include <string>

class Texture
{
public:

	Texture() = default;
	Texture(Texture&& sourceTexture) noexcept = default;
	Texture& operator=(Texture&& sourceTexture) noexcept = default;
	~Texture() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Texture(const Texture& sourceTexture) = delete;
	Texture& operator=(const Texture& sourceTexture) = delete;

	//NOTE: 구현에 사용하는 외부 헬퍼 함수가 wchar_t*를 요구하기에 wstring을 본 메서드의 매개변수로 둠
	void Create(ID3D12Device* pDevice, DirectX::ResourceUploadBatch& resourceUpload, std::wstring fileName)
	{
		std::wstring filePath = L"../Texture/" + fileName;

		bool isCubeMap = false;

		DirectX::CreateDDSTextureFromFileEx
		(
			pDevice,
			resourceUpload,
			filePath.c_str(),
			0,
			D3D12_RESOURCE_FLAG_NONE,
			DirectX::DDS_LOADER_DEFAULT,
			m_texture.ReleaseAndGetAddressOf(),
			nullptr,
			&isCubeMap
		);
	}

	ID3D12Resource* GetTexture() const
	{
		return m_texture.Get();
	}

	void SetShaderResourceViewGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
	{
		m_shaderResourceViewGPUHandle = handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetShaderResourceViewGPUHandle() const
	{
		return m_shaderResourceViewGPUHandle;
	}

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture = {};

	D3D12_GPU_DESCRIPTOR_HANDLE m_shaderResourceViewGPUHandle = {};
};