#pragma once
#include <inttypes.h>

// install hook at src, redirec to dest, copy trampoline_len bytes to make room for hook
// you need to pick a trampoline_len that matches the number of opcodes you want to
// move and it needs to be at least 14 bytes
bool install_hook(uintptr_t target_func, void *hook_func, size_t trampoline_len);
