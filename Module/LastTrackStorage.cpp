#include <Windows.h>

#include <string>
#include <map>

#include "LastTrackStorage.h"
#include "Config.h"

using namespace std;

// whether each deck has been logged
bool deck_logged[NUM_DECKS] = { false };

// Store one id per deck
uint32_t deck_track_ids[NUM_DECKS] = { 0 };

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

void set_logged(uint32_t deck, bool logged)
{
    if (deck >= NUM_DECKS) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    deck_logged[deck] = logged;
    LeaveCriticalSection(&last_track_critsec);
}

bool get_logged(uint32_t deck)
{
    if (deck >= NUM_DECKS) {
        // say it's logged so it doesn't log again
        return true;
    }
    EnterCriticalSection(&last_track_critsec);
    bool result = deck_logged[deck];
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

// the current master index
void set_track_id(uint32_t deck, uint32_t id)
{
    if (deck >= NUM_DECKS) {
        return;
    }
    EnterCriticalSection(&last_track_critsec);
    deck_track_ids[deck] = id;
    LeaveCriticalSection(&last_track_critsec);
}

uint32_t get_track_id(uint32_t deck)
{
    if (deck >= NUM_DECKS) {
        // say it's logged so it doesn't log again
        return 0;
    }
    EnterCriticalSection(&last_track_critsec);
    uint32_t result = deck_track_ids[deck];
    LeaveCriticalSection(&last_track_critsec);
    return result;
}
