#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "Config.h"
#include "hook.h"
#include "Log.h"

using namespace std;

// offset of notify master change from base of rekordbox.exe
#define PLAY_TRACK_OFFSET               0x908D70
// the number of bytes to copy out into a trampoline
#define PLAY_TRACK_TRAMPOLINE_LEN       0x13

// determined that track title is 0x138 bytes into
struct deck_struct
{
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
    deck_struct decks[2];
};

// a pointer of this type is passed into the eventPlay function
// we only really need the event_info contained inside
struct event_struct
{
    void *qword0;
    play_track_event *event_info;
};

// logs a track to the global log
void global_log(string line)
{
    // log to the global log
    if (!append_file(get_log_file(), line + "\n")) {
        error("Failed to log track to global log");
    }
}

// the actual hook function that eventPlay is redirected to
void play_track_hook(event_struct *event)
{
    play_track_event *track_info = event->event_info;
    // keep track of the last tracks we had selected 
    static string last_track1;
    static string last_track2;
    // grab the current tracks we have selected
    string track1 = track_info->decks[0].track_title;
    string track2 = track_info->decks[1].track_title;
    deck_struct *new_track_deck = nullptr;
    // check if either of the tracks changed and if one did then log that 
    // track. Unfortunately if you change both tracks at the same time 
    // then press play it will always log the track that was loaded onto 
    // the second deck regardless of which deck you pressed play on.
    if (last_track1 != track1 && track1[0]) {
        new_track_deck = &track_info->decks[0];
    }
    if (last_track2 != track2 && track2[0]) {
        new_track_deck = &track_info->decks[1];
    }
    if (new_track_deck) {
        info("Playing track: %s - %s", new_track_deck->track_title, new_track_deck->track_artist);
        // set the last track + artist in global storage so that the
        // notifyMasterChange hook can pull it from here
        set_last_track(new_track_deck->track_title);
        set_last_artist(new_track_deck->track_artist);
        // log the full track - artist to the global log
        global_log(string(new_track_deck->track_title) + " - " + string(new_track_deck->track_artist));
    }
    // store current tracks for the next time play button is pressed
    last_track1 = track1;
    last_track2 = track2;
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
