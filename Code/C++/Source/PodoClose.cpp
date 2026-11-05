#define NOMINMAX

//Podo
#include "Podo.h"

//ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

//Win32
#include <windows.h>

void Podo::Close()
{
	FlushCommandQueue();

	CloseFenceEvent();
	CloseImGui();
}

void Podo::CloseImGui()
{
	if (m_renderConfigureImGuiInitialized == false)
	{
		return;
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_renderConfigureImGuiDescriptorHeapAllocator.Destroy();

	m_renderConfigureImGuiInitialized = false;
}

void Podo::CloseFenceEvent()
{
	CloseHandle(m_fenceEvent);
	m_fenceEvent = nullptr;
}