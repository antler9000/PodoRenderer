#define NOMINMAX

//Podo
#include "Podo.h"
#include "Option.h"
#include "Camera.h"
#include "Light.h"
#include "Shadow.h"
#include "Object.h"
#include "Asset.h"
#include "Material.h"
#include "Texture.h"
#include "Mesh.h"
#include "Root.h"
#include "Alloc.h"
#include "Changed.h"
#include "Debug.h"

//ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <ResourceUploadBatch.h>
#include <d3dx12_root_signature.h>
#include <d3dx12_default.h>
#include <d3dx12_core.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

//DXGI
#include <dxgi1_6.h>
#include <dxgi1_5.h>
#include <dxgi1_4.h>
#include <dxgi1_3.h>
#include <dxgi1_2.h>
#include <dxgi.h>
#include <dxgicommon.h>
#include <dxgiformat.h>

//C++
#include <algorithm>
#include <string>
#include <vector>
#include <utility>
#include <cstdlib>
#include <climits>
#include <cstdint>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using std::wstring;

void Podo::Reset()
{
	FlushCommandQueue();

	if (NeedResetScreenMode() == true)
	{
		m_needResetScreenMode = false;

		ResetScreenMode();

		m_needResetFactory		= true;
		m_needResetSwapChain	= true;
	}

	if (NeedResetFactory() == true)
	{
		m_needResetFactory = false;

		ResetFactory();
		ResetAdapterAndOutput();

		m_needResetDevice		|= m_isAdapterChanged(m_dxgiAdapter.Get());		
		m_needResetSwapChain	|= m_isOutputChanged(m_dxgiOutputDesc);
	}

	if (NeedResetDevice() == true)
	{
		m_needResetDevice = false;

		ResetDevice();
		ResetFence();
		ResetFenceEvent();
		ResetCommandQueue();
		ResetCommandAllocator();
		ResetCommandList();

		m_needResetSwapChain			= true;
		m_needResetWorkloads			= true;
		m_needResetDescriptors			= true;
		m_needResetRenderSceneConfigure	= true;
		m_needResetRenderGUIConfigure	= true;
	}
	
	if (NeedResetSwapChain() == true)
	{
		m_needResetSwapChain = false;

		ResetHDRSwapChainSupport();
		ResetSwapChain();
		ResetBackBufferInfo();
		ResetViewPort();
		ResetScissorRectangle();
		ResetDepthStencilBuffer();	

		bool isRTVFormatChanged = m_isRTVFormatChanged(m_optionHDR.IsActive() ? screenRTVFormatHDR : screenRTVFormatSDR);

		//NOTE: RTV, DSV뿐만 아니라 워크로드들의 서술자까지 재초기화되도록 하는 문제점이 있음
		m_needResetDescriptors			 = true;
		m_needResetRenderSceneConfigure	|= isRTVFormatChanged;
		m_needResetRenderGUIConfigure	|= isRTVFormatChanged;
	}

	if (NeedResetWorkload() == true)
	{
		m_needResetWorkloads = false;

		ResetMeshes();
		ResetTextures();
		ResetMaterials();
		ResetAssets();
		ResetObjects();
		ResetLights();
		ResetCameras();

		//NOTE: 라이트 객체 개수에 맞춰 PSO가 재초기화되야하는 문제점이 있음
		m_needResetDescriptors			= true;
		m_needResetRenderSceneConfigure	= true;
	}

	if (NeedResetDescriptor() == true)
	{
		m_needResetDescriptors = false;

		ResetDescriptorHeapRTV();
		ResetDescriptorHeapDSV();
		ResetDescriptorHeapCBVSRVUAV();
		ResetRTV();
		ResetDSV();
		ResetCBVSRVUAV();

		m_needResetRenderGUIConfigure = true;
	}

	if (NeedResetRenderSceneConfigure() == true)
	{
		m_needResetRenderSceneConfigure = false;

		ResetShadowRootSignature();
		ResetMainRootSignature();
		ResetPipelineStateObjects();
	}

	if (NeedResetRenderGUIConfigure() == true)
	{
		m_needResetRenderGUIConfigure = false;

		ResetImGui();
	}

	m_optionFullScreen.DebugPrint();
	m_optionWindowSave.DebugPrint();
	m_optionVSync.DebugPrint();
	m_optionTearing.DebugPrint();
	m_optionHDR.DebugPrint();
	m_optionGUI.DebugPrint();
}

void Podo::ResetScreenMode()
{
	if (m_optionFullScreen.IsActive() == true)
	{
		ResetFullScreenMode();
	}
	else
	{
		ResetWindowMode();
	}
}

void Podo::ResetFullScreenMode()
{
	SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

	HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = {};
	monitorInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &monitorInfo);

	LONG monitorBaseX	= monitorInfo.rcMonitor.left;
	LONG monitorBaseY	= monitorInfo.rcMonitor.top;
	LONG monitorWidth	= monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	LONG monitorHeight	= monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	SetWindowPos
	(
		m_hWnd,
		HWND_TOP,
		monitorBaseX,
		monitorBaseY,
		monitorWidth,
		monitorHeight,
		SWP_NOOWNERZORDER | SWP_FRAMECHANGED
	);
}

void Podo::ResetWindowMode()
{
	SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

	SetWindowPos
	(
		m_hWnd,
		HWND_TOP,
		m_optionWindowSave.GetWindowPosX(),
		m_optionWindowSave.GetWindowPosY(),
		m_optionWindowSave.GetWindowWidth(),
		m_optionWindowSave.GetWindowHeight(),
		SWP_NOOWNERZORDER | SWP_FRAMECHANGED
	);
}

void Podo::ResetFactory()
{
	unsigned int factoryFlags = 0;

#ifdef _DEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

	//NOTE: 쿼리 출력 매개변수는 BOOL 타입이므로, bool 타입인 m_optionTearing.featureSupported를 인자로 사용하면 안 됨
	BOOL tearingQuery = FALSE;
	m_dxgiFactory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&tearingQuery,
		sizeof(tearingQuery)
	);
	m_optionTearing.SetFeatureSupported(tearingQuery);
}

void Podo::ResetAdapterAndOutput()
{
	m_dxgiAdapter.Reset();

	ComPtr<IDXGIAdapter3> tempAdapter = nullptr;

	HRESULT result = S_OK;
	for (int i = 0; result != DXGI_ERROR_NOT_FOUND; i++)
	{
		result = m_dxgiFactory->EnumAdapterByGpuPreference
		(
			i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(tempAdapter.ReleaseAndGetAddressOf())
		);

		if (FAILED(result) == true)
		{
			continue;
		}

		if (ResetOutput(tempAdapter.Get()) == true)
		{
			m_dxgiAdapter = tempAdapter;

			return;
		}
	}
}

bool Podo::ResetOutput(IDXGIAdapter3* pAdapter)
{
	m_dxgiOutput.Reset();
	m_dxgiOutput6.Reset();

	ComPtr<IDXGIOutput> tempOutput = nullptr;

	HMONITOR targetMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

	HRESULT enumResult = S_OK;
	for (int i = 0; enumResult != DXGI_ERROR_NOT_FOUND; i++)
	{
		enumResult = pAdapter->EnumOutputs(i, tempOutput.ReleaseAndGetAddressOf());

		if (SUCCEEDED(enumResult) == true)
		{
			DXGI_OUTPUT_DESC tempOutputDesc;
			tempOutput->GetDesc(&tempOutputDesc);

			if (tempOutputDesc.Monitor == targetMonitor)
			{
				m_dxgiOutput = tempOutput;

				HRESULT asResult = m_dxgiOutput.As(&m_dxgiOutput6);
				if (SUCCEEDED(asResult) == true)
				{
					m_dxgiOutput6->GetDesc1(&m_dxgiOutputDesc);
					if (m_dxgiOutputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
					{
						m_optionHDR.SetOutputSupported(false);
					}
					else
					{
						m_optionHDR.SetOutputSupported(true);
					}
				}
				else
				{
					m_optionHDR.SetOutputSupported(false);
				}

				return true;
			}
		}
	}

	return false;
}

void Podo::ResetDevice()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debug;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf())));
	debug->EnableDebugLayer();
	debug->SetEnableGPUBasedValidation(true);
#endif

	ThrowIfFailed
	(
		D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))
	);

	{
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
		ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));
	}

	{
		D3D12_FEATURE_DATA_FORMAT_SUPPORT backBufferFormatSDRQuery =
		{
			screenBackBufferFormatSDR,
			D3D12_FORMAT_SUPPORT1_NONE,
			D3D12_FORMAT_SUPPORT2_NONE
		};
		D3D12_FEATURE_DATA_FORMAT_SUPPORT backBufferFormatHDRQuery =
		{
			screenBackBufferFormatHDR,
			D3D12_FORMAT_SUPPORT1_NONE,
			D3D12_FORMAT_SUPPORT2_NONE
		};
		D3D12_FEATURE_DATA_FORMAT_SUPPORT depthStencilFormatQuery =
		{
			screenDepthStencilBufferFormat,
			D3D12_FORMAT_SUPPORT1_NONE,
			D3D12_FORMAT_SUPPORT2_NONE
		};

		ThrowIfFailed
		(
			m_device->CheckFeatureSupport
			(
				D3D12_FEATURE_FORMAT_SUPPORT, &depthStencilFormatQuery, sizeof(depthStencilFormatQuery)
			)
		);
		ThrowIfFailed
		(
			m_device->CheckFeatureSupport
			(
				D3D12_FEATURE_FORMAT_SUPPORT, &backBufferFormatSDRQuery, sizeof(backBufferFormatSDRQuery)
			)
		);
		HRESULT hdrQueryResult = m_device->CheckFeatureSupport
		(
			D3D12_FEATURE_FORMAT_SUPPORT, &backBufferFormatHDRQuery, sizeof(backBufferFormatHDRQuery)
		);

		ThrowIfFalse(depthStencilFormatQuery.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
		ThrowIfFalse(backBufferFormatSDRQuery.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
		if (SUCCEEDED(hdrQueryResult) == true)
		{
			m_optionHDR.SetFormatSupported((backBufferFormatHDRQuery.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0);
		}
		else
		{
			m_optionHDR.SetFormatSupported(false);
		}
	}
}

void Podo::ResetFence()
{
	ThrowIfFailed(m_device->CreateFence(m_fenceCurrent, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
}

void Podo::ResetFenceEvent()
{
	CloseFenceEvent();

	m_fenceEvent = CreateEventExW
	(
		nullptr,
		nullptr,
		0,
		EVENT_MODIFY_STATE | SYNCHRONIZE
	);

	ThrowIfNull(m_fenceEvent);
}

void Podo::ResetCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask	= 0;
	ThrowIfFailed
	(
		m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf()))
	);
}

void Podo::ResetCommandAllocator()
{
	ThrowIfFailed
	(
		m_device->CreateCommandAllocator
		(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetCommandList()
{
	ThrowIfFailed
	(
		m_device->CreateCommandList
		(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())
		)
	);

	m_commandList->Close();
}

void Podo::ResetHDRSwapChainSupport()
{
	for (unsigned int i = 0; i < screenBackBufferCount; i++)
	{
		m_screenBackBuffers[i].Reset();
	}

	m_screenSwapChain.Reset();

	RECT rectClient = {};
	ThrowIfFalse(GetClientRect(m_hWnd, &rectClient));
	LONG widthClient	= rectClient.right - rectClient.left;
	LONG heightClient	= rectClient.bottom - rectClient.top;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width					= std::max((int)widthClient, 10);
	swapChainDesc.Height				= std::max((int)heightClient, 10);
	swapChainDesc.Format				= screenBackBufferFormatHDR;
	swapChainDesc.Stereo				= false;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.SampleDesc.Quality	= 0;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= screenBackBufferCount;
	swapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags					= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
										| (m_optionTearing.IsActive() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	ComPtr<IDXGISwapChain1> tempSwapChain = nullptr;

	ThrowIfFailed
	(
		m_dxgiFactory->CreateSwapChainForHwnd
		(
			m_commandQueue.Get(),
			m_hWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			tempSwapChain.ReleaseAndGetAddressOf()
		)
	);

	ComPtr<IDXGISwapChain3> tempSwapChain3 = nullptr;

	ThrowIfFailed((tempSwapChain.As(&tempSwapChain3)));

	unsigned int colorSpaceHDRQuery = 0;
	HRESULT queryResult = tempSwapChain3->CheckColorSpaceSupport(screenBackBufferColorSpaceHDR, &colorSpaceHDRQuery);
	if (SUCCEEDED(queryResult) == true)
	{
		m_optionHDR.SetColorSpaceSupported((colorSpaceHDRQuery & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0);
	}
	else
	{
		m_optionHDR.SetColorSpaceSupported(false);
	}
}

void Podo::ResetSwapChain()
{
	for (unsigned int i = 0; i < screenBackBufferCount; i++)
	{
		m_screenBackBuffers[i].Reset();
	}

	m_screenSwapChain.Reset();

	RECT rectClient = {};
	ThrowIfFalse(GetClientRect(m_hWnd, &rectClient));
	LONG widthClient	= rectClient.right - rectClient.left;
	LONG heightClient	= rectClient.bottom - rectClient.top;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width					= std::max((int)widthClient, 10);
	swapChainDesc.Height				= std::max((int)heightClient, 10);;
	swapChainDesc.Format				= m_optionHDR.IsActive() ? screenBackBufferFormatHDR : screenBackBufferFormatSDR;
	swapChainDesc.Stereo				= false;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.SampleDesc.Quality	= 0;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= screenBackBufferCount;
	swapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags					= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
										| (m_optionTearing.IsActive() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	ComPtr<IDXGISwapChain1> tempSwapChain = nullptr;

	ThrowIfFailed
	(
		m_dxgiFactory->CreateSwapChainForHwnd
		(
			m_commandQueue.Get(),
			m_hWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			tempSwapChain.ReleaseAndGetAddressOf()
		)
	);

	//NOTE: ALT+ENTER을 누를 때 "삐-" 시스템 소리가 안 나게 하려면 스왑 체인이 생성된 현 시점에 아래 메서드를 호출해야 함
	ThrowIfFailed
	(
		m_dxgiFactory->MakeWindowAssociation
		(
			m_hWnd,
			DXGI_MWA_NO_ALT_ENTER
		)
	);

	ThrowIfFailed(tempSwapChain.As(&m_screenSwapChain));

	if (m_optionHDR.IsActive() == true)
	{
		m_screenSwapChain->SetColorSpace1(screenBackBufferColorSpaceHDR);
	}
	else
	{
		m_screenSwapChain->SetColorSpace1(screenBackBufferColorSpaceSDR);
	}
}

void Podo::ResetBackBufferInfo()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	m_screenSwapChain->GetDesc1(&swapChainDesc);
	m_screenBackBufferWidth			= swapChainDesc.Width;
	m_screenBackBufferHeight		= swapChainDesc.Height;
	m_screenBackBufferAspectRatio	= (m_screenBackBufferHeight ? (static_cast<float>(m_screenBackBufferWidth) / m_screenBackBufferHeight) : 0);

	m_screenBackBufferIndex = m_screenSwapChain->GetCurrentBackBufferIndex();

	for (unsigned int i = 0; i < screenBackBufferCount; i++)
	{
		ThrowIfFailed(m_screenSwapChain->GetBuffer(i, IID_PPV_ARGS(m_screenBackBuffers[i].ReleaseAndGetAddressOf())));
	}
}

void Podo::ResetViewPort()
{
	m_screenViewport.TopLeftX	= 0.0f;
	m_screenViewport.TopLeftY	= 0.0f;
	m_screenViewport.Width		= static_cast<float>(m_screenBackBufferWidth);
	m_screenViewport.Height		= static_cast<float>(m_screenBackBufferHeight);
	m_screenViewport.MinDepth	= 0.0f;
	m_screenViewport.MaxDepth	= 1.0f;
}

void Podo::ResetScissorRectangle()
{
	m_screenScissorRect.left	= 0;
	m_screenScissorRect.top		= 0;
	m_screenScissorRect.right	= m_screenBackBufferWidth;
	m_screenScissorRect.bottom	= m_screenBackBufferHeight;
}

void Podo::ResetDepthStencilBuffer()
{
	D3D12_RESOURCE_DESC depthStencilBufferDesc = {};
	depthStencilBufferDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilBufferDesc.Alignment			= 0;
	depthStencilBufferDesc.Width				= m_screenBackBufferWidth;
	depthStencilBufferDesc.Height				= m_screenBackBufferHeight;
	depthStencilBufferDesc.DepthOrArraySize		= 1;
	depthStencilBufferDesc.MipLevels			= 1;
	depthStencilBufferDesc.Format				= screenDepthStencilBufferFormat;
	depthStencilBufferDesc.SampleDesc.Count		= 1;
	depthStencilBufferDesc.SampleDesc.Quality	= 0;
	depthStencilBufferDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilBufferDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format				= screenDepthStencilBufferFormat;
	clearValue.DepthStencil.Depth	= 1.0f;
	clearValue.DepthStencil.Stencil	= 0;

	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed
	(
		m_device->CreateCommittedResource
		(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(m_screenDepthStencilBuffer.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetMeshes()
{
	m_workloadMeshes.clear();

	std::vector<Vertex> cubeVertices =
	{
		// -Z
		{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
		{{-0.5f, +0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		{{+0.5f, +0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{+0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},

		// +Z
		{{-0.5f, -0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {1.0f, 1.0f}},
		{{-0.5f, +0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {1.0f, 0.0f}},
		{{+0.5f, +0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.0f, 0.0f}},
		{{+0.5f, -0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.0f, 1.0f}},

		// -X
		{{-0.5f, -0.5f, +0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		{{-0.5f, +0.5f, +0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{-0.5f, +0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},

		// +X
		{{+0.5f, -0.5f, -0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},

		// +Y
		{{-0.5f, +0.5f, -0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 1.0f}},
		{{-0.5f, +0.5f, +0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 0.0f}},
		{{+0.5f, +0.5f, +0.5f}, { 0.0f, +1.0f,  0.0f}, {1.0f, 0.0f}},
		{{+0.5f, +0.5f, -0.5f}, { 0.0f, +1.0f,  0.0f}, {1.0f, 1.0f}},

		// -Y
		{{-0.5f, -0.5f, +0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		{{+0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{+0.5f, -0.5f, +0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}
	};

	std::vector<uint32_t> cubeIndices =
	{
		0,	1,	2,		0,	2,	3,	// -Z
		4,	6,	5,		4,	7,  6,	// +Z
		8,	9,	10,		8,	10,	11,	// -X
		12,	13,	14,		12,	14,	15,	// +X
		16,	17,	18,		16,	18,	19,	// +Y
		20,	21,	22,		20,	22,	23	// -Y
	};

	//NOTE: 업로드 힙을 거쳐야 하기에 번거로운 디폴트 힙 자원 생성은 DirectXTK12의 ResourceUploadBatch 헬퍼를 이용하기로 함
	ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();
	{
		{
			Mesh cubeMesh;
			cubeMesh.Create(m_device.Get(), resourceUpload, std::move(cubeVertices), std::move(cubeIndices));

			//NOTE: 실제 자원의 업로드 및 복사는 일어나지 않았지만, boxMesh의 초기화는 곧바로 일어나므로 바로 이렇게 move해도 됨
			m_workloadMeshes["Cube"] = std::move(cubeMesh);
		}
	}

	//NOTE: resourceUpload.End(..)에서 반환되는 future 객체는 소멸될 때 업로드 작업이 완료될때까지 CPU 대기가 발생하므로 멤버 변수에 받아둠
	m_meshResourceUpload = resourceUpload.End(m_commandQueue.Get());
}

void Podo::ResetTextures()
{
	m_workloadTextures.clear();

	ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();
	{
		{
			Texture steelTexture;

			steelTexture.Create(m_device.Get(), resourceUpload, L"Steel.dds");

			m_workloadTextures["Steel"] = std::move(steelTexture);
		}

		{
			Texture woodTexture;

			woodTexture.Create(m_device.Get(), resourceUpload, L"Wood.dds");

			m_workloadTextures["Wood"] = std::move(woodTexture);
		}

		{
			Texture plasticTexture;

			plasticTexture.Create(m_device.Get(), resourceUpload, L"Plastic.dds");

			m_workloadTextures["Plastic"] = std::move(plasticTexture);

		}

		{
			Texture paperTexture;

			paperTexture.Create(m_device.Get(), resourceUpload, L"Paper.dds");

			m_workloadTextures["Paper"] = std::move(paperTexture);
		}
	}

	m_textureResourceUpload = resourceUpload.End(m_commandQueue.Get());
}

void Podo::ResetMaterials()
{
	m_workloadMaterials.clear();

	{
		Material steelMaterial;
		steelMaterial.Create(m_device.Get());
		steelMaterial.SetTexturePtr(&m_workloadTextures.at("Steel"));

		XMFLOAT3	diffuseAlbedo	= { 0.20f, 0.20f, 0.20f };
		XMFLOAT3	fresnelR0		= { 0.90f, 0.90f, 0.90f };
		float		shininess		= 0.50f;
		steelMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		steelMaterial.SetFresnelR0(fresnelR0);
		steelMaterial.SetShininess(shininess);
		steelMaterial.UpdateBuffer();

		m_workloadMaterials["Steel"] = std::move(steelMaterial);
	}

	{
		Material woodMaterial;
		woodMaterial.Create(m_device.Get());
		woodMaterial.SetTexturePtr(&m_workloadTextures.at("Wood"));

		XMFLOAT3	diffuseAlbedo	= { 0.80f, 0.70f, 0.40f };
		XMFLOAT3	fresnelR0		= { 0.05f, 0.05f, 0.05f };
		float		shininess		= 0.10f;
		woodMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		woodMaterial.SetFresnelR0(fresnelR0);
		woodMaterial.SetShininess(shininess);
		woodMaterial.UpdateBuffer();

		m_workloadMaterials["Wood"] = std::move(woodMaterial);
	}

	{
		Material plasticMaterial;
		plasticMaterial.Create(m_device.Get());
		plasticMaterial.SetTexturePtr(&m_workloadTextures.at("Plastic"));

		XMFLOAT3	diffuseAlbedo	= { 0.70f, 1.00f, 0.70f };
		XMFLOAT3	fresnelR0		= { 0.10f, 0.10f, 0.10f };
		float		shininess		= 0.80f;
		plasticMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		plasticMaterial.SetFresnelR0(fresnelR0);
		plasticMaterial.SetShininess(shininess);
		plasticMaterial.UpdateBuffer();

		m_workloadMaterials["Plastic"] = std::move(plasticMaterial);
	}

	{
		Material paperMaterial;
		paperMaterial.Create(m_device.Get());
		paperMaterial.SetTexturePtr(&m_workloadTextures.at("Paper"));

		XMFLOAT3	diffuseAlbedo	= { 0.90f, 0.90f, 0.90f };
		XMFLOAT3	fresnelR0		= { 0.05f, 0.05f, 0.05f };
		float		shininess		= 0.05f;
		paperMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		paperMaterial.SetFresnelR0(fresnelR0);
		paperMaterial.SetShininess(shininess);
		paperMaterial.UpdateBuffer();

		m_workloadMaterials["Paper"] = std::move(paperMaterial);
	}
}

void Podo::ResetAssets()
{
	{
		Asset steelCube;
		steelCube.SetMeshPtr(&m_workloadMeshes.at("Cube"));
		steelCube.SetMaterialPtr(&m_workloadMaterials.at("Steel"));

		m_workloadAssets["SteelCube"] = std::move(steelCube);
	}

	{
		Asset woodCube;
		woodCube.SetMeshPtr(&m_workloadMeshes.at("Cube"));
		woodCube.SetMaterialPtr(&m_workloadMaterials.at("Wood"));

		m_workloadAssets["WoodCube"] = std::move(woodCube);
	}

	{
		Asset plasticCube;
		plasticCube.SetMeshPtr(&m_workloadMeshes.at("Cube"));
		plasticCube.SetMaterialPtr(&m_workloadMaterials.at("Plastic"));

		m_workloadAssets["PlasticCube"] = std::move(plasticCube);
	}

	{
		Asset paperCube;
		paperCube.SetMeshPtr(&m_workloadMeshes.at("Cube"));
		paperCube.SetMaterialPtr(&m_workloadMaterials.at("Paper"));

		m_workloadAssets["PaperCube"] = std::move(paperCube);
	}
}

void Podo::ResetObjects()
{
	m_workloadObjects.clear();

	{
		Object steelBoxObject;
		steelBoxObject.Create(m_device.Get());
		steelBoxObject.SetAssetPtr(&m_workloadAssets.at("SteelCube"));

		XMFLOAT3 scale			= { 1.0f, 1.0f, 1.0f };
		XMVECTOR lookDirection	= XMVectorSet(0.0f, 0.0f,  1.0f, 0.0f);
		XMVECTOR upDirection	= XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f);
		XMVECTOR position		= XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);
		steelBoxObject.SetScale(scale);
		steelBoxObject.SetRotation(lookDirection, upDirection);
		steelBoxObject.SetPosition(position);
		steelBoxObject.UpdateBuffer();

		m_workloadObjects["SteelBox"] = std::move(steelBoxObject);
	}

	{
		Object woodBoxObject;
		woodBoxObject.Create(m_device.Get());
		woodBoxObject.SetAssetPtr(&m_workloadAssets.at("WoodCube"));

		XMFLOAT3 scale			= { 1.0f, 1.0f, 1.0f };
		XMVECTOR lookDirection	= XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR upDirection	= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR position		= XMVectorSet(0.0f, 0.0f, 2.0f, 1.0f);
		woodBoxObject.SetScale(scale);
		woodBoxObject.SetRotation(lookDirection, upDirection);
		woodBoxObject.SetPosition(position);
		woodBoxObject.UpdateBuffer();

		m_workloadObjects["WoodBox"] = std::move(woodBoxObject);
	}

	{
		Object plasticPillarObject;
		plasticPillarObject.Create(m_device.Get());
		plasticPillarObject.SetAssetPtr(&m_workloadAssets.at("PlasticCube"));

		XMFLOAT3 scale			= { 1.0f, 6.0f, 1.0f };
		XMVECTOR lookDirection	= XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR upDirection	= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR position		= XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		plasticPillarObject.SetScale(scale);
		plasticPillarObject.SetRotation(lookDirection, upDirection);
		plasticPillarObject.SetPosition(position);
		plasticPillarObject.UpdateBuffer();

		m_workloadObjects["PlasticPillar"] = std::move(plasticPillarObject);
	}

	{
		Object paperFloorObject;
		paperFloorObject.Create(m_device.Get());
		paperFloorObject.SetAssetPtr(&m_workloadAssets.at("PaperCube"));

		XMFLOAT3 scale			= { 10.0f, 0.1f, 10.0f };
		XMVECTOR lookDirection	= XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR upDirection	= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR position		= XMVectorSet(0.0f, -3.0f, 0.0f, 1.0f);
		paperFloorObject.SetScale(scale);
		paperFloorObject.SetRotation(lookDirection, upDirection);
		paperFloorObject.SetPosition(position);
		paperFloorObject.UpdateBuffer();

		m_workloadObjects["PaperFloor"] = std::move(paperFloorObject);
	}
}

void Podo::ResetLights()
{
	m_workloadLights.clear();

	{
		Light sun1;
		sun1.Create(m_device.Get(), true, 2048, 2048);

		XMFLOAT3 strength	= { 0.6f, 0.5f, 0.5f };
		XMVECTOR direction	= XMVectorSet(0.0f,  -1.0f, 0.0f, 0.0f);
		XMVECTOR poistion	= XMVectorSet(0.0f, 100.0f, 0.0f, 1.0f);
		sun1.SetStrength(strength);
		sun1.SetDirection(direction);
		sun1.SetPosition(poistion);
		sun1.SetViewVolume(50.0f, 50.0f, 0.1f, 1000.0f);
		sun1.UpdateBuffer();

		m_workloadLights["Sun1"] = std::move(sun1);
	}

	{
		Light sun2;
		sun2.Create(m_device.Get(), true, 2048, 2048);

		XMFLOAT3 strength	= { 0.6f, 0.5f, 0.5f };
		XMVECTOR direction	= XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
		XMVECTOR poistion	= XMVectorSet(0.0f, 100.0f, 0.0f, 1.0f);
		sun2.SetStrength(strength);
		sun2.SetDirection(direction);
		sun2.SetPosition(poistion);
		sun2.SetViewVolume(50.0f, 50.0f, 0.1f, 1000.0f);
		sun2.UpdateBuffer();

		m_workloadLights["Sun2"] = std::move(sun2);
	}
}

void Podo::ResetCameras()
{
	m_workloadCameras.clear();

	{
		Camera mainCamera;
		mainCamera.Create(m_device.Get());

		XMFLOAT3 ambientLightStrength	= { 0.05f, 0.05f, 0.05f };
		XMVECTOR position				= XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f);
		XMVECTOR target					= XMVectorSet(0.0f, 0.0f,  0.0f, 1.0f);
		XMVECTOR upDirection			= XMVectorSet(0.0f, 1.0f,  0.0f, 1.0f);
		mainCamera.SetDirectionalLightsNum(static_cast<unsigned int>(m_workloadLights.size()));
		mainCamera.SetAmbientLightStrength(ambientLightStrength);
		mainCamera.SetPosition(position);
		mainCamera.SetTarget(target);
		mainCamera.SetUpDirection(upDirection);
		mainCamera.SetViewVolume(cameraHorizontalFOV, m_screenBackBufferAspectRatio, cameraNearZ, cameraFarZ);
		mainCamera.UpdateBuffer();

		m_workloadCameras["MainCamera"] = std::move(mainCamera);
	}
}

void Podo::ResetDescriptorHeapRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= screenBackBufferCount;
	descriptorHeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask			= 0;

	ThrowIfFailed
	(
		m_device->CreateDescriptorHeap
		(
			&descriptorHeapDesc,
			IID_PPV_ARGS(m_descriptorHeapRTV.ReleaseAndGetAddressOf())
		)
	);

	m_descriptorHeapRTVIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorHeapRTVStartCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
}

void Podo::ResetDescriptorHeapDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= 1 + static_cast<unsigned int>(m_workloadLights.size());
	descriptorHeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask			= 0;

	ThrowIfFailed
	(
		m_device->CreateDescriptorHeap
		(
			&descriptorHeapDesc,
			IID_PPV_ARGS(m_descriptorHeapDSV.ReleaseAndGetAddressOf())
		)
	);

	m_descriptorHeapDSVIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_descriptorHeapDSVStartCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart());
}

void Podo::ResetDescriptorHeapCBVSRVUAV()
{
	//NOTE: 각 라이트 객체는 스스로의 CBV와 하위 쉐도우 맵의 SRV를 하나씩 사용하므로 서술자 2개가 필요함
	//		또한 라이트가 0개인 경우에도 쉐도우와 라이트 시작점 핸들이 힙 바깥을 가리키지 않도록 힙의 끝에 빈 공간 1개를 추가함
	size_t workloadDescriptorDemand =
		m_workloadTextures.size()	+
		m_workloadMaterials.size()	+
		m_workloadObjects.size()	+
		2 * m_workloadLights.size() +
		1;
	
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= imGuiDescriptorCapacity + static_cast<unsigned int>(workloadDescriptorDemand);
	descriptorHeapDesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask			= 0;

	ThrowIfFailed
	(
		m_device->CreateDescriptorHeap
		(
			&descriptorHeapDesc,
			IID_PPV_ARGS(m_descriptorHeapCBVSRVUAV.ReleaseAndGetAddressOf())
		)
	);

	//NOTE: GUI 렌더링을 위한 SRV를 둘 곳을 고정적으로 남겨두고, 씬 렌더링은 그 뒷부분부터 사용하기로 함
	m_descriptorHeapCBVSRVUAVIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorHeapCBVSRVUAVStartCPUHandleForGUI = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart());
	m_descriptorHeapCBVSRVUAVStartGPUHandleForGUI = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());

	m_descriptorHeapCBVSRVUAVStartCPUHandleForScene = m_descriptorHeapCBVSRVUAVStartCPUHandleForGUI;
	m_descriptorHeapCBVSRVUAVStartCPUHandleForScene.Offset(imGuiDescriptorCapacity, m_descriptorHeapCBVSRVUAVIncrementSize);
	m_descriptorHeapCBVSRVUAVStartGPUHandleForScene = m_descriptorHeapCBVSRVUAVStartGPUHandleForGUI;
	m_descriptorHeapCBVSRVUAVStartGPUHandleForScene.Offset(imGuiDescriptorCapacity, m_descriptorHeapCBVSRVUAVIncrementSize);
}

void Podo::ResetRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartCPUHandle;
	for (unsigned int i = 0; i < screenBackBufferCount; i++)
	{
		D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
		renderTargetViewDesc.Format					= m_optionHDR.IsActive() ? screenRTVFormatHDR : screenRTVFormatSDR;
		renderTargetViewDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice		= 0;
		renderTargetViewDesc.Texture2D.PlaneSlice	= 0;

		m_device->CreateRenderTargetView(m_screenBackBuffers[i].Get(), &renderTargetViewDesc, cpuHandleRTV.Offset(i, m_descriptorHeapRTVIncrementSize));
	}
}

void Podo::ResetDSV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapDSVStartCPUHandle;

	{
		m_device->CreateDepthStencilView(m_screenDepthStencilBuffer.Get(), nullptr, m_descriptorHeapDSVStartCPUHandle);

		cpuHandle.Offset(1, m_descriptorHeapDSVIncrementSize);
	}

	//NOTE: 라이트와 쉐도우가 동일한 인덱스를 가지기 위해, 쉐도우를 사용하지 않는 광원이 있더라도 빈 칸을 당겨 채우지 않음
	for (auto& [name, light] : m_workloadLights)
	{
		if (light.GetIsShadowOn() == true)
		{
			Shadow* pShadow = light.GetShadowPtr();

			D3D12_DEPTH_STENCIL_VIEW_DESC shadowDepthStencilViewDesc = {};
			shadowDepthStencilViewDesc.Format			= DXGI_FORMAT_D32_FLOAT;
			shadowDepthStencilViewDesc.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
			shadowDepthStencilViewDesc.Flags			= D3D12_DSV_FLAG_NONE;

			m_device->CreateDepthStencilView(pShadow->GetResource(), &shadowDepthStencilViewDesc, cpuHandle);

			pShadow->SetDepthStencilViewCPUHandle(cpuHandle);
		}

		cpuHandle.Offset(1, m_descriptorHeapDSVIncrementSize);
	}
}

void Podo::ResetCBVSRVUAV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCBVSRVUAVStartCPUHandleForScene;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeapCBVSRVUAVStartGPUHandleForScene;

	m_descriptorHeapCBVSRVUAVStartGPUHandleForTextures = gpuHandle;
	for (auto& [name, texture] : m_workloadTextures)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format							= DXGI_FORMAT_BC7_UNORM_SRGB;
		srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip		= 0;
		srvDesc.Texture2D.MipLevels				= -1;
		srvDesc.Texture2D.PlaneSlice			= 0;
		srvDesc.Texture2D.ResourceMinLODClamp	= 0.0f;

		m_device->CreateShaderResourceView(texture.GetResource(), &srvDesc, cpuHandle);

		texture.SetShaderResourceViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	m_descriptorHeapCBVSRVUAVStartGPUHandleForMaterials = gpuHandle;
	for (auto& [name, material] : m_workloadMaterials)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= material.GetResourceGPUAddress();
		cbvDesc.SizeInBytes		= material.GetResourceWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		material.SetConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	m_descriptorHeapCBVSRVUAVStartGPUHandleForObjects = gpuHandle;
	for (auto& [name, object] : m_workloadObjects)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= object.GetResourceGPUAddress();
		cbvDesc.SizeInBytes		= object.GetResourceWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		object.SetConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	//NOTE: 라이트와 쉐도우가 동일한 인덱스를 가지기 위해, 쉐도우를 사용하지 않는 광원이 있더라도 빈 칸을 당겨 채우지 않음
	m_descriptorHeapCBVSRVUAVStartGPUHandleForShadows = gpuHandle;
	for (auto& [name, light] : m_workloadLights)
	{
		Shadow* pShadow = light.GetShadowPtr();

		if (light.GetIsShadowOn() == true)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc	= {};
			srvDesc.Format							= DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MostDetailedMip		= 0;
			srvDesc.Texture2D.MipLevels				= 1;
			srvDesc.Texture2D.PlaneSlice			= 0;
			srvDesc.Texture2D.ResourceMinLODClamp	= 0.0f;

			m_device->CreateShaderResourceView(pShadow->GetResource(), &srvDesc, cpuHandle);

			pShadow->SetShaderResourceViewGPUHandle(gpuHandle);
		}

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	m_descriptorHeapCBVSRVUAVStartGPUHandleForLights = gpuHandle;
	for (auto& [name, light] : m_workloadLights)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= light.GetResourceGPUAddress();
		cbvDesc.SizeInBytes		= light.GetResourceWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		light.SetConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}
}

void Podo::ResetShadowRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[SHADOW_ROOT_PARAMETER_COUNT] = {};

	//NOTE: 하나의 프레임에 여러 대상이 사용되는 것들은 임시로 서술자가 1개인 CBV 서술자 테이블을 사용함
	unsigned int objectConstantBufferRegister	= 0;
	unsigned int objectDescriptorsNum			= 1;
	CD3DX12_DESCRIPTOR_RANGE objectTable[1]		= {};
	objectTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		objectDescriptorsNum,
		objectConstantBufferRegister,
		0,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[SHADOW_OBJECT].InitAsDescriptorTable(1, objectTable, D3D12_SHADER_VISIBILITY_VERTEX);

	unsigned int lightConstantBufferRegister	= objectConstantBufferRegister + objectDescriptorsNum;
	unsigned int lightDescriptorsNum			= 1;
	CD3DX12_DESCRIPTOR_RANGE lightTable[1]		= {};
	lightTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		lightDescriptorsNum,
		lightConstantBufferRegister,
		0,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[SHADOW_LIGHT].InitAsDescriptorTable(1, lightTable, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc
	(
		SHADOW_ROOT_PARAMETER_COUNT,
		rootParameter,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature
	(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr)
	{
		OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
	}

	ThrowIfFailed(hr);

	ThrowIfFailed
	(
		m_device->CreateRootSignature
		(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(m_renderSceneConfigureShadowRootSignature.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetMainRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[MAIN_ROOT_PARAMETER_COUNT] = {};

	//NOTE: 하나의 프레임에 여러 대상이 사용되는 것들은 임시로 서술자가 1개인 SRV 서술자 테이블이나 CBV 서술자 테이블을 사용함
	unsigned int textureShaderResouceRegister		= 0;
	unsigned int textureShaderResourceRegisterSpace	= 0;
	unsigned int textureDescriptorsNum				= 1;
	CD3DX12_DESCRIPTOR_RANGE textureTable[1]		= {};
	textureTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		textureDescriptorsNum,
		textureShaderResouceRegister,
		textureShaderResourceRegisterSpace,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[MAIN_TEXTURE].InitAsDescriptorTable(1, textureTable, D3D12_SHADER_VISIBILITY_PIXEL);

	unsigned int materialConstantBufferRegister			= 0;
	unsigned int materialConstantBufferRegisterSpace	= 0;
	unsigned int materialDescriptorsNum					= 1;
	CD3DX12_DESCRIPTOR_RANGE materialTable[1]			= {};
	materialTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		materialDescriptorsNum,
		materialConstantBufferRegister,
		materialConstantBufferRegisterSpace,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[MAIN_MATERIAL].InitAsDescriptorTable(1, materialTable, D3D12_SHADER_VISIBILITY_PIXEL);

	unsigned int objectConstantBufferRegister		= materialConstantBufferRegister + materialDescriptorsNum;
	unsigned int objectConstantBufferRegisterSpace	= 0;
	unsigned int objectDescriptorsNum				= 1;
	CD3DX12_DESCRIPTOR_RANGE objectTable[1]			= {};
	objectTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		objectDescriptorsNum,
		objectConstantBufferRegister,
		objectConstantBufferRegisterSpace,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[MAIN_OBJECT].InitAsDescriptorTable(1, objectTable, D3D12_SHADER_VISIBILITY_VERTEX);
	
	//NOTE:	쉐도우와 라이트는 하나의 그리기 명령 안에서도 모든 대상이 필요하므로, 모든 대상을 묶을 수 있는 너비의 테이블을 이용함
	unsigned int shadowShaderResourceRegister		= 0;
	unsigned int shadowShaderResourceRegisterSpace	= 1;
	unsigned int shadowDescriptorsNum				= (static_cast<unsigned int>(m_workloadLights.size()) == 0) ? 1 : static_cast<unsigned int>(m_workloadLights.size());
	CD3DX12_DESCRIPTOR_RANGE shadowTable[1] = {};
	shadowTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		shadowDescriptorsNum,
		shadowShaderResourceRegister,
		shadowShaderResourceRegisterSpace,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[MAIN_SHADOW].InitAsDescriptorTable(1, shadowTable, D3D12_SHADER_VISIBILITY_PIXEL);

	unsigned int lightConstantBufferRegister		= 0;
	unsigned int lightConstantBufferRegisterSpace	= 1;
	unsigned int lightDescriptorsNum				= (static_cast<unsigned int>(m_workloadLights.size()) == 0) ? 1 : static_cast<unsigned int>(m_workloadLights.size());
	CD3DX12_DESCRIPTOR_RANGE lightTable[1]			= {};
	lightTable[0].Init
	(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		lightDescriptorsNum,
		lightConstantBufferRegister,
		lightConstantBufferRegisterSpace,
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	);
	rootParameter[MAIN_LIGHT].InitAsDescriptorTable(1, lightTable, D3D12_SHADER_VISIBILITY_PIXEL);

	//NOTE: 카메라는 하나의 프레임 렌더링에 하나의 대상만 쓰이므로, 임시로 루트 서술자를 사용함
	unsigned int cameraConstantBufferRegister		= objectConstantBufferRegister + objectDescriptorsNum;
	unsigned int cameraConstantBufferRegisterSpace	= 0;
	unsigned int cameraDescriptorsNum				= 1;
	rootParameter[MAIN_CAMERA].InitAsConstantBufferView
	(
		cameraConstantBufferRegister,
		cameraConstantBufferRegisterSpace,
		D3D12_SHADER_VISIBILITY_ALL
	);

	std::vector<CD3DX12_STATIC_SAMPLER_DESC> samplerDescs = {};

	CD3DX12_STATIC_SAMPLER_DESC anisotropicWrapSamplerDesc
	(
		0,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		4
	);
	samplerDescs.push_back(anisotropicWrapSamplerDesc);

	CD3DX12_STATIC_SAMPLER_DESC comparisonBorderSamplderDesc
	(
		1,
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		0.0f,
		1,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK
	);
	samplerDescs.push_back(comparisonBorderSamplderDesc);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc
	(
		MAIN_ROOT_PARAMETER_COUNT,
		rootParameter,
		static_cast<unsigned int>(samplerDescs.size()),
		samplerDescs.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature
	(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr)
	{
		OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
	}

	ThrowIfFailed(hr);

	ThrowIfFailed
	(
		m_device->CreateRootSignature
		(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(m_renderSceneConfigureMainRootSignature.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetPipelineStateObjects()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{"POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { inputElementDesc, _countof(inputElementDesc) };

	ComPtr<ID3DBlob> shadowVertexShader;
	ComPtr<ID3DBlob> shadowPixelShader;
	ThrowIfFailed(D3DReadFileToBlob(L"ShadowVertexShader.cso", &shadowVertexShader));
	ThrowIfFailed(D3DReadFileToBlob(L"ShadowPixelShader.cso", &shadowPixelShader));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateObjectDesc		= {};
	pipelineStateObjectDesc.InputLayout								= inputLayoutDesc;
	pipelineStateObjectDesc.pRootSignature							= m_renderSceneConfigureShadowRootSignature.Get();
	pipelineStateObjectDesc.VS										= CD3DX12_SHADER_BYTECODE(shadowVertexShader.Get());
	pipelineStateObjectDesc.PS										= CD3DX12_SHADER_BYTECODE(shadowPixelShader.Get());
	pipelineStateObjectDesc.RasterizerState							= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.RasterizerState.DepthBias				= 3000;
	pipelineStateObjectDesc.RasterizerState.SlopeScaledDepthBias	= 1.0f;
	pipelineStateObjectDesc.RasterizerState.DepthBiasClamp			= 0.0f;
	pipelineStateObjectDesc.BlendState								= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.DepthStencilState						= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.SampleMask								= UINT_MAX;
	pipelineStateObjectDesc.PrimitiveTopologyType					= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateObjectDesc.NumRenderTargets						= 0;
	pipelineStateObjectDesc.SampleDesc.Count						= 1;
	pipelineStateObjectDesc.SampleDesc.Quality						= 0;
	pipelineStateObjectDesc.DSVFormat								= DXGI_FORMAT_D32_FLOAT;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(m_renderSceneConfigureShadowPipelineStateObject.ReleaseAndGetAddressOf())));

	ComPtr<ID3DBlob> mainVertexShader;
	ComPtr<ID3DBlob> mainPixelShader;
	ThrowIfFailed(D3DReadFileToBlob(L"MainVertexShader.cso", &mainVertexShader));
	ThrowIfFailed(D3DReadFileToBlob(L"MainPixelShader.cso", &mainPixelShader));
	pipelineStateObjectDesc.pRootSignature							= m_renderSceneConfigureMainRootSignature.Get();
	pipelineStateObjectDesc.VS										= CD3DX12_SHADER_BYTECODE(mainVertexShader.Get());
	pipelineStateObjectDesc.PS										= CD3DX12_SHADER_BYTECODE(mainPixelShader.Get());
	pipelineStateObjectDesc.RasterizerState							= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);;
	pipelineStateObjectDesc.NumRenderTargets						= 1;
	pipelineStateObjectDesc.RTVFormats[0]							= m_optionHDR.IsActive() ? screenRTVFormatHDR : screenRTVFormatSDR;
	pipelineStateObjectDesc.DSVFormat								= screenDepthStencilBufferFormat;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(m_renderSceneConfigureMainPipelineStateObject.ReleaseAndGetAddressOf())));
}

void Podo::ResetImGui()
{
	CloseImGui();

	m_renderGUIConfigureImGuiDescriptorHeapAllocator.Create(m_device.Get(), m_descriptorHeapCBVSRVUAV.Get(), imGuiDescriptorCapacity);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device				= m_device.Get();
	initInfo.CommandQueue		= m_commandQueue.Get();
	initInfo.NumFramesInFlight	= 1;
	initInfo.RTVFormat			= m_optionHDR.IsActive() ? screenRTVFormatHDR : screenRTVFormatSDR;
	initInfo.UserData			= &m_renderGUIConfigureImGuiDescriptorHeapAllocator;
	initInfo.SrvDescriptorHeap	= m_descriptorHeapCBVSRVUAV.Get();
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* pInitInfo, D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
		{
			ImGuiDescriptorHeapAllocator* allocator = static_cast<ImGuiDescriptorHeapAllocator*>(pInitInfo->UserData);

			return allocator->Alloc(pOutCpuHandle, outGpuHandle);
		};
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* pInitInfo, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
		{
			ImGuiDescriptorHeapAllocator* allocator = static_cast<ImGuiDescriptorHeapAllocator*>(pInitInfo->UserData);

			return allocator->Free(cpuHandle, gpuHandle);
		};

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(m_hWnd);
	ImGui_ImplDX12_Init(&initInfo);

	ImGui::StyleColorsClassic();

	ImGuiStyle& style = ImGui::GetStyle();
	float imGuiScale = 2.0f * m_optionGUI.GetMasterScale();
	style.ScaleAllSizes(imGuiScale);
	style.FontScaleDpi = imGuiScale;

	ImVec4* pColors = style.Colors;
	pColors[ImGuiCol_Text]				= ImVec4(0.57f, 0.50f, 0.35f, 1.00f);
	pColors[ImGuiCol_TextDisabled]		= ImVec4(0.11f, 0.11f, 0.09f, 1.00f);
	pColors[ImGuiCol_FrameBg]			= ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
	pColors[ImGuiCol_FrameBgHovered]	= ImVec4(0.02f, 0.02f, 0.01f, 1.00f);
	pColors[ImGuiCol_FrameBgActive]		= ImVec4(0.05f, 0.04f, 0.01f, 1.00f);
	pColors[ImGuiCol_CheckMark]			= ImVec4(0.63f, 0.42f, 0.08f, 1.00f);
	pColors[ImGuiCol_SliderGrab]		= ImVec4(0.29f, 0.21f, 0.07f, 1.00f);
	pColors[ImGuiCol_SliderGrabActive]	= ImVec4(0.63f, 0.39f, 0.06f, 1.00f);
	pColors[ImGuiCol_WindowBg]			= ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	pColors[ImGuiCol_TitleBg]			= ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
	pColors[ImGuiCol_TitleBgActive]		= ImVec4(0.02f, 0.02f, 0.01f, 1.00f);
	pColors[ImGuiCol_TitleBgCollapsed]	= ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	pColors[ImGuiCol_Button]			= ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
	pColors[ImGuiCol_ButtonHovered]		= ImVec4(0.09f, 0.08f, 0.05f, 1.00f);
	pColors[ImGuiCol_ButtonActive]		= ImVec4(0.18f, 0.11f, 0.02f, 1.00f);
	pColors[ImGuiCol_Border]			= ImVec4(0.07f, 0.06f, 0.04f, 0.55f);
	pColors[ImGuiCol_Separator]			= ImVec4(0.09f, 0.07f, 0.05f, 0.65f);
	pColors[ImGuiCol_SeparatorHovered]	= ImVec4(0.17f, 0.11f, 0.09f, 0.78f);
	pColors[ImGuiCol_SeparatorActive]	= ImVec4(0.26f, 0.17f, 0.15f, 0.90f);

	m_renderGUIConfigureImGuiInitialized = true;
}