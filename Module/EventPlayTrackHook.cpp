#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "Config.h"
#include "hook.h"
#include "Log.h"

using namespace std;

// offset of notifyMasterChange from base of rekordbox.exe
// and the number of bytes to copy out into a trampoline
#ifdef REKORDBOX_650
#define PLAY_TRACK_OFFSET               0x908D70
#define PLAY_TRACK_TRAMPOLINE_LEN       0x13
#endif
#ifdef REKORDBOX_585
#define PLAY_TRACK_OFFSET               0x7A5DE0
#define PLAY_TRACK_TRAMPOLINE_LEN       0x13
#endif

// determined that track title is 0x138 bytes into
struct deck_struct
{
    // this appears to be the same size for version 5.8.5 and 6.5.0
    uint8_t pad[0x138];
    char *track_title; 
    char *track_artist; 
    char *track_genre; 
    uint8_t pad2[0x90];
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
void play_track_hook(event_struct *event)
{
    // lets not segfault
    if (!event || !event->event_info || event->deck_idx > 8) {
        return;
    }
    play_track_event *track_info = event->event_info;
    // the deck index is offset by 1
    uint32_t deck_idx = event->deck_idx - 1;
    // grab the current deck we have pressed play on
    deck_struct *new_track_deck = &track_info->decks[deck_idx];

    info("Playing track on %u: %s - %s", deck_idx,
        new_track_deck->track_title, new_track_deck->track_artist);

    // if we are playing a new song
    if (get_last_track(deck_idx) != new_track_deck->track_title ||
        get_last_artist(deck_idx) != new_track_deck->track_artist) {

        // if we're loading the new song onto the same deck then
        // notifyMasterChange will not be called because this deck
        // is already the master -- so we must log it ourselves
        if (get_master() == deck_idx) {
            // log it to our track list
            update_output_files(new_track_deck->track_title, new_track_deck->track_artist);
            // we logged this track now
            set_logged(deck_idx, true);
        } else {
            // otherwise if we load a track on a different master, even if it's the same track we 
            // just played, we still want to mark it for logging when we fade into that deck
            set_logged(deck_idx, false);
        }
    }

    // set the last track + artist in global storage so that the
    // notifyMasterChange hook can pull it from here
    set_last_track(new_track_deck->track_title, deck_idx);
    set_last_artist(new_track_deck->track_artist, deck_idx);
}

bool hook_event_play_track()
{
    // determine address of target function to hook
    uintptr_t ep_addr = (uintptr_t)GetModuleHandle(NULL) + PLAY_TRACK_OFFSET;
    info("event_play_func: %p", ep_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(ep_addr, play_track_hook, PLAY_TRACK_TRAMPOLINE_LEN)) {
        error("Failed to install hook on event play");
        return false;
    }
    return true;
}
