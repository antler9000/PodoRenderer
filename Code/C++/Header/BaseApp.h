#pragma once
#define NOMINMAX

//Podo
#include "Debug.h"

//Win32
#include <windows.h>

//NOTE: WindowProc에서 생성할 pThis 객체의 클래스가 자식 클래스일 수 있도록 템플릿 타입 매개변수를 사용함
template <class DerivedApp>
class BaseApp
{
protected:

	BaseApp(const wchar_t* pAppName, HINSTANCE hInstance, int nCmdShow) : m_pAppName(pAppName), m_hInstance(hInstance)
	{
		InitWindow(nCmdShow);
	}

	//NOTE:	예외에 의해 소멸된 후 소멸된 객체의 HandleMessage(..) 메서드가 창 프로시저에 의해 호출되지 않도록 창을 삭제함
	//		그 후 DestoryWindow(..)가 WM_DESTROY를 거쳐 나온 WM_QUIT 메시지가 이후 예외 메시지 박스를 종료시키지 않도록 소진시킴
	~BaseApp()
	{
		if (m_hWnd == nullptr)
		{
			return;
		}

		DestroyWindow(m_hWnd);

		MSG msg = {};
		while (PeekMessage(&msg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE))
		{

		}
	}

	//NOTE: 앱의 기본 생성자, 복사, 이동을 금지함
	BaseApp() = delete;
	BaseApp(const BaseApp& sourceApp) = delete;
	BaseApp(BaseApp&& sourceApp) noexcept = delete;
	BaseApp& operator=(const BaseApp& sourceApp) = delete;
	BaseApp& operator=(BaseApp&& sourceApp) = delete;

protected:

	const wchar_t*	m_pAppName	= {};

	HINSTANCE		m_hInstance	= {};
	HWND			m_hWnd		= {};

private:

	//NOTE:	static 메서드는 객체에 얽힌 메서드가 아니기에 this가 없으므로, 멤버 변수에 접근할 수가 없음
	//		이를 해결하기 위해 CreatWindowEx의 마지막 매개변수, SetWindowLongPtr 함수, GetWindowLongPtr 함수를 이용함
	static LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DerivedApp* pThis = nullptr;

		if (uMsg == WM_CREATE)
		{
			CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = static_cast<DerivedApp*>(pCreateStruct->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
		}
		else
		{
			LONG_PTR pUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
			pThis = reinterpret_cast<DerivedApp*>(pUserData);
		}

		if (pThis != nullptr)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

private:

	void InitWindow(int show)
	{
		ThrowIfFalse(m_haveNotCreated);

		m_haveNotCreated = false;

		const wchar_t* windowClassName = L"Window Class";

		WNDCLASS wc = {};
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = BaseApp::WindowProcedure;
		wc.hInstance = m_hInstance;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = windowClassName;

		ThrowIfFalse(RegisterClass(&wc));

		DerivedApp* pThis = static_cast<DerivedApp*>(this);

		m_hWnd = CreateWindowEx
		(
			0,
			windowClassName,
			m_pAppName,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			m_hInstance,
			(LPVOID)pThis	//NOTE: WindowProcedure에서 이 객체에 접근할 수 있도록 하기 위함
		);

		ThrowIfNull(m_hWnd);
	}

private:

	inline static bool	m_haveNotCreated = true;
};