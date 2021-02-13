#pragma once

// logging macro functions
#define    info(msg, ...) log_msg("*", msg, __VA_ARGS__)
#define   error(msg, ...) log_msg("-", msg, __VA_ARGS__)
#define success(msg, ...) log_msg("+", msg, __VA_ARGS__)

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *fmt, ...);

// initialize the logging system
bool initialize_log();
