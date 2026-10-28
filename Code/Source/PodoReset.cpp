#define NOMINMAX

//Podo
#include "Podo.h"
#include "Option.h"
#include "Object.h"
#include "Asset.h"
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
#include <d3d12sdklayers.h>
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

		ResetAssets();
		ResetObjects();
		ResetCamera();

		ResetCBVSRVUAV();
	}

	if (NeedResetRenderConfigure() == true)
	{
		m_needResetRenderConfigure = false;

		//TODO: PSO 초기화와 GUI 초기화 요청을 분할하기
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
	m_device.Reset();

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
	m_commandList.Reset();

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
	descriptorHeapDesc.NumDescriptors = m_screenBackBufferCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;

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
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;

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
	UINT totalCount = m_descriptorHeapCBVSRVUAVCapacityForGUI + m_descriptorHeapCBVSRVUAVCapacityForRender;
	descriptorHeapDesc.NumDescriptors = totalCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;

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

	m_descriptorHeapCBVSRVUAVStartHandleCPUForRender = m_descriptorHeapCBVSRVUAVStartHandleCPUForGUI;
	m_descriptorHeapCBVSRVUAVStartHandleCPUForRender.Offset(m_descriptorHeapCBVSRVUAVCapacityForGUI, m_descriptorHeapCBVSRVUAVIncrementSize);
	m_descriptorHeapCBVSRVUAVStartHandleGPUForRender = m_descriptorHeapCBVSRVUAVStartHandleGPUForGUI;
	m_descriptorHeapCBVSRVUAVStartHandleGPUForRender.Offset(m_descriptorHeapCBVSRVUAVCapacityForGUI, m_descriptorHeapCBVSRVUAVIncrementSize);
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

void Podo::ResetAssets()
{
	m_workloadAssets.clear();

	std::vector<Vertex> boxVertices =
	{
		// -Z(Red)
		{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.5f, 0.0f, 0.0f}},
		{{-0.5f, +0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.5f, 0.0f, 0.0f}},
		{{+0.5f, +0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.5f, 0.0f, 0.0f}},
		{{+0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.5f, 0.0f, 0.0f}},

		// +Z(Red)
		{{-0.5f, -0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.5f, 0.0f, 0.0f}},
		{{-0.5f, +0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.5f, 0.0f, 0.0f}},
		{{+0.5f, +0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.5f, 0.0f, 0.0f}},
		{{+0.5f, -0.5f, +0.5f}, { 0.0f,  0.0f, +1.0f}, {0.5f, 0.0f, 0.0f}},

		// -X(Green)
		{{-0.5f, -0.5f, +0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{-0.5f, +0.5f, +0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{-0.5f, +0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},

		// +X(Green)
		{{+0.5f, -0.5f, -0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{+0.5f, +0.5f, -0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{+0.5f, +0.5f, +0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},
		{{+0.5f, -0.5f, +0.5f}, {+1.0f,  0.0f,  0.0f}, {0.0f, 0.5f, 0.0f}},

		// +Y(Blue)
		{{-0.5f, +0.5f, -0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{-0.5f, +0.5f, +0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{+0.5f, +0.5f, +0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{+0.5f, +0.5f, -0.5f}, { 0.0f, +1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},

		// -Y(Blue)
		{{-0.5f, -0.5f, +0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{+0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}},
		{{+0.5f, -0.5f, +0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f, 0.5f}}
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

	DirectX::ResourceUploadBatch resourceUpload(m_device.Get());
	resourceUpload.Begin();
	{
		Asset boxAsset;
		boxAsset.Create(m_device.Get(), resourceUpload, std::move(boxVertices), std::move(boxIndices));

		//NOTE: ComPtr<ID3D12Resource> 초기화는 곧바로 일어나므로, 바로 이렇게 move해도 됨
		m_workloadAssets["Box"] = std::move(boxAsset);
	}
	auto uploadFinished = resourceUpload.End(m_commandQueue.Get());

	//NOTE: 커맨드 큐에 제출된 자원 복사 작업은 이후 제출될 렌더 명령들과 순서가 지켜지기에 굳이 대기가 필요하지 않지만,
	//		End(..)가 반환하는 std::future에 의해 어쩔 수 없이 대기가 발생함을 코드로 표기해놓기로 함
	uploadFinished.wait();
}

void Podo::ResetObjects()
{
	m_workloadObjects.clear();

	{
		Object boxObject;
		boxObject.Create(m_device.Get(), &m_workloadAssets.at("Box"));

		DirectX::XMVECTOR scale			= DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= DirectX::XMVectorSet(0.0f, 0.0f, 3.0f, 1.0f);
		boxObject.SetScale(scale);
		boxObject.SetRotation(lookDirection, upDirection);
		boxObject.SetPosition(position);
		boxObject.UpdateObjectConstantBuffer();

		m_workloadObjects["HorizontalBoxObject"] = std::move(boxObject);
	}

	{
		Object boxObject;
		boxObject.Create(m_device.Get(), &m_workloadAssets.at("Box"));

		DirectX::XMVECTOR scale			= DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= DirectX::XMVectorSet(2.0f, 0.0f, 5.0f, 1.0f);
		boxObject.SetScale(scale);
		boxObject.SetRotation(lookDirection, upDirection);
		boxObject.SetPosition(position);
		boxObject.UpdateObjectConstantBuffer();

		m_workloadObjects["VerticalBoxObject"] = std::move(boxObject);
	}
}

void Podo::ResetCamera()
{
	m_workloadCamera.Create(m_device.Get());
}

void Podo::ResetCBVSRVUAV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCBVSRVUAVStartHandleCPUForRender;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeapCBVSRVUAVStartHandleGPUForRender;

	for (auto& [name, object] : m_workloadObjects)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation	= object.GetObjectConstantBufferGPUAddress();
		cbvDesc.SizeInBytes		= object.GetObjectConstantBufferWidth();

		m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		object.SetObjectConstantBufferViewGPUHandle(gpuHandle);

		cpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
		gpuHandle.Offset(1, m_descriptorHeapCBVSRVUAVIncrementSize);
	}
}

void Podo::ResetRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[ROOT_PARAMETER_COUNT] = {};

	CD3DX12_DESCRIPTOR_RANGE objectConstantTable[1] = {};
	UINT objectDescriptorsNum		= 1;
	UINT objectBaseShaderRegister	= 0;
	objectConstantTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, objectDescriptorsNum, objectBaseShaderRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	rootParameter[OBJECT_CONSTANT].InitAsDescriptorTable(1, objectConstantTable);

	UINT cameraShaderRegister = objectBaseShaderRegister + objectDescriptorsNum;
	rootParameter[CAMERA_CONSTANT].InitAsConstantBufferView(cameraShaderRegister, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc
	(
		ROOT_PARAMETER_COUNT,
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
			IID_PPV_ARGS(m_renderConfigureRootSignature.ReleaseAndGetAddressOf())
		)
	);
}

void Podo::ResetPipelineStateObject()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		
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

	m_imGuiDescriptorHeapAllocator.Create(m_device.Get(), m_descriptorHeapCBVSRVUAV.Get(), m_descriptorHeapCBVSRVUAVCapacityForGUI);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device				= m_device.Get();
	initInfo.CommandQueue		= m_commandQueue.Get();
	initInfo.NumFramesInFlight	= 1;
	initInfo.RTVFormat			= m_optionHDR.IsActive() ? m_screenRTVFormatHDR : m_screenRTVFormatSDR;
	initInfo.SrvDescriptorHeap = m_descriptorHeapCBVSRVUAV.Get();
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
		{
			return m_imGuiDescriptorHeapAllocator.Alloc(pOutCpuHandle, outGpuHandle);
		};
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
		{
			return m_imGuiDescriptorHeapAllocator.Free(cpuHandle, gpuHandle);
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

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text]				= ImVec4(0.570482f, 0.507079f, 0.354692f, 1.0f);
	colors[ImGuiCol_TextDisabled]		= ImVec4(0.119280f, 0.119280f, 0.094630f, 1.0f);
	colors[ImGuiCol_FrameBg]			= ImVec4(0.010816f, 0.011645f, 0.010023f, 1.0f);
	colors[ImGuiCol_FrameBgHovered]		= ImVec4(0.028622f, 0.025843f, 0.015325f, 1.0f);
	colors[ImGuiCol_FrameBgActive]		= ImVec4(0.054972f, 0.043234f, 0.017389f, 1.0f);
	colors[ImGuiCol_CheckMark]			= ImVec4(0.638283f, 0.420033f, 0.083535f, 1.0f);
	colors[ImGuiCol_SliderGrab]			= ImVec4(0.295700f, 0.214041f, 0.073239f, 1.0f);
	colors[ImGuiCol_SliderGrabActive]	= ImVec4(0.638283f, 0.393123f, 0.063724f, 1.0f);
	colors[ImGuiCol_WindowBg]			= ImVec4(0.006571f, 0.006941f, 0.006571f, 1.0f);
	colors[ImGuiCol_TitleBg]			= ImVec4(0.012510f, 0.012510f, 0.010816f, 1.0f);
	colors[ImGuiCol_TitleBgActive]		= ImVec4(0.027212f, 0.024515f, 0.018478f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed]	= ImVec4(0.004896f, 0.005103f, 0.004896f, 1.0f);
	colors[ImGuiCol_Button]				= ImVec4(0.036306f, 0.037972f, 0.033105f, 1.0f);
	colors[ImGuiCol_ButtonHovered]		= ImVec4(0.094630f, 0.083535f, 0.050876f, 1.0f);
	colors[ImGuiCol_ButtonActive]		= ImVec4(0.187317f, 0.119280f, 0.027212f, 1.0f);
	colors[ImGuiCol_Border]				= ImVec4(0.073239f, 0.063724f, 0.043234f, 0.55f);
	colors[ImGuiCol_Separator]			= ImVec4(0.094630f, 0.078288f, 0.050876f, 0.65f);
	colors[ImGuiCol_SeparatorHovered]	= ImVec4(0.170645f, 0.119280f, 0.094630f, 0.78f);
	colors[ImGuiCol_SeparatorActive]	= ImVec4(0.263273f, 0.170645f, 0.154872f, 0.90f);

	m_renderConfigureImGuiInitialized = true;
}