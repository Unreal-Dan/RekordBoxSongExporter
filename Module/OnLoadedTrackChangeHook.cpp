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

struct songinfo
{
    uint8_t pad[16];
    char *songTitle;
    char *songArtist;
    char *songAlbum;
    float songBPM;
    uint32_t unkown1;
    char *artworkPath;
    char *filePath;
    char *songKey;
};

struct djplayer_uiplayer
{
    uint8_t pad1[0x148];
    uint16_t deckNum;
    uint8_t pad2[0x1ee];
    songinfo *songInfo; // @0x338
    uint8_t pad3[0x28];
    uint32_t browserId; // @0x368
};

// the actual hook function that notifyMasterChange is redirected to
void __fastcall on_loaded_track_change_hook(djplayer_uiplayer *uiplayer)
{
    if (!uiplayer || !uiplayer->songInfo) {
        return;
    }
    // offset by 1
    uint32_t deck_idx = uiplayer->deckNum - 1;
    songinfo *songInfo = uiplayer->songInfo;

    // bpm comes from here
    if (songInfo->songBPM) {
        set_last_bpm(deck_idx, songInfo->songBPM);
    }

    // handle if we load onto the same deck
    if (get_master() == deck_idx) {
        // log it to our track list
        update_output_files(deck_idx);
        // we logged this track now
        set_logged(deck_idx, true);
    } else {
        // otherwise just let notifyMasterChange handle it
        set_logged(deck_idx, false);
    }
}

bool hook_on_loaded_track_change()
{
    // offset of notifyMasterChange from base of rekordbox.exe
    // and the number of bytes to copy out into a trampoline
    uint32_t trampoline_len = 0xE;
    uint32_t func_offset = 0;
    switch (config.rbox_version) {
    case RBVER_650:
        func_offset = 0xA6D310;
        break;
    case RBVER_585:
        //func_offset = 0x14CEF80;
        error("not implemented");
        break;
    default:
        error("Unknown version");
        return false;
    };

    // determine address of target function to hook
    uintptr_t oltc_addr = (uintptr_t)GetModuleHandle(NULL) + func_offset;
    info("onLoadedTrackChange: %p", oltc_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(oltc_addr, on_loaded_track_change_hook, trampoline_len)) {
        error("Failed to hook onLoadedTrackChange");
        return false;
    }
    return true;
}