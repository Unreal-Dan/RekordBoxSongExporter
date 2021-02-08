#include <windows.h>
#include <inttypes.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#define info(msg, ...) _log("*", msg, __VA_ARGS__)
#define error(msg, ...) _log("-", msg, __VA_ARGS__)
#define success(msg, ...) _log("+", msg, __VA_ARGS__)

#define TRACK_FILE "C:\\Users\\danie\\Documents\\current_tracks.txt"
#define TRACK_LOG_FILE "C:\\Users\\danie\\Documents\\played_tracks.txt"

void _log(const char *prefix, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	printf("[%s] ", prefix);
	vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

struct play_track_event
{
    uint8_t pad[0x138];
	char *track_title; // 0x140
    uint8_t pad2[0x1D8];
    char *track2_title;
};

struct event_struct
{
  void *qword0;
  play_track_event *event_info;
};

void clear_track_file()
{
    FILE *f = NULL;
    if (fopen_s(&f, TRACK_FILE, "w") == ERROR_SUCCESS && f) {
		fclose(f);
    }
}

void log_track(const char *track)
{
    // log to the global log
	std::ofstream trackLogFile(TRACK_LOG_FILE, std::ios::app);
    trackLogFile << track << "\n";
    // update the last 10 file
	std::fstream trackFile(TRACK_FILE);
	std::stringstream fileData;
    fileData << track << "\n";
	fileData << trackFile.rdbuf();
	trackFile.close();
	trackFile.open(TRACK_FILE, std::fstream::out | std::fstream::trunc);
	std::istringstream iss(fileData.str());
    std::string line;
    for (uint32_t i = 0; (i < 10) && std::getline(iss, line); i++) {
        trackFile << line << "\n";
	}
}

void play_track_hook(event_struct *event)
{
    play_track_event *track_info = event->event_info;
    static const char *old_track1 = nullptr;
    static const char *old_track2 = nullptr;
    const char *track1 = track_info->track_title;
    const char *track2 = track_info->track2_title;
    const char *new_track = nullptr;
    if (old_track1 != track1 && track1[0]) {
        new_track = track1;
    }
    if (old_track2 != track2 && track2[0]) {
        new_track = track2;
    }
    if (new_track) {
		info("Playing track: %s", new_track);
        log_track(new_track);
    }
    old_track1 = track1;
    old_track2 = track2;
}

// 13 byte push 64 bit value absolute
void write_push64(uintptr_t addr, uintptr_t value)
{
    // push
    *(uint8_t *)(addr + 0) = 0x68;
    *(uint32_t *)(addr + 1) = (uint32_t)(value & 0xFFFFFFFF);
    // mov [rsp + 4], dword
    *(uint8_t *)(addr + 5) = 0xC7;
    *(uint8_t *)(addr + 6) = 0x44;
    *(uint8_t *)(addr + 7) = 0x24;
    *(uint8_t *)(addr + 8) = 0x04;
    *(uint32_t *)(addr + 9) = (uint32_t)((value >> 32) & 0xFFFFFFFF);
}

// 14 byte absolute jmp to 64bit dest written to src
void write_jump(uintptr_t src, uintptr_t dest)
{
    // push dest
    write_push64(src, dest);
    // retn
    *(int32_t *)(src + 13) = 0xC3;
    info("jmp written %p -> %p", src, dest);
}

DWORD mainThread(void *param)
{
    uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t event_play_addr = base + 0x908D70;
    void *event_play_func = (void *)event_play_addr;
    FILE *con = NULL;
    if (!AllocConsole() || freopen_s(&con, "CONOUT$", "w", stdout) != 0) {
        error("Failed to initialize console");
        return 1;
    }
    success("Initialized console");

    clear_track_file();

    info("Cleared track file");

    Sleep(3000);

    info("event_play_func: %p + 0x908D70 = %p", base, event_play_func);

    // allocate trampoline
    uintptr_t trampoline_addr = (uintptr_t)VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline_addr) {
        error("Failed to allocate trampoline");
        return 1;
    }

    // write the indirect call to our func
    write_jump(trampoline_addr, (uintptr_t)play_track_hook); // 0xE bytes
    // write the retn address
    write_push64(trampoline_addr + 0xE, trampoline_addr + 0x1B); // 0xD bytes

    void *trampoline = (void *)trampoline_addr;
    success("Allocated trampoline: %p", trampoline);

#define TRAMPOLINE_LEN 0x13

    // copy trampoline bytes
    uintptr_t trampoline_after_call = trampoline_addr + 0x1B;
    memcpy((void *)trampoline_after_call, event_play_func, TRAMPOLINE_LEN);
    info("Copied trampoline bytes");

    // src = 5 bytes after the ret at end of tramp
    // dest = after patched bytes in event play
    uintptr_t trampoline_ret_start = trampoline_after_call + TRAMPOLINE_LEN;
    write_jump(trampoline_ret_start, event_play_addr + TRAMPOLINE_LEN);

    // unprotect event play func
    DWORD oldProt = 0;
    VirtualProtect(event_play_func, 16, PAGE_EXECUTE_READWRITE, &oldProt);

    // src = event play addr
    // dest = the trampoline
    write_jump(event_play_addr, trampoline_addr);

    success("Success");

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
