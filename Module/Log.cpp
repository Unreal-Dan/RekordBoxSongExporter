#include <Windows.h>
#include <stdio.h>

#include <string>

#include "Log.h"

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("[%s] ", prefix);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

// initialize a console for logging
bool initialize_log()
{
    FILE *con = NULL;
    if (!AllocConsole() || freopen_s(&con, "CONOUT$", "w", stdout) != 0) {
        error("Failed to initialize console");
        return false;
    }
    success("Initialized console");
    return true;
}
