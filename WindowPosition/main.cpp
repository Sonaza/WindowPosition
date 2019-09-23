#include "Windows.h"
#include "psapi.h"
#pragma comment(lib, "psapi.lib")

#include <vector>
#include <string>
#include <cstdio>

struct WindowAlignment
{
	int left;
	int right;
	int top;
	int bottom;
};

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
		CloseHandle(hProc);
	}
	else
	{
		printf("Failed to handle\n");
	}

	return std::string(buffer);
}

void resizeWindow(HWND window, HMONITOR monitor, WindowAlignment alignment)
{
	if (window == nullptr)
		return;

	// Get a default monitor
	if (monitor == nullptr)
		monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);

	if (monitor == nullptr)
		return;

	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfoA(monitor, &monitorInfo))
	{
		int screenWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
		int screenHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

		int windowX = int((alignment.left / 100.f) * screenWidth) + monitorInfo.rcWork.left;
		int windowY = int((alignment.top / 100.f) * screenHeight) + monitorInfo.rcWork.top;
		int windowWidth = int(((alignment.right - alignment.left) / 100.f) * screenWidth + 0.5f);
		int windowHeight = int(((alignment.bottom - alignment.top) / 100.f) * screenHeight + 0.5f);

		SetWindowPos(window, nullptr, windowX, windowY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOOWNERZORDER);
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

BOOL CALLBACK monitorEnumProc(HMONITOR monitor, HDC dc, LPRECT rekt, LPARAM userdata)
{
	std::vector<HMONITOR> &monitors = *(std::vector<HMONITOR>*)userdata;
	monitors.push_back(monitor);
	return true;
}

void printUsageInstructions()
{
	printf("Usage: WindowPosition.exe <process name> <monitor index> <left offset> <right offset> <top offset> <bottom offet>\n\n");
	
	printf("  Process name is the executable file name running on the computer.\n\n");
	
	printf("  Monitor index declares which monitor the window will be placed on.\n");
	printf("  The value should be 1 or higher and not exceed the number of monitors connected.\n\n");

	printf("  Offsets are described in absolute percentages between [0-100].\n");
	printf("  For example setting left to 50 and right to 100 aligns\n");
	printf("  a window on the right side half portion of the screen.\n\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	if (argc < 7)
	{
		printUsageInstructions();
		return 0;
	}

	std::string processName = argv[1];
	HWND window = findWindowFromProcessName(processName);
	if (window == nullptr)
	{
		printf("Error: a window for specified process name couldn't be found.\n\n");
		printUsageInstructions();
		return 1;
	}

	size_t monitorIndex = std::atoi(argv[2]);
	if (monitorIndex == 0)
	{
		printf("Error: Invalid monitor index.\n\n");
		printUsageInstructions();
		return 4;
	}
	monitorIndex = monitorIndex - 1;

	std::vector<HMONITOR> monitors;
	if (EnumDisplayMonitors(nullptr, nullptr, &monitorEnumProc, (LPARAM)&monitors) == 0)
	{
		printf("Error: EnumDisplayMonitors failed.\n\n");
		printUsageInstructions();
		return 2;
	}

	if (monitors.empty())
	{
		printf("Error: Could not retrieve any monitors.\n\n");
		printUsageInstructions();
		return 3;
	}

	if (monitorIndex >= monitors.size())
	{
		printf("Error: Monitor index exceeds the number of available monitors.\n\n");
		printUsageInstructions();
		return 4;
	}

	WindowAlignment wa;
	wa.left   = std::atoi(argv[3]);
	wa.right  = std::atoi(argv[4]);
	wa.top    = std::atoi(argv[5]);
	wa.bottom = std::atoi(argv[6]);

	if ((wa.left < 0   || wa.left > 100)  ||
		(wa.right < 0  || wa.right > 100) ||
		(wa.top < 0    || wa.top > 100)   ||
		(wa.bottom < 0 || wa.bottom > 100))
	{
		printf("Error: Position offsets exceed the percentage value range [0-100].\n\n");
		printUsageInstructions();
		return 5;
	}

	if ((wa.left >= wa.right) || (wa.top >= wa.bottom))
	{
		printf("Error: Position offsets leave no window area.\n\n");
		printUsageInstructions();
		return 5;
	}

	resizeWindow(window, monitors[monitorIndex], wa);

	return 0;
}