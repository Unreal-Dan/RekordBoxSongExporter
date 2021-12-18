#pragma once
#include <inttypes.h>

// Lookup a signature in a module
// ex:
//      sig = sig_scan(NULL, "\x11\x22\x33\x44\x90\x90\x90\x90\x55\x66\x77", 11);
//      0x90 is wildcard because int3 is unlikely to appear in a signature
//      NULL module means lookup in current executable
uintptr_t sig_scan(const char *module_name, const char *sig, size_t sig_len);
