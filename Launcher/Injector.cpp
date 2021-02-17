#include <Windows.h>
#include <shlwapi.h>
#include <inttypes.h>

#include <string>

#include "Injector.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

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
    HWND hwnd = NULL;
    DWORD procID = 0;
    uint32_t i = 0;
    do {
        if (i > 0) {
            Sleep(3000);
        }
        hwnd = FindWindowA(NULL, "rekordbox");
        i++;
    } while (hwnd == NULL && i < 15);
    if (!hwnd) {
        MessageBox(NULL, "Failed to find window", "Error", 0);
        return NULL;
    }
    if (!GetWindowThreadProcessId(hwnd, &procID)) {
        MessageBox(NULL, "Failed to resolve process id", "Error", 0);
        return NULL;
    }
    return OpenProcess(PROCESS_ALL_ACCESS, false, procID);
}

// launch rekordbox
bool launch_rekordbox(string rekordbox)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    // get just the path part
    string rekordbox_path = rekordbox.substr(0, rekordbox.find_last_of("\\") + 1);
    printf("path: %s\n", rekordbox_path.c_str());
    // Create Process on the rekordbox exe passed in
    if (!CreateProcess(rekordbox.c_str(), NULL, NULL, NULL, false, 0, NULL, rekordbox_path.c_str(), &si, &pi)) {
        MessageBox(NULL, "Failed to launch rekordbox", "Error", 0);
        return false;
    }
    return true;
}

// inject the module dll into remote proc
bool inject_module(HANDLE hProc)
{
    char buffer[MAX_PATH] = { 0 };
    // grab path of current executable
    DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
    // strip off the filename
    if (!len || !PathRemoveFileSpec(buffer)) {
        return false;
    }
    // append the module name, it should fit
    strncat_s(buffer, "\\Module.dll", sizeof(buffer) - strlen(buffer));
    DWORD remoteThreadID = 0;
    // allocate and write the string containing the path of the dll to rekordbox
    void *remoteString = inject_string(hProc, buffer);
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

bool write_config(string version, bool use_artist)
{
    char path[MAX_PATH] = {0};
    GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
    PathRemoveFileSpec(path);
    string configPath = path;
    configPath += "\\config.ini";
    string raw_version = version.substr(version.find_last_of(" ") + 1);
    if (!WritePrivateProfileString("RBSongExporterConfig", "rbox_version", raw_version.c_str(), configPath.c_str()) ||
        !WritePrivateProfileString("RBSongExporterConfig", "use_artist", use_artist ? "1" : "0", configPath.c_str())) {
        MessageBoxA(NULL, "Failed to write config", "Error", 0);
        return false;
    }
    return true;
}

bool inject(string rekordbox, string version, bool use_artist)
{
    // write out the config file
    if (!write_config(version, use_artist)) {
        return false;
    }
    // spawn rekordbox
    if (!launch_rekordbox(rekordbox)) {
        return false;
    }
    // open rekordbox so we can modify it
    HANDLE hProc = find_rekordbox_window();
    if (!hProc) {
        return false;
    }
    // inject the dll module
    if (!inject_module(hProc)) {
        return false;
    }
    // success
    PlaySound("MouseClick", NULL, SND_SYNC);
    return true;
}

