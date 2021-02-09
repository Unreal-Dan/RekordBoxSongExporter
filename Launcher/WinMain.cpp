#include <Windows.h>
#include <inttypes.h>

#pragma comment(lib, "Winmm.lib")

#define RBOX_EXE   "C:\\Program Files\\Pioneer\\rekordbox 6.5.0\\rekordbox.exe"
#define MODULE_DLL "C:\\Users\\danie\\source\\repos\\RekordBoxSongExporter\\x64\\Release\\Module.dll"

// write a string into a remote process
void *inject_string(HANDLE hProc, const char *str)
{
    size_t size = strlen(str) + 1;
    void *remoteMem = VirtualAllocEx(hProc, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        MessageBox(NULL, "Failed to allocate memory for string", "Error", 0);
        return NULL;
    }
    SIZE_T out = 0;
    if (!WriteProcessMemory(hProc, remoteMem, str, size, &out)) {
        MessageBox(NULL, "Failed to write string", "Error", 0);
        return NULL;
    }
    return remoteMem;
}

// look for the rekordbox window and open it
HANDLE find_rekordbox_window()
{
    HWND hWnd = NULL;
    DWORD procID = 0;
    uint32_t i = 0;
    do {
        if (i > 0) {
            Sleep(3000);
        }
        hWnd = FindWindowA(NULL, "rekordbox");
        i++;
    } while (hWnd == NULL && i < 15);
    if (!hWnd) {
        MessageBox(NULL, "Failed to find window", "Error", 0);
        return NULL;
    }
    if (!GetWindowThreadProcessId(hWnd, &procID)) {
        MessageBox(NULL, "Failed to resolve process id", "Error", 0);
        return NULL;
    }
    return OpenProcess(PROCESS_ALL_ACCESS, false, procID);
}

// launch rekordbox
bool launch_rekordbox()
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    if (!CreateProcessA(RBOX_EXE, NULL, NULL, NULL, false, 0, NULL, NULL, &si, &pi)) {
        MessageBox(NULL, "Failed to launch rekordbox", "Error", 0);
        return false;
    }
    return true;
}

// inject the module dll into remote proc
bool inject_module(HANDLE hProc)
{
    DWORD remoteThreadID = 0;
    // allocate and write the string containing the path of the dll to rekordbox
    void *remoteString = inject_string(hProc, MODULE_DLL);
    if (!remoteString) {
        return false;
    }
    // spawn a thread on LoadLibrary(dllPath) in rekordbox
    HANDLE hRemoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, remoteString, 0, &remoteThreadID);
    if (!hRemoteThread || WaitForSingleObject(hRemoteThread, INFINITE) != WAIT_OBJECT_0) {
        MessageBoxA(NULL, "Failed to inject module", "Error", 0);
        return false;
    }
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    // spawn rekordbox
    if (!launch_rekordbox()) {
        return 1;
    }
    // open rekordbox so we can modify it
    HANDLE hProc = find_rekordbox_window();
    if (!hProc) {
        return 1;
    }
    // inject the dll module
    if (!inject_module(hProc)) {
        return 1;
    }
    // success
    PlaySound("MouseClick", NULL, SND_SYNC);
    return 0;
}
