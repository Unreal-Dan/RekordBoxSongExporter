#include <Windows.h>

#include <string>
#include <deque>
#include <queue>

#include "LastTrackStorage.h"
#include "NetworkClient.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Log.h"

using namespace std;

// This is the file containing the last config.max_tracks songs
#define CUR_TRACKS_FILE "current_tracks.txt"
// This file contains the current track only
#define CUR_TRACK_FILE  "current_track.txt"
// This file contains the last track only
#define LAST_TRACK_FILE "last_track.txt"
// this is the logfile for all songs played
#define TRACK_LOG_FILE  "played_tracks.txt"

// simple local class to describe a track and it's metadata
class track_data
{
public:
    // initialize a new track with the index of the deck to load
    // the track information from
    track_data(uint32_t deck_idx) : idx(deck_idx)
    {
        // lookup a rowdata object
        row_data *rowdata = lookup_row_data(deck_idx);
        if (!rowdata) {
            return;
        }
        // steal all the track information so we have it stored in our own containers
        title = rowdata->getTitle();
        artist = rowdata->getArtist();
        album = rowdata->getAlbum();
        genre = rowdata->getGenre();
        label = rowdata->getLabel();
        key = rowdata->getKey();
        orig_artist = rowdata->getOrigArtist();
        remixer = rowdata->getRemixer();
        composer = rowdata->getComposer();
        comment = rowdata->getComment();
        mix_name = rowdata->getMixName();
        lyricist = rowdata->getLyricist();
        date_created = rowdata->getDateCreated();
        date_added = rowdata->getDateAdded();
        // track number is an integer
        track_number = to_string(rowdata->getTrackNumber());
        // the bpm is an integer like 15150 which represents 151.50 bpm
        bpm = to_string(rowdata->getBpm());
        // so we just shove a dot in there and it's good
        bpm.insert(bpm.length() - 2, ".");
        // cleanup the rowdata object we got from rekordbox
        // so that we can just use our local containers
        destroy_row_data(rowdata);
    }
    uint32_t idx; // the deck index this track is loaded on
    string title;
    string artist;
    string album;
    string genre;
    string label;
    string key;
    string orig_artist;
    string remixer;
    string composer;
    string comment;
    string mix_name;
    string lyricist;
    string date_created;
    string date_added;
    string track_number;
    string bpm;
};

// update the various log files with a track string, only meant to be called
// from the logging thread so making this static
static void update_log_files(const string &track);
// Pop the next deck change index out of the queue, only meant to
// be called from the logging thread so making this static
static uint32_t pop_deck_update();
// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to);
// calc time since the first time this was called
static string get_timestamp_since_start();
// helper to build output line based on configured output format
static string build_output(track_data *track);
// create and truncate a single file, only call this from the logger thread
static bool clear_file(const string &filename);
// append data to a file, only call this from the logger thread
static bool append_file(const string &filename, const string &data);
// just a wrapper around clear and append
static bool rewrite_file(const string &filename, const string &data);

// queue of indexes of decks that changed
// For example if deck 1 changes then 1 gets pushed into this
queue<uint32_t> deck_changes;

// critical section to protect track queue
CRITICAL_SECTION track_list_critsec;
// semaphore to signal when a deck has changed and there are
// entries in the deck_changes queue
HANDLE hLogSem;

string get_log_file()
{
    return get_dll_path() + "\\" TRACK_LOG_FILE;
}

string get_cur_track_file()
{
    return get_dll_path() + "\\" CUR_TRACK_FILE;
}

string get_last_track_file()
{
    return get_dll_path() + "\\" LAST_TRACK_FILE;
}

string get_cur_tracks_file()
{
    return get_dll_path() + "\\" CUR_TRACKS_FILE;
}



// initialize all the output files
bool initialize_output_files()
{
    // only clear the output files if we're going to write to them
    if (!config.use_server) {
        // clears all the track files
        if (!clear_file(get_last_track_file()) ||
            !clear_file(get_cur_tracks_file()) ||
            !clear_file(get_cur_track_file()) ||
            !clear_file(get_log_file())) {
            error("Failed to clear output files");
            return false;
        }
        // log them to the debug console
        info("log file: %s", get_log_file().c_str());
        info("tracks file: %s", get_cur_tracks_file().c_str());
        info("last track file: %s", get_last_track_file().c_str());
        info("current track file: %s", get_cur_track_file().c_str());
    }

    // critical section to ensure all output files update together
    InitializeCriticalSection(&track_list_critsec);
    
    // semaphore will signal when there was a deck change and
    // a track needs to be logged to the output files
    hLogSem = CreateSemaphore(NULL, 0, 10000, "LoggingSemaphore");
    if (!hLogSem) {
        error("Failed to create logging semaphore");
        return false;
    }
    success("Created logging semaphore");

    return true;
}

// Push the index of a deck which has changed into the queue 
// for the logging thread to log the track of that deck
void push_deck_update(uint32_t deck_idx)
{
    // Ensure the deck changes queue is protected by a lock
    EnterCriticalSection(&track_list_critsec);
    // prepend this new track to the list
    deck_changes.push(deck_idx);
    // end of atomic operations
    LeaveCriticalSection(&track_list_critsec);
    // signal the semaphore to wake up the logging thread
    ReleaseSemaphore(hLogSem, 1, NULL);
}

// run the listener loop which waits for messages to update te output files
void run_listener()
{
    // Only write to log files in this safe thread, for some reason windows 8.1
    // doesn't like when we call CreateFile inside the threads that we hook.

    // This loop will wake up anytime a deck switches because the hook on 
    // notifyMasterChange() will call update_output_files(deck_idx) and that
    // will push the deck index which changed and simultaneously signal the log 
    // semaphore to wake this thread up
    while (WaitForSingleObject(hLogSem, INFINITE) == WAIT_OBJECT_0) {
        // pop the deck index that changed off the queue
        uint32_t deck_idx = pop_deck_update();

        // Create a new track via the deck index, this will actually lookup the 
        // row data ID of the deck index and then use that ID to call rekordbox 
        // functions and fetch the row data from the browser which is then used 
        // to fetch the full track information and store it locally within the 
        // track_data object
        track_data track(deck_idx);

#ifdef _DEBUG
        info("Logging deck %u:", track.idx);
        if (track.title.length())        { info("\ttitle: %s", track.title.c_str()); }
        if (track.artist.length())       { info("\tartist: %s", track.artist.c_str()); }
        if (track.album.length())        { info("\talbum: %s", track.album.c_str()); }
        if (track.genre.length())        { info("\tgenre: %s", track.genre.c_str()); }
        if (track.label.length())        { info("\tlabel: %s", track.label.c_str()); }
        if (track.key.length())          { info("\tkey: %s", track.key.c_str()); }
        if (track.orig_artist.length())  { info("\torig artist: %s", track.orig_artist.c_str()); }
        if (track.remixer.length())      { info("\tremixer: %s", track.remixer.c_str()); }
        if (track.composer.length())     { info("\tcomposer: %s", track.composer.c_str()); }
        if (track.comment.length())      { info("\tcomment: %s", track.comment.c_str()); }
        if (track.mix_name.length())     { info("\tmix name: %s", track.mix_name.c_str()); }
        if (track.lyricist.length())     { info("\tlyricist: %s", track.lyricist.c_str()); }
        if (track.date_created.length()) { info("\tdate created: %s", track.date_created.c_str()); }
        if (track.date_added.length())   { info("\tdate added: %s", track.date_added.c_str()); }
        if (track.track_number.length()) { info("\ttrack number: %s", track.track_number.c_str()); }
        if (track.bpm.length())          { info("\tbpm: %s", track.bpm.c_str()); }
#endif

        // build the string that will be logged to files
        string track_str = build_output(&track);

        // server mode?
        if (config.use_server) {
            // in server mode send the track to the server for logging
            send_network_message(track_str.c_str());
            info("Sending track [%s] to server %s...", 
                track_str.c_str(), config.server_ip.c_str());
        } else {
            // otherwise in non-server mode just directly update the log 
            // files with this new track
            update_log_files(track_str);
            info("Logging track [%s]...", track_str.c_str());
        }
    }
}

// update the various log files based on a track, only meant to be called
// from the logging thread so making this static
static void update_log_files(const string &track)
{
    // deque of tracks, deque instead of queue for iteration
    // static because we want to hold onto the previous tracks so 
    // we can update the various multi-line logs and last_track
    static deque<string> tracks;

    // store the track in the tracks list
    tracks.push_front(track);
    // make sure the list doesn't go beyond config.max_tracks
    if (tracks.size() > config.max_tracks) {
        tracks.pop_back();
    }

    // update the last x file by iterating tracks and writing
    if (tracks.size() > 0) {
        // concatenate the tracks into a single string
        string tracks_str;
        if (tracks.size() > 1) {
            for (auto it = tracks.begin() + 1; it != tracks.end(); it++) {
                tracks_str += it[0] + "\r\n";
            }
            // rewrite the tracks file with all of the lines at once
            if (!rewrite_file(get_cur_tracks_file(), tracks_str)) {
                error("Failed to write to tracks file");
            }
        }

        // rewrite the current track file with only the current track
        if (!rewrite_file(get_cur_track_file(), tracks.at(0))) {
            error("Failed to append to current track file");
        }
    }
    // rewrite the last track file with the previous track
    if (tracks.size() > 1) {
        if (!rewrite_file(get_last_track_file(), tracks.at(1))) {
            error("Failed to write last track file");
        }
    }
    // append the artist and track to the global log
    string log_entry;
    if (config.use_timestamps) {
        // timestamp with a space after it
        log_entry += get_timestamp_since_start() + " ";
    }
    // the rest of the current track output
    log_entry += tracks.front() + "\r\n";
    if (!append_file(get_log_file(), log_entry)) {
        error("Failed to log track to global log");
    }
}

// Pop the next deck change index out of the queue, only meant to
// be called from the logging thread so making this static
static uint32_t pop_deck_update()
{
    // Everything in this loop needs to happen at once otherwise
    EnterCriticalSection(&track_list_critsec);
    // First step is to grab the deck index which changed from the queue
    uint32_t deck_idx = deck_changes.front();
    deck_changes.pop();
    // end of atomic operations
    LeaveCriticalSection(&track_list_critsec);
    return deck_idx;
}

// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to) 
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

// calc time since the first time this was called
static string get_timestamp_since_start()
{
    static DWORD start_timestamp = 0;
    if (!start_timestamp) {
        start_timestamp = GetTickCount();
        return string("(00:00:00)");
    }
    DWORD elapsed = GetTickCount() - start_timestamp;
    // grab hours
    DWORD hr = elapsed / 3600000;
    elapsed -= (3600000 * hr);
    // grab minutes
    DWORD min = elapsed / 60000;
    elapsed -= (60000 * min);
    // grab seconds
    DWORD sec = elapsed / 1000;
    elapsed -= (1000 * sec);
    // return the timestamp
    char buf[256] = { 0 };
    if (snprintf(buf, sizeof(buf), "(%02u:%02u:%02u)", hr, min, sec) < 1) {
        return string("(00:00:00)");
    }
    return string(buf);
}

// helper to build output line based on configured output format
static string build_output(track_data *track)
{
    // start with out format
    string out = config.out_format;
    // replace various fields
    replace(out, "%title%", track->title);
    replace(out, "%artist%", track->artist);
    replace(out, "%album%", track->album);
    replace(out, "%genre%", track->genre);
    replace(out, "%label%", track->label);
    replace(out, "%key%", track->key);
    replace(out, "%orig_artist%", track->orig_artist);
    replace(out, "%remixer%", track->remixer);
    replace(out, "%composer%", track->composer);
    replace(out, "%comment%", track->comment);
    replace(out, "%mix_name%", track->mix_name);
    replace(out, "%lyricist%", track->lyricist);
    replace(out, "%date_created%", track->date_created);
    replace(out, "%date_added%", track->date_added);
    replace(out, "%track_number%", track->track_number);
    replace(out, "%bpm%", track->bpm);
    replace(out, "%time%", get_timestamp_since_start());
    return out;
}

// create and truncate a single file, only call this from the logger thread
static bool clear_file(const string &filename)
{
    // open with CREATE_ALWAYS to truncate any existing file and create any missing
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        error("Failed to open for truncate %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// append data to a file, only call this from the logger thread
static bool append_file(const string &filename, const string &data)
{
    // open for append, create new, or open existing
    HANDLE hFile = CreateFile(filename.c_str(), FILE_APPEND_DATA, 0, NULL, CREATE_NEW|OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        error("Failed to open file for append %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(hFile, data.c_str(), (DWORD)data.length(), &written, NULL)) {
        error("Failed to write to %s (%d)", filename.c_str(), GetLastError());
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// just a wrapper around clear and append
static bool rewrite_file(const string &filename, const string &data)
{
    return clear_file(filename) && append_file(filename, data);
}


