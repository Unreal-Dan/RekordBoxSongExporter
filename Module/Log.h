#pragma once

#ifdef _DEBUG
// logging macro functions
#define    info(msg, ...) log_msg("*", NULL, msg, __VA_ARGS__)
#define   error(msg, ...) log_msg("-", __FUNCTION__, msg, __VA_ARGS__)
#define success(msg, ...) log_msg("+", NULL, msg, __VA_ARGS__)
#else
#define    info(msg, ...)
#define   error(msg, ...)
#define success(msg, ...)
#endif

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *func, const char *fmt, ...);

// initialize the logging system
bool initialize_log();
