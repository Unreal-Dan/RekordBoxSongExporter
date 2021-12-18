#include <Windows.h>
#include <shlwapi.h>
#include <inttypes.h>

#include <string>

#include "Injector.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// write a string into a remote process
static void *inject_string(HANDLE hProc, const char *str);
// look for the rekordbox window and open it
static HANDLE find_rekordbox_window(uint32_t retry_count);
// launch rekordbox
static bool launch_rekordbox(const string &rekordbox);
// inject the module dll into remote proc
static bool inject_module(HANDLE hProc);

// optionally load rekordbox and inject the module then play an annoying sound
bool inject(const string &rekordbox)
{
    // see if rekordbox is already running, only check once no retries
    HANDLE hProc = find_rekordbox_window(0);
    if (!hProc) {
        // if not try to launch rekordbox
        if (!launch_rekordbox(rekordbox)) {
            return false;
        }
        // retry 15 times to see if we can find rekordbox window now
        hProc = find_rekordbox_window(15);
        if (!hProc) {
            return false;
        }
    }
    // inject the dll module
    if (!inject_module(hProc)) {
        return false;
    }

    // this ding is driving me nuts debugging so it's a release only feature now
#ifndef _DEBUG
    // give them a success ding so they feel like something happened
    PlaySound("MouseClick", NULL, SND_SYNC);
#endif
    return true;
}

// write a string into a remote process
static void *inject_string(HANDLE hProc, const char *str)
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
static HANDLE find_rekordbox_window(uint32_t retry_count)
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
    } while (hwnd == NULL && i < retry_count);
    if (!hwnd) {
        // not found
        return NULL;
    }
    if (!GetWindowThreadProcessId(hwnd, &procID)) {
        MessageBox(NULL, "Failed to resolve process id", "Error", 0);
        return NULL;
    }
    return OpenProcess(PROCESS_ALL_ACCESS, false, procID);
}

// launch rekordbox
static bool launch_rekordbox(const string &rekordbox)
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
static bool inject_module(HANDLE hProc)
{
    char buffer[MAX_PATH] = { 0 };
    // grab path of current executable
    DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
    // strip off the filename
    if (!len || !PathRemoveFileSpec(buffer)) {
        MessageBoxA(NULL, "Failed to resolve module path", "Error", 0);
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