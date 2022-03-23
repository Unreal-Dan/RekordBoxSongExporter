#include <Windows.h>
#include <winternl.h>

#include "SigScan.h"
#include "Hook.h"
#include "Log.h"

static bool compare_sig(uintptr_t addr, const char *sig, size_t len);
static char *convert_sig(const char *signature, size_t *out_len);
static uintptr_t __fastcall hook_callback(hook_arg_t hook_arg, func_args *args);

uintptr_t sig_scan(const char *signature, const char *module_name)
{
    uintptr_t start = 0;
    uintptr_t end = 0;
    size_t len = 0;
    if (!signature || !*signature) {
        error("Null or empty signature");
        return 0;
    }
    uintptr_t m_base = (uintptr_t)GetModuleHandle(module_name);
    if (!m_base) {
        error("Could not find module %s: %d", module_name ? module_name : "base", GetLastError());
        return 0;
    }
    PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)m_base;
    PIMAGE_NT_HEADERS nt_hdr = (PIMAGE_NT_HEADERS)(m_base + dos_hdr->e_lfanew);
    if (!nt_hdr) {
        error("Null NT header");
        return 0;
    }
    char *real_sig = convert_sig(signature, &len);
    if (!real_sig) {
        error("Could not convert signature %s", signature);
        return 0;
    }
    PIMAGE_SECTION_HEADER sec_hdr = (PIMAGE_SECTION_HEADER)((uintptr_t)nt_hdr + sizeof(IMAGE_NT_HEADERS));
    uintptr_t sec_count = 0;
    do {
        // search each .text segment of this module
        if (strncmp((char *)sec_hdr->Name, ".text", 5) == 0) {
            uintptr_t start = m_base + sec_hdr->VirtualAddress;
            uintptr_t end = start + sec_hdr->SizeOfRawData - len;
            info("Searching %p -> %p for sig", start, end);
            // iterate each byte of section and check for signature match
            for (uintptr_t i = start; i < end; i++) {
                if (compare_sig(i, real_sig, len)) {
                    success("Found sig @ %p", (void *)i);
                    free(real_sig);
                    return i;
                }
            }
        }
        sec_hdr = (PIMAGE_SECTION_HEADER)((uintptr_t)sec_hdr + sizeof(IMAGE_SECTION_HEADER));
        sec_count++;
    } while(sec_count < nt_hdr->FileHeader.NumberOfSections);
    error("Could not find sig");
    free(real_sig);
    return 0;
}

#ifdef _DEBUG
// useful for debugging and hooking functions to see what they do
Hook *hook_sig(const char *name, const char *signature, hook_callback_fn func)
{
    uintptr_t sig = sig_scan(signature);
    if (!sig) {
        error("Failed to find sig %s", name);
        return NULL;
    }
    Hook *hook = Hook::instant_hook(sig, func ? func : hook_callback, (hook_arg_t)name);
    if (!hook) {
        error("Failed to hook sig %s", name);
        return NULL;
    }
    success("Hooked %s", name);
    return hook;
}

static uintptr_t __fastcall hook_callback(hook_arg_t hook_arg, func_args *args)
{
    info("%s(%p, %p, %p, %p)", (const char *)hook_arg, args->arg1, args->arg2, args->arg3, args->arg4);
    return 0;
}
#endif

// compare a raw signature with memory
static bool compare_sig(uintptr_t addr, const char *sig, size_t len)
{
    do {
        // if sig byte doesn't match address, and sig byte isn't a wildcard
        if (*(uint8_t *)sig != *(uint8_t *)addr && *sig != '\x90') {
            // then sig doesn't match
            return false;
        }
        // otherwise step sig/addr forward for next byte comparison
        addr++;
        sig++;
        len--;
    } while (len > 0);
    return true;
}

// converts a string containing a readable sig to a raw signature
// "aa BB cc 12" -> "\xAA\xBB\xCC\x12"
// ignores extraneous spaces
static char *convert_sig(const char *signature, size_t *out_len)
{
    size_t len = strlen(signature);
    if (!len) {
        return NULL;
    }
    char *result = (char *)calloc(len / 2, sizeof(char));
    if (!result) {
        return NULL;
    }
    size_t j = 0;
    size_t k = 0;
    for (size_t i = 0; i < len; ++i) {
        char letter = signature[i];
        if (letter == ' ') {
            continue;
        }
        // whether the letter is on an odd/even index
        size_t odd = (k % 2);
        // is it a wildcard?
        if (letter == '?') {
            // if it's a wildcard the raw wildcard byte is 0x90
            // so we set the letter to either 9 or 0 based on odd/even
            letter = odd ? 0x0 : 0x9;
        } else if (letter >= '0' && letter <= '9') {
            letter -= '0';
        } else {
            // lowercase the letter
            letter |= 0x20;
            if (letter >= 'a' && letter <= 'z') {
                letter -= ('a' - 0xA);
            } else {
                // invalid letter!
            }
        }
        result[j] |= letter << (4 * !odd);
        j += odd;
        k++;
    }
    *out_len = j;
    return result;
}
