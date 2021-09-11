#include <Windows.h>
#include <stdio.h>

#include <string>

#include "Log.h"

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *func, const char *fmt, ...)
{
    char buffer[512] = { 0 };
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);
    printf("[%s] %s%s%s\n", prefix, func ? func : "", func ? "(): " : "", buffer);
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
    success("Initialized console");
#endif
    return true;
}
