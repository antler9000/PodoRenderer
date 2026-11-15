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
#include <cstdint>

using namespace DirectX;
using std::wstring;

void Podo::Update()
{
	PIXScopedEvent(PIX_COLOR_INDEX(1), L"CPU: 01. Frame Time");

	{
		PIXScopedEvent(PIX_COLOR_INDEX(2), L"CPU: 02. Wait Fence");

		UpdateCheckFence();
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

	{
		PIXScopedEvent(PIX_COLOR_INDEX(11), L"CPU: 14. Set Fence");

		UpdateSetFence();
	}
}

void Podo::UpdateCheckFence()
{
	std::uint64_t completedFence = m_fence->GetCompletedValue();

	if (m_fenceCurrent - completedFence > frameResourceNum)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceCurrent - frameResourceNum, m_fenceEvent));

		auto waitResult = WaitForSingleObject(m_fenceEvent, INFINITE);
		if (waitResult != WAIT_OBJECT_0)
		{
			Throw("wait fence failed");
		}
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
		horizontalSteelBoxObject.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	{
		Object& verticalWoodBoxObject = m_workloadObjects.at("WoodBox");

		XMVECTOR position = XMVectorSet(0.0f, oscillation, 2.0f, 1.0f);
		verticalWoodBoxObject.SetPosition(position);
		verticalWoodBoxObject.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
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
		RedSun.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
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
		blueMoon.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	{
		//NOTE: 스왑체인의 종횡비 변화를 전체 워크로드 초기화없이 카메라에 반영하기 위해, 매 프레임 카메라 업데이트를 수행해야 함
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
		mainCamera.SetViewVolume(XM_PIDIV4, m_screenBackBufferAspectRatio, 0.1f, 1000.0f);
		mainCamera.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	//NOTE: 이번 프레임 자원이 이전 프레임 자원보다 더 과거의 상태일 수 있으므로 확인 후 업데이트해야 함
	for (auto& [name, material] : m_workloadMaterials)
	{
		material.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	for (auto& [name, object] : m_workloadObjects)
	{
		object.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	for (auto& [name, light] : m_workloadLights)
	{
		light.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}

	for (auto& [name, camera] : m_workloadCameras)
	{
		camera.CheckAndUpdateFrameResource(CurrentFrameResourceIndex());
	}
}

void Podo::UpdateRender()
{
	if (IsRenderStopped() == true)
	{
		return;
	}

	ID3D12GraphicsCommandList*	pCommandList		= m_pCommandLists[CurrentFrameResourceIndex()].Get();
	ID3D12CommandAllocator*		pCommandAllocator	= m_pCommandAllocators[CurrentFrameResourceIndex()].Get();

	{
		PIXScopedEvent(PIX_COLOR_INDEX(5), L"CPU: 05. Reset Command List");

		ThrowIfFailed(pCommandAllocator->Reset());
		ThrowIfFailed(pCommandList->Reset(pCommandAllocator, nullptr));
	}

	{
		PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(1), L"GPU: 1. Frame Time");

		{
			PIXScopedEvent(PIX_COLOR_INDEX(6), L"CPU: 06. Prepare Draw");
			PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(2), L"GPU: 2. Prepare Draw");

			ID3D12DescriptorHeap* ppDescriptorHeap[] = { m_descriptorHeapCBVSRVUAV.Get() };
			pCommandList->SetDescriptorHeaps(_countof(ppDescriptorHeap), ppDescriptorHeap);
			pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			CD3DX12_RESOURCE_BARRIER barrierToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
			pCommandList->ResourceBarrier(1, &barrierToRenderTarget);
		}

		if (m_engineState == ENGINE_STATE_RUN)
		{
			PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 07. Draw Scene");
			PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(3), L"GPU: 3. Draw Scene");

			UpdateScene();
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(8), L"CPU: 10. Draw GUI");
			PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(4), L"GPU: 6. Draw GUI");

			if (m_engineState != ENGINE_STATE_RUN)
			{
				CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartCPUHandle;
				cpuHandleRTV.Offset(m_screenBackBufferIndex, m_descriptorHeapRTVIncrementSize);

				pCommandList->OMSetRenderTargets(1, &cpuHandleRTV, TRUE, nullptr);
				pCommandList->ClearRenderTargetView(cpuHandleRTV, DirectX::Colors::Black, 0, nullptr);
			}

			UpdateGUI();
		}

		{
			PIXScopedEvent(PIX_COLOR_INDEX(9), L"CPU: 11. Prepare Present");
			PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(5), L"GPU: 7. Prepare Present");

			CD3DX12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition
			(
				m_screenBackBuffers[m_screenBackBufferIndex].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT
			);
			pCommandList->ResourceBarrier(1, &barrierToPresent);
		}
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(10), L"CPU: 12. Submit Command List");

		ThrowIfFailed(pCommandList->Close());
		ID3D12CommandList* ppCommandList[] = { pCommandList };
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
	ID3D12GraphicsCommandList* pCommandList = m_pCommandLists[CurrentFrameResourceIndex()].Get();

	{
		PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 08. Shadow Pass");
		PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(3), L"GPU: 4. Shadow Pass");

		UpdateShadowScene();
	}

	{
		PIXScopedEvent(PIX_COLOR_INDEX(7), L"CPU: 09. Main Pass");
		PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(3), L"GPU: 5. Main Pass");

		UpdateMainScene();
	}
}

void Podo::UpdateShadowScene()
{	
	ID3D12GraphicsCommandList*	pCommandList		= m_pCommandLists[CurrentFrameResourceIndex()].Get();
	ID3D12CommandAllocator*		pCommandAllocator	= m_pCommandAllocators[CurrentFrameResourceIndex()].Get();

	pCommandList->SetPipelineState(m_renderSceneConfigureShadowPipelineStateObject.Get());
	pCommandList->SetGraphicsRootSignature(m_renderSceneConfigureShadowRootSignature.Get());

	for (const auto& [name, light] : m_workloadLights)
	{
		if (light.GetIsShadowOn() == false)
		{
			continue;
		}

		Shadow* pShadow = light.GetShadowPtr();

		CD3DX12_RESOURCE_BARRIER barrierToDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition
		(
			pShadow->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		pCommandList->ResourceBarrier(1, &barrierToDepthWrite);

		D3D12_CPU_DESCRIPTOR_HANDLE	depthStencilViewHandle	= pShadow->GetDepthStencilViewCPUHandle();
		D3D12_VIEWPORT				viewport				= pShadow->GetViewport();
		RECT						scissorRect				= pShadow->GetScissorRect();
		pCommandList->OMSetRenderTargets(0, nullptr, TRUE, &depthStencilViewHandle);
		pCommandList->RSSetViewports(1, &viewport);
		pCommandList->RSSetScissorRects(1, &scissorRect);

		pCommandList->ClearDepthStencilView
		(
			pShadow->GetDepthStencilViewCPUHandle(),
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
		);

		pCommandList->SetGraphicsRootDescriptorTable(SHADOW_LIGHT, light.GetCBVLocationGPUHandle(CurrentFrameResourceIndex()));

		//NOTE: 오브젝트를 중심으로 순회하는 반복문을 임시로 사용함
		for (const auto& [name, object] : m_workloadObjects)
		{
			pCommandList->SetGraphicsRootDescriptorTable(SHADOW_OBJECT, object.GetConstantBufferViewGPUHandle(CurrentFrameResourceIndex()));

			const Asset* pAsset = object.GetAssetPtr();

			const Mesh* pMesh = pAsset->GetMeshPtr();
			pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
			pCommandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

			pCommandList->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
		}

		CD3DX12_RESOURCE_BARRIER  barrierToPixelShader = CD3DX12_RESOURCE_BARRIER::Transition(
			pShadow->GetResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		pCommandList->ResourceBarrier(1, &barrierToPixelShader);
	}
}

void Podo::UpdateMainScene()
{
	ID3D12GraphicsCommandList*	pCommandList		= m_pCommandLists[CurrentFrameResourceIndex()].Get();
	ID3D12CommandAllocator*		pCommandAllocator	= m_pCommandAllocators[CurrentFrameResourceIndex()].Get();

	pCommandList->SetPipelineState(m_renderSceneConfigureMainPipelineStateObject.Get());
	pCommandList->SetGraphicsRootSignature(m_renderSceneConfigureMainRootSignature.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandleRTV = m_descriptorHeapRTVStartCPUHandle;
	cpuHandleRTV.Offset(m_screenBackBufferIndex, m_descriptorHeapRTVIncrementSize);

	pCommandList->OMSetRenderTargets(1, &cpuHandleRTV, true, &m_descriptorHeapDSVStartCPUHandle);
	pCommandList->RSSetViewports(1, &m_screenViewport);
	pCommandList->RSSetScissorRects(1, &m_screenScissorRect);

	pCommandList->ClearRenderTargetView(cpuHandleRTV, DirectX::Colors::Black, 0, nullptr);
	pCommandList->ClearDepthStencilView
	(
		m_descriptorHeapDSVStartCPUHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr
	);

	const Camera& camera = m_workloadCameras.at("MainCamera");
	pCommandList->SetGraphicsRootConstantBufferView(MAIN_CAMERA, camera.GetResourceGPUAddress(CurrentFrameResourceIndex()));

	pCommandList->SetGraphicsRootDescriptorTable
	(
		MAIN_LIGHT,
		CD3DX12_GPU_DESCRIPTOR_HANDLE
		(
			m_descriptorHeapCBVSRVUAVStartGPUHandleForLights,
			CurrentFrameResourceIndex() * static_cast<unsigned int>(m_workloadLights.size()),
			m_descriptorHeapCBVSRVUAVIncrementSize
		)
	);

	pCommandList->SetGraphicsRootDescriptorTable
	(
		MAIN_SHADOW,
		m_descriptorHeapCBVSRVUAVStartGPUHandleForShadows
	);

	//NOTE: 오브젝트를 중심으로 순회하는 반복문을 임시로 사용함
	for (const auto& [name, object] : m_workloadObjects)
	{
		pCommandList->SetGraphicsRootDescriptorTable(MAIN_OBJECT, object.GetConstantBufferViewGPUHandle(CurrentFrameResourceIndex()));

		const Asset* pAsset = object.GetAssetPtr();

		const Material* pMaterial = pAsset->GetMaterialPtr();
		pCommandList->SetGraphicsRootDescriptorTable(MAIN_MATERIAL, pMaterial->GetCBVLocationGPUHandle(CurrentFrameResourceIndex()));

		const Texture* pTexture = pMaterial->GetTexturePtr();
		pCommandList->SetGraphicsRootDescriptorTable(MAIN_TEXTURE, pTexture->GetShaderResourceViewGPUHandle());

		const Mesh* pMesh = pAsset->GetMeshPtr();
		pCommandList->IASetVertexBuffers(0, 1, pMesh->GetVertexBufferView());
		pCommandList->IASetIndexBuffer(pMesh->GetIndexBufferView());

		pCommandList->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
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
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandLists[CurrentFrameResourceIndex()].Get());
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
		m_needResetRenderGUIConfigure = true;
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

		m_needResetScreenMode			= true;
		m_needResetFactory				= true;
		m_needResetDevice				= true;
		m_needResetSwapChain			= true;
		m_needResetWorkloads			= true;
		m_needResetRenderSceneConfigure	= true;
	}

	ImGui::End();
}

void Podo::UpdateSetFence()
{
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceCurrent));

	m_fenceCurrent++;
}
