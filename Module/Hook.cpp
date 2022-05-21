#include <Windows.h>
#include <inttypes.h>

#include "Hook.h"
#include "Log.h"

#include <capstone/x86.h>

// sizes of various assembly operations on each architecture to simplify
// calculations of assembly thunk sizes
#ifdef _WIN64 // x64
#define IP_REG          X86_REG_RIP // capstone constants for RIP/RSP
#define SP_REG          X86_REG_RSP
#define CS_MODE         CS_MODE_64  // capstone mode 64
#define PUSH_SIZE       13          // push dword; mov rsp + 4 dword
#define SET_CX_SIZE     10          // movabs rcx, ID
#define SET_DX_SP_SIZE  3           // mov rdx, rsp
#define PUSHAD_SIZE     10          // push rax; push rcx; push rdx...
#define POPAD_SIZE      10          // popd rax; pop rcx; pop rdx...
#define SUB_SP_SIZE     4           // sub rsp, xx
#define ADD_SP_SIZE     4           // add rsp, xx
#define RETN_SIZE       1           // ret
#else // x86
#define IP_REG          X86_REG_EIP
#define SP_REG          X86_REG_ESP
#define CS_MODE         CS_MODE_32  // capstone mode 32
#define PUSH_SIZE       5           // push dword
#define SET_CX_SIZE     5           // mov ecx, 0x11223344
#define SET_DX_SP_SIZE  5           // mov edx, esp, add edx, 0x24
#define PUSHAD_SIZE     8           // push ecx, push edx, push ebx
#define POPAD_SIZE      8           // pop ecx, pop edx, pop ebx
#define SUB_SP_SIZE     3           // sub esp, xx
#define ADD_SP_SIZE     3           // add esp, xx
#define RETN_SIZE       3           // ret x
#endif

// absolute jump is push then ret
#define ABS_JUMP_SIZE   (PUSH_SIZE + 1)
// relative jump is just jmp dword offset
#define REL_JUMP_SIZE   5

Hook::Hook() :
    m_capstone_handle(),
    m_arg(0),
    m_old_bytes(),
    m_target_func(0),
    m_hook_func(0),
    m_trampoline(0),
    m_asm_len(0),
    m_trampoline_len(0),
    m_ret_amt(0),
    m_num_conditionals(0),
    m_initialized(false),
    m_hooked(false)
{
    if (cs_open(CS_ARCH_X86, CS_MODE, &m_capstone_handle) == CS_ERR_OK) {
        cs_option(m_capstone_handle, CS_OPT_DETAIL, CS_OPT_ON);
    }
}

Hook::Hook(uintptr_t target_func, hook_callback_fn hook_func, hook_arg_t arg) :
    Hook()
{
    init(target_func, hook_func, arg);
}

Hook::~Hook()
{
    cleanup_hook();
    cs_close(&m_capstone_handle);
}

void Hook::init(uintptr_t target_func, hook_callback_fn hook_func, hook_arg_t arg)
{
    m_target_func = target_func;
    m_hook_func = (uintptr_t)hook_func;
    m_arg = arg;
}

// install hook at target func and redirect to hook func and pass arg to hook func
bool Hook::install_hook(bool do_callback, uint32_t num_args)
{
    if (!m_target_func || !m_hook_func) {
        return false;
    }

    // the amount to correct stack if user specified it
    m_ret_amt = (uint16_t)(num_args * sizeof(uintptr_t));

    // calculate how many bytes to move to ensure we are on an opcode boundary
    if (!calculate_asm_length()) {
        error("Failed to calculate relocatable asm length");
        return false;
    }

    // copy out the old bytes so we can restore them with unhook
    m_old_bytes.assign((uint8_t *)m_target_func, (uint8_t *)m_target_func + m_asm_len);

    // allocate the trampoline and write out it's contents
    if (!create_trampoline(do_callback)) {
        error("Failed to allocate trampoline");
        return false;
    }

    // hook is now initialized and able to rehook/unhook'd
    m_initialized = true;

    // hook trampoline is initialized, just call rehook to install the actual hook
    if (!rehook()) {
        return false;
    }

    return true;
}

// unhook the hook component without cleaning up
bool Hook::unhook()
{
    if (!m_initialized) {
        return false;
    }
    if (!m_hooked) {
        return true;
    }
    DWORD old_prot = 0;
    // unprotect src function so we can trash it
    if (!VirtualProtect((void *)m_target_func, m_asm_len, PAGE_EXECUTE_READWRITE, &old_prot)) {
        error("Failed to unprotect target function for unhook");
        return false;
    }
    // restore old bytes
    memcpy((void *)m_target_func, m_old_bytes.data(), m_old_bytes.size());
    // restore the protections
    if (!VirtualProtect((void *)m_target_func, m_asm_len, old_prot, &old_prot)) {
        error("Failed to restore protection on target after unhook");
        return false;
    }
    m_hooked = false;
    return true;
}

// install the actual meat and potatoes of the hook
bool Hook::rehook()
{
    if (!m_initialized) {
        return false;
    }
    if (m_hooked) {
        return true;
    }
    DWORD old_prot = 0;
    // unprotect src function so we can trash it
    if (!VirtualProtect((void *)m_target_func, m_asm_len, PAGE_EXECUTE_READWRITE, &old_prot)) {
        error("Failed to unprotect target function for hook");
        return false;
    }
    // install our hook
    uintptr_t offset = write_relative_jump(m_target_func, m_trampoline);
    // set the rest of the bytes to nops
    memset((void *)(m_target_func + offset), 0x90, (size_t)(m_asm_len - offset));
    // restore the protections
    if (!VirtualProtect((void *)m_target_func, m_asm_len, old_prot, &old_prot)) {
        error("Failed to restore protection on target function");
        return false;
    }
    m_hooked = true;
    return true;
}

void Hook::cleanup_hook()
{
    unhook();
    if (m_trampoline) {
        VirtualFree((void *)m_trampoline, 0, MEM_RELEASE);
    }
    m_arg = 0;
    m_old_bytes.clear();
    m_target_func = 0;
    m_hook_func = 0;
    m_trampoline = 0;
    m_asm_len = 0;
    m_trampoline_len = 0;
    m_num_conditionals = 0;
    m_initialized = false;
}

// ==================
// private functions

// calculates the minimum number of bytes we need to relocate
bool Hook::calculate_asm_length()
{
    // disassemble first 1024 bytes of the target, this could probably be done
    // more in a efficient manner with some sort of loop
    cs_insn* instruction_info;
    size_t count = cs_disasm(m_capstone_handle, (uint8_t *)m_target_func,
        1024, m_target_func, 0, &instruction_info);
    // iterate instructions and count bytes til the hook will fit (big_enough = true)
    bool big_enough = false;
    // also attempt to look for a ret x opcode to decode the number of args of stdcall
    // but only do that if the user didn't specify a number of bytes already
    bool found_ret = (m_ret_amt != 0);
    // if we enter padding, don't exit into the next func
    bool in_padding = false;
    int32_t disp = 0;
    for (uint32_t i = 0; i < count; i++) {
        cs_insn* curins = (cs_insn*)&instruction_info[i];
        cs_x86 *x86 = &(curins->detail->x86);
        // if we leave padding do not count the byte
        if (in_padding && curins->bytes[0] != 0xCC) {
            break;
        }
        // count opcodes till reaching enough bytes to fit a relative jump
        if (!big_enough) {
            m_asm_len += curins->size;
            if (m_asm_len >= REL_JUMP_SIZE) {
                big_enough = true;
            }
        }
        // look for conditional jumps like je xxx or jz xxx because we will need to bounce them
        // through proxy jumps since they won't be able to reach from the trampoline location
        if (is_conditional_jump(curins->bytes, curins->size)) {
            // each conditional jump adds ABS_JUMP_SIZE bytes to the end of the trampoline
            m_num_conditionals++;
        }
        // if haven't found ret yet
        if (!found_ret) {
            // if the opcode is 'ret' then this isn't stdcall or the args are 0
            if (curins->bytes[0] == 0xc3) {
                m_ret_amt = 0;
                found_ret = true;
            } else if (curins->bytes[0] == 0xC2) {
                // otherwise if it's 'ret x' then decode the number of args to clean off the stack
                memcpy(&m_ret_amt, curins->bytes + 1, sizeof(uint16_t));
                found_ret = true;
            }
        }
        // once inside padding, don't leave
        if (!in_padding && curins->bytes[0] == 0xCC) {
            in_padding = true;
        }
        // if we found enough opcodes to fit the hook, and we fount a ret opcode then we're done
        if (big_enough && found_ret) {
            break;
        }
    }
    if (!big_enough) {
        error("Only counted %u bytes needed %u", m_asm_len, REL_JUMP_SIZE);
        m_asm_len = 0;
    }
    cs_free(instruction_info, count);
    // found_ret is opportunistic, it's ok if we didn't find one
    return big_enough;
}

uintptr_t Hook::allocate_within_2gb_up(uintptr_t base, size_t size)
{
    MEMORY_BASIC_INFORMATION info;
    SYSTEM_INFO sysinfo;
    memset(&sysinfo, 0, sizeof(sysinfo));
    memset(&info, 0, sizeof(info));
    GetSystemInfo(&sysinfo);
    uintptr_t app_end = (uintptr_t)sysinfo.lpMaximumApplicationAddress;
    size_t fail_count = 0;
    for (uintptr_t ptr = base; ptr < app_end; ptr = (uintptr_t)info.BaseAddress + 1) {
        if (!VirtualQuery((void *)ptr, &info, sizeof(info))) {
            break;
        }
        if (info.State != MEM_FREE || info.RegionSize < size) {
            continue;
        }
        // VirtualAlloc requires 64k aligned addresses
        uintptr_t page_base = (uintptr_t)info.BaseAddress & ~0xffff;
        uintptr_t mem = (uintptr_t)VirtualAlloc((void*)page_base, size,
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (mem) {
            return mem;
        }
        fail_count++;
    }
    if (fail_count > 0) {
        // Allocation commonly fails, just search for another region
        error("Failed allocation up attempt %zu times", fail_count);
    }
    return 0;
}

uintptr_t Hook::allocate_within_2gb_down(uintptr_t base, size_t size)
{
    MEMORY_BASIC_INFORMATION info;
    SYSTEM_INFO sysinfo;
    memset(&sysinfo, 0, sizeof(sysinfo));
    memset(&info, 0, sizeof(info));
    GetSystemInfo(&sysinfo);
    uintptr_t app_base = (uintptr_t)sysinfo.lpMinimumApplicationAddress;
    size_t fail_count = 0;
    for (uintptr_t ptr = base; ptr > app_base; ptr = (uintptr_t)info.BaseAddress - 1) {
        if (!VirtualQuery((void *)ptr, &info, sizeof(info))) {
            break;
        }
        if (info.State != MEM_FREE || info.RegionSize < size) {
            continue;
        }
        // VirtualAlloc requires 64k aligned addresses
        uintptr_t page_base = (uintptr_t)info.BaseAddress & ~0xffff;
        uintptr_t mem = (uintptr_t)VirtualAlloc((void*)page_base, size,
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (mem) {
            return mem;
        }
        fail_count++;
    }
    if (fail_count > 0) {
        // Allocation commonly fails, just search for another region
        error("Failed allocation down attempt %zu times", fail_count);
    }
    return 0;
}


uintptr_t Hook::allocate_within_2gb(uintptr_t base_addr, size_t size)
{
    uintptr_t mem = 0;
    uint32_t tries = 3;
    // This is subject to race conditions and always will be, so we try again!
    do {
        mem = allocate_within_2gb_down(base_addr, size);
        if (mem) {
            return mem;
        }
        mem = allocate_within_2gb_up(base_addr, size);
        if (mem) {
            return mem;
        }
    } while (!mem && tries++ < 3);
    return 0;
}

bool Hook::create_trampoline(bool do_callback)
{
    // calculate total trampoline size based on:
    //   backup registers
    //   setup user callback args
    //   call user callback
    //   restore registers
    //   if !do_callback
    //      retn
    //   else
    //      run relocated asm
    //      perform jump back
    m_trampoline_len = PUSHAD_SIZE +     // push all registers
                       SET_CX_SIZE +     // set ecx/rcx to ID
                       SET_DX_SP_SIZE +  // set edx/rdx to esp/rsp
#ifdef _WIN64
                       SUB_SP_SIZE +     // need to make room on stack on x64
#endif
                       PUSH_SIZE +       // push return value
                       ABS_JUMP_SIZE +   // abs jump to detour func
#ifdef _WIN64
                       ADD_SP_SIZE +     // need to make room on stack on x64
#endif
                       POPAD_SIZE;      // pop all registers

    // if we're doing the callback into the original function for the user
    if (do_callback) {
        // if any of the relocated asm has condition jmps then we need to make
        // proxy jmps at the end of the trampoline
        uintptr_t proxy_jmp_area = (uintptr_t)m_num_conditionals * ABS_JUMP_SIZE;
        // if we're doing the callback then we need to relocate the asm into the trampoline
        // and perform an absolute jump back. The relocated asm needs a conditional jump
        // area at the end of it to handle proxy jumps back to the original function because
        // the conditional relative jumps that were moved can't jump that far
        m_trampoline_len += (uint32_t)((uintptr_t)m_asm_len +  // size of the relocated opcodes
                                       ABS_JUMP_SIZE +         // the size of abs jump back
                                       proxy_jmp_area);         // space for proxy jmps back
    } else {
        // otherwise if we're not doing the callback we will just return
        m_trampoline_len += RETN_SIZE;
    }

    // allocate trampoline near the target
    m_trampoline = allocate_within_2gb(m_target_func, m_trampoline_len);
    if (!m_trampoline) {
        error("Failed to allocate %u byte trampoline near %p: %d",
            m_trampoline_len, m_target_func, GetLastError());
        return false;
    }

    // write pos in trampoline and make sure we wrote the expected amount
    if (write_trampoline(m_trampoline, do_callback) != m_trampoline_len) {
        error("Trampoline size incorrect");
        return false;
    }

    return true;
}

uintptr_t Hook::write_trampoline(uintptr_t pos, bool do_callback)
{
    uintptr_t start_pos = pos;
    // push all registers
    pos += write_push_registers(pos);
    // set rcx/ecx to hook arg
    pos += write_set_cx(pos, m_arg);
    // set rdx/edx to rsp/esp
    pos += write_set_dx_sp(pos);
#ifdef _WIN64
    // adjust the stack on x64
    pos += write_sub_sp(pos, 40);
#endif
    // calculate return address for the call to user callback
    uintptr_t ret_addr = pos + PUSH_SIZE + ABS_JUMP_SIZE;
    // push ret_addr for simulated call to user callback
    pos += write_push(pos, ret_addr);
    // do absolute jump (simulated call) to user callback
    pos += write_absolute_jump(pos, m_hook_func);
#ifdef _WIN64
    // restore stack on x64
    pos += write_add_sp(pos, 40);
#endif
    // pop all registers
    pos += write_pop_registers(pos);
    // if the user opted to not have the callback automatically called
    if (!do_callback) {
        // then just return
        pos += write_retn(pos, m_ret_amt);
        return pos - start_pos;
    }
    // otherwise relocate the first
    uintptr_t proxy_jumps_size = (uintptr_t)m_num_conditionals * ABS_JUMP_SIZE;
    // the conditional area is where absolute jumps are placed back to the
    // destination of conditional jumps
    uintptr_t conditional_area = m_trampoline_len - proxy_jumps_size;
    // relocate m_asm_len bytes of asm from m_target_func to trampoline
    pos += relocate_asm(pos, m_target_func, m_asm_len, conditional_area);
    // write the jmp at the end of the trampoline back to target func
    pos += write_absolute_jump(pos, m_target_func + m_asm_len);
    // add space for conditionals
    pos += proxy_jumps_size;
    return pos - start_pos; // return size of trampoline
}

// This is used in relocating the asm from target to trampoline, sometimes there could be a call in the
// first opcode which fetches the address of EIP/RIP into a specific register. This function will detect
// a call to a function which has something like: mov/lea reg, [rip + x]
// The reason this is problematic is because the EIP/RIP will be different if the call is relocated, so
// instead of relocating the call the opcode will just be fixed up
bool Hook::detect_ip_retrieval(cs_insn *curins, uintptr_t from, uintptr_t to, x86_reg *fixup_reg)
{
    cs_insn *instructioninfo;
    if (!curins) {
        return false;
    }
    cs_x86 *x86 = &(curins->detail->x86);
    cs_x86_op *op = &(x86->operands[0]);
    uintptr_t offset = (to - from);
    size_t count = cs_disasm(m_capstone_handle,
            (uint8_t *)(uintptr_t)(op->imm - offset), 16,
            (uintptr_t)op->imm - offset, 0, &instructioninfo);

    if (count < 2) {
        return false;
    }

    cs_x86 *op1 = &(instructioninfo[0].detail->x86);

    if (op1->op_count != 2) {
        return false;
    }
    if (op1->operands[0].type != X86_OP_REG) {
        return false;
    }
    if (op1->operands[1].type != X86_OP_MEM) {
        return false;
    }
    if (op1->operands[1].mem.base != SP_REG) {
        return false;
    }
    if (op1->operands[1].mem.scale != 1) {
        return false;
    }
    if (op1->operands[1].mem.disp != 0) {
        return false;
    }

    // the register that is the destination of esp/rsp
    if (fixup_reg) {
        *fixup_reg = op1->operands[0].reg;
    }

    return true;
}

// relocate a chunk of assembly to a new location and fixup the opcodes (from/to cannot overlap)
uintptr_t Hook::relocate_asm(uintptr_t to, uintptr_t from, uintptr_t amount, uintptr_t conditional_area)
{
    cs_insn *instructioninfo;
    uint8_t *code = (uint8_t *)from;
    // copy the real fn bytes to relocated location
    memcpy((void *)to, code, amount);
    // disassemble the relocated asm and walk it to fix it up
    size_t count = cs_disasm(m_capstone_handle, (uint8_t *)to, amount, to, 0, &instructioninfo);
    uint32_t i = 0;
    uint32_t j = 0;
    // need to track the number of conditional jumps because they get
    // pointed to a table of proxy jumps at the end of the trampoline
    // and we need to know which index to point at in the table
    uint32_t num_conditionals = 0;

    x86_reg fixup_reg = X86_REG_INVALID;
    intptr_t difference = (to - from);

    for (i = 0; i < count; i++) {
        cs_insn *curins = (cs_insn*)&instructioninfo[i];
        cs_x86 *x86 = &(curins->detail->x86);
        for (j = 0; j < x86->op_count; j++) {
            cs_x86_op *op = &(x86->operands[j]);
            if (op->type == X86_OP_MEM) {
                // we need to detect memory accesses like
                //   lea rcx, [rip+0xBAD]
                // because that offset needs to be adjusted
                // We even have a 'fixup reg' which could be any register like
                // ecx, eax, edx, which was detected to contain rip and as such
                // any instructions like:
                //   lea rcx, [eax + 0xBAD]
                // need to be fixed up because 'eax' was detected as containing 'eip'
                if (op->mem.base == X86_REG_INVALID) {
                    continue;
                }
                // if we're not relative to the instruction pointer
                // and we don't have a fixup reg or the fixup reg isn't our mem base
                if (op->mem.base != IP_REG && (fixup_reg == X86_REG_INVALID || op->mem.base != fixup_reg)) {
                    // then this opcode is probably something like:
                    //   lea ecx, [eax + 0x8]
                    // which is just an object member access or something
                    continue;
                }
                // then relocate this because it's something like
                //   lea rcx, [rip + 0xbad]
                // or
                //   call get_rip_in_eax
                //   lea rcx, [eax + 0xbad]
                // in both cases the offset refers to an address relative to the instruction pointer
                if (!relocate(curins, difference, x86->encoding.disp_size, x86->encoding.disp_offset)) {
                    error("Failed to relocate %p", curins->address);
                }
            } else if (op->type == X86_OP_IMM) {
                // imm types are like call 0xdeadbeef
                // exclude types like sub rsp,0x20
                if (x86->op_count > 1 && x86->operands[0].reg != fixup_reg && fixup_reg != X86_REG_INVALID) {
                    continue;
                }
                char *mnemonic = curins->mnemonic;
                // if this is just a condition jmp, not a call, then just relocate the jmp
                if (is_conditional_jump(curins->bytes, curins->size)) {
                    // walk through the proxy jump table with each conditional jump
                    uintptr_t cond_ptr = to + conditional_area + ((uintptr_t)num_conditionals * ABS_JUMP_SIZE);
                    relocate_conditional_jmp(curins, cond_ptr, amount, from, to, x86->encoding.imm_size, x86->encoding.imm_offset);
                    num_conditionals++;
                    continue;
                }
                // types like push 0x20 slip through, check mnemonic for 'call' or 'jmp'
                // probably more types than just these, update list as they're found
                if (fixup_reg == X86_REG_INVALID && strcmp(mnemonic, "call") != 0 && strcmp(mnemonic, "jmp") != 0) {
                    continue;
                }
                // if the opcode is a call
                if (strcmp(mnemonic, "call") == 0) {
                    // detect if the destination of the call is something like:
                    //   pop eax
                    //   ret
                    // This kind of "instruction pointer retrieval function"
                    // indicates that any following opcodes that offset the given
                    // register such as lea rcx, [eax + 0x100] are actually doing
                    // eip/rip relative addresses and need to be adjusted
                    detect_ip_retrieval(curins, from, to, &fixup_reg);
                }
                // relocate this imm call
                if (!relocate(curins, difference, x86->encoding.imm_size, x86->encoding.imm_offset)) {
                    // if cannot relocate it's because the new dest is too far
                    uintptr_t cond_ptr = to + conditional_area + ((uintptr_t)num_conditionals * ABS_JUMP_SIZE);
                    relocate_conditional_jmp(curins, cond_ptr, amount, from, to, x86->encoding.imm_size, x86->encoding.imm_offset);
                    num_conditionals++;
                }
            }
        }
    }

    count = cs_disasm(m_capstone_handle, code, amount, (uintptr_t)code, 0, &instructioninfo);
    cs_free(instructioninfo, count);
    return amount;
}

// relocates an instruction by adjusting it's displacement by a difference
bool Hook::relocate(cs_insn *curins, intptr_t difference, uint8_t dispsize, uint8_t dispoffset)
{
    int32_t disp = 0;
    memcpy(&disp, curins->bytes + dispoffset, dispsize);
    // check for int underflow
    if (difference > INT32_MAX || difference < INT32_MIN) {
        error("Cannot relocate %d by %zd", disp, difference);
        // cannot relocate this
        return false;
    }
    disp -= (int32_t)difference;
    memcpy((void *)(uintptr_t)(curins->address + dispoffset), &disp, dispsize);
    return true;
}

// detects conditional jumps like jl xyz and jz xyz
bool Hook::is_conditional_jump(uint8_t *bytes, uint16_t size)
{
    if (size < 1) {
        return false;
    }
    if (bytes[0] == 0x0f && size > 1) {
        if (bytes[1] >= 0x80 && bytes[1] <= 0x8f) {
            return true;
        }
    }
    if (bytes[0] >= 0x70 && bytes[0] <= 0x7f) {
        return true;
    }
    if (bytes[0] == 0xe3) {
        return true;
    }
    return false;
}

// this will generate a proxy jump in the proxy jump area at the end of the trampoline
// then it will relocate the conditional jump by adjusting it's destination to point to
// the proxy jump in the jump table and then the proxy jump will go back to the original
// intended destination of the conditional jump
void Hook::relocate_conditional_jmp(cs_insn *curins, uintptr_t new_dest, uintptr_t codesize,
    uintptr_t from, uintptr_t to, uint8_t dispsize, uint8_t dispoffset)
{
    uintptr_t tramp_end = to + codesize;
    int32_t disp = 0;
    memcpy(&disp, curins->bytes + dispoffset, dispsize);
    uintptr_t orig_dest = curins->address + (disp - (to - from)) + curins->size;
    write_absolute_jump(new_dest, orig_dest);
    // set relative jmp to go to our absolute
    disp = calculate_relative_displacement(curins->address, new_dest, curins->size);
    *(uintptr_t *)(curins->address + dispoffset) = disp;
}

// determines relative displacement between two addresses with instruction size
// offset for calculcating calls and jmps easier
int32_t Hook::calculate_relative_displacement(uintptr_t from, uintptr_t to, uint32_t ins_size)
{
    if (to < from) {
        return (int32_t)(0 - (from - to) - ins_size);
    }
    return (int32_t)(to - (from + ins_size));
}

// absolute jump via push + ret, 14 bytes on x64, 6 bytes on x86
uintptr_t Hook::write_absolute_jump(uintptr_t src, uintptr_t dest)
{
    // push dest and grab number of bytes written
    uintptr_t amt = write_push(src, dest);
    // retn after the number of bytes that were used for push
    *(uint8_t *)(src + amt) = 0xC3;
    // return number of bytes written
    return ABS_JUMP_SIZE;
}

// 5 byte relative jmp to dest via offset from current position
uintptr_t Hook::write_relative_jump(uintptr_t src, uintptr_t dest)
{
    uintptr_t off = 0;
    // retn after the number of bytes that were used for push
    *(uint8_t *)(src + off++) = 0xe9;
    *(int32_t *)(src + off++) = calculate_relative_displacement(src, dest, 5);
    // return number of bytes written
    return REL_JUMP_SIZE;
}

// writes a 1 bytes ret on x64 or 3 byte ret x on x86
uintptr_t Hook::write_retn(uintptr_t addr, uint16_t amount)
{
    uintptr_t off = 0;
#ifdef _WIN64
    *(uint8_t *)(addr + off++) = 0xC3;
#else
    *(uint8_t *)(addr + off++) = 0xC2;
    *(uint16_t *)(addr + off) = amount;
    off += sizeof(uint16_t);
#endif
    return RETN_SIZE;
}

// push of a 32 or 64 bit value
uintptr_t Hook::write_push(uintptr_t addr, uintptr_t value)
{
    uintptr_t off = 0;
    // push
    *(uint8_t *)(addr + off++) = 0x68;
    *(uint32_t *)(addr + off) = (uint32_t)(value & 0xFFFFFFFF);
    off += sizeof(uint32_t);
#ifdef _WIN64 // x64 store upper half directly onto stack
    // mov [rsp + 4], dword
    *(uint8_t *)(addr + off++) = 0xC7;
    *(uint8_t *)(addr + off++) = 0x44;
    *(uint8_t *)(addr + off++) = 0x24;
    *(uint8_t *)(addr + off++) = 0x04;
    *(uint32_t *)(addr + off) = (uint32_t)((value >> 32) & 0xFFFFFFFF);
    off += sizeof(uint32_t);
#endif
    return PUSH_SIZE;
}

uintptr_t Hook::write_set_cx(uintptr_t addr, uintptr_t value)
{
    uintptr_t off = 0;
#ifdef _WIN64 // x64 RAX prefix
    *(uint8_t *)(addr + off++) = 0x48;
#endif
    // mov ecx, value
    *(uint8_t *)(addr + off++) = 0xb9;
    *(uintptr_t *)(addr + off) = value;
    return SET_CX_SIZE;
}

uintptr_t Hook::write_set_dx_sp(uintptr_t addr)
{
    // this is a special function that stores a copy of ESP/RSP into
    // the EDX/RDX register for the purpose of passing a pointer to
    // the current stack position to our hook function (edx/rdx is
    // the second argument if we use fastcall and x64) Then this needs
    // to offset the stack pointer address by some amount equal to
    // the amount we have moved it since the func args were pushed
    uintptr_t off = 0;
#ifdef _WIN64 // x64 RAX prefix
    *(uint8_t *)(addr + off++) = 0x48;
#endif
    // mov edx, esp
    *(uint8_t *)(addr + off++) = 0x89;
    *(uint8_t *)(addr + off++) = 0xe2;
#ifndef _WIN64
    // 0x24 is the number of bytes between the current SP and the address
    // of the arguments to the function that were pushed in x86
    // add edx, 0x24
    *(uint8_t *)(addr + off++) = 0x83;
    *(uint8_t *)(addr + off++) = 0xc2;
    *(uint8_t *)(addr + off++) = 0x24;
#endif
    return SET_DX_SP_SIZE;
}

uintptr_t Hook::write_add_sp(uintptr_t addr, uint8_t amount)
{
    uintptr_t off = 0;
#ifdef _WIN64 // x64 RAX prefix
    *(uint8_t *)(addr + off++) = 0x48;
#endif
    // sub esp, xxx
    *(uint8_t *)(addr + off++) = 0x83;
    *(uint8_t *)(addr + off++) = 0xc4;
    *(uint8_t *)(addr + off++) = amount;
    return SUB_SP_SIZE;
}

uintptr_t Hook::write_sub_sp(uintptr_t addr, uint8_t amount)
{
    uintptr_t off = 0;
#ifdef _WIN64 // x64 RAX prefix
    *(uint8_t *)(addr + off++) = 0x48;
#endif
    // sub esp, xxx
    *(uint8_t *)(addr + off++) = 0x83;
    *(uint8_t *)(addr + off++) = 0xec;
    *(uint8_t *)(addr + off++) = amount;
    return SUB_SP_SIZE;
}

uintptr_t Hook::write_push_registers(uintptr_t addr)
{
    uintptr_t off = 0;
#ifndef _WIN64
    *(uint8_t *)(addr + off++) = 0x51; // push ecx twice
    *(uint8_t *)(addr + off++) = 0x51; // push ecx
    *(uint8_t *)(addr + off++) = 0x52; // push edx
    *(uint8_t *)(addr + off++) = 0x53; // push ebx
    *(uint8_t *)(addr + off++) = 0x54; // push esp
    *(uint8_t *)(addr + off++) = 0x55; // push ebp
    *(uint8_t *)(addr + off++) = 0x56; // push esi
    *(uint8_t *)(addr + off++) = 0x57; // push edi
#else
    *(uint8_t *)(addr + off++) = 0x41; // push r11
    *(uint8_t *)(addr + off++) = 0x53;
    *(uint8_t *)(addr + off++) = 0x41; // push r10
    *(uint8_t *)(addr + off++) = 0x52;
    *(uint8_t *)(addr + off++) = 0x41; // push r9
    *(uint8_t *)(addr + off++) = 0x51;
    *(uint8_t *)(addr + off++) = 0x41; // push r8
    *(uint8_t *)(addr + off++) = 0x50;
    *(uint8_t *)(addr + off++) = 0x52; // push rdx
    *(uint8_t *)(addr + off++) = 0x51; // push rcx
#endif
    return PUSHAD_SIZE;
}

uintptr_t Hook::write_pop_registers(uintptr_t addr)
{
    uintptr_t off = 0;
    // reverse order of the push function above
#ifndef _WIN64
    *(uint8_t *)(addr + off++) = 0x5f; // pop edi
    *(uint8_t *)(addr + off++) = 0x5e; // pop esi
    *(uint8_t *)(addr + off++) = 0x5d; // pop ebp
    *(uint8_t *)(addr + off++) = 0x5c; // pop esp
    *(uint8_t *)(addr + off++) = 0x5b; // pop ebx
    *(uint8_t *)(addr + off++) = 0x5a; // pop edx
    *(uint8_t *)(addr + off++) = 0x59; // pop ecx
    *(uint8_t *)(addr + off++) = 0x59; // pop ecx twice
#else
    *(uint8_t *)(addr + off++) = 0x59; // pop rcx
    *(uint8_t *)(addr + off++) = 0x5a; // pop rdx
    *(uint8_t *)(addr + off++) = 0x41; // pop r8
    *(uint8_t *)(addr + off++) = 0x58;
    *(uint8_t *)(addr + off++) = 0x41; // pop r9
    *(uint8_t *)(addr + off++) = 0x59;
    *(uint8_t *)(addr + off++) = 0x41; // pop r10
    *(uint8_t *)(addr + off++) = 0x5a;
    *(uint8_t *)(addr + off++) = 0x41; // pop r11
    *(uint8_t *)(addr + off++) = 0x5b;
#endif
    return POPAD_SIZE;
}
