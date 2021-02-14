#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "NotifyMasterChangeHook.h"
#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// offset of notify master change from base of rekordbox.exe
#define NOTIFY_MASTER_CHANGE_OFFSET     0x1772d70
// the number of bytes to copy out into a trampoline
#define NOTIFY_MASTER_TRAMPOLINE_LEN    0x10

using namespace std;

// beginning to reverse this stuff and hopefully find track name
// referenced from the sync master or something like that
struct sync_master
{
    void *idk;
};

struct sync_manager
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

// the sync masters are listed in order and the current sync master is
// pointed to by a member, but we need to know which sync master in
// the list is being used -- this is effectively the same as the deck id
// that we use to cache the tracks from EventPlayTrack
uint32_t determine_master_id(const sync_manager *syncManager)
{
    if (!syncManager) {
        return 0;
    }
    for (uint32_t i = 0; i < syncManager->numSyncMasters; ++i) {
        if (syncManager->curSyncMaster == syncManager->syncMasterList[i]) {
            return i;
        }
    }
    return 0;
}

// the actual hook function that notifyMasterChange is redirected to
void notify_master_change_hook(sync_manager *syncManager)
{
    // grab the master id we switched to
    uint32_t master_id = determine_master_id(syncManager);
    // grab the cached track for that id
    string last_track = get_last_track(master_id);
    string last_artist = get_last_artist(master_id);
    if (!last_track.length() && !last_artist.length()) {
        return;
    }
    // yep they faded into a new song
    info("Master Changed to: %s - %s",
        last_track.c_str(), last_artist.c_str());
    // set the new master
    set_master(master_id);
    // only if this deck hasn't been logged yet
    if (!get_logged(master_id)) {
        // log it to our track list
        update_track_list(last_track, last_artist);
        // we logged this deck now, must wait for a
        // new song to be loaded to unset this
        set_logged(master_id, true);
    }
}

bool hook_notify_master_change()
{
    // determine address of target function to hook
    uintptr_t nmc_addr = (uintptr_t)GetModuleHandle(NULL) + NOTIFY_MASTER_CHANGE_OFFSET;
    info("notify_master_change: %p", nmc_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(nmc_addr, notify_master_change_hook, NOTIFY_MASTER_TRAMPOLINE_LEN)) {
        error("Failed to install hook on event play");
        return false;
    }
    return true;
}
