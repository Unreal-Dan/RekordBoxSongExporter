#include <Windows.h>
#include <winternl.h>

#include "SigScan.h"
#include "Log.h"

static bool compare_sig(uintptr_t addr, const char *sig, size_t len);
static bool get_code_start_end(const char *module_name, uintptr_t *start, uintptr_t *end);

uintptr_t sig_scan(const char *module_name, const char *signature, size_t len)
{
    uintptr_t start = 0;
    uintptr_t end = 0;
    if (!signature || !len) {
        error("Null signature or len 0");
        return 0;
    }
    uintptr_t m_base = (uintptr_t)GetModuleHandle(module_name);
    if (!m_base) {
        error("Could not find module %s", module_name);
        return 0;
    }
    PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)m_base;
    PIMAGE_NT_HEADERS nt_hdr = (PIMAGE_NT_HEADERS)(m_base + dos_hdr->e_lfanew);
    if (!nt_hdr) {
        error("Null NT header");
        return false;
    }
    PIMAGE_SECTION_HEADER sec_hdr = (PIMAGE_SECTION_HEADER)((uintptr_t)nt_hdr + sizeof(IMAGE_NT_HEADERS));
    uintptr_t sec_count = 0;
    do {
        // search each .text segment of this module
        if (strncmp((char *)sec_hdr->Name, ".text", 5) == 0) {
            uintptr_t start = m_base + sec_hdr->VirtualAddress;
            uintptr_t end = start + sec_hdr->SizeOfRawData - len;
            info("Searching %p -> %p for sig", start, end);
            for (uintptr_t i = start; i < end; i++) {
                if ((i - start) >= 0x7A5310) {
                    int a = 2;
                }
                if (compare_sig(i, signature, len)) {
                    success("Found sig @ %p", (void *)i);
                    return i;
                }
            }
        }
        sec_hdr = (PIMAGE_SECTION_HEADER)((uintptr_t)sec_hdr + sizeof(IMAGE_SECTION_HEADER));
        sec_count++;
    } while(sec_count < nt_hdr->FileHeader.NumberOfSections);
    error("Could not find sig");
    return 0;
}

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

static bool get_code_start_end(const char *module_name, uintptr_t *start, uintptr_t *end)
{
   return false; 
}

