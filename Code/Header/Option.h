#pragma once
#define NOMINMAX

//Podo
#include "Debug.h"

//Win32
#include <windows.h>

//C++
#include <format>
#include <string>

struct OptionFullScreen
{
	bool userEnabled = false;

	bool IsActive() const
	{
		return IsSupported() && userEnabled;
	}

	bool IsSupported() const
	{
		return true;
	}

	void SetUserEnabled(bool setting)
	{
		if ((setting == true) && (IsSupported() == true))
		{
			userEnabled = true;
		}
		else
		{
			userEnabled = false;
		}
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		OutputDebugStringW(userEnabled ? L"[PODO DEBUG] FullScreen: 유저 활성화 On\n" : L"[PODO DEBUG] FullScreen: 유저 활성화 off\n");
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => FullScreen On \n" : L"[PODO DEBUG] => FullScreen Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};

struct OptionWindowSave
{
	LONG posX	= 0;
	LONG posY	= 0;
	LONG width	= 1600;
	LONG height	= 900;

	bool IsActive() const
	{
		return IsSupported();
	}

	bool IsSupported() const
	{
		return true;
	}

	void SetWindowRect(HWND hWnd)
	{
		RECT windowRect = {};
		ThrowIfFalse(GetWindowRect(hWnd, &windowRect));

		LONG newPosX	= windowRect.left;
		LONG newPosY	= windowRect.top;
		LONG newWidth	= windowRect.right - windowRect.left;
		LONG newHeight	= windowRect.bottom - windowRect.top;

		if ((10 < newWidth) && (10 < newHeight))
		{
			posX	= newPosX;
			posY	= newPosY;
			width	= newWidth;
			height	= newHeight;
		}
	}

	LONG GetWindowPosX() const
	{
		return posX;
	}

	LONG GetWindowPosY() const
	{
		return posY;
	}

	LONG GetWindowWidth() const
	{
		return width;
	}

	LONG GetWindowHeight() const
	{
		return height;
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		std::wstring posXString = std::format(L"[PODO DEBUG] WindowSave: PosX {}\n", posX);
		std::wstring posYString = std::format(L"[PODO DEBUG] WindowSave: PosY {}\n", posY);
		std::wstring widthString = std::format(L"[PODO DEBUG] WindowSave: Width {}\n", width);
		std::wstring heightString = std::format(L"[PODO DEBUG] WindowSave: Height {}\n", height);
		OutputDebugStringW(posXString.c_str());
		OutputDebugStringW(posYString.c_str());
		OutputDebugStringW(widthString.c_str());
		OutputDebugStringW(heightString.c_str());
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => WindowSave On \n" : L"[PODO DEBUG] => WindowSave Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};

struct OptionVSync
{
	bool userEnabled = false;

	bool IsActive() const
	{
		return IsSupported() && userEnabled;
	}

	bool IsSupported() const
	{
		return true;
	}

	void SetUserEnabled(bool setting)
	{
		if ((setting == true) && (IsSupported() == true))
		{
			userEnabled = true;
		}
		else
		{
			userEnabled = false;
		}
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		OutputDebugStringW(userEnabled ? L"[PODO DEBUG] VSync: 유저 활성화 On\n" : L"[PODO DEBUG] VSync: 유저 활성화 off\n");
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => VSync On \n" : L"[PODO DEBUG] => VSync Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};

struct OptionTearing
{
	bool featureSupported = false;

	bool IsActive() const
	{
		return IsSupported();
	}

	bool IsSupported() const
	{
		return featureSupported;
	}

	void SetFeatureSupported(bool setting)
	{
		featureSupported = setting;
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		OutputDebugStringW(featureSupported ? L"[PODO DEBUG] Tearing: 피처 지원 On\n" : L"[PODO DEBUG] Tearing: 피처 지원 off\n");
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => Tearing On \n" : L"[PODO DEBUG] => Tearing Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};

struct OptionHDR
{
	bool outputSupported		= false;
	bool formatSupported		= false;
	bool colorSpaceSupported	= false;
	bool userEnabled			= false;

	bool IsActive() const
	{
		return IsSupported() && userEnabled;
	}

	bool IsSupported() const
	{
		return outputSupported && formatSupported && colorSpaceSupported;
	}

	void SetOutputSupported(bool setting)
	{
		outputSupported = setting;
		if (setting == false)
		{
			userEnabled = false;
		}
	}

	void SetFormatSupported(bool setting)
	{
		formatSupported = setting;
		if (setting == false)
		{
			userEnabled = false;
		}
	}

	void SetColorSpaceSupported(bool setting)
	{
		colorSpaceSupported = setting;
		if (setting == false)
		{
			userEnabled = false;
		}
	}

	void SetUserEnabled(bool setting)
	{
		if ((setting == true) && (IsSupported() == true))
		{
			userEnabled = true;
		}
		else
		{
			userEnabled = false;
		}
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		OutputDebugStringW(outputSupported ? L"[PODO DEBUG] HDR: 아웃풋 지원 On\n" : L"[PODO DEBUG] HDR: 아웃풋 지원 off\n");
		OutputDebugStringW(formatSupported ? L"[PODO DEBUG] HDR: 포맷 지원 On\n" : L"[PODO DEBUG] HDR: 포맷 지원 off\n");
		OutputDebugStringW(colorSpaceSupported ? L"[PODO DEBUG] HDR: 색 공간 지원 On\n" : L"[PODO DEBUG] HDR: 색 공간 지원 off\n");
		OutputDebugStringW(userEnabled ? L"[PODO DEBUG] HDR: 유저 활성화 On\n" : L"[PODO DEBUG] HDR: 유저 활성화 off\n");
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => HDR On \n" : L"[PODO DEBUG] => HDR Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};

struct OptionGUI
{
	int masterSize = 100;

	bool IsActive() const
	{
		return IsSupported();
	}

	bool IsSupported() const
	{
		return true;
	}

	void SetMasterSize(int setting)
	{
		masterSize = setting;
	}

	void SetMasterScale(float setting)
	{
		masterSize = static_cast<int>(setting) * 100;
	}

	int GetMasterSize() const
	{
		return masterSize;
	}
	
	float GetMasterScale() const
	{
		return static_cast<float>(masterSize) / 100;
	}

	void DebugPrint() const
	{
#ifdef _DEBUG
		std::wstring masterSizeString = std::format(L"[PODO DEBUG] GUI: 마스터 사이즈 {}%\n", masterSize);
		OutputDebugStringW(masterSizeString.c_str());
		OutputDebugStringW(IsActive() ? L"[PODO DEBUG] => GUI On \n" : L"[PODO DEBUG] => GUI Off \n");
		OutputDebugStringW(L"\n");
#endif
	}
};