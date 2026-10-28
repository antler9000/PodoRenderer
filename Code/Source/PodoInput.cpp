#define NOMINMAX

//Podo
#include "Podo.h"

//ImGui
#include "imgui.h"

//Win32
#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//NOTE:		WindowProc이 수행 중에는 해당 스레드의 메시지 큐에 쌓인 다른 메시지들을 처리하지 못하므로,
//			되도록 이 안에서는 짧은 로직만 수행하도록 하고, 긴 대기가 필요한 로직은 별도 스레드로 처리하자
LRESULT Podo::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT imGuiHandled = ImGui_ImplWin32_WndProcHandler(m_hWnd, uMsg, wParam, lParam);
	if (imGuiHandled)
	{
		return 0;
	}

	switch (uMsg)
	{
		case WM_SYSKEYDOWN:
		{
			//NOTE: ALT+ENTER을 누르는 경우
			if (wParam == VK_RETURN && (lParam & 0x40000000) == 0)
			{
				m_optionFullScreen.SetUserEnabled(!m_optionFullScreen.userEnabled);
				m_needResetScreenMode = true;

				return 0;
			}

			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		}

		case WM_SYSCHAR:
		{
			//NOTE: ALT+ENTER을 누르는 경우 윈도우 알림음이 안 나도록 함
			if (wParam == '\r')
			{
				return 0;
			}

			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		}

		//NOTE: 창 테두리를 직접 끌어서 위치나 크기를 변화시키기를 시작하는 경우
		case WM_ENTERSIZEMOVE:
		{
			m_isWindowResizing = true;
			m_isWindowMoving = true;

			return 0;
		}

		//NOTE: 창 테두리를 직접 끌어서 위치나 크기를 변화시키기를 멈추는 경우
		case WM_EXITSIZEMOVE:
		{
			m_isWindowResizing = false;
			m_isWindowMoving = false;

			m_needResetFactory = true;

			if (m_optionFullScreen.IsActive() == false)
			{
				m_optionWindowSave.SetWindowRect(m_hWnd);
			}

			return 0;
		}

		//NOTE: 창 테두리를 끌어 크기를 변화시키는 경우, 최대화 버튼을 누르거나 최소화 버튼을 누르는 경우, SetWindowPos(..)를 호출한 경우
		case WM_SIZE:
		{
			switch (wParam)
			{
				case SIZE_MINIMIZED:
				{
					m_isWindowMinimized = true;
					break;
				}
				case SIZE_RESTORED:
				case SIZE_MAXIMIZED:
				{
					m_isWindowMinimized = false;
					break;
				}
			}

			if (m_isWindowResizing == true)
			{
				return 0;
			}

			m_needResetFactory = true;

			if (m_optionFullScreen.IsActive() == false)
			{
				m_optionWindowSave.SetWindowRect(m_hWnd);
			}

			return 0;
		}

		//NOTE: 창 테두리를 끌어 위치를 이동시키는 경우, 창 테두리를 끌어 크기를 변화시킬 때 좌상단 기준점이 변하는 경우,
		//		최대화 버튼이나 최소화 버튼을 누르는 경우, SetWindowPos(..)를 호출한 경우
		case WM_MOVE:
		{
			if (m_isWindowMoving == true)
			{
				return 0;
			}

			m_needResetFactory = true;

			if (m_optionFullScreen.IsActive() == false)
			{
				m_optionWindowSave.SetWindowRect(m_hWnd);
			}

			return 0;
		}

		//NOTE: NVIDIA 제어판의 해상도-주사율-GSYNC 설정이 바뀌는 경우, 윈도우 디스플레이 설정의 HDR 설정이 켜지거나 꺼지는 경우
		case WM_DISPLAYCHANGE:
		{
			m_needResetFactory = true;

			return 0;
		}
		
		//NOTE: ALT+F4 단축키를 누르는 경우, 우상단의 창 닫기 버튼을 누르는 경우, 작업바의 창 닫기 버튼을 누르는 경우,
		//		작업관리자 프로세스 종료를 누르는 경우
		case WM_CLOSE:
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;

			return 0;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);

			return 0;
		}
	}

	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}