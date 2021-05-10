#include <Windows.h>
#include <inttypes.h>

#include "Log.h"

// 13 byte push of a 64 bit value absolute via push + mov [rsp + 4]
static uintptr_t write_push64(uintptr_t addr, uintptr_t value)
{
    // push
    *(uint8_t *)(addr + 0) = 0x68;
    *(uint32_t *)(addr + 1) = (uint32_t)(value & 0xFFFFFFFF);
    // mov [rsp + 4], dword
    *(uint8_t *)(addr + 5) = 0xC7;
    *(uint8_t *)(addr + 6) = 0x44;
    *(uint8_t *)(addr + 7) = 0x24;
    *(uint8_t *)(addr + 8) = 0x04;
    *(uint32_t *)(addr + 9) = (uint32_t)((value >> 32) & 0xFFFFFFFF);
    // the number of bytes written
    return 13;
}

// 14 byte absolute jmp to 64bit dest via push + ret written to src
static uintptr_t write_jump(uintptr_t src, uintptr_t dest)
{
    // push dest and grab number of bytes written
    uintptr_t amt = write_push64(src, dest);
    // retn after the number of bytes that were used for push
    *(uint8_t *)(src + amt) = 0xC3;
    amt += sizeof(uint8_t);
    info("jmp written %p -> %p", src, dest);
    // return number of bytes written
    return amt;
}

// write several register pushes, used to backup registers before running hook
static uintptr_t write_push_registers(uintptr_t addr)
{
    *(uint8_t *)(addr + 0) = 0x51; // push rcx
    *(uint8_t *)(addr + 1) = 0x52; // push rdx
    *(uint8_t *)(addr + 2) = 0x41; // push r8
    *(uint8_t *)(addr + 3) = 0x50;
    *(uint8_t *)(addr + 4) = 0x41; // push r9
    *(uint8_t *)(addr + 5) = 0x51;
    *(uint8_t *)(addr + 6) = 0x41; // push r10
    *(uint8_t *)(addr + 7) = 0x52;
    *(uint8_t *)(addr + 8) = 0x41; // push r11
    *(uint8_t *)(addr + 9) = 0x53;
    // number of bytes written
    return 10;
}

// write several register pushes, used to backup registers before running hook
static uintptr_t write_pop_registers(uintptr_t addr)
{
    // reverse order of the push function above
    *(uint8_t *)(addr + 0) = 0x41; // pop r11
    *(uint8_t *)(addr + 1) = 0x5b;
    *(uint8_t *)(addr + 2) = 0x41; // pop r10
    *(uint8_t *)(addr + 3) = 0x5a;
    *(uint8_t *)(addr + 4) = 0x41; // pop r9
    *(uint8_t *)(addr + 5) = 0x59;
    *(uint8_t *)(addr + 6) = 0x41; // pop r8
    *(uint8_t *)(addr + 7) = 0x58;
    *(uint8_t *)(addr + 8) = 0x5a; // pop rdx
    *(uint8_t *)(addr + 9) = 0x59; // pop rcx
    // number of bytes written
    return 10;
}

// install hook at src, redirec to dest, copy trampoline_len bytes to make room for hook
// you need to pick a trampoline_len that matches the number of opcodes you want to
// move and it needs to be at least 14 bytes
bool install_hook(uintptr_t target_func, void *hook_func, size_t trampoline_len)
{
    if (!target_func || !hook_func || trampoline_len < 14) {
        return false;
    }

    // allocate trampoline
    uintptr_t trampoline_addr = (uintptr_t)VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline_addr) {
        error("Failed to allocate trampoline");
        return false;
    }
    success("Allocated trampoline: %p", (void *)trampoline_addr);

    // A variable to keep track of the current location we're writing opcodes
    // all the write functions return the number of bytes written
    uintptr_t cur_write_pos = trampoline_addr;

    // push all registers at start of shellcode because we're paranoid
    cur_write_pos += write_push_registers(cur_write_pos);

    // write a push for the retn address so our function can return back to here
    // this is needed because we use absolute jmps instead of trying to calculate
    // offsets to perform calls, which means we need to push the ret address ourself
    // 0x1B = size of the current push and next jmp
    cur_write_pos += write_push64(cur_write_pos, cur_write_pos + 0x1B);

    // write the jmp into the start of the trampoline which jumps to the hook function
    cur_write_pos += write_jump(cur_write_pos, (uintptr_t)hook_func); 

    // pop all registers after the call to the hook function
    cur_write_pos += write_pop_registers(cur_write_pos);

    // copy the opcodes from the target function which we will overwrite with a jmp
    memcpy((void *)cur_write_pos, (void *)target_func, trampoline_len);
    cur_write_pos += trampoline_len;

    // this jump will go from the end of the trampoline back to original function
    // src = after the copied bytes that form the trampoline
    // dest = after trampoline_len bytes in src function
    cur_write_pos += write_jump(cur_write_pos, target_func + trampoline_len);

    // unprotect src function so we can trash it
    DWORD oldProt = 0;
    if (!VirtualProtect((void *)target_func, 16, PAGE_EXECUTE_READWRITE, &oldProt)) {
        error("Failed to unprotect target function for hook");
        return false;
    }

    // install the meat and potatoes hook, redirect source to trampoline
    // this will overwrite rekordbox's function with a jump to our code
    // src = event play addr
    // dest = the trampoline
    write_jump(target_func, trampoline_addr);

    success("Hook Successful");

    return true;
}
