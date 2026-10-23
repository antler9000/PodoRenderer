#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS

//Podo
#include "Podo.h"
#include "State.h"
#include "Timer.h"
#include "Object.h"
#include "Asset.h"
#include "Root.h"
#include "Debug.h"

//ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

//Win32
#include <windows.h>

//D3D12
#include <pix3.h>
#include <d3dx12_root_signature.h>
#include <d3dx12_barriers.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

//DXGI
#include <dxgi.h>

//C++
#include <format>
#include <string>
#include <cstdlib>

using namespace DirectX;
using std::wstring;

void Podo::Update()
{
	PIXScopedEvent(PIX_COLOR_INDEX(1), L"CPU: 01. Frame Time");

	{
		PIXScopedEvent(PIX_COLOR_INDEX(2), L"CPU: 02. Wait GPU");

		FlushCommandQueue();
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(3), L"CPU: 03. Non-Render Logic");

		UpdateTimers();
		UpdateCaption();
		UpdateWorld();
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(4), L"CPU: 04. Render Logic");

		UpdateRender();
	}
}

void Podo::UpdateTimers()
{
	m_worldTimerTotal.Update();
	m_worldTimerFrame.Update();

	m_worldTimerFrame.Mark();
}

void Podo::UpdateCaption()
{
#if defined(_DEBUG)
	static Timer captionTimer;

	captionTimer.Start();
	captionTimer.Update();

	//NOTE: SetWindowTextW를 너무 자주 호출하면 시스템 부하로 인해 윈도우 전체가 먹통이 되니 반복에 텀을 주자
	if (captionTimer.GetTimeMilli() > 100.0f)
	{
		int fps = (m_worldTimerFrame.GetTimeMilli() != 0) ? static_cast<int>(1000 / m_worldTimerFrame.GetTimeMilli()) : 0;

		wstring caption = std::format
		(
			L"{} (월드 경과 시간: {:06.1F} s / 월드 프레임 시간: {:0.4f} ms / FPS: {:3d} fps)",
			m_pAppName,
			m_worldTimerTotal.GetTimeMilli() / 1000,
			m_worldTimerFrame.GetTimeMilli(),
			(fps > 999) ? 999 : fps
		);

		SetWindowTextW(m_hWnd, caption.c_str());

		captionTimer.Mark();
	}
#endif
}

void Podo::UpdateWorld()
{
	if (IsWorldStopped() == true)
	{
		return;
	}

	float timeSecond	= static_cast<float>(m_worldTimerTotal.GetTimeMilli()) / 1000.0f;
	float oscillation	= XMScalarSin(timeSecond) * 1.5f;

	{
		Object& horizontalBoxObject = m_workloadObjects.at("HorizontalBoxObject");

		DirectX::XMVECTOR scale			= DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= XMVectorSet(oscillation, 0.0f, 3.0f, 1.0f);
		horizontalBoxObject.SetScale(scale);
		horizontalBoxObject.SetRotation(lookDirection, upDirection);
		horizontalBoxObject.SetPosition(position);
		horizontalBoxObject.UpdateObjectConstantBuffer();
	}

	{
		Object& verticalBoxObject = m_workloadObjects.at("VerticalBoxObject");

		DirectX::XMVECTOR scale			= DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR lookDirection	= DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR upDirection	= DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR position		= XMVectorSet(2.0f, oscillation, 5.0f, 1.0f);
		verticalBoxObject.SetScale(scale);
		verticalBoxObject.SetRotation(lookDirection, upDirection);
		verticalBoxObject.SetPosition(position);
		verticalBoxObject.UpdateObjectConstantBuffer();
	}

	{
		constexpr float radius	= 10.0f;
		constexpr float height	= 5.0f;
		constexpr float speed	= 0.3f;

		float orbitAngle = timeSecond * speed;

		XMVECTOR cameraPosition	= XMVectorSet
		(
			XMScalarCos(orbitAngle) * radius,
			height,
			XMScalarSin(orbitAngle) * radius,
			1.0f
		);
		XMVECTOR cameraTarget	= XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMVECTOR cameraUp		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		m_workloadCamera.SetPosition(cameraPosition);
		m_workloadCamera.SetTarget(cameraTarget);
		m_workloadCamera.SetUpDirection(cameraUp);
		m_workloadCamera.UpdateCameraConstantBuffer(m_screenBackBufferAspectRatio);
	}
}

void Podo::UpdateRender()
{
	if (IsRenderStopped() == true)
	{
		return;
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(5), L"CPU: 05. Reset Command List");

		ThrowIfFailed(m_commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
	}

	{
		PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(1), L"GPU: 1. Frame Time");

		{
			PIXScopedEvent(PIX_COLOR_INDEX(6), L"CPU: 06. Bind Resources");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(2), L"GPU: 2. Bind Resources");

			ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorHeapCBVSRVUAV.Get() };
			m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

			CD3DX12_RESOURCE_BARRIER barrierPresentToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
			m_commandList->ResourceBarrier(1, &barrierPresentToRenderTarget);

			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartHandleCPU;
			cpuHandleRTV.Offset(m_screenBackBufferIndex, m_descriptorHeapRTVIncrementSize);

			m_commandList->OMSetRenderTargets(1, &cpuHandleRTV, true, &m_descriptorHeapDSVStartHandleCPU);
			m_commandList->RSSetViewports(1, &m_screenViewPort);
			m_commandList->RSSetScissorRects(1, &m_screenScissorRectangle);

			m_commandList->ClearRenderTargetView(cpuHandleRTV, DirectX::Colors::Black, 0, nullptr);
			m_commandList->ClearDepthStencilView
			(
				m_descriptorHeapDSVStartHandleCPU,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				1.0f,
				0,
				0,
				nullptr
			);
		}

		if(m_engineState == ENGINE_STATE_RUN)
		{
			PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 07. Draw Scene");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(3), L"GPU: 3. Draw Scene");

			m_commandList->SetPipelineState(m_renderConfigurePipelineStateObject.Get());
			m_commandList->SetGraphicsRootSignature(m_renderConfigureRootSignature.Get());
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			m_commandList->SetGraphicsRootConstantBufferView
			(
				CAMERA_CONSTANT,
				m_workloadCamera.GetCameraConstantBufferGPUAddress()
			);

			for (const auto& [name, object] : m_workloadObjects)
			{
				const Asset* asset = object.GetAsset();

				m_commandList->IASetVertexBuffers(0, 1, asset->GetVertexBufferView());
				m_commandList->IASetIndexBuffer(asset->GetIndexBufferView());
				m_commandList->SetGraphicsRootDescriptorTable
				(
					OBJECT_CONSTANT,
					object.GetObjectConstantBufferViewGPUHandle()
				);

				m_commandList->DrawIndexedInstanced(asset->GetIndexCount(), 1, 0, 0, 0);
			}
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(8), L"CPU: 08. Draw GUI");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(4), L"GPU: 4. Draw GUI");

			UpdateGUI();
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(9), L"CPU: 09. Unbind Resources");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(5), L"GPU: 5. Unbind Resources");

			CD3DX12_RESOURCE_BARRIER barrierRenderTargetToPresent = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT
			);
			m_commandList->ResourceBarrier(1, &barrierRenderTargetToPresent);
		}
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(10), L"CPU: 10. Submit Command List");

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(11), L"CPU: 11. Present");

		if (m_optionVSync.IsActive() == true)
		{
			ThrowIfFailed(m_screenSwapChain->Present(1, 0));
		}
		else
		{
			ThrowIfFailed(m_screenSwapChain->Present(0, m_optionTearing.IsActive() ? DXGI_PRESENT_ALLOW_TEARING : 0));
		}

		m_screenBackBufferIndex = m_screenSwapChain->GetCurrentBackBufferIndex();
	}
}

void Podo::UpdateGUI()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiViewport* pImGuiViewPort = ImGui::GetMainViewport();
	ImVec2 imGuiCenterPos = pImGuiViewPort->GetCenter();

	switch (m_engineState)
	{
		case ENGINE_STATE_PREPARE:
		{
			UpdatePrepareStateGUI(pImGuiViewPort, imGuiCenterPos);

			break;
		}

		case ENGINE_STATE_RUN:
		{
			UpdateRunStateGUI(pImGuiViewPort, imGuiCenterPos);

			break;
		}
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
}

void Podo::UpdatePrepareStateGUI(ImGuiViewport* pImGuiViewPort, ImVec2 imGuiCenterPos)
{
	ImGui::SetNextWindowPos(imGuiCenterPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(700.0f, 400.0f) * m_optionGUI.GetMasterScale(), ImGuiCond_Always);

	ImGui::Begin("Prepare", nullptr, m_imGuiBasicFlag);

	bool backButtonClicked = ImGui::Button("Start", m_imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
	if (backButtonClicked == true)
	{
		m_engineState = ENGINE_STATE_RUN;
		WorldTimersReset();
		WorldTimersStart();

//NOTE:	USE_PIX 전처리 상수가 정의되어있지 않은 구성에선 PIXLoadLatestWinPixTimingCapturerLibrary()에서 nullptr이 반환되어 실패 해석이 복잡해짐
//		또한 관리자 권한으로 실행하지 않으면 PIXBeginCapture(..)에서 실패가 반환되며 마찬가지로 실패 해석이 복잡해짐
//		따라서 Profile 구성에서만 해당 함수들이 호출되도록 전처리 조건문을 작성하였음
#if !defined(_DEBUG) && defined(USE_PIX)
		ThrowIfNull(PIXLoadLatestWinPixTimingCapturerLibrary());

		PIXCaptureParameters captureParameters = {};
		captureParameters.TimingCaptureParameters.FileName				= L"PodoVideoEngineProfile.wpix";
		captureParameters.TimingCaptureParameters.CaptureCpuSamples		= true;
		captureParameters.TimingCaptureParameters.CpuSamplesPerSecond	= 4000;
		captureParameters.TimingCaptureParameters.CaptureGpuTiming		= true;

		ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_TIMING, &captureParameters));
#endif
	}

	ImGui::SameLine();

	bool exitToWindowButtonClicked = ImGui::Button("Exit", m_imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
	if (exitToWindowButtonClicked == true)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}

	bool	previousFullScreenState	= m_optionFullScreen.IsActive();
	bool	previousHDRState		= m_optionHDR.IsActive();
	int		previousGUIState		= m_optionGUI.masterSize;

	ImGui::Dummy(m_imGuiSpacingBasicSize * m_optionGUI.GetMasterScale());

	{
		ImGui::Text("Display");
		ImGui::BeginDisabled(m_optionFullScreen.IsSupported() == false);
		ImGui::Checkbox("Full Screen", &m_optionFullScreen.userEnabled);
		ImGui::EndDisabled();
		ImGui::BeginDisabled(m_optionVSync.IsSupported() == false);
		ImGui::Checkbox("VSync", &m_optionVSync.userEnabled);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(m_optionHDR.IsSupported() == false);
		ImGui::Checkbox("HDR", &m_optionHDR.userEnabled);
		ImGui::EndDisabled();
	}

	ImGui::Dummy(m_imGuiSpacingBasicSize * m_optionGUI.GetMasterScale());
	ImGui::Separator();

	{
		ImGui::Text("GUI");
		const char* masterSizeStringArray[] = { "50%", "75%", "100%", "125%", "150%" };
		int selectedIndex = (m_optionGUI.masterSize - 50) / 25;
		if (ImGui::Combo("Master Size", &selectedIndex, masterSizeStringArray, _countof(masterSizeStringArray)) == true)
		{
			m_optionGUI.SetMasterSize(50 + 25 * selectedIndex);
		}
	}

	bool	nowFullScreenState	= m_optionFullScreen.IsActive();
	bool	nowHDRState			= m_optionHDR.IsActive();
	int		nowGUIState			= m_optionGUI.masterSize;

	if (previousFullScreenState != nowFullScreenState)
	{
		m_needResetScreenMode = true;
	}
	if (previousHDRState != nowHDRState)
	{
		m_needResetSwapChain = true;
	}
	if (previousGUIState != nowGUIState)
	{
		//TODO: PSO 초기화는 하지 않도록 초기화 단계를 분할하기
		m_needResetRenderConfigure = true;
	}

	ImGui::End();
}

void Podo::UpdateRunStateGUI(ImGuiViewport* pImGuiViewPort, ImVec2 imGuiCenterPos)
{
	ImGuiWindowFlags loadingGuiFlag = m_imGuiBasicFlag | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;

	ImGui::SetNextWindowPos(ImVec2(0.0f,0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f) * m_optionGUI.GetMasterScale(), ImGuiCond_Always);

	ImGui::Begin("Runtime", nullptr, loadingGuiFlag);

	bool menuButtonClicked	= ImGui::Button("End", m_imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
	bool escKeyPressed		= ImGui::IsKeyPressed(ImGuiKey_Escape, false);
	if (menuButtonClicked == true || escKeyPressed == true)
	{
		m_engineState = ENGINE_STATE_PREPARE;
		WorldTimersReset();
		WorldTimersStop();

#if !defined(_DEBUG) && defined(USE_PIX)
		ThrowIfFailed(PIXEndCapture(false));
#endif

		m_needResetScreenMode		= true;
		m_needResetFactory			= true;
		m_needResetDevice			= true;
		m_needResetSwapChain		= true;
		m_needResetWorkload			= true;
		m_needResetRenderConfigure	= true;
	}

	ImGui::End();
}