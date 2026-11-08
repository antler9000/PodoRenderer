#define NOMINMAX

//Podo
#include "Podo.h"
#include "Option.h"
#include "Camera.h"
#include "Light.h"
#include "Object.h"
#include "Material.h"
#include "Texture.h"
#include "Mesh.h"
#include "Root.h"
#include "Alloc.h"
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
#include <stdexcept>

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

		//TODO: 어댑터가 안 바뀌었으면 요청 생략하도록 곤치기
		m_needResetDevice		= true;
		//TODO: 아웃풋이 안 바뀌었으면 요청 생략하도록 곤치기
		m_needResetSwapChain	= true;
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

		ResetDescriptorHeapRTV();
		ResetDescriptorHeapDSV();
		ResetDescriptorHeapCBVSRVUAV();

		m_needResetSwapChain		= true;
		m_needResetWorkload			= true;
		m_needResetRenderConfigure	= true;
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

		ResetRTV();
		ResetDSV();

		//TODO: RTV 포맷이 안 바뀌었으면 요청 생략하도록 곤치기
		m_needResetRenderConfigure	= true;
	}

	if (NeedResetWorkload() == true)
	{
		m_needResetWorkload = false;

		ResetMeshes();
		ResetTextures();
		ResetMaterials();
		ResetObjects();
		ResetLights();
		ResetCameras();

		ResetCBVSRVUAV();
	}

	if (NeedResetRenderConfigure() == true)
	{
		m_needResetRenderConfigure = false;

		//TODO: Scene 렌더 설정 초기화와 GUI 렌더 설정 초기화 요청을 분할하기
		ResetRootSignature();
		ResetPipelineStateObject();
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
	UINT factoryFlags = 0;

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

		if (SUCCEEDED(result) == true)
		{
			if (ResetOutput(tempAdapter.Get()) == true)
			{
				m_dxgiAdapter = tempAdapter;

				return;
			}
		}
	}

	throw std::runtime_error("can't find pAdapter that connected with most intersecting output");
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
			m_screenBackBufferFormatSDR,
			D3D12_FORMAT_SUPPORT1_NONE,
			D3D12_FORMAT_SUPPORT2_NONE
		};
		D3D12_FEATURE_DATA_FORMAT_SUPPORT backBufferFormatHDRQuery =
		{
			m_screenBackBufferFormatHDR,
			D3D12_FORMAT_SUPPORT1_NONE,
			D3D12_FORMAT_SUPPORT2_NONE
		};
		D3D12_FEATURE_DATA_FORMAT_SUPPORT depthStencilFormatQuery =
		{
			m_screenDepthStencilBufferFormat,
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


void Podo::ResetDescriptorHeapRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= m_screenBackBufferCount;
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
	m_descriptorHeapRTVStartHandleCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
}

void Podo::ResetDescriptorHeapDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= 1;
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
	m_descriptorHeapDSVStartHandleCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart());
}

void Podo::ResetDescriptorHeapCBVSRVUAV()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors	= m_descriptorHeapCBVSRVUAVCapacityForGUI + m_descriptorHeapCBVSRVUAVCapacityForRender;
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

	//NOTE: ImGui가 SRV를 둘 곳을 고정적으로 남겨두고, 그 뒷부분부터 사용하기로 함
	m_descriptorHeapCBVSRVUAVIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorHeapCBVSRVUAVStartHandleCPUForGUI = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart());
	m_descriptorHeapCBVSRVUAVStartHandleGPUForGUI = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());

	m_descriptorHeapCBVSRVUAVStartHandleCPUForScene = m_descriptorHeapCBVSRVUAVStartHandleCPUForGUI;
	m_descriptorHeapCBVSRVUAVStartHandleCPUForScene.Offset(m_descriptorHeapCBVSRVUAVCapacityForGUI, m_descriptorHeapCBVSRVUAVIncrementSize);
	m_descriptorHeapCBVSRVUAVStartHandleGPUForScene = m_descriptorHeapCBVSRVUAVStartHandleGPUForGUI;
	m_descriptorHeapCBVSRVUAVStartHandleGPUForScene.Offset(m_descriptorHeapCBVSRVUAVCapacityForGUI, m_descriptorHeapCBVSRVUAVIncrementSize);
}

void Podo::ResetHDRSwapChainSupport()
{
	for (UINT i = 0; i < m_screenBackBufferCount; i++)
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
	swapChainDesc.Format				= m_screenBackBufferFormatHDR;
	swapChainDesc.Stereo				= false;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.SampleDesc.Quality	= 0;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= m_screenBackBufferCount;
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

	UINT colorSpaceHDRQuery = 0;
	HRESULT queryResult = tempSwapChain3->CheckColorSpaceSupport(m_screenBackBufferColorSpaceHDR, &colorSpaceHDRQuery);
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
	for (UINT i = 0; i < m_screenBackBufferCount; i++)
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
	swapChainDesc.Format				= m_optionHDR.IsActive() ? m_screenBackBufferFormatHDR : m_screenBackBufferFormatSDR;
	swapChainDesc.Stereo				= false;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.SampleDesc.Quality	= 0;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount			= m_screenBackBufferCount;
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
		m_screenSwapChain->SetColorSpace1(m_screenBackBufferColorSpaceHDR);
	}
	else
	{
		m_screenSwapChain->SetColorSpace1(m_screenBackBufferColorSpaceSDR);
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

	for (UINT i = 0; i < m_screenBackBufferCount; i++)
	{
		ThrowIfFailed(m_screenSwapChain->GetBuffer(i, IID_PPV_ARGS(m_screenBackBuffers[i].ReleaseAndGetAddressOf())));
	}
}

void Podo::ResetViewPort()
{
	m_screenViewPort.TopLeftX	= 0.0f;
	m_screenViewPort.TopLeftY	= 0.0f;
	m_screenViewPort.Width		= FLOAT(m_screenBackBufferWidth);
	m_screenViewPort.Height		= FLOAT(m_screenBackBufferHeight);
	m_screenViewPort.MinDepth	= 0.0f;
	m_screenViewPort.MaxDepth	= 1.0f;
}

void Podo::ResetScissorRectangle()
{
	m_screenScissorRectangle.left	= 0;
	m_screenScissorRectangle.top	= 0;
	m_screenScissorRectangle.right	= m_screenBackBufferWidth;
	m_screenScissorRectangle.bottom	= m_screenBackBufferHeight;
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
	depthStencilBufferDesc.Format				= m_screenDepthStencilBufferFormat;
	depthStencilBufferDesc.SampleDesc.Count		= 1;
	depthStencilBufferDesc.SampleDesc.Quality	= 0;
	depthStencilBufferDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilBufferDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format				= m_screenDepthStencilBufferFormat;
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

void Podo::ResetRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartHandleCPU;
	for (UINT i = 0; i < m_screenBackBufferCount; i++)
	{
		D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
		renderTargetViewDesc.Format					= m_optionHDR.IsActive() ? m_screenRTVFormatHDR : m_screenRTVFormatSDR;
		renderTargetViewDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice		= 0;
		renderTargetViewDesc.Texture2D.PlaneSlice	= 0;

		m_device->CreateRenderTargetView(m_screenBackBuffers[i].Get(), &renderTargetViewDesc, cpuHandleRTV.Offset(i, m_descriptorHeapRTVIncrementSize));
	}
}

void Podo::ResetDSV()
{
	m_device->CreateDepthStencilView(m_screenDepthStencilBuffer.Get(), nullptr, m_descriptorHeapDSVStartHandleCPU);
}

void Podo::ResetMeshes()
{
	m_workloadMeshes.clear();

	std::vector<Vertex> boxVertices =
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

	std::vector<uint32_t> boxIndices =
	{
		0,	1,	2,		0,	2,	3,	// -Z
		4,	6,	5,		4,	7,  6,	// +Z
		8,	9,	10,		8,	10,	11,	// -X
		12,	13,	14,		12,	14,	15,	// +X
		16,	17,	18,		16,	18,	19,	// +Y
		20,	21,	22,		20,	22,	23	// -Y
	};

	//NOTE: 업로드 힙을 거쳐야 하기에 번거로운 디폴트 힙 자원 생성은 DirectXTK12의 ResourceUploadBatch 헬퍼를 이용하기로 함
	//		다만 uploadFinished이 소멸될 때마다 불필요한 CPU 대기가 발생한다는 문제점이 있음
	DirectX::ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();
	{
		{
			Mesh boxMesh;
			boxMesh.Create(m_device.Get(), resourceUpload, std::move(boxVertices), std::move(boxIndices));

			//NOTE: 실제 자원의 업로드 및 복사는 일어나지 않았지만, boxMesh의 초기화는 곧바로 일어나므로 바로 이렇게 move해도 됨
			m_workloadMeshes["Box"] = std::move(boxMesh);
		}
	}
	auto uploadFinished = resourceUpload.End(m_commandQueue.Get());

	uploadFinished.wait();
}

void Podo::ResetTextures()
{
	m_workloadTextures.clear();

	DirectX::ResourceUploadBatch resourceUpload(m_device.Get());
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
	}
	auto uploadFinished = resourceUpload.End(m_commandQueue.Get());

	uploadFinished.wait();
}

void Podo::ResetMaterials()
{
	m_workloadMaterials.clear();

	{
		Material steelMaterial;
		steelMaterial.Create(m_device.Get(), &m_workloadTextures["Steel"]);

		DirectX::XMFLOAT3	diffuseAlbedo	= { 0.90f, 0.90f, 0.90f };
		DirectX::XMFLOAT3	fresnelR0		= { 0.35f, 0.35f, 0.35f };
		float				shininess		= 0.45f;
		steelMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		steelMaterial.SetFresnelR0(fresnelR0);
		steelMaterial.SetShininess(shininess);
		steelMaterial.UpdateBuffer();

		m_workloadMaterials["Steel"] = std::move(steelMaterial);
	}

	{
		Material woodMaterial;
		woodMaterial.Create(m_device.Get(), &m_workloadTextures["Wood"]);

		DirectX::XMFLOAT3	diffuseAlbedo	= { 0.78f, 0.72f, 0.42f };
		DirectX::XMFLOAT3	fresnelR0		= { 0.05f, 0.05f, 0.05f };
		float				shininess		= 0.12f;
		woodMaterial.SetDiffuseAlbedo(diffuseAlbedo);
		woodMaterial.SetFresnelR0(fresnelR0);
		woodMaterial.SetShininess(shininess);
		woodMaterial.UpdateBuffer();

		m_workloadMaterials["Wood"] = std::move(woodMaterial);
	}
}

void Podo::ResetObjects()
{
	m_workloadObjects.clear();

	{
		Object boxObject;
		boxObject.Create(m_device.Get(), &m_workloadMeshes.at("Box"), &m_workloadMaterials.at("Steel"));

		DirectX::XMFLOAT3 scale			= DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= DirectX::XMVectorSet(0.0f, 0.0f, 3.0f, 1.0f);
		boxObject.SetScale(scale);
		boxObject.SetRotation(lookDirection, upDirection);
		boxObject.SetPosition(position);
		boxObject.UpdateBuffer();

		m_workloadObjects["HorizontalBoxObject"] = std::move(boxObject);
	}

	{
		Object boxObject;
		boxObject.Create(m_device.Get(), &m_workloadMeshes.at("Box"), &m_workloadMaterials.at("Wood"));

		DirectX::XMFLOAT3 scale			= DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= DirectX::XMVectorSet(2.0f, 0.0f, 5.0f, 1.0f);
		boxObject.SetScale(scale);
		boxObject.SetRotation(lookDirection, upDirection);
		boxObject.SetPosition(position);
		boxObject.UpdateBuffer();

		m_workloadObjects["VerticalBoxObject"] = std::move(boxObject);
	}
}

void Podo::ResetLights()
{
	m_workloadLights.clear();

	{
		Light mainLight;
		mainLight.Create(m_device.Get());

		DirectionalLight sun		= {};
		sun.strength				= { 1.0f,  0.9f, 0.9f };
		sun.direction				= { 1.0f, -0.3f, 0.0f };
		DirectX::XMFLOAT3 ambient	= { 0.1f,  0.1f, 0.1f };
		mainLight.SetDirectionalLight("Sun", sun);
		mainLight.SetAmbientLight(ambient);
		mainLight.UpdateBuffer();

		m_workloadLights["MainLight"] = std::move(mainLight);
	}
}

void Podo::ResetCameras()
{
	m_workloadCameras.clear();

	{
		Camera mainCamera;
		mainCamera.Create(m_device.Get());

		DirectX::XMVECTOR position		= DirectX::XMVectorSet(0.0f, 5.0f, -5.0f, 1.0f);
		DirectX::XMVECTOR target		= DirectX::XMVectorSet(0.0f, 0.0f,  0.0f, 1.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f,  0.0f, 1.0f);
		mainCamera.SetPosition(position);
		mainCamera.SetTarget(target);
		mainCamera.SetUpDirection(upDirection);
		mainCamera.UpdateBuffer(m_screenBackBufferAspectRatio);

		m_workloadCameras["MainCamera"] = std::move(mainCamera);
	}
}

void Podo::ResetCBVSRVUAV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCBVSRVUAVStartHandleCPUForScene;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeapCBVSRVUAVStartHandleGPUForScene;

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

		m_device->CreateShaderResourceView(texture.GetTexture(), &srvDesc, cpuHandle);

		texture.SetShaderResourceViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	for (auto& [name, material] : m_workloadMaterials)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= material.GetBufferGPUAddress();
		cbvDesc.SizeInBytes		= material.GetBufferWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		material.SetConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}

	for (auto& [name, object] : m_workloadObjects)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= object.GetBufferGPUAddress();
		cbvDesc.SizeInBytes		= object.GetBufferWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		object.SetConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}
}

void Podo::ResetRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[ROOT_PARAMETER_COUNT] = {};

	//NOTE: 임시로 SRV 서술자 테이블 매개변수를 텍스처 자원 바인딩에 사용함
	UINT textureShaderResourceRegister	= 0;
	UINT textureDescriptorsNum			= 1;
	CD3DX12_DESCRIPTOR_RANGE textureTable[1] = {};
	textureTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureDescriptorsNum, textureShaderResourceRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	rootParameter[TEXTURE_CONSTANT].InitAsDescriptorTable(1, textureTable, D3D12_SHADER_VISIBILITY_PIXEL);

	//NOTE: 임시로 CBV 서술자 테이블 매개변수를 머테리얼과 오브젝트 자원 바인딩에 사용함
	UINT materialConstantBufferRegister	= 0;
	UINT materialDescriptorsNum			= 1;
	CD3DX12_DESCRIPTOR_RANGE materialConstantTable[1] = {};
	materialConstantTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, materialDescriptorsNum, materialConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	rootParameter[MATERIAL_CONSTANT].InitAsDescriptorTable(1, materialConstantTable, D3D12_SHADER_VISIBILITY_PIXEL);

	UINT objectConstantBufferRegister	= materialConstantBufferRegister + materialDescriptorsNum;
	UINT objectDescriptorsNum			= 1;
	CD3DX12_DESCRIPTOR_RANGE objectConstantTable[1] = {};
	objectConstantTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, objectDescriptorsNum, objectConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	rootParameter[OBJECT_CONSTANT].InitAsDescriptorTable(1, objectConstantTable, D3D12_SHADER_VISIBILITY_VERTEX);
	
	//NOTE: 라이트와 카메라는 하나의 프레임 렌더링에 하나의 자원 인스턴스만 쓰이므로, 임시로 루트 서술자를 사용함
	UINT lightConstantBufferRegister	= objectConstantBufferRegister + objectDescriptorsNum;
	UINT lightDescriptorsNum			= 1;
	rootParameter[LIGHT_CONSTANT].InitAsConstantBufferView(lightConstantBufferRegister, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	UINT cameraConstantBufferRegister	= lightConstantBufferRegister + lightDescriptorsNum;
	UINT cameraDescriptorsNum			= 1;
	rootParameter[CAMERA_CONSTANT].InitAsConstantBufferView(cameraConstantBufferRegister, 0, D3D12_SHADER_VISIBILITY_ALL);

	//NOTE: 샘플러 힙을 구성할 필요가 없는 루트 서명의 정적 샘플러를 편의 목적으로 이용하기로 함
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> samplerDescs = {};

	const CD3DX12_STATIC_SAMPLER_DESC pointWrapSamplerDesc
	(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);
	samplerDescs.push_back(pointWrapSamplerDesc);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrapSamplerDesc
	(
		1,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);
	samplerDescs.push_back(linearWrapSamplerDesc);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrapSamplerDesc
	(
		2,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		4
	);
	samplerDescs.push_back(anisotropicWrapSamplerDesc);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc
	(
		ROOT_PARAMETER_COUNT,
		rootParameter,
		static_cast<UINT>(samplerDescs.size()),
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
			IID_PPV_ARGS(m_renderConfigureRootSignature.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetPipelineStateObject()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{"POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { inputElementDesc, _countof(inputElementDesc) };

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShader));
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShader));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateObjectDesc	= {};
	pipelineStateObjectDesc.InputLayout							= inputLayoutDesc;
	pipelineStateObjectDesc.pRootSignature						= m_renderConfigureRootSignature.Get();
	pipelineStateObjectDesc.VS									= CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	pipelineStateObjectDesc.PS									= CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	pipelineStateObjectDesc.RasterizerState						= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.BlendState							= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.DepthStencilState					= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.SampleMask							= UINT_MAX;
	pipelineStateObjectDesc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateObjectDesc.NumRenderTargets					= 1;
	pipelineStateObjectDesc.RTVFormats[0]						= m_optionHDR.IsActive() ? m_screenRTVFormatHDR : m_screenRTVFormatSDR;
	pipelineStateObjectDesc.SampleDesc.Count					= 1;
	pipelineStateObjectDesc.SampleDesc.Quality					= 0;
	pipelineStateObjectDesc.DSVFormat							= m_screenDepthStencilBufferFormat;
	
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(m_renderConfigurePipelineStateObject.ReleaseAndGetAddressOf())));
}

void Podo::ResetImGui()
{
	CloseImGui();

	m_renderConfigureImGuiDescriptorHeapAllocator.Create(m_device.Get(), m_descriptorHeapCBVSRVUAV.Get(), m_descriptorHeapCBVSRVUAVCapacityForGUI);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device				= m_device.Get();
	initInfo.CommandQueue		= m_commandQueue.Get();
	initInfo.NumFramesInFlight	= 1;
	initInfo.RTVFormat			= m_optionHDR.IsActive() ? m_screenRTVFormatHDR : m_screenRTVFormatSDR;
	initInfo.UserData			= &m_renderConfigureImGuiDescriptorHeapAllocator;
	initInfo.SrvDescriptorHeap = m_descriptorHeapCBVSRVUAV.Get();
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

	m_renderConfigureImGuiInitialized = true;
}