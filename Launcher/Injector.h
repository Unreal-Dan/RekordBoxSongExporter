#pragma once
#include <string>

class Injector
{
public:
    Injector();
    Injector(const std::string &exe_path, const std::string &module_name);
    ~Injector();

    // initialize the injector with a target exe + module
    void init(const std::string &exe_path, const std::string &module_name);

    // launch the process and inject the module
    bool inject();

private:
    // launch process frozen
    bool launch_suspended();
    // locate the entrypoint
    bool fetch_entrypoint();
    // write some memory to a target process
    bool read_proc_mem(uintptr_t addr, void *data, size_t amount);
    // write some memory to a target process
    bool write_proc_mem(uintptr_t addr, void *data, size_t amount);
    // create a string in the remote process
    uintptr_t inject_string(const std::string &str);
    // inject the module into remote proc
    bool inject_module();

    std::string m_exe_path;
    std::string m_exe_basepath;
    std::string m_module_path;

    // proc/thread handles
    HANDLE m_process;
    HANDLE m_thread;

    // the entrypoint
    uintptr_t m_entry;
};

