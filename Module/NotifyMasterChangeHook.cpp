#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "NotifyMasterChangeHook.h"
#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

using namespace std;

// beginning to reverse this stuff and hopefully find track name
// referenced from the sync master or something like that
struct sync_master
{
    void *idk;
};

struct sync_manager_650
{
    uint8_t pad[0xF0];
    // the current sync master
    sync_master *curSyncMaster;
    // the list of sync masters
    sync_master **syncMasterList;
    void *unknown;
    // the number of sync masters in the list
    uint32_t numSyncMasters;
};

struct sync_manager_585
{
    uint8_t pad[0xF8];
    // the current sync master
    sync_master *curSyncMaster;
    // the list of sync masters
    sync_master **syncMasterList;
    void *unknown;
    // the number of sync masters in the list
    uint32_t numSyncMasters;
};

class sync_manager
{
public:
    // the sync masters are listed in order and the current sync master is
    // pointed to by a member, but we need to know which sync master in
    // the list is being used -- this is effectively the same as the deck id
    // that we use to cache the tracks from EventPlayTrack
    uint32_t get_master_id()
    {
        uint32_t num_masters = num_sync_masters();
        sync_master **list = sync_master_list();
        sync_master *cur = cur_sync_master();
        for (uint32_t i = 0; i < num_masters; ++i) {
            if (cur == list[i]) {
                return i;
            }
        }
        return 0;
    }
    sync_master **sync_master_list()
    {
        switch (config.rbox_version) {
        case RBVER_650:
            return sm_650.syncMasterList;
        case RBVER_585:
            return sm_585.syncMasterList;
        default:
            break;
        }
        return nullptr;
    }
    sync_master *cur_sync_master()
    {
        switch (config.rbox_version) {
        case RBVER_650:
            return sm_650.curSyncMaster;
        case RBVER_585:
            return sm_585.curSyncMaster;
        default:
            break;
        }
        return nullptr;
    }
    uint32_t num_sync_masters()
    {
        switch (config.rbox_version) {
        case RBVER_650:
            return sm_650.numSyncMasters;
        case RBVER_585:
            return sm_585.numSyncMasters;
        default:
            break;
        }
        return 0;
    }

    // anonymous union of sync manager versions
    union
    {
        sync_manager_650 sm_650;
        sync_manager_585 sm_585;
    };
};

// the actual hook function that notifyMasterChange is redirected to
void notify_master_change_hook(sync_manager *syncManager)
{
    // grab the master id we switched to
    uint32_t master_id = syncManager->get_master_id();
    // grab the cached track for that id
    string last_track = get_last_track(master_id);
    string last_artist = get_last_artist(master_id);
    if (!last_track.length() && !last_artist.length()) {
        return;
    }
    // yep they faded into a new song
    info("Master Changed to %d: %s - %s",
        master_id, last_track.c_str(), last_artist.c_str());
    // set the new master
    set_master(master_id);
    // only if this deck hasn't been logged yet
    if (!get_logged(master_id)) {
        // log it to our track list
        update_output_files(last_track, last_artist);
        // we logged this deck now, must wait for a
        // new song to be loaded to unset this
        set_logged(master_id, true);
    }
}

bool hook_notify_master_change()
{
    // offset of notifyMasterChange from base of rekordbox.exe
    // and the number of bytes to copy out into a trampoline
    uint32_t trampoline_len = 0x10;
    uint32_t func_offset = 0;
    switch (config.rbox_version) {
    case RBVER_650:
        func_offset = 0x1772d70;
        break;
    case RBVER_585:
        func_offset = 0x14CEF80;
        break;
    default:
        error("Unknown version");
        return false;
    };

    // determine address of target function to hook
    uintptr_t nmc_addr = (uintptr_t)GetModuleHandle(NULL) + func_offset;
    info("notify_master_change: %p", nmc_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(nmc_addr, notify_master_change_hook, trampoline_len)) {
        error("Failed to install hook on event play");
        return false;
    }
    return true;
}
