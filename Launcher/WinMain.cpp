#include <Windows.h>
#include <inttypes.h>

#pragma comment(lib, "Winmm.lib")

#define RBOX_EXE	"C:\\Program Files\\Pioneer\\rekordbox 6.5.0\\rekordbox.exe"
#define MODULE_DLL	"C:\\Users\\danie\\source\\repos\\RekordBoxSongExporter\\x64\\Release\\SongExporterModule.dll"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	CreateProcessA(RBOX_EXE, NULL, NULL, NULL, false, 0, NULL, NULL, &si, &pi);
	HWND hWnd = NULL;
	uint32_t i = 0;
	do {
		Sleep(10000);
		hWnd = FindWindowA(NULL, "rekordbox");
		i++;
	} while (hWnd == NULL && i < 15);
	if (!hWnd) {
		MessageBoxA(NULL, "Failed to find window", "Error", 0);
		return 0;
	}
	DWORD procID = 0;
	if (!GetWindowThreadProcessId(hWnd, &procID)) {
		MessageBoxA(NULL, "Failed to resolve process id", "Error", 0);
		return 0;
	}
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, procID);
	void *remoteMem = VirtualAllocEx(hProc, NULL, 256, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteMem) {
		MessageBoxA(NULL, "Failed to allocate", "Error", 0);
		return 0;
	}
	SIZE_T out = 0;
	if (!WriteProcessMemory(hProc, remoteMem, MODULE_DLL, sizeof(MODULE_DLL), &out)) {
		MessageBoxA(NULL, "Failed to write", "Error", 0);
		return 0;
	}
	DWORD remoteThreadID = 0;
	HANDLE hRemoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, remoteMem, 0, &remoteThreadID);
	if (!hRemoteThread || WaitForSingleObject(hRemoteThread, INFINITE) != WAIT_OBJECT_0) {
		MessageBoxA(NULL, "Failed to inject module", "Error", 0);
		return 0;
	}
	// success
	PlaySound("MouseClick", NULL, SND_SYNC);
	return 0;
}
