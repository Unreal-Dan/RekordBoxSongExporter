#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "NotifyMasterChangeHook.h"
#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

#define NOTIFY_MASTER_CHANGE_SIG "\x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\x48\x8B\x89\x40\x01\x00\x00\x48\x85\xC9\x0F"
#define NOTIFY_MASTER_CHANGE_SIG_LEN (sizeof(NOTIFY_MASTER_CHANGE_SIG) - 1)

using namespace std;

// this class acts as a version agnostic interface for the sync manager object
class sync_manager
{
public:
    // this retrieves the 'master id' which is the deck id of the sync master
    uint32_t get_master_id()
    {
        // We have the pointer to the current sync master as well as the list of
        // sync masters, we need to search the list for the pointer to the current
        // in order to get the index of the current sync master. This is the deck id
        uint32_t num_masters = num_sync_masters();
        void **list = sync_master_list();
        void *cur = cur_sync_master();
        for (uint32_t i = 0; i < num_masters; ++i) {
            if (cur == list[i]) {
                return i;
            }
        }
        return 0;
    }
    
    // using explicit cases for each version to improve readability
    // even though a fallthrough could work 
    void **sync_master_list()
    {
        switch (config.version) {
        case RBVER_585: return sm_585.syncMasterList;
        case RBVER_650: return sm_6xx.syncMasterList;
        case RBVER_651: return sm_6xx.syncMasterList;
        case RBVER_652: return sm_6xx.syncMasterList;
        case RBVER_653: return sm_6xx.syncMasterList;
        default:        
            if (config.version >= RBVER_661) {
                return sm_6xx.syncMasterList;
            }
            return NULL;
        }
    }
    void *cur_sync_master()
    {
        switch (config.version) {
        case RBVER_585: return sm_585.curSyncMaster;
        case RBVER_650: return sm_6xx.curSyncMaster;
        case RBVER_651: return sm_6xx.curSyncMaster;
        case RBVER_652: return sm_6xx.curSyncMaster;
        case RBVER_653: return sm_6xx.curSyncMaster;
        default:
            if (config.version >= RBVER_661) {
                return sm_6xx.curSyncMaster;
            }
            return NULL;
        }
    }
    uint32_t num_sync_masters() 
    {
        switch (config.version) {
        case RBVER_585: return sm_585.numSyncMasters;
        case RBVER_650: return sm_6xx.numSyncMasters;
        case RBVER_651: return sm_6xx.numSyncMasters;
        case RBVER_652: return sm_6xx.numSyncMasters;
        case RBVER_653: return sm_6xx.numSyncMasters;
        default:
            if (config.version >= RBVER_661) {
                return sm_6xx.numSyncMasters;
            }
            return 0;
        }
    }
private:

    // this is the sync manager struct for 5.8.5
    struct sync_manager_585
    {
        // this pad size is really the only difference
        uint8_t pad[0xF8];
        void *curSyncMaster;
        void **syncMasterList;
        void *unknown;
        uint32_t numSyncMasters;
    };

    // This structure seems to stay the same for all 6.x.x versions
    struct sync_manager_6xx
    {
        // this pad is 0xF8 is the previous major versions
        uint8_t pad[0xF0];
        // the current sync master
        void *curSyncMaster;
        // the list of sync masters
        void **syncMasterList;
        void *unknown;
        // the number of sync masters in the list
        uint32_t numSyncMasters;
    };

    // anonymous union of sync manager versions
    union
    {
        // sync manager for 5.8.5
        sync_manager_585 sm_585;
        // sync manager is the same for 6.x.x versions
        sync_manager_6xx sm_6xx;
    };
};

// the actual hook function that notifyMasterChange is redirected to
void notify_master_change_hook(sync_manager *syncManager)
{
    // grab the master id we switched to, the master id is the same as the deck index
    uint32_t master_id = syncManager->get_master_id();
    // set the new master
    set_master(master_id);
    info("Master Changed to %d", master_id);
    // make sure this deck hasn't already been logged since it changed
    if (get_logged(master_id)) {
        return;
    }
    push_deck_update(master_id);
    set_logged(master_id, true);
}

bool hook_notify_master_change()
{
    // offset of notifyMasterChange from base of rekordbox.exe
    // and the number of bytes to copy out into a trampoline
    uint32_t trampoline_len = 0x10;
    uint32_t func_offset = 0;
    uintptr_t nmc_addr = 0;
    switch (config.version) {
    case RBVER_585:
        func_offset = 0x14CEF80;
        break;
    case RBVER_650:
        func_offset = 0x1772d70;
        break;
    case RBVER_651:
        func_offset = 0x179E840;
        break;
    case RBVER_652:
        func_offset = 0x17A8A40;
        break;
    case RBVER_653:
        func_offset = 0x169D240;
        break;
    default: // RBVER_661+
        nmc_addr = sig_scan(NULL, NOTIFY_MASTER_CHANGE_SIG, NOTIFY_MASTER_CHANGE_SIG_LEN);
        break;
    };
    if (!nmc_addr) {
        if (!func_offset) {
            error("Failed to locate NotifyMasterChange");
            return false;
        }
        nmc_addr = rb_base() + func_offset;
    }
    // determine address of target function to hook
    info("notify_master_change: %p", nmc_addr);
    // install hook on notify_master_change that redirects to notify_master_change_hook
    if (!install_hook(nmc_addr, notify_master_change_hook, trampoline_len)) {
        error("Failed to hook notifyMasterChange");
        return false;
    }
    return true;
}
