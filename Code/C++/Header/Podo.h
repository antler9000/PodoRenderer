#pragma once
#define NOMINMAX

//Podo
#include "BaseApp.h"
#include "Option.h"
#include "State.h"
#include "Timer.h"
#include "Camera.h"
#include "Light.h"
#include "Object.h"
#include "Asset.h"
#include "Material.h"
#include "Texture.h"
#include "Mesh.h"
#include "Alloc.h"
#include "Changed.h"

//ImGui
#include "imgui.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <d3dx12_root_signature.h>
#include <d3d12.h>
#include <DirectXMath.h>

//DXGI
#include <dxgi1_6.h>
#include <dxgi1_4.h>
#include <dxgi.h>
#include <dxgicommon.h>
#include <dxgiformat.h>

//C++
#include <map>
#include <string>
#include <fstream>
#include <future>

class Podo : public BaseApp<Podo>
{
public:

	Podo(HINSTANCE hInstance, int nCmdShow) : BaseApp(L"Podo Renderer", hInstance, nCmdShow)
	{
		OptionRestore();

		Reset();
	}

	~Podo()
	{
		OptionSave();
		
		Close();
	}

	int RunMessageLoop()
	{
		MSG msg = {};

		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != FALSE)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (NeedReset() == true)
			{
				Reset();
			}
			else
			{
				if (IsUpdateStopped() == true)
				{
					Sleep(100);
				}
				else
				{
					Update();
				}
			}
		}

		return (int)msg.wParam;
	}

	//NOTE: 이 메서드는 BaseApp에 작석된 WindowProcedure 정적 메서드에서 호출함
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

	void Reset();
	void ResetScreenMode();
	void ResetFullScreenMode();
	void ResetWindowMode();
	void ResetFactory();
	void ResetAdapterAndOutput();
	bool ResetOutput(IDXGIAdapter3* pAdapter);
	void ResetDevice();
	void ResetFence();
	void ResetFenceEvent();
	void ResetCommandQueue();
	void ResetCommandAllocator();
	void ResetCommandList();
	void ResetHDRSwapChainSupport();
	void ResetSwapChain();
	void ResetBackBufferInfo();
	void ResetViewPort();
	void ResetScissorRectangle();
	void ResetDepthStencilBuffer();
	void ResetMeshes();
	void ResetTextures();
	void ResetMaterials();
	void ResetAssets();
	void ResetObjects();
	void ResetLights();
	void ResetCameras();
	void ResetDescriptorHeapRTV();
	void ResetDescriptorHeapDSV();
	void ResetDescriptorHeapCBVSRVUAV();
	void ResetRTV();
	void ResetDSV();
	void ResetCBVSRVUAV();
	void ResetShadowRootSignature();
	void ResetMainRootSignature();
	void ResetPipelineStateObjects();
	void ResetImGui();

	void Close();
	void CloseFenceEvent();
	void CloseImGui();

	void Update();
	void UpdateTimers();
	void UpdateCaption();
	void UpdateWorld();
	void UpdateRender();
	void UpdateScene();
	void UpdateShadowScene();
	void UpdateMainScene();
	void UpdateGUI();
	void UpdatePrepareStateGUI(ImGuiViewport* pImGuiViewPort, ImVec2 imGuiCenterPos);
	void UpdateRunStateGUI(ImGuiViewport* pImGuiViewPort, ImVec2 imGuiCenterPos);

private:

	void FlushCommandQueue();
	void OptionSave();
	void OptionRestore();
	bool OptionReadBool(std::ifstream& fin, std::string optionName, bool& outOptionEnabled);
	bool OptionReadInt(std::ifstream& fin, std::string optionName, int& outOptionValue);

private:

	static constexpr	unsigned int					screenBackBufferCount						= 2;
	static constexpr	DXGI_FORMAT						screenBackBufferFormatSDR					= DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr	DXGI_FORMAT						screenBackBufferFormatHDR					= DXGI_FORMAT_R10G10B10A2_UNORM;
	static constexpr	DXGI_FORMAT						screenRTVFormatSDR							= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	static constexpr	DXGI_FORMAT						screenRTVFormatHDR							= DXGI_FORMAT_R10G10B10A2_UNORM;
	static constexpr	DXGI_COLOR_SPACE_TYPE			screenBackBufferColorSpaceSDR				= DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	static constexpr	DXGI_COLOR_SPACE_TYPE			screenBackBufferColorSpaceHDR				= DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
	static constexpr	DXGI_FORMAT						screenDepthStencilBufferFormat				= DXGI_FORMAT_D24_UNORM_S8_UINT;

	static constexpr	float							cameraHorizontalFOV							= DirectX::XM_PIDIV4;
	static constexpr	float							cameraNearZ									= 0.1f;
	static constexpr	float							cameraFarZ									= 1000.0f;

	static constexpr	unsigned int					imGuiDescriptorCapacity						= 64;
	static constexpr	ImVec2							imGuiSpacingBasicSize						= ImVec2(0.0f, 10.0f);
	static constexpr	ImVec2							imGuiSmallButtonBasicSize					= ImVec2(120.0f, 40.0f);
	static constexpr	ImVec2							imGuiMediumButtonBasicSize					= ImVec2(240.0f, 40.0f);
	static constexpr	ImVec2							imGuiLargeButtonBasicSize					= ImVec2(360.0f, 40.0f);
	static constexpr	ImGuiWindowFlags				imGuiBasicFlag								= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
																									| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

private:

	template <typename Interface>
	using ComPtr = Microsoft::WRL::ComPtr<Interface>;

	ComPtr<IDXGIFactory6>						m_dxgiFactory										= {};	//NOTE: (기본) 성능순 어댑터 획득
	ComPtr<IDXGIAdapter3>						m_dxgiAdapter										= {};	//NOTE: (기본) 자원의 메모리 상주성 관리
	ComPtr<IDXGIOutput>							m_dxgiOutput										= {};
	ComPtr<IDXGIOutput6>						m_dxgiOutput6										= {};	//NOTE: (옵션) HDR 모니터 정보 획득
	DXGI_OUTPUT_DESC1							m_dxgiOutputDesc									= {};	//NOTE: (옵션) HDR 모니터 정보 획득
	
	ComPtr<ID3D12Device>						m_device											= {};
	
	ComPtr<ID3D12Fence>							m_fence												= {};
	UINT64										m_fenceCurrent										= {};
	HANDLE										m_fenceEvent										= {};
	
	ComPtr<ID3D12CommandQueue>					m_commandQueue										= {};
	ComPtr<ID3D12CommandAllocator>				m_commandAllocator									= {};
	ComPtr<ID3D12GraphicsCommandList>			m_commandList										= {};
	
	ComPtr<IDXGISwapChain3>						m_screenSwapChain									= {};	//NOTE: (기본) 백 버퍼 인덱스 추적
	ComPtr<ID3D12Resource>						m_screenBackBuffers[screenBackBufferCount]			= {};
	unsigned int								m_screenBackBufferIndex								= {};
	unsigned int								m_screenBackBufferWidth								= {};
	unsigned int								m_screenBackBufferHeight							= {};
	float										m_screenBackBufferAspectRatio						= {};
	D3D12_VIEWPORT								m_screenViewport									= {};
	D3D12_RECT									m_screenScissorRect									= {};
	ComPtr<ID3D12Resource>						m_screenDepthStencilBuffer							= {};
	
	std::map<std::string, Mesh>					m_workloadMeshes									= {};
	std::map<std::string, Texture>				m_workloadTextures									= {};
	std::map<std::string, Material>				m_workloadMaterials									= {};
	std::map<std::string, Asset>				m_workloadAssets									= {};
	std::map<std::string, Object>				m_workloadObjects									= {};
	std::map<std::string, Light>				m_workloadLights									= {};
	std::map<std::string, Camera>				m_workloadCameras									= {};
	
	std::future<void>							m_meshResourceUpload								= {};
	std::future<void>							m_textureResourceUpload								= {};

	ComPtr<ID3D12DescriptorHeap>				m_descriptorHeapRTV									= {};
	ComPtr<ID3D12DescriptorHeap>				m_descriptorHeapDSV									= {};
	ComPtr<ID3D12DescriptorHeap>				m_descriptorHeapCBVSRVUAV							= {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE				m_descriptorHeapRTVStartCPUHandle					= {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE				m_descriptorHeapDSVStartCPUHandle					= {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartCPUHandleForGUI		= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForGUI		= {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartCPUHandleForScene		= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForScene		= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForTextures	= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForMaterials	= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForObjects	= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForShadows	= {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				m_descriptorHeapCBVSRVUAVStartGPUHandleForLights	= {};
	unsigned int								m_descriptorHeapRTVIncrementSize					= {};
	unsigned int								m_descriptorHeapDSVIncrementSize					= {};
	unsigned int								m_descriptorHeapCBVSRVUAVIncrementSize				= {};

	ComPtr<ID3D12RootSignature>					m_renderSceneConfigureShadowRootSignature			= {};
	ComPtr<ID3D12RootSignature>					m_renderSceneConfigureMainRootSignature				= {};
	ComPtr<ID3D12PipelineState>					m_renderSceneConfigureShadowPipelineStateObject		= {};
	ComPtr<ID3D12PipelineState>					m_renderSceneConfigureMainPipelineStateObject		= {};

	bool										m_renderGUIConfigureImGuiInitialized				= {};
	ImGuiDescriptorHeapAllocator				m_renderGUIConfigureImGuiDescriptorHeapAllocator	= {};

	IsAdapterChanged							m_isAdapterChanged									= {};
	IsOutputChanged								m_isOutputChanged									= {};
	IsRTVFormatChanged							m_isRTVFormatChanged								= {};

	bool										NeedReset() const									{ return (NeedResetScreenMode() || NeedResetFactory() || NeedResetDevice() || NeedResetSwapChain() || NeedResetWorkload() || NeedResetRenderSceneConfigure()) || NeedResetRenderGUIConfigure(); }
	bool										NeedResetScreenMode() const							{ return m_needResetScreenMode; }
	bool										m_needResetScreenMode								= true;
	bool										NeedResetFactory() const							{ return (m_needResetFactory || (m_dxgiFactory->IsCurrent() == FALSE)); }
	bool										m_needResetFactory									= true;
	bool										NeedResetDevice() const								{ return m_needResetDevice; }
	bool										m_needResetDevice									= true;
	bool										NeedResetSwapChain() const							{ return m_needResetSwapChain; }
	bool										m_needResetSwapChain								= true;
	bool										NeedResetWorkload() const							{ return m_needResetWorkloads; }
	bool										m_needResetWorkloads									= true;
	bool										NeedResetDescriptor() const							{ return m_needResetDescriptors; };
	bool										m_needResetDescriptors								= true;
	bool										NeedResetRenderSceneConfigure() const				{ return m_needResetRenderSceneConfigure; }
	bool										m_needResetRenderSceneConfigure						= true;
	bool										NeedResetRenderGUIConfigure() const					{ return m_needResetRenderGUIConfigure; }
	bool										m_needResetRenderGUIConfigure						= true;

	bool										IsUpdateStopped() const								{ return (IsWorldStopped() && IsRenderStopped()); }
	bool										IsWorldStopped() const								{ return (m_engineState != ENGINE_STATE_RUN); }
	bool										IsRenderStopped() const								{ return (m_isWindowMinimized); }
	bool										m_isWindowResizing									= {};
	bool										m_isWindowMoving									= {};
	bool										m_isWindowMinimized									= {};

	void										WorldTimersReset()									{ m_worldTimerTotal.Reset(); m_worldTimerFrame.Reset(); }
	void										WorldTimersStop()									{ m_worldTimerTotal.Stop(); m_worldTimerFrame.Stop(); }
	void										WorldTimersStart()									{ m_worldTimerTotal.Start(); m_worldTimerFrame.Start(); }
	Timer										m_worldTimerTotal									= {};
	Timer										m_worldTimerFrame									= {};

	EngineState									m_engineState										= ENGINE_STATE_PREPARE;

private:

	OptionFullScreen	m_optionFullScreen	= {};
	OptionWindowSave	m_optionWindowSave	= {};
	OptionVSync			m_optionVSync		= {};
	OptionTearing		m_optionTearing		= {};
	OptionHDR			m_optionHDR			= {};
	OptionGUI			m_optionGUI			= {};
};