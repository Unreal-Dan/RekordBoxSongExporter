#include <windows.h>
#include <inttypes.h>
#include <shlwapi.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#pragma comment(lib, "Shlwapi.lib")

// These are amount of bytes we copy out of the target hook function in order
// to allow us to overwrite the starting bytes of the function with a jump 
// to our functions below. These numbers are determined by looking at the 
// target functions assembly and picking a number of bytes that aligns on 
// the boundary of an opcode and is at least 14 (0xE) bytes for the hook itself
#define PLAY_TRACK_TRAMPOLINE_LEN       0x13
#define NOTIFY_MASTER_TRAMPOLINE_LEN    0x10

// logging macro functions
#define info(msg, ...) _log("*", msg, __VA_ARGS__)
#define error(msg, ...) _log("-", msg, __VA_ARGS__)
#define success(msg, ...) _log("+", msg, __VA_ARGS__)

// This is the number of songs that will be kept in current_tracks.txt
#define MAX_SONGS       2

// This is the file containing the last 10 songs
#define TRACK_FILE      "\\current_tracks.txt"

// this is the logfile for all songs played
#define TRACK_LOG_FILE  "\\played_tracks.txt"

// handle to the current dll itself
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// some globals to share stuff between the hooks
char *last_track = nullptr;
char *last_artist = nullptr;
// the hooks may be on separate threads so this
// protects the shared globals
CRITICAL_SECTION last_track_critsec;

// log a message with a prefix in brackets
void _log(const char *prefix, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] ", prefix);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

// determined that track title is 0x138 bytes into
struct deck_struct
{
    uint8_t pad[0x138];
    char *track_title; 
    char *track_artist; 
    char *track_genre; 
    uint8_t pad2[0x90];
};

// structure reversed out of rekordbox that appears to
// contain the track info for each deck
struct play_track_event
{
    deck_struct decks[2];
};

// a pointer of this type is passed into the eventPlay function
// we only really need the event_info contained inside
struct event_struct
{
    void *qword0;
    play_track_event *event_info;
};

std::string get_dll_path()
{
    char path[MAX_PATH];
    DWORD len = GetModuleFileName((HINSTANCE)&__ImageBase, path, sizeof(path));
    if (!len || !PathRemoveFileSpec(path)) {
        error("Failed to process file path");
    }
    return std::string(path);
}

std::string get_log_file()
{
    return get_dll_path() + TRACK_LOG_FILE;
}

std::string get_track_file()
{
    return get_dll_path() + TRACK_FILE;
}

// clears the last-10 tracks file 
void clear_track_files()
{
    FILE *f = NULL;
    if (fopen_s(&f, get_track_file().c_str(), "w") == ERROR_SUCCESS && f) {
        fclose(f);
    }
    if (fopen_s(&f, get_log_file().c_str(), "w") == ERROR_SUCCESS && f) {
        fclose(f);
    }
}

// logs a track to the global log
void log_track(const char *track, const char *artist)
{
    // log to the global log
    std::ofstream trackLogFile(get_log_file(), std::ios::app);
    trackLogFile << track << " - " << artist << "\n";
}

// updates the last-10 tracks log file
void update_track_list(const char *track, const char *artist)
{
    static std::vector<std::string> track_list;
    static std::vector<std::string> artist_list;


    // update the last 10 file
    std::string track_file = get_track_file();
    std::fstream trackFile(track_file);
    std::stringstream fileData;
    fileData << track << " - " << artist << "\n";
    fileData << trackFile.rdbuf();
    trackFile.close();
    trackFile.open(track_file, std::fstream::out | std::fstream::trunc);
    std::istringstream iss(fileData.str());
    std::string line;
    for (uint32_t i = 0; (i < MAX_SONGS) && std::getline(iss, line); i++) {
        trackFile << line << "\n";
    }
}

// dupe the track into global threadsafe last-track storage
void set_last_track(const char *track, const char *artist)
{
    if (!track || !artist) { 
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    // update track
    if (last_track) {
        free(last_track);
    }
    last_track = _strdup(track);
    // update artist
    if (last_artist) {
        free(last_artist);
    }
    last_artist = _strdup(artist);
    LeaveCriticalSection(&last_track_critsec);
}

// grab the last track out of global threadsafe last-track storage
// don't forget to free the result
void get_last_track(char **track, char **artist)
{
    if (!track || !artist) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    if (last_track) {
        *track = _strdup(last_track);
    }
    if (last_artist) {
        *artist = _strdup(last_artist);
    }
    LeaveCriticalSection(&last_track_critsec);
}

// the actual hook function that eventPlay is redirected to
void play_track_hook(event_struct *event)
{
    play_track_event *track_info = event->event_info;
    // keep track of the last tracks we had selected 
    static const char *old_track1 = nullptr;
    static const char *old_track2 = nullptr;
    // grab the current tracks we have selected
    const char *track1 = track_info->decks[0].track_title;
    const char *track2 = track_info->decks[1].track_title;
    deck_struct *new_track_deck = nullptr;
    // check if either of the tracks changed and if one did then log that 
    // track. Unfortunately if you change both tracks at the same time 
    // then press play it will always log the track that was loaded onto 
    // the second deck regardless of which deck you pressed play on.
    if (old_track1 != track1 && track1[0]) {
        new_track_deck = &track_info->decks[0];
    }
    if (old_track2 != track2 && track2[0]) {
        new_track_deck = &track_info->decks[1];
    }
    if (new_track_deck) {
        info("Playing track: %s - %s", new_track_deck->track_title, new_track_deck->track_artist);
        set_last_track(new_track_deck->track_title, new_track_deck->track_artist);
        log_track(new_track_deck->track_title, new_track_deck->track_artist);
    }
    // store current tracks for the next time play button is pressed
    old_track1 = track1;
    old_track2 = track2;
}

// the actual hook function that notifyMasterChange is redirected to
void notify_master_change_hook(event_struct *event)
{
    // 128 should be enough to tell if a track name matches right?
    static char last_notified_track[128] = {0};
    static char last_notified_artist[128] = {0};
    char *last_artist = nullptr;
    char *last_track = nullptr;
    get_last_track(&last_track, &last_artist);
    if (last_track && last_artist) {
        // make sure they didn't fade back into the same song
        if (strcmp(last_track, last_notified_track) != 0 ||
            strcmp(last_artist, last_notified_artist) != 0) {
            // yep they faded into a new song
            info("Master Changed to: %s - %s", last_track, last_artist);
            // log it to our track list
            update_track_list(last_track, last_artist);
            // store the last song for next time
            strncpy_s(last_notified_track, last_track, sizeof(last_notified_track));
            strncpy_s(last_notified_artist, last_artist, sizeof(last_notified_artist));
        }
    }
    if (last_track) free(last_track);
    if (last_artist) free(last_artist);
}

// 13 byte push of a 64 bit value absolute via push + mov [rsp + 4]
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

// 14 byte absolute jmp to 64bit dest via push + ret written to src
void write_jump(uintptr_t src, uintptr_t dest)
{
    // push dest
    write_push64(src, dest);
    // retn
    *(int32_t *)(src + 13) = 0xC3;
    info("jmp written %p -> %p", src, dest);
}

// install hook at src, redirec to dest, copy trampoline_len bytes to make room for hook
// you need to pick a trampoline_len that matches the number of opcodes you want to
// move and it needs to be at least 14 bytes
bool install_hook(uintptr_t target_func, void *hook_func, size_t trampoline_len)
{
    if (!target_func || !hook_func || trampoline_len < 14) {
        return false;
    }

    // allocate trampoline
    uintptr_t trampoline_addr = (uintptr_t)VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline_addr) {
        error("Failed to allocate trampoline");
        return false;
    }

    // write the jmp into the start of the trampoline which jumps to the hook function
    write_jump(trampoline_addr, (uintptr_t)hook_func); // 0xE bytes
    // write a push for the retn address so our function can return back to here
    // this is needed because we use absolute jmps instead of trying to calculate
    // offsets to perform calls, which means we need to push the ret address ourself
    write_push64(trampoline_addr + 0xE, trampoline_addr + 0x1B); // 0xD bytes

    void *trampoline = (void *)trampoline_addr;
    success("Allocated trampoline: %p", trampoline);

    // copy trampoline bytes
    uintptr_t trampoline_after_call = trampoline_addr + 0x1B; // 0x1B = 0xE + 0xD (the jmp + push above)
    memcpy((void *)trampoline_after_call, (void *)target_func, trampoline_len);
    info("Copied trampoline bytes");

    // the beginning of the return from the trampoline
    uintptr_t trampoline_ret_start = trampoline_after_call + trampoline_len;
    // this jump will go from the end of the trampoline back to original function
    // src = after the copied bytes that form the trampoline
    // dest = after trampoline_len bytes in src function
    write_jump(trampoline_ret_start, target_func + trampoline_len);

    // unprotect src function so we can trash it
    DWORD oldProt = 0;
    VirtualProtect((void *)target_func, 16, PAGE_EXECUTE_READWRITE, &oldProt);

    // install the meat and potatoes hook, redirect source to trampoline
    // src = event play addr
    // dest = the trampoline
    write_jump(target_func, trampoline_addr);

    success("Hook Successful");

    return true;
}

bool hook_event_play()
{
    // determine address of target function to hook
    uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t ep_addr = base + 0x908D70;
    info("event_play_func: %p + 0x908D70 = %p", base, ep_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(ep_addr, play_track_hook, PLAY_TRACK_TRAMPOLINE_LEN)) {
        error("Failed to install hook on event play");
        return false;
    }
    return true;
}

bool hook_notify_master_change()
{
    // determine address of target function to hook
    uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t nmc_addr = base + 0x1772d70;
    info("notify_master_change: %p + 0x1772d70 = %p", base, nmc_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(nmc_addr, notify_master_change_hook, NOTIFY_MASTER_TRAMPOLINE_LEN)) {
        error("Failed to install hook on event play");
        return false;
    }
    return true;
}

DWORD mainThread(void *param)
{
    FILE *con = NULL;
    if (!AllocConsole() || freopen_s(&con, "CONOUT$", "w", stdout) != 0) {
        error("Failed to initialize console");
        return 1;
    }
    success("Initialized console");

    clear_track_files();

    info("Cleared track files");

    info("log file: %s", get_log_file().c_str());
    info("track file: %s", get_track_file().c_str());

    // setup the critical section because our two hooks will be on 
    // different threads and they will be sharing data
    // we don't clean this up because we're assholes, we don't provide
    // a way to inject more than one of this dll so it's fine
    InitializeCriticalSection(&last_track_critsec);

    // wait to let rekordbox completely open, hooking too soon can cause problems
    Sleep(3000);

    // hook when we press the play/cue button and a new track has been loaded
    if (!hook_event_play()) {
        return 1;
    }

    // hook when the master changes
    if (!hook_notify_master_change()) {
        return 1;
    }

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
