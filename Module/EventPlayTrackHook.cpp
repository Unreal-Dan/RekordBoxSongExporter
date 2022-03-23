#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "hook.h"
#include "Log.h"

// Function with 'eventPlayTrack' inside it
#define EVENT_PLAY_SIG "40 53 57 48 81 EC 98 00 00 00 48 C7 44 24 30 FE FF FF FF 48 8B F9 44 8B 01"

using namespace std;

Hook g_play_track_hook;

// NOTE: this is unused now, it was the old source of track information
//       but it only has a few fields. The new source is handled in the
//       output thread by looking up row track data
struct deck_struct
{
    // this appears to be the same size for version 5.8.5 and 6.5.0
    uint8_t pad[0x138];
    char *track_title;
    char *track_artist;
    char *track_genre;
    void *unkown1;
    void *unkown2;
    char *track_composer;
    char *track_lyricist;
    char *track_label;
    uint8_t pad2[0x68];
};

// structure reversed out of rekordbox that appears to
// contain the track info for each deck
struct play_track_event
{
    deck_struct decks[4];
};

// a pointer of this type is passed into the eventPlay function
// we only really need the event_info contained inside
struct event_struct
{
    uint32_t deck_idx; // this is the index of the deck we are pressing play on
    uint32_t unknown;
    play_track_event *event_info;
};

// the actual hook function that eventPlay is redirected to
uintptr_t __fastcall play_track_hook(hook_arg_t hook_arg, func_args *args)
{
    event_struct *event = (event_struct *)args->arg1;
    // lets not segfault
    if (!event || !event->event_info || event->deck_idx > 8) {
        return 0;
    }
    // grab the deck idx for this play event
    // the deck index is offset by 1
    uint32_t deck_idx = (event->deck_idx <= NUM_DECKS) ? event->deck_idx - 1 : 0;
    // we should be able to fetch a uiplayer object for this deck idx
    djplayer_uiplayer *player = lookup_player(deck_idx);
    // if we're loading the same song then it doesn't matter
    if (!player || get_track_id(deck_idx) == player->getTrackBrowserID()) {
        return 0;
    }
    // update the last track of this deck
    set_track_id(deck_idx, player->getTrackBrowserID());
    // if we're playing/cueing a new track on the master deck
    if (get_master() == deck_idx) {
        // Then we need to update the output files because notifyMasterChange isn't called
        push_deck_update(deck_idx, UPDATE_TYPE_NORMAL);
        // we mark this deck as logged so notifyMasterChange doesn't log this track again
        set_logged(deck_idx, true);
        // the edge case where we loaded a track onto the master
        info("Played track on Master %d", deck_idx);
    } else {
        // otherwise we simply mark this deck for logging by notifyMasterChange
        set_logged(deck_idx, false);
        // the edge case where we loaded a track onto the master
        info("Played track on %d", deck_idx);
    }
    return 0;
}

bool hook_event_play_track()
{
    // offset of eventPlayTrack from base of rekordbox.exe
    // and the number of bytes to copy out into a trampoline
    uint32_t trampoline_len = 0x13;
    uint32_t func_offset = 0;
    uintptr_t ep_addr = 0;
    // Search for string 'eventPlayTrack (%d)' to find this function
    switch (config.version) {
    case RBVER_585:
        func_offset = 0x7A5DE0;
        break;
    case RBVER_650:
        func_offset = 0x908D70;
        break;
    case RBVER_651:
        func_offset = 0x911610;
        break;
    case RBVER_652:
        func_offset = 0x91E4D0;
        break;
    case RBVER_653:
        func_offset = 0x793D80;
        break;
    default: // RBVER_661+
        ep_addr = sig_scan(EVENT_PLAY_SIG);
        break;
    };
    if (!ep_addr) {
        if (!func_offset) {
            error("Failed to locate EventPlayTrack");
            return false;
        }
        ep_addr = rb_base() + func_offset;
    }
    // determine address of target function to hook
    info("event_play_func: %p", ep_addr);
    g_play_track_hook.init(ep_addr, play_track_hook, NULL);
    // install hook on event_play_addr that redirects to play_track_hook
    if (!g_play_track_hook.install_hook()) {
        error("Failed to hook eventPlay");
        return false;
    }
    return true;
}
