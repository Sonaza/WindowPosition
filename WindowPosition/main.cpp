#include "includewindows.h"
#include "log.h"

#include <vector>
#include <string>
#include <cstdio>
#include <inttypes.h>
#include <sstream>

struct WindowAlignment
{
	double left;
	double right;
	double top;
	double bottom;
};

std::string lastProgramErrorString = "";

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
		dprintf("Failed to get handle\n");
	}

	return std::string(buffer);
}

bool iequals(const std::string &a, const std::string &b)
{
	return std::equal(a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
		return tolower(a) == tolower(b);
	});
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

		int windowX = int((alignment.left / 100.0) * screenWidth) + monitorInfo.rcWork.left;
		int windowY = int((alignment.top / 100.0) * screenHeight) + monitorInfo.rcWork.top;
		int windowWidth = int(((alignment.right - alignment.left) / 100.0) * screenWidth + 0.5);
		int windowHeight = int(((alignment.bottom - alignment.top) / 100.0) * screenHeight + 0.5);

		dprintf("New pos/size: (%d, %d) (%d, %d)\n", windowX, windowY, windowWidth, windowHeight);

		SetWindowPos(window, nullptr, windowX, windowY, windowWidth, windowHeight, SWP_NOZORDER | SWP_NOOWNERZORDER);
	}
	else
	{
		dprintf("Failed to retrieve monitor data.\n");
	}
}

struct EnumWindowData
{
	std::string processName;
	std::string windowTitle;
	std::string className;

	HWND resultHwnd;
};

BOOL CALLBACK enumWindowCallback(HWND hwnd, LPARAM lParam)
{
	EnumWindowData &data = *(EnumWindowData*)lParam;

	if (!IsWindowVisible(hwnd))
		return true;

	const std::string currentProcessName = getProcessName(hwnd);

	dprintf("\n0x%08" PRIX64 " : %s\n", (ptrdiff_t)hwnd, currentProcessName.c_str());

	if (!iequals(currentProcessName, data.processName))
		return true;

	char titleBuffer[1024] = {};
	if (GetWindowTextA(hwnd, titleBuffer, 1024) == 0)
		return true;

	dprintf("  Title    : %s\n", titleBuffer);

	if (!data.windowTitle.empty())
	{
		if (!iequals(titleBuffer, data.windowTitle))
			return true;
	}

	if (!data.className.empty())
	{
		char classBuffer[1024] = {};
		if (GetClassNameA(hwnd, classBuffer, 1024) == 0)
			return true;

		dprintf("  Class    : %s\n", classBuffer);

		if (!iequals(classBuffer, data.className))
			return true;
	}

	dprintf("  FOUND THINGALING!\n");
	data.resultHwnd = hwnd;

	return false; // false halts the EnumWindows
}

HWND findWindow(const std::string &processName, const std::string &windowTitle, const std::string &className)
{
	EnumWindowData data = {};
	data.processName = processName;
	data.windowTitle = windowTitle;
	data.className = className;
	EnumWindows(enumWindowCallback, (LPARAM)&data);
	
	if (data.resultHwnd != nullptr)
		return data.resultHwnd;

	lastProgramErrorString = "Window process with module name " + processName + " not found.";
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
	dprintf("Usage: WindowPosition.exe <monitor index> <left offset> <right offset> <top offset> <bottom offet> <process name> [window title] [window class]\n\n");
	
	dprintf("  Monitor index declares which monitor the window will be placed on.\n");
	dprintf("  The value should be 1 or higher and not exceed the number of monitors connected.\n\n");

	dprintf("  Offsets are described in absolute percentages between [0-100].\n");
	dprintf("  For example setting left to 50 and right to 100 aligns\n");
	dprintf("  a window on the right side half portion of the screen.\n");
	dprintf("  Floating point values are also allowed for more fine tuned control.\n\n");

	dprintf("  The process name is the executable file name running on the computer.\n");
	dprintf("  Optionally a window title and a window class can be specified to focus the search.\n\n");

	dprintf("\n");
}

void printErrorString(const std::string &errorStr)
{
	dprintf("Error: %s\n", errorStr.c_str());

	if (!lastProgramErrorString.empty())
	{
		dprintf("       %s\n\n", lastProgramErrorString.c_str());
	}
	else
	{
		dprintf("\n");
	}
}

double todouble(const char *s)
{
	std::stringstream ss;
	double out;
	ss << s;
	ss >> out;
	return out;
}

int main(int argc, char *argv[])
{
	if (argc != 7 && argc != 8 && argc != 9)
	{
		printUsageInstructions();
		return 0;
	}

	for (int i = 0; i < argc; i++)
	{
		dprintf("%d : %s\n", i, argv[i]);
	}

	std::string processName = argv[6];
	
	std::string windowTitle;
	if (argc >= 8) windowTitle = argv[7];

	std::string className;
	if (argc >= 9) className = argv[8];

	HWND window = findWindow(processName, windowTitle, className);
	if (window == nullptr)
	{
		printErrorString("A window for specified process name couldn't be found.");
		printUsageInstructions();
		return 1;
	}

	size_t monitorIndex = std::atoi(argv[1]);
	if (monitorIndex == 0)
	{
		printErrorString("Invalid monitor index.");
		printUsageInstructions();
		return 4;
	}
	monitorIndex = monitorIndex - 1;

	std::vector<HMONITOR> monitors;
	if (EnumDisplayMonitors(nullptr, nullptr, &monitorEnumProc, (LPARAM)&monitors) == 0)
	{
		printErrorString("EnumDisplayMonitors failed.");
		printUsageInstructions();
		return 2;
	}

	if (monitors.empty())
	{
		printErrorString("Could not retrieve any monitors.");
		printUsageInstructions();
		return 3;
	}

	if (monitorIndex >= monitors.size())
	{
		printErrorString("Monitor index exceeds the number of available monitors.");
		printUsageInstructions();
		return 4;
	}

	WindowAlignment wa;
	wa.left   = todouble(argv[2]);
	wa.right  = todouble(argv[3]);
	wa.top    = todouble(argv[4]);
	wa.bottom = todouble(argv[5]);

	dprintf("WTF %0.3f %0.3f %0.3f %0.3f\n", wa.left, wa.right, wa.top, wa.bottom);

	if ((wa.left   < 0.0 || wa.left   > 100.0) ||
		(wa.right  < 0.0 || wa.right  > 100.0) ||
		(wa.top    < 0.0 || wa.top    > 100.0) ||
		(wa.bottom < 0.0 || wa.bottom > 100.0))
	{
		printErrorString("Position offsets exceed the percentage value range [0-100].");
		printUsageInstructions();
		return 5;
	}

	if ((wa.left >= wa.right) || (wa.top >= wa.bottom))
	{
		printErrorString("Position offsets leave no window area.");
		printUsageInstructions();
		return 5;
	}

	resizeWindow(window, monitors[monitorIndex], wa);

	return 0;
}