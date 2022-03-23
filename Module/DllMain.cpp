#include <windows.h>
#include <inttypes.h>
#include <stdio.h>

#include "NotifyMasterChangeHook.h"
#include "EventPlayTrackHook.h"
#include "DjEngineInterface.h"
#include "LastTrackStorage.h"
#include "NetworkClient.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
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

uintptr_t cfxunit = 0;
Hook *cfx_unit_hook = NULL;
uintptr_t *list = NULL;
Hook *hook = NULL;
Hook *hook2 = NULL;
uintptr_t *entry = NULL;



static uintptr_t __fastcall cfx_unit_vtable_callback(hook_arg_t hook_arg, func_args *args)
{
    // [*] 2   getNumOfEffect(000001B2E96EA530, 0000000000000007, 000001B2E9711DA0, 0000000000000000)
    // [*] 18  createEffectBase(000001B2E96EA530, 0000000000000007, 0000000000000000, 00007FF6483618D0)
    // [*] 5   setParameter(000001B2E96EA530, 000001B2E9711DA0, 0000000000000000, 0000000000000002)
    // [*] 13  setEffectBeat(000001B2E96EA530, 0000000100000004, 0000000000000002, 0000000000000000)
    info("fx_%u(%p, %p, %p, %p)", (int)hook_arg, args->arg1, args->arg2, args->arg3, args->arg4); 
    if (hook_arg == 18) {
        list = *(uintptr_t **)(args->arg1 + 0x78);
        info("Entry List: %p[%d] = %p", list, (uint32_t)args->arg3, list[(uint32_t)args->arg3]);
        hook->unhook();
        uintptr_t rv = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))hook->get_target_func())(args->arg1, args->arg2, args->arg3, args->arg4);
        hook->rehook();
        info("Entry List: %p[%d] = %p", list, (uint32_t)args->arg3, list[(uint32_t)args->arg3]);
        entry = (uintptr_t *)list[(uint32_t)args->arg3];
        return rv;
    }
    return 0;
}

static uintptr_t __fastcall cfx_unit_callback(hook_arg_t hook_arg, func_args *args)
{
    cfxunit = (uintptr_t)args->arg1;
    cfx_unit_hook->unhook();
    // lol
    uintptr_t rv = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))cfx_unit_hook->get_target_func())(args->arg1, args->arg2, args->arg3, args->arg4);
    cfx_unit_hook->rehook();
    info("MultiFxLayer: %p", args->arg1);
    static bool a = false;
    if (!a) {
        a = true;
    } else {
        return rv;
    }
    list = *(uintptr_t **)(rv + 0x78);
    void **vtable = *(void ***)rv;
    for (int i = 1; i < 19; ++i) {
        // pure virtuals
        if (i == 17) {
            continue;
        }
        if (i != 18) {
            continue;
        }
        hook = Hook::instant_hook((uintptr_t)vtable[i], cfx_unit_vtable_callback, (hook_arg_t)i, false);
        if (!hook) {
            error("Failed to hook vt %u: %p", i, ((uintptr_t *)vtable)[i] - rb_base());
            return NULL;
        }
        success("Hooked %u: %p", i, ((uintptr_t *)vtable)[i] - rb_base());
    }
    return rv;
}

Hook *efx_sel_hook = NULL;
typedef uintptr_t (*efx_sel_fn_t)(uintptr_t, int);

uintptr_t efx_obj = NULL;
int efx_num = 0;
efx_sel_fn_t efx_sel = NULL;

static void *wait_keypress(void *arg)
{
    int key = 0;
    static bool on = false;
    while (1) {
        if (GetAsyncKeyState(VK_DOWN) & 1) {
            if (efx_num > 0) {
                efx_num--;
            }
            efx_sel(efx_obj, efx_num);
        }
        if (GetAsyncKeyState(VK_UP) & 1) {
            if (efx_num < 16) {
                efx_num++;
            }
            efx_sel(efx_obj, efx_num);
        }
        Sleep(100);
    }
    return NULL;
}

static uintptr_t __fastcall efx_sel_callback(hook_arg_t hook_arg, func_args *args)
{
    info("efx_sel(%p, %p, %p, %p)", args->arg1, args->arg2, args->arg3, args->arg4); 
    efx_obj = args->arg1;
    efx_num = args->arg2;
    return 0;
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
    // Hook RekordboxDjMultiFxLayer::RekordboxDjMultiFxLayer()
    //cfx_unit_hook = Hook::instant_hook(sig_scan("4C 8B DC 49 89 4B 08 57 48 83 EC 50 49 C7 43 E8 FE FF FF FF 49 89 5B 10 49 89 73 18 49 8B D8 48  8B FA 48 8B F1 48 8D 05 D4 38 EA 01 49 89 43 D8"),
    //    cfx_unit_callback, 0, false);

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_keypress, NULL, 0, NULL);
    efx_sel = (efx_sel_fn_t)sig_scan("40 57 41 57 48 83 EC 38 48 8B 01 44 8B FA 48 8B F9 FF 50 10 44 3B F8");

    // Hook EffectLayerBase::selectEffect
    efx_sel_hook = Hook::instant_hook((uintptr_t)efx_sel, efx_sel_callback);
    
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
    // initialize the dj engine interface
    if (!init_djengine_interface()) {
        return 1;
    }

    // server mode
    if (config.use_server) {
        if (!init_network_client()) {
            return 1;
        }
    }

    // hook when we press the play/cue button and a new track has been loaded
    if (!hook_event_play_track()) {
        return 1;
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
