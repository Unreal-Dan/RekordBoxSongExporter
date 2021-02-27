#include <windows.h>
#include <inttypes.h>
#include <stdio.h>

#include "NotifyMasterChangeHook.h"
#include "EventPlayTrackHook.h"
#include "LastTrackStorage.h"
#include "NetworkClient.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Log.h"

// The main thread/program for the module
DWORD mainThread(void *param)
{
    // setup the logging console
    if (!initialize_log()) {
        return 1;
    }
    // load the configuration ini file
    if (!initialize_config()) {
        return 1;
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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        // spawn a thread and return immediately 
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
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
