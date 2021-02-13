#include <Windows.h>

#include <string>

#include "LastTrackStorage.h"

using namespace std;

// some globals to share stuff between the hooks
string last_track;
string last_artist;

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
void set_last_track(string track)
{
    EnterCriticalSection(&last_track_critsec);
    last_track = track;
    LeaveCriticalSection(&last_track_critsec);
}
void set_last_artist(string artist)
{
    EnterCriticalSection(&last_track_critsec);
    last_artist = artist;
    LeaveCriticalSection(&last_track_critsec);
}

// grab info out of global threadsafe storage
string get_last_track()
{
    EnterCriticalSection(&last_track_critsec);
    string result = last_track;
    LeaveCriticalSection(&last_track_critsec);
    return result;
}
string get_last_artist()
{
    EnterCriticalSection(&last_track_critsec);
    string result = last_artist;
    LeaveCriticalSection(&last_track_critsec);
    return result;
}
