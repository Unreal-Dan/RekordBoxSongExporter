#pragma once
#include <inttypes.h>

#include <capstone/capstone.h>

#include <vector>

// structure that is lined up with stack data to allow accessing
// the arguments on the stack
class func_args {
public:
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
    uintptr_t arg4;
    // arg 5 and 6 probably only work on x86
    uintptr_t arg5;
    uintptr_t arg6;
};

// the hook arg is just some data the user passes to their callback
typedef uintptr_t hook_arg_t;

// using fastcall so that arg 1 and 2 is passed through ecx/edx to make
// writing the assembly hook code similar for x86 and x64
typedef uintptr_t (__fastcall *hook_callback_fn)(hook_arg_t, func_args *);

class Hook {
public:
    Hook();
    Hook(uintptr_t target_func, hook_callback_fn hook_func, hook_arg_t arg);
    ~Hook();

    //=========================================================================
    // initialize the hook on the target_func that redirects to the hook_func
    // and pass along the arg to the hook_func
    void init(uintptr_t target_func, hook_callback_fn hook_func, hook_arg_t arg);

    //=========================================================================
    // Install the hook on the target function
    //
    //   do_callback  (optional) automatically call target after user callback
    //   num_args     (optional) The number of args to clean off stack
    //
    // This will install a 'pre-hook' on the target_func and redirects to the
    // user-defined hook function. If you want a 'post-hook' then you need to
    // set do_callback = false and do Unhook(); Callback(); Rehook(); inside
    // the hook function
    //
    // num_args is only applicable if do_callback = false otherwise the
    // original function will clean the stack itself.
    //
    // num_args is also auto detected by searching for a ret x opcode inside
    // the target function, but if a ret x opcode cannot be found a value
    // may be required
    bool install_hook(bool do_callback = true, uint32_t num_args = 0);

    //=========================================================================
    // unhook and rehook the already installed hook
    // This is useful for creating a 'post-hook' instead of a 'pre-hook'
    bool unhook();
    bool rehook();

    //=========================================================================
    // cleanup the hook, ready to init again
    void cleanup_hook();

    //=========================================================================
    // address of target function that was hooked
    uintptr_t get_target_func() { return m_target_func; }
    // ge the number of detected or provided args for the hook
    uint32_t get_num_args() { return m_ret_amt / sizeof(uintptr_t); }
    // whether the hook is 'hooked'
    bool is_hooked() { return m_hooked; }

private:
    bool calculate_asm_length();
    bool create_trampoline(bool do_callback);
    uintptr_t write_trampoline(uintptr_t pos, bool do_callback);
    uintptr_t allocate_within_2gb_up(uintptr_t base, size_t size);
    uintptr_t allocate_within_2gb_down(uintptr_t base, size_t size);
    uintptr_t allocate_within_2gb(uintptr_t base_addr, size_t size);
    bool detect_ip_retrieval(cs_insn *curins, uintptr_t from, uintptr_t to, x86_reg *fixup_reg);
    uintptr_t relocate_asm(uintptr_t to, uintptr_t from, uintptr_t amount, uintptr_t conditional_area);
    bool relocate(cs_insn *curins, intptr_t difference, uint8_t dispsize, uint8_t dispoffset);
    bool is_conditional_jump(uint8_t *bytes, uint16_t size);
    void relocate_conditional_jmp(cs_insn *curins, uintptr_t new_dest, uintptr_t codesize,
        uintptr_t from, uintptr_t to, uint8_t dispsize, uint8_t dispoffset);
    int32_t calculate_relative_displacement(uintptr_t from, uintptr_t to, uint32_t ins_size);
    uintptr_t write_absolute_jump(uintptr_t src, uintptr_t dest);
    uintptr_t write_relative_jump(uintptr_t src, uintptr_t dest);
    uintptr_t write_retn(uintptr_t addr, uint16_t amount);
    uintptr_t write_push(uintptr_t addr, uintptr_t value);
    uintptr_t write_set_cx(uintptr_t addr, uintptr_t value);
    uintptr_t write_set_dx_sp(uintptr_t addr);
    uintptr_t write_add_sp(uintptr_t addr, uint8_t amount);
    uintptr_t write_sub_sp(uintptr_t addr, uint8_t amount);
    uintptr_t write_push_registers(uintptr_t location);
    uintptr_t write_pop_registers(uintptr_t location);

    // the capstone handle for disassembly
    csh m_capstone_handle;

    // the arg to pass to hook
    hook_arg_t m_arg;

    // the array of bytes that were relocated (without changes)
    std::vector<uint8_t> m_old_bytes;

    // the target function to hook
    uintptr_t m_target_func;
    // the hook routine
    uintptr_t m_hook_func;
    // the allocated trampoline
    uintptr_t m_trampoline;
    // the length of the asm to relocate
    uint32_t m_asm_len;
    // the length of the trampoline (relocated + extra)
    uint32_t m_trampoline_len;
    // the amount the stack is corrected in a ret
    uint16_t m_ret_amt;

    // the number of conditional jmps that were relocated
    uint32_t m_num_conditionals;

    // whether the hook is initialized and ready to install
    bool m_initialized;
    // whether the hook is actually installed
    bool m_hooked;
};