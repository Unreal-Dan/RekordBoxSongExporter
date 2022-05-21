#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "Injector.h"

using namespace std;

#ifdef _WIN64
static const uint8_t sleep_loop_bytes[] = {
    // mov r10, 1000
    0x49, 0xC7, 0xC2, 0xE8, 0x03, 0x00, 0x00,
    // mov eax, 0x34
    0xB8, 0x34, 0x00, 0x00, 0x00,
    // syscall
    0x0F, 0x05,
    // jmp $-16
    0xEB, 0xF0,
};
#else
static const uint8_t sleep_loop_bytes[] = {
    // jmp $-2 busy loop
    0xEB, 0xFE,
};
#endif

Injector::Injector() :
    m_exe_path(),
    m_exe_basepath(),
    m_module_path(),
    m_process(0),
    m_thread(0),
    m_entry(0)
{
}

Injector::Injector(const std::string &exe_path, const std::string &module_path) :
    Injector()
{
    init(exe_path, module_path);
}

Injector::~Injector()
{
}

void Injector::init(const string &exe_path, const string &module_path)
{
    m_exe_path = exe_path;
    m_module_path =  module_path;
    m_exe_basepath = m_exe_path.substr(0, m_exe_path.find_last_of("\\") + 1);
}

// launch exe suspended and inject the module then resume
bool Injector::inject()
{
    // launch the process suspended
    if (!launch_suspended()) {
        MessageBoxA(NULL, "Failed to launch suspended", "Error", 0);
        return false;
    }
    // locate the entrypoint by grabbing current eip/rip
    if (!fetch_entrypoint()) {
        MessageBoxA(NULL, "Failed to locate entry", "Error", 0);
        return false;
    }
    uint8_t entry_bytes[sizeof(sleep_loop_bytes)] = {0};
    // backup the entrypoint opcodes
    if (!read_proc_mem(m_entry, (void *)entry_bytes, sizeof(sleep_loop_bytes))) {
        MessageBoxA(NULL, "Failed to backup entry", "Error", 0);
        return false;
    }
    // patch the entrypoint with an infinite loop
    if (!write_proc_mem(m_entry, (void *)sleep_loop_bytes, sizeof(sleep_loop_bytes))) {
        MessageBoxA(NULL, "Failed to patch entry", "Error", 0);
        return false;
    }
    // pause and restore original entry point
    if (ResumeThread(m_thread) == -1) {
        MessageBoxA(NULL, "Failed to suspend after injection", "Error", 0);
        return false;
    }
    // inject the module now
    if (!inject_module()) {
        MessageBoxA(NULL, "Failed to inject module", "Error", 0);
        return false;
    }
    // pause and restore original entry point
    if (SuspendThread(m_thread) == -1) {
        MessageBoxA(NULL, "Failed to suspend after injection", "Error", 0);
        return false;
    }
    // restore original entrypoint
    if (!write_proc_mem(m_entry, entry_bytes, sizeof(entry_bytes))) {
        MessageBoxA(NULL, "Failed to suspend after injection", "Error", 0);
        return false;
    }
    // resume main thread
    if (ResumeThread(m_thread) == -1) {
        MessageBoxA(NULL, "Failed to resume after injection", "Error", 0);
        return false;
    }
    CloseHandle(m_process);
    CloseHandle(m_thread);
    return true;
}

// launch process frozen
bool Injector::launch_suspended()
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    DWORD creation_flags = CREATE_SUSPENDED;
    // Create Process on the exe passed in
    if (!CreateProcess(m_exe_path.c_str(), NULL, NULL, NULL, false,
        creation_flags, NULL, m_exe_basepath.c_str(), &si, &pi)) {
        MessageBoxA(NULL, "Failed to launch exe", "Error", 0);
        return false;
    }
    m_process = pi.hProcess;
    m_thread = pi.hThread;
    return true;
}

bool Injector::fetch_entrypoint()
{
    CONTEXT context;
    context.ContextFlags = CONTEXT_CONTROL;
    GetThreadContext(m_thread, &context);
#ifdef _WIN64
    m_entry = context.Rip;
#else
    m_entry = context.Eip;
#endif
    return true;
}

// write some memory to a target process
bool Injector::read_proc_mem(uintptr_t addr, void *data, size_t amount)
{
    SIZE_T bytes_read = 0;
    DWORD old_prot = 0;
    if (!data || !amount) {
        return false;
    }
    if (!ReadProcessMemory(m_process, (void *)addr, data, amount, &bytes_read)) {
        return false;
    }
    return true;
}

// write some memory to a target process
bool Injector::write_proc_mem(uintptr_t addr, void *data, size_t amount)
{
    SIZE_T written = 0;
    DWORD old_prot = 0;
    if (!data || !amount) {
        return false;
    }
    if (!VirtualProtectEx(m_process, (void *)addr, amount, PAGE_EXECUTE_READWRITE, &old_prot)) {
        return false;
    }
    if (!WriteProcessMemory(m_process, (void *)addr, data, amount, &written)) {
        return false;
    }
    if (!VirtualProtectEx(m_process, (void *)addr, amount, old_prot, &old_prot)) {
        return false;
    }
    return true;
}

// create a string in the remote process
uintptr_t Injector::inject_string(const string &str)
{
    uintptr_t mem = (uintptr_t)VirtualAllocEx(m_process, NULL, str.length() + 1,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        MessageBoxA(NULL, "Failed to allocate memory for string", "Error", 0);
        return 0;
    }
    if (!write_proc_mem(mem, (void *)str.c_str(), str.size() + 1)) {
        MessageBoxA(NULL, "Failed to write string", "Error", 0);
        return NULL;
    }
    return mem;
}

// inject the module dll into remote proc
bool Injector::inject_module()
{
    DWORD remoteThreadID = 0;
    // allocate and write the string containing the path of the dll to exe
    uintptr_t remote_str = inject_string(m_module_path);
    if (!remote_str) {
        MessageBoxA(NULL, "Failed to inject module path", "Error", 0);
        return false;
    }
    // spawn a thread on LoadLibrary(dllPath) in exe
    HANDLE hRemoteThread = CreateRemoteThread(m_process, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA,
        (void *)remote_str, 0, &remoteThreadID);
    if (!hRemoteThread) {
        MessageBoxA(NULL, "Failed to inject module", "Error", 0);
        return false;
    }
    CloseHandle(hRemoteThread);
    return true;
}