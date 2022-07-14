#include <windows.h>
#include <inttypes.h>
#include <stdio.h>

#include "NotifyMasterChangeHook.h"
#include "DjEngineInterface.h"
#include "LastTrackStorage.h"
#include "NetworkClient.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "BpmControl.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// sig for MainComponent::InitializeUiManager (look for 'WindowID')
// We wait for this function to complete before doing anything, the reasoning
// is that we use the uimanager for some features so it's better to initialize
// our code after the uimanager has been initialized, also it ensures the hack
// is always initialized at the same time so that startup is deterministic
#define INIT_UIMANAGER_SIG "48 8B C4 55 57 41 56 48 8B EC 48 83 EC 70 48 C7 45 D8 FE FF FF FF 48 89 58 10 48 89 70 18 48 8B 05 90 90 90 90 48"

// hook on the init_uimanager function to catch when rekordbox has initialized
static Hook init_uimanager_hook;

// the MainComponent::InitializeUiManager compatible prototype
typedef uintptr_t (*init_uimanager_fn_t)(uintptr_t arg1);
init_uimanager_fn_t init_uimanager = NULL;

// install hooks on the rekordbox initialization function
static bool hook_uimanager_init();
// the rekordbox uimanager initialization callback
static uintptr_t __fastcall init_uimanager_callback(hook_arg_t hook_arg, func_args *args);
// The main thread/program for the module
static DWORD mainThread(void *param);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    // this will run before rekordbox can do anything because the injector will
    // pause on the entrypoint and inject the module then resume rekordbox
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        // hook the uimanager initialization function to wait for rekordbox to initialize
        if (!hook_uimanager_init()) {
            MessageBox(NULL, "Failed to hook initialization", "Error", 0);
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// install hooks on the rekordbox initialization function
static bool hook_uimanager_init()
{
    // setup the logging console
    if (!initialize_log()) {
        return 0;
    }
    // scan for the init_uimanager function
    init_uimanager = (init_uimanager_fn_t)sig_scan(INIT_UIMANAGER_SIG);
    if (!init_uimanager) {
        error("Failed to locate init_uimanager");
        return false;
    }
    // initialize the hook
    init_uimanager_hook.init((uintptr_t)init_uimanager, init_uimanager_callback, NULL);
    // install a hook and do not automatically callback original, this allows
    // us to unhook then callback within the original to create a 'post-hook'
    if (!init_uimanager_hook.install_hook(false)) {
        error("Failed to hook uimanager initialization");
        return false;
    }
    
    return true;
}

// the rekordbox uimanager initialization callback
static uintptr_t __fastcall init_uimanager_callback(hook_arg_t hook_arg, func_args *args)
{
    // unhook so that we can call the function and then run logic after it
    init_uimanager_hook.unhook();
    // callback original function so we can run logic after the call
    uintptr_t rv = init_uimanager(args->arg1);
    // spawn the main thread which does all our logic now that we know rbox has initialized
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
    if (!hThread) {
        error("Failed to spawn main thread");
    }
    return rv;
}

// The main thread/program for the module
static DWORD mainThread(void *param)
{
    // load the configuration ini file
    if (!initialize_config()) {
        return 0;
    }
    // setup the output files
    if (!initialize_output_files()) {
        return 1;
    }
    // setup last track storage
    if (!initialize_last_track_storage()) {
        return 1;
    }
    // initialize the functions used to lookup rowdata
    if (!init_row_data_funcs()) {
        return 1;
    }
    // initialize the bpm change hook
    if (!init_bpm_control()) {
        return 1;
    }

    // server mode
    if (config.use_server) {
        if (!init_network_client()) {
            return 1;
        }
    }

    // hook when the master changes
    if (!hook_notify_master_change()) {
        return 1;
    }

    // success
    success("Success");

    // listen for any messages from the hook functions about which tracks
    // to log to the output files, we must do this on the main thread to
    // avoid crashes and other issues when doing things in the hooks
    run_listener();

    // network cleanup if in server mode
    if (config.use_server) {
        cleanup_network_client();
    }

    return 0;
}
