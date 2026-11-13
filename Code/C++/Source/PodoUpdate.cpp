#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS

//Podo
#include "Podo.h"
#include "State.h"
#include "Timer.h"
#include "Camera.h"
#include "Light.h"
#include "Shadow.h"
#include "Object.h"
#include "Asset.h"
#include "Material.h"
#include "Texture.h"
#include "Mesh.h"
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

	//NOTE: SetWindowTextW를 너무 자주 호출하면 시스템 부하로 인해 윈도우 전체가 먹통이 되어 반복에 텀을 주었음
	if (captionTimer.GetTimeMilli() > 100.0f)
	{
		int fps = (m_worldTimerFrame.GetTimeMilli() != 0) ? static_cast<int>(1000 / m_worldTimerFrame.GetTimeMilli()) : 0;

		wstring caption = std::format
		(
			L"{} (월드 전체 시간: {:06.1F} s / 월드 프레임 시간: {:0.4f} ms / FPS: {:3d} fps)",
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
		Object& horizontalSteelBoxObject = m_workloadObjects.at("SteelBox");

		XMVECTOR position = XMVectorSet(oscillation, 0.0f, -2.0f, 1.0f);
		horizontalSteelBoxObject.SetPosition(position);
		horizontalSteelBoxObject.UpdateBuffer();
	}

	{
		Object& verticalWoodBoxObject = m_workloadObjects.at("WoodBox");

		XMVECTOR position = XMVectorSet(0.0f, oscillation, 2.0f, 1.0f);
		verticalWoodBoxObject.SetPosition(position);
		verticalWoodBoxObject.UpdateBuffer();
	}

	{
		Light& RedSun = m_workloadLights.at("Sun1");

		float radius	= 100.0f;
		float height	= 100.0f;
		float speed		= 0.2f;

		float orbitAngle = timeSecond * speed;

		XMVECTOR direction	= XMVectorSet(-XMScalarCos(orbitAngle) * radius, -height, -XMScalarSin(orbitAngle) * radius, 0.0f);
		XMVECTOR poistion	= XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) - direction;
		RedSun.SetDirection(direction);
		RedSun.SetPosition(poistion);
		RedSun.UpdateBuffer();
	}

	{
		Light& blueMoon = m_workloadLights.at("Sun2");

		float radius	= 100.0f;
		float height	= 100.0f;
		float speed		= 0.4f;
		float offset	= XM_PI;

		float orbitAngle = timeSecond * speed + offset;

		XMVECTOR direction	= XMVectorSet(-XMScalarCos(orbitAngle) * radius, -height, -XMScalarSin(orbitAngle) * radius, 0.0f);
		XMVECTOR poistion	= XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) - direction;
		blueMoon.SetDirection(direction);
		blueMoon.SetPosition(poistion);
		blueMoon.UpdateBuffer();
	}

	{
		Camera& mainCamera = m_workloadCameras.at("MainCamera");

		float radius	= 10.0f;
		float height	= 5.0f;
		float speed		= 0.3f;

		float orbitAngle = timeSecond * speed;

		XMVECTOR cameraPosition	= XMVectorSet(XMScalarCos(orbitAngle) * radius, height, XMScalarSin(orbitAngle) * radius, 1.0f);
		XMVECTOR cameraTarget	= XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		mainCamera.SetDirectionalLightsNum(static_cast<unsigned int>(m_workloadLights.size()));
		mainCamera.SetPosition(cameraPosition);
		mainCamera.SetTarget(cameraTarget);
		mainCamera.UpdateBuffer();
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
			PIXScopedEvent(PIX_COLOR_INDEX(6), L"CPU: 06. Prepare Draw");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(2), L"GPU: 2. Prepare Draw");

			ID3D12DescriptorHeap* ppDescriptorHeap[] = { m_descriptorHeapCBVSRVUAV.Get() };
			m_commandList->SetDescriptorHeaps(_countof(ppDescriptorHeap), ppDescriptorHeap);
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			CD3DX12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
			m_commandList->ResourceBarrier(1, &barrierToRenderTarget);
		}

		if (m_engineState == ENGINE_STATE_RUN)
		{
			PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 07. Draw Scene");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(3), L"GPU: 3. Draw Scene");

			UpdateScene();
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(8), L"CPU: 10. Draw GUI");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(4), L"GPU: 6. Draw GUI");

			if (m_engineState != ENGINE_STATE_RUN)
			{
				CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartCPUHandle;
				cpuHandleRTV.Offset(m_screenBackBufferIndex, m_descriptorHeapRTVIncrementSize);

				m_commandList->OMSetRenderTargets(1, &cpuHandleRTV, TRUE, nullptr);
				m_commandList->ClearRenderTargetView(cpuHandleRTV, DirectX::Colors::Black, 0, nullptr);
			}

			UpdateGUI();
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(9), L"CPU: 11. Prepare Present");
			PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(5), L"GPU: 7. Prepare Present");

			CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT
			);
			m_commandList->ResourceBarrier(1, &barrierToPresent);
		}
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(10), L"CPU: 12. Submit Command List");

		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(11), L"CPU: 13. Present");

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

void Podo::UpdateScene()
{
	{
		PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 08. Shadow Pass");
		PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(3), L"GPU: 4. Shadow Pass");

		UpdateShadowScene();
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 09. Main Pass");
		PIXScopedEvent(m_commandList.Get(), PIX_COLOR_INDEX(3), L"GPU: 5. Main Pass");

		UpdateMainScene();
	}
}

void Podo::UpdateShadowScene()
{	
	m_commandList->SetPipelineState(m_renderConfigureShadowPipelineStateObject.Get());
	m_commandList->SetGraphicsRootSignature(m_renderConfigureShadowRootSignature.Get());

	for (const auto& [name, light] : m_workloadLights)
	{
		if (light.GetIsShadowOn() == false)
		{
			continue;
		}

		Shadow* pShadow = light.GetShadowPtr();

		CD3DX12_RESOURCE_BARRIER barrierToDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition
		(
			pShadow->GetTexture(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		m_commandList->ResourceBarrier(1, &barrierToDepthWrite);

		D3D12_CPU_DESCRIPTOR_HANDLE	depthStencilViewHandle	= pShadow->GetDepthStencilViewCPUHandle();
		D3D12_VIEWPORT				viewport				= pShadow->GetViewport();
		RECT						scissorRect				= pShadow->GetScissorRect();
		m_commandList->OMSetRenderTargets(0, nullptr, TRUE, &depthStencilViewHandle);
		m_commandList->RSSetViewports(1, &viewport);
		m_commandList->RSSetScissorRects(1, &scissorRect);

		m_commandList->ClearDepthStencilView
		(
			pShadow->GetDepthStencilViewCPUHandle(),
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
		);

		m_commandList->SetGraphicsRootDescriptorTable(SHADOW_LIGHT, light.GetConstantBufferViewGPUHandle());

		//NOTE: 오브젝트를 중심으로 순회하는 반복문을 임시로 사용함
		for (const auto& [name, object] : m_workloadObjects)
		{
			m_commandList->SetGraphicsRootDescriptorTable(SHADOW_OBJECT, object.GetConstantBufferViewGPUHandle());

			const Asset* pAsset = object.GetAsset();

			const Mesh* pMesh = pAsset->GetMesh();
			m_commandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
			m_commandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

			m_commandList->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
		}

		CD3DX12_RESOURCE_BARRIER  barrierToPixelShader = CD3DX12_RESOURCE_BARRIER::Transition(
			pShadow->GetTexture(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		m_commandList->ResourceBarrier(1, &barrierToPixelShader);
	}
}

void Podo::UpdateMainScene()
{
	m_commandList->SetPipelineState(m_renderConfigureMainPipelineStateObject.Get());
	m_commandList->SetGraphicsRootSignature(m_renderConfigureMainRootSignature.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartCPUHandle;
	cpuHandleRTV.Offset(m_screenBackBufferIndex, m_descriptorHeapRTVIncrementSize);

	m_commandList->OMSetRenderTargets(1, &cpuHandleRTV, true, &m_descriptorHeapDSVStartCPUHandle);
	m_commandList->RSSetViewports(1, &m_screenViewport);
	m_commandList->RSSetScissorRects(1, &m_screenScissorRect);

	m_commandList->ClearRenderTargetView(cpuHandleRTV, DirectX::Colors::Black, 0, nullptr);
	m_commandList->ClearDepthStencilView
	(
		m_descriptorHeapDSVStartCPUHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr
	);

	const Camera& camera = m_workloadCameras.at("MainCamera");
	m_commandList->SetGraphicsRootConstantBufferView(MAIN_CAMERA, camera.GetBufferGPUAddress());

	m_commandList->SetGraphicsRootDescriptorTable(MAIN_LIGHT, m_descriptorHeapCBVSRVUAVStartGPUHandleForLights);

	m_commandList->SetGraphicsRootDescriptorTable(MAIN_SHADOW, m_descriptorHeapCBVSRVUAVStartGPUHandleForShadows);

	//NOTE: 오브젝트를 중심으로 순회하는 반복문을 임시로 사용함
	for (const auto& [name, object] : m_workloadObjects)
	{
		m_commandList->SetGraphicsRootDescriptorTable(MAIN_OBJECT, object.GetConstantBufferViewGPUHandle());

		const Asset* pAsset = object.GetAsset();

		const Material* pMaterial = pAsset->GetMaterial();
		m_commandList->SetGraphicsRootDescriptorTable(MAIN_MATERIAL, pMaterial->GetConstantBufferViewGPUHandle());

		const Texture* pTexture = pMaterial->GetTexture();
		m_commandList->SetGraphicsRootDescriptorTable(MAIN_TEXTURE, pTexture->GetShaderResourceViewGPUHandle());

		const Mesh* pMesh = pAsset->GetMesh();
		m_commandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
		m_commandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

		m_commandList->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
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

	ImGui::Begin("Prepare", nullptr, imGuiBasicFlag);

	bool backButtonClicked = ImGui::Button("Start", imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
	if (backButtonClicked == true)
	{
		m_engineState = ENGINE_STATE_RUN;
		WorldTimersReset();
		WorldTimersStart();

#if !defined(_DEBUG) && defined(USE_PIX)
		ThrowIfNull(PIXLoadLatestWinPixTimingCapturerLibrary());

		PIXCaptureParameters captureParameters = {};
		captureParameters.TimingCaptureParameters.FileName				= L"PodoRendererProfile.wpix";
		captureParameters.TimingCaptureParameters.CaptureCpuSamples		= true;
		captureParameters.TimingCaptureParameters.CpuSamplesPerSecond	= 4000;
		captureParameters.TimingCaptureParameters.CaptureGpuTiming		= true;

		//NOTE: 관리자 권한으로 실행하지 않으면 Throw가 발생함
		ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_TIMING, &captureParameters));
#endif
	}

	ImGui::SameLine();

	bool exitToWindowButtonClicked = ImGui::Button("Exit", imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
	if (exitToWindowButtonClicked == true)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}

	bool	previousFullScreenState	= m_optionFullScreen.IsActive();
	bool	previousHDRState		= m_optionHDR.IsActive();
	int		previousGUIState		= m_optionGUI.masterSize;

	ImGui::Dummy(imGuiSpacingBasicSize * m_optionGUI.GetMasterScale());

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

	ImGui::Dummy(imGuiSpacingBasicSize * m_optionGUI.GetMasterScale());
	ImGui::Separator();

	{
		ImGui::Text("GUI");
		const char* masterSizeCStringArray[] = { "50%", "75%", "100%", "125%", "150%" };
		int selectedIndex = (m_optionGUI.masterSize - 50) / 25;
		if (ImGui::Combo("Master Size", &selectedIndex, masterSizeCStringArray, _countof(masterSizeCStringArray)) == true)
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
		//TODO: Scene 렌더 설정 초기화와 GUI 렌더 설정 초기화 요청을 분할 후 반영하기
		m_needResetRenderConfigure = true;
	}

	ImGui::End();
}

void Podo::UpdateRunStateGUI(ImGuiViewport* pImGuiViewPort, ImVec2 imGuiCenterPos)
{
	ImGuiWindowFlags loadingGuiFlag = imGuiBasicFlag | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;

	ImGui::SetNextWindowPos(ImVec2(0.0f,0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f) * m_optionGUI.GetMasterScale(), ImGuiCond_Always);

	ImGui::Begin("Runtime", nullptr, loadingGuiFlag);

	bool menuButtonClicked	= ImGui::Button("End", imGuiSmallButtonBasicSize * m_optionGUI.GetMasterScale());
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