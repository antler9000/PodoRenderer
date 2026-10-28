#pragma once
#define NOMINMAX

//Podo
#include "BaseApp.h"
#include "Option.h"
#include "State.h"
#include "Timer.h"
#include "Camera.h"
#include "Object.h"
#include "Asset.h"
#include "Alloc.h"

//ImGui
#include "imgui.h"

//Win32
#include <windows.h>
#include <wrl/client.h>

//D3D12
#include <d3dx12_root_signature.h>
#include <d3d12.h>

//DXGI
#include <dxgi1_6.h>
#include <dxgi1_4.h>
#include <dxgi.h>
#include <dxgicommon.h>
#include <dxgiformat.h>

//C++
#include <unordered_map>
#include <string>
#include <fstream>

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
	void ResetDescriptorHeapRTV();
	void ResetDescriptorHeapDSV();
	void ResetDescriptorHeapCBVSRVUAV();
	void ResetHDRSwapChainSupport();
	void ResetSwapChain();
	void ResetBackBufferInfo();
	void ResetViewPort();
	void ResetScissorRectangle();
	void ResetDepthStencilBuffer();
	void ResetRTV();
	void ResetDSV();
	void ResetAssets();
	void ResetObjects();
	void ResetCamera();
	void ResetCBVSRVUAV();
	void ResetRootSignature();
	void ResetPipelineStateObject();
	void ResetImGui();

	void Close();
	void CloseFenceEvent();
	void CloseImGui();

	void Update();
	void UpdateTimers();
	void UpdateCaption();
	void UpdateWorld();
	void UpdateRender();
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

	//NOTE: ImGui에 넘겨주는 콜백 함수 속에서 기능해야 하므로 static으로 둠
	static inline		ImGuiDescriptorHeapAllocator	m_imGuiDescriptorHeapAllocator				= {};

private:

	static constexpr	UINT							m_screenBackBufferCount						= 2;
	static constexpr	DXGI_FORMAT						m_screenBackBufferFormatSDR					= DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr	DXGI_FORMAT						m_screenBackBufferFormatHDR					= DXGI_FORMAT_R10G10B10A2_UNORM;
	static constexpr	DXGI_FORMAT						m_screenRTVFormatSDR						= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	static constexpr	DXGI_FORMAT						m_screenRTVFormatHDR						= DXGI_FORMAT_R10G10B10A2_UNORM;
	static constexpr	DXGI_COLOR_SPACE_TYPE			m_screenBackBufferColorSpaceSDR				= DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	static constexpr	DXGI_COLOR_SPACE_TYPE			m_screenBackBufferColorSpaceHDR				= DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
	static constexpr	DXGI_FORMAT						m_screenDepthStencilBufferFormat			= DXGI_FORMAT_D24_UNORM_S8_UINT;

	static constexpr	UINT							m_descriptorHeapCBVSRVUAVCapacityForGUI		= 64;
	static constexpr	UINT							m_descriptorHeapCBVSRVUAVCapacityForRender	= 64;

	static constexpr	ImVec2							m_imGuiSpacingBasicSize						= ImVec2(0.0f, 10.0f);
	static constexpr	ImVec2							m_imGuiSmallButtonBasicSize					= ImVec2(120.0f, 40.0f);
	static constexpr	ImVec2							m_imGuiMediumButtonBasicSize				= ImVec2(240.0f, 40.0f);
	static constexpr	ImVec2							m_imGuiLargeButtonBasicSize					= ImVec2(360.0f, 40.0f);
	static constexpr	ImGuiWindowFlags				m_imGuiBasicFlag							= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
																									| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

private:

	template <typename Interface>
	using ComPtr = Microsoft::WRL::ComPtr<Interface>;

	ComPtr<IDXGIFactory6>					m_dxgiFactory;													//NOTE: (기본) 성능순 어댑터 획득
	ComPtr<IDXGIAdapter3>					m_dxgiAdapter;													//NOTE: (기본) 자원의 메모리 상주성 관리
	ComPtr<IDXGIOutput>						m_dxgiOutput;
	ComPtr<IDXGIOutput6>					m_dxgiOutput6;													//NOTE: (옵션) HDR 모니터 정보 획득
	DXGI_OUTPUT_DESC1						m_dxgiOutputDesc									= {};		//NOTE: (옵션) HDR 모니터 정보 획득
	
	ComPtr<ID3D12Device>					m_device;
	
	ComPtr<ID3D12Fence>						m_fence;
	UINT64									m_fenceCurrent										= 0;
	HANDLE									m_fenceEvent										= nullptr;
	
	ComPtr<ID3D12CommandQueue>				m_commandQueue;
	ComPtr<ID3D12CommandAllocator>			m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList>		m_commandList;
	
	ComPtr<ID3D12DescriptorHeap>			m_descriptorHeapRTV;
	ComPtr<ID3D12DescriptorHeap>			m_descriptorHeapDSV;
	ComPtr<ID3D12DescriptorHeap>			m_descriptorHeapCBVSRVUAV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE			m_descriptorHeapRTVStartHandleCPU;
	CD3DX12_CPU_DESCRIPTOR_HANDLE			m_descriptorHeapDSVStartHandleCPU;
	CD3DX12_CPU_DESCRIPTOR_HANDLE			m_descriptorHeapCBVSRVUAVStartHandleCPUForGUI;
	CD3DX12_GPU_DESCRIPTOR_HANDLE			m_descriptorHeapCBVSRVUAVStartHandleGPUForGUI;
	CD3DX12_CPU_DESCRIPTOR_HANDLE			m_descriptorHeapCBVSRVUAVStartHandleCPUForRender;
	CD3DX12_GPU_DESCRIPTOR_HANDLE			m_descriptorHeapCBVSRVUAVStartHandleGPUForRender;
	UINT									m_descriptorHeapRTVIncrementSize					= 0;
	UINT									m_descriptorHeapDSVIncrementSize					= 0;
	UINT									m_descriptorHeapCBVSRVUAVIncrementSize				= 0;

	ComPtr<IDXGISwapChain3>					m_screenSwapChain;												//NOTE: (기본) 백 버퍼 인덱스 추적
	ComPtr<ID3D12Resource>					m_screenBackBuffers[m_screenBackBufferCount];
	UINT									m_screenBackBufferIndex								= 0;
	UINT									m_screenBackBufferWidth								= 0;
	UINT									m_screenBackBufferHeight							= 0;
	float									m_screenBackBufferAspectRatio						= 0.0f;
	D3D12_VIEWPORT							m_screenViewPort									= {};
	D3D12_RECT								m_screenScissorRectangle							= {};
	ComPtr<ID3D12Resource>					m_screenDepthStencilBuffer;
	
	std::unordered_map<std::string, Asset>	m_workloadAssets;
	std::unordered_map<std::string, Object>	m_workloadObjects;
	Camera									m_workloadCamera;

	ComPtr<ID3D12RootSignature>				m_renderConfigureRootSignature;
	ComPtr<ID3D12PipelineState>				m_renderConfigurePipelineStateObject;
	bool									m_renderConfigureImGuiInitialized					= false;

	bool									NeedReset() const									{ return (NeedResetScreenMode() || NeedResetFactory() || NeedResetDevice() || NeedResetSwapChain() || NeedResetWorkload() || NeedResetRenderConfigure()); }
	bool									NeedResetScreenMode() const							{ return m_needResetScreenMode; }
	bool									m_needResetScreenMode								= true;
	bool									NeedResetFactory() const							{ return (m_needResetFactory || (m_dxgiFactory->IsCurrent() == FALSE)); }
	bool									m_needResetFactory									= true;
	bool									NeedResetDevice() const								{ return m_needResetDevice; }
	bool									m_needResetDevice									= true;
	bool									NeedResetSwapChain() const							{ return m_needResetSwapChain; }
	bool									m_needResetSwapChain								= true;
	bool									NeedResetWorkload() const							{ return m_needResetWorkload; }
	bool									m_needResetWorkload									= true;
	bool									NeedResetRenderConfigure() const					{ return m_needResetRenderConfigure; }
	bool									m_needResetRenderConfigure							= true;

	bool									IsUpdateStopped() const								{ return (IsWorldStopped() && IsRenderStopped()); }
	bool									IsWorldStopped() const								{ return (m_engineState != ENGINE_STATE_RUN); }
	bool									IsRenderStopped() const								{ return (m_isWindowResizing || m_isWindowMoving || m_isWindowMinimized); }
	bool									m_isWindowResizing									= false;
	bool									m_isWindowMoving									= false;
	bool									m_isWindowMinimized									= false;

	void									WorldTimersReset()									{ m_worldTimerTotal.Reset(); m_worldTimerFrame.Reset(); }
	void									WorldTimersStop()									{ m_worldTimerTotal.Stop(); m_worldTimerFrame.Stop(); }
	void									WorldTimersStart()									{ m_worldTimerTotal.Start(); m_worldTimerFrame.Start(); }
	Timer									m_worldTimerTotal;
	Timer									m_worldTimerFrame;

	EngineState								m_engineState										= ENGINE_STATE_PREPARE;

private:

	OptionFullScreen	m_optionFullScreen;
	OptionWindowSave	m_optionWindowSave;
	OptionVSync			m_optionVSync;
	OptionTearing		m_optionTearing;
	OptionHDR			m_optionHDR;
	OptionGUI			m_optionGUI;
};