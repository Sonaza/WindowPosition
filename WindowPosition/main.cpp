#include "Windows.h"
#include "psapi.h"
#pragma comment(lib, "psapi.lib")

#include <string>
#include <thread>
#include <chrono>
#include <cstdio>

std::string getLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string();

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

	std::string message(messageBuffer, size);

	LocalFree(messageBuffer);

	return message;
}

std::string getProcessName(HWND hWnd)
{
	TCHAR buffer[MAX_PATH] = { 0 };
	DWORD dwProcId = 0;

	GetWindowThreadProcessId(hWnd, &dwProcId);

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
	if (hProc != nullptr)
	{
		GetModuleBaseNameA(hProc, nullptr, buffer, MAX_PATH);

// 		HMODULE hModule = nullptr;
// 		DWORD needed = 0;
// 
// 		if (EnumProcessModules(hProc, &hModule, sizeof(hModule), &needed))
// 		{
// 			GetModuleBaseNameA(hProc, nullptr, buffer, MAX_PATH);
// 		}

		CloseHandle(hProc);
	}
	else
	{
		printf("Failed to handle\n");
	}

	return std::string(buffer);
}

void resizeWindow(HWND windowToSave, int windowWidth)
{
	if (windowToSave == nullptr)
		return;

	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);

	HMONITOR monitor = MonitorFromWindow(windowToSave, MONITOR_DEFAULTTOPRIMARY);
	if (GetMonitorInfoA(monitor, &monitorInfo))
	{
// 		int windowWidth = 1120;
		int windowHeight = monitorInfo.rcWork.bottom;
		int windowX = monitorInfo.rcWork.right - windowWidth;
		int windowY = 0;

		SetWindowPos(windowToSave, nullptr, windowX, windowY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOOWNERZORDER);
	}
}

HWND findWindowFromProcessName(const std::string &processName)
{
	HWND topWindow = GetTopWindow(nullptr);
	HWND currentWindow = topWindow;

	if (currentWindow == nullptr)
	{
		printf("Window is null");
		return nullptr;
	}

	do
	{
		if (IsWindowVisible(currentWindow))
		{
			char buffer[1024] = { 0 };
			if (GetWindowTextA(currentWindow, buffer, 1024) > 0)
			{
				if (getProcessName(currentWindow) == processName)
				{
					printf("0x%04X : %s\n", (ptrdiff_t)currentWindow, buffer);
					printf("   %s\n", processName.c_str());

					return currentWindow;
				}
			}
		}

		currentWindow = GetNextWindow(currentWindow, GW_HWNDNEXT);
	} while (currentWindow != nullptr && currentWindow != topWindow);

	return nullptr;
}

int main()
{
	std::string processToSave = "Spotify.exe";

	HWND windowToSave = findWindowFromProcessName(processToSave);

	resizeWindow(windowToSave, 1120);

	return 0;
}