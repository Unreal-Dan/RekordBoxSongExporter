#include <Windows.h>

#include <string>

#include "LastTrackStorage.h"

using namespace std;

class track_info
{
public:
    track_info() : last_track(), last_artist(), is_logged(false) {}
    // some globals to share stuff between the hooks
    string last_track;
    string last_artist;
    // this will be set to true once this artist/track has been logged
    bool is_logged;
};

// don't think this can be anything else
#define NUM_DECKS 4

// support track info for up to 4 decks
track_info decks[NUM_DECKS];

// keep track of the current master IDX, initialize to a value that it
// will never be so we can easily detect if it's not initialized yet
uint32_t current_master_idx = UINT32_MAX;

// the hooks may be on separate threads so this
// protects the shared globals
CRITICAL_SECTION last_track_critsec;

bool initialize_last_track_storage()
{
    // setup the critical section because our two hooks will be on 
    // different threads and they will be sharing data
    // we don't clean this up because we don't provide a way 
    // to inject more than one of this dll anyway
    InitializeCriticalSection(&last_track_critsec);
    // yeah this can't fail right now but maybe in the future
    return true;
}

// store info in global threadsafe storage
void set_last_track(string track, uint32_t deck)
{
    if (deck >= NUM_DECKS) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    decks[deck].last_track = track;
    LeaveCriticalSection(&last_track_critsec);
}
void set_last_artist(string artist, uint32_t deck)
{
    if (deck >= NUM_DECKS) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    decks[deck].last_artist = artist;
    LeaveCriticalSection(&last_track_critsec);
}
void set_logged(uint32_t deck, bool logged)
{
    if (deck >= NUM_DECKS) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    decks[deck].is_logged = logged;
    LeaveCriticalSection(&last_track_critsec);
}

// grab info out of global threadsafe storage
string get_last_track(uint32_t deck)
{
    string result;
    if (deck >= NUM_DECKS) {
        return result;
    }
    EnterCriticalSection(&last_track_critsec);
    result = decks[deck].last_track;
    LeaveCriticalSection(&last_track_critsec);
    return result;
}
string get_last_artist(uint32_t deck)
{
    string result;
    if (deck >= NUM_DECKS) {
        return result;
    }
    EnterCriticalSection(&last_track_critsec);
    result = decks[deck].last_artist;
    LeaveCriticalSection(&last_track_critsec);
    return result;
}
bool get_logged(uint32_t deck)
{
    if (deck >= NUM_DECKS) {
        // say it's logged so it doesn't log again
        return true;
    }
    EnterCriticalSection(&last_track_critsec);
    bool result = decks[deck].is_logged;
    LeaveCriticalSection(&last_track_critsec);
    return result;
}

// the current master index
void set_master(uint32_t master)
{
    EnterCriticalSection(&last_track_critsec);
    current_master_idx = master;
    LeaveCriticalSection(&last_track_critsec);
}

uint32_t get_master()
{
    EnterCriticalSection(&last_track_critsec);
    uint32_t master_idx = current_master_idx;
    LeaveCriticalSection(&last_track_critsec);
    return master_idx;
}
