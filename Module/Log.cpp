#include <Windows.h>
#include <stdio.h>

#include <string>

#include "Config.h"
#include "Hook.h"
#include "Log.h"

using namespace std;

// console handle
HANDLE out_handle = NULL;
// output file log
HANDLE log_handle = NULL;
// path of outfile
string log_path;

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *func, const char *fmt, ...)
{
    char final_buffer[4096] = { 0 };
    char buffer[512] = { 0 };
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);

    size_t len = snprintf(final_buffer, sizeof(final_buffer), "[%s] %s%s%s\n", prefix, func ? func : "", func ? "(): " : "", buffer);

#ifdef _DEBUG
    // print to console with color
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(out_handle, &info);
    switch (prefix[0]) {
    case '+':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY|FOREGROUND_GREEN);
        break;
    case '-':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY|FOREGROUND_RED);
        break;
    case '*':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY);
    default:
        break;
    }
    printf("%s", final_buffer);
    SetConsoleTextAttribute(out_handle, info.wAttributes);
#endif
    // open the log if not already open
    if (!log_handle && log_path.length() > 0) {
        log_handle = CreateFile(log_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (!log_handle) {
            error("Failed to open log: %d", GetLastError());
            return;
        }
        // truncate
        SetEndOfFile(log_handle);
    }
    if (log_handle) {
        DWORD written = 0;
        if (WriteFile(log_handle, final_buffer, len, &written, NULL)) {
            return;
        }
        FlushFileBuffers(log_handle);
    }
}

// initialize a console for logging
bool initialize_log()
{
#ifdef _DEBUG
    FILE *con = NULL;
    if (!AllocConsole() || freopen_s(&con, "CONOUT$", "w", stdout) != 0) {
        error("Failed to initialize console");
        return false;
    }
    out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    success("Initialized console");
#endif
    log_path = get_dll_path() + "\\log.txt";
    info("Log path: %s", log_path.c_str());
    return true;
}
