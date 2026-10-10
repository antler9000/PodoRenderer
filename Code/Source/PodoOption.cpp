#define NOMINMAX

//Podo
#include "Podo.h"

//Win32
#include <windows.h>

//C++
#include <algorithm>
#include <string>
#include <fstream>
#include <cstdio>

void Podo::OptionSave()
{
	std::ofstream fout("SavedSettingsTemp.txt");

	if (!fout)
	{
		return;
	}

	fout << "FullScreen"		<< " " << (m_optionFullScreen.userEnabled	? "Yes" : "No") << '\n';
	fout << "WindowSavePosX"	<< " " << ((int)m_optionWindowSave.posX)					<< '\n';
	fout << "WindowSavePosY"	<< " " << ((int)m_optionWindowSave.posY)					<< '\n';
	fout << "WindowSaveWidth"	<< " " << ((int)m_optionWindowSave.width)					<< '\n';
	fout << "WindowSaveHeight"	<< " " << ((int)m_optionWindowSave.height)					<< '\n';
	fout << "VSync"				<< " " << (m_optionVSync.userEnabled		? "Yes" : "No") << '\n';
	fout << "HDR"				<< " " << (m_optionHDR.userEnabled			? "Yes" : "No") << '\n';
	fout << "GUIMasterSize"		<< " " << (m_optionGUI.masterSize)							<< '\n';

	fout.close();

	if (fout.fail() == true)
	{
		return;
	}

	BOOL moveResult = MoveFileExW(L"SavedSettingsTemp.txt", L"SavedSettings.txt", MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	if (moveResult == 0)
	{
		return;
	}
}

void Podo::OptionRestore()
{
	std::ifstream fin("SavedSettings.txt");

	if (!fin)
	{
		return;
	}

	bool result = true;

	bool	fullScreenEnabledTemp	= false;
	int		windowSavePosXTemp		= 0;
	int		windowSavePosYTemp		= 0;
	int		windowSaveWidthTemp		= 1600;
	int		windowSaveHeightTemp	= 900;
	bool	vSyncEnabledTemp		= false;
	bool	hdrEnabledTemp			= false;
	int		guiMasterSizeTemp		= 0;

	result &= OptionReadBool(fin, "FullScreen", fullScreenEnabledTemp);
	result &= OptionReadInt(fin, "WindowSavePosX", windowSavePosXTemp);
	result &= OptionReadInt(fin, "WindowSavePosY", windowSavePosYTemp);
	result &= OptionReadInt(fin, "WindowSaveWidth", windowSaveWidthTemp);
	result &= OptionReadInt(fin, "WindowSaveHeight", windowSaveHeightTemp);
	result &= OptionReadBool(fin, "VSync", vSyncEnabledTemp);
	result &= OptionReadBool(fin, "HDR", hdrEnabledTemp);
	result &= OptionReadInt(fin, "GUIMasterSize", guiMasterSizeTemp);

	if (result == false)
	{
		return;
	}

	m_optionFullScreen.userEnabled		= fullScreenEnabledTemp;
	m_optionWindowSave.posX				= (LONG)windowSavePosXTemp;
	m_optionWindowSave.posY				= (LONG)windowSavePosYTemp;
	m_optionWindowSave.width			= (LONG)windowSaveWidthTemp;
	m_optionWindowSave.height			= (LONG)windowSaveHeightTemp;
	m_optionVSync.userEnabled			= vSyncEnabledTemp;
	m_optionHDR.userEnabled				= hdrEnabledTemp;
	m_optionGUI.masterSize				= std::clamp(guiMasterSizeTemp, 50, 150);
}

bool Podo::OptionReadBool(std::ifstream& fin, std::string optionName, bool& outOptionEnabled)
{
	std::string buffer;

	fin >> buffer;
	if (buffer != optionName)
	{
		return false;
	}

	fin >> buffer;
	if (buffer == "Yes")
	{
		outOptionEnabled = true;
	}
	else if (buffer == "No")
	{
		outOptionEnabled = false;
	}
	else
	{
		return false;
	}

	return true;
}

bool Podo::OptionReadInt(std::ifstream& fin, std::string optionName, int& outOptionValue)
{
	std::string buffer;

	fin >> buffer;
	if (buffer != optionName)
	{
		return false;
	}
	fin >> outOptionValue;

	return true;
}