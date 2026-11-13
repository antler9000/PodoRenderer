#pragma once

//Podo
#include "Debug.h"

//Win32
#include "windows.h"

//DXGI
#include <dxgi1_6.h>
#include <dxgi1_4.h>
#include <dxgi.h>
#include <dxgicommon.h>
#include <dxgiformat.h>

struct IsAdapterChanged
{
public:

	bool operator()(IDXGIAdapter3* newAdapter)
	{
		DXGI_ADAPTER_DESC newAdapterDesc = {};
		ThrowIfFailed(newAdapter->GetDesc(&newAdapterDesc));
		if ((oldAdapterLuid.HighPart != newAdapterDesc.AdapterLuid.HighPart) || (oldAdapterLuid.LowPart != newAdapterDesc.AdapterLuid.LowPart))
		{
			oldAdapterLuid = newAdapterDesc.AdapterLuid;

			return true;
		}
		else
		{
			return false;
		}
	}

private:

	LUID oldAdapterLuid = {};
};

struct IsOutputChanged
{
public:

	bool operator()(DXGI_OUTPUT_DESC1 newOutputDesc)
	{
		if (oldOutputColorSpace != newOutputDesc.ColorSpace)
		{
			oldOutputColorSpace = newOutputDesc.ColorSpace;
			
			return true;
		}
		else
		{
			return false;
		}
	}

private:

	DXGI_COLOR_SPACE_TYPE oldOutputColorSpace = {};
};

struct IsRTVFormatChanged
{
public:

	bool operator()(DXGI_FORMAT newRTVFormat)
	{
		if (oldRTVFormat != newRTVFormat)
		{
			oldRTVFormat = newRTVFormat;

			return true;
		}
		else
		{

			return false;
		}
	}

private:

	DXGI_FORMAT oldRTVFormat = {};
};