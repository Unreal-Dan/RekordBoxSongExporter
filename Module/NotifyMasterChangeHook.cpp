#include <Windows.h>
#include <inttypes.h>

#include <string>
#include <deque>

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

// updates the last_track, current_track, and current_tracks files
void update_track_list(string track, string artist)
{
    // simple local class for the static track listing
    class track_list_entry
    {
    public:
        track_list_entry(string track, string artist, string genre) :
            track(track), artist(artist), genre(genre) {}
        string track;
        string artist;
        string genre;
    };

    // static list of tracks
    static deque<track_list_entry> track_list;
    // prepend this new track to the list
    track_list.push_front(track_list_entry(track, artist, ""));
    // make sure the list doesn't go beyond MAX_TRACKS
    if (track_list.size() > MAX_TRACKS) {
        // remove from the end
        track_list.pop_back();
    }
        
    // update the last x file by iterating track_list and writing
    if (track_list.size() > 0) {
        // concatenate the track_list into a single string
        std::string tracks;
        for (auto it = track_list.begin(); it != track_list.end(); it++) {
            tracks.append(it->track).append(" - ").append(it->artist).append( "\n");
        }
        // update the tracks file
        if (!clear_file(get_cur_tracks_file())) {
            error("Failed to clear tracks file");
        }
        if (!append_file(get_cur_tracks_file(), tracks.c_str())) {
            error("Failed to write to tracks file");
        }

        // update the current track file
        if (!clear_file(get_cur_track_file())) {
            error("Failed to clear current track file");
        }
        if (!append_file(get_cur_track_file(), track_list.at(0).track)) {
            error("Failed to append to current track file");
        }
    }
    // update the last track file
    if (track_list.size() > 1) {
        if (!clear_file(get_last_track_file())) {
            error("Failed to clear last track file");
        }
        if (!append_file(get_last_track_file(), track_list.at(1).track)) {
            error("Failed to write last track file");
        }
    }
}

// beginning to reverse this stuff and hopefully find track name
// referenced from the sync master or something like that
struct sync_master
{
    void *idk;
};

struct sync_manager
{
    uint8_t pad[0xF0];
    sync_master *curSyncMaster;
};

// the actual hook function that notifyMasterChange is redirected to
void notify_master_change_hook(sync_manager *syncManager)
{
    // 128 should be enough to tell if a track name matches right?
    static string last_notified_track;
    static string last_notified_artist;
    string last_track = get_last_track();
    string last_artist = get_last_artist();
    if (!last_track.length() || !last_artist.length()) {
        return;
    }
    // make sure they didn't fade back into the same song
    if (last_track == last_notified_track && last_artist == last_notified_artist) {
        return;
    }
    // yep they faded into a new song
    info("Master Changed to: %s - %s", 
        last_track.c_str(), last_artist.c_str());
    // log it to our track list
    update_track_list(last_track, last_artist);
    // store the last song for next time
    last_notified_track = last_track;
    last_notified_artist = last_artist;
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
