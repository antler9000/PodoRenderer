#pragma once

//Podo
#include "Debug.h"

//Win32
#include <wrl/client.h>

//D3D12
#include <d3dx12_core.h>
#include <d3d12.h>

//DXGI
#include <dxgiformat.h>

class Shadow
{
public:

	Shadow() = default;
	Shadow(Shadow&& sourceShadow) noexcept = default;
	Shadow& operator=(Shadow&& sourceShadow) noexcept = default;
	~Shadow() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Shadow(const Shadow& sourceShadow) = delete;
	Shadow& operator=(const Shadow& sourceShadow) = delete;

	void Create(ID3D12Device* pDevice, unsigned int width, unsigned int height)
	{
		m_viewport		= { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		m_scissorRect	= { 0, 0, static_cast<int>(width), static_cast<int>(height)};

		D3D12_RESOURCE_DESC depthStencilBufferDesc	= {};
		depthStencilBufferDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilBufferDesc.Alignment			= 0;
		depthStencilBufferDesc.Width				= width;
		depthStencilBufferDesc.Height				= height;
		depthStencilBufferDesc.DepthOrArraySize		= 1;
		depthStencilBufferDesc.MipLevels			= 1;
		depthStencilBufferDesc.Format				= DXGI_FORMAT_R32_TYPELESS;
		depthStencilBufferDesc.SampleDesc.Count		= 1;
		depthStencilBufferDesc.SampleDesc.Quality	= 0;
		depthStencilBufferDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilBufferDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue	= {};
		clearValue.Format				= DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth	= 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		ThrowIfFailed
		(
			pDevice->CreateCommittedResource
			(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&depthStencilBufferDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearValue,
				IID_PPV_ARGS(m_texture.ReleaseAndGetAddressOf())
			)
		);
	}

	D3D12_VIEWPORT GetViewport() const
	{
		return m_viewport;
	}

	D3D12_RECT GetScissorRect() const
	{
		return m_scissorRect;
	}

	ID3D12Resource* GetTexture() const
	{
		return m_texture.Get();
	}

	void SetDepthStencilViewCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		m_depthStencilViewCPUHandle = handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewCPUHandle() const
	{
		return m_depthStencilViewCPUHandle;
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

	D3D12_VIEWPORT	m_viewport		= {0.0f, 0.0f, 1024.0f, 1024.0f, 0.0f, 1.0f};
	D3D12_RECT		m_scissorRect	= {0, 0, 1024, 1024};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture = {};

	D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilViewCPUHandle		= {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_shaderResourceViewGPUHandle	= {};
};