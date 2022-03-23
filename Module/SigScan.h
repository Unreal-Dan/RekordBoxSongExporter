#pragma once
#include <inttypes.h>

#include "Hook.h"

// Lookup a signature in current exe (or specific module
// ex:
//      sig = sig_scan("11 22 33 44 ?? ?? ?? ?? 55 66 77");
//      ?? is wildcard byte
uintptr_t sig_scan(const char *sig, const char *module_name = NULL);

#ifdef _DEBUG
// hook a signature with a generic callback that prints the name and first 4 args
// useful for debugging and hooking functions to see what they do
Hook *hook_sig(const char *name, const char *signature, hook_callback_fn func = NULL);
#endif
