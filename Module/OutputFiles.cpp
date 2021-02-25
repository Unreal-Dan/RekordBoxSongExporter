#include <Windows.h>

#include <string>
#include <deque>
#include <queue>

#include "LastTrackStorage.h"
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

// simple local class for entries in the track deque
class track_entry
{
public:
    // initialize a new entry with the index of the deck to load
    // the track information from
    track_entry(uint32_t deck_idx) : idx(deck_idx)
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

// helper for in-place replacements
static bool replace(string &str, const string &from, const string &to);
// calc time since the first time this was called
static string get_timestamp_since_start();
// custom float to string that limits to .2f without any C++ stream funny business
static string float_to_string(float f);
// helper to build output line based on configured output format
static string build_output(track_entry *entry);
// truncate a single file, only call this from the logger thread
static bool clear_file(string filename);
// append data to a file, only call this from the logger thread
static bool append_file(string filename, string data);
// tell logger thread to rewrite a file with data
static bool rewrite_file(string filename, string data);

// index of decks that change
queue<uint32_t> deck_changes;

// critical section to protect track queue
CRITICAL_SECTION track_list_critsec;
HANDLE hLogSem;

// initialize all the output files
bool initialize_output_files()
{
    // clears all the track files
    if (!clear_file(get_last_track_file()) ||
        !clear_file(get_cur_tracks_file()) ||
        !clear_file(get_cur_track_file()) ||
        !clear_file(get_log_file())) {
        error("Failed to clear output files");
        return false;
    }

    // log them to the console
    info("log file: %s", get_log_file().c_str());
    info("tracks file: %s", get_cur_tracks_file().c_str());
    info("last track file: %s", get_last_track_file().c_str());
    info("current track file: %s", get_cur_track_file().c_str());

    // critical section to ensure all output files update together
    InitializeCriticalSection(&track_list_critsec);
    
    // semaphore will signal when there are messages to push to the
    // output files, or if any output files need to be cleared
    hLogSem = CreateSemaphore(NULL, 0, 10000, "LoggingSemaphore");
    if (!hLogSem) {
        error("Failed to create logging semaphore");
        return false;
    }
    success("Created logging semaphore");

    return true;
}

// run the listener loop which waits for messages to update te output files
void run_listener()
{
    // deque of tracks, deque instead of queue for iteration
    static deque<track_entry> tracks;

    // only write to log files in our safe thread, for some reason windows 8.1
    // doesn't like when we call CreateFile inside the threads that we hook so 
    // we use a threadsafe queue of messages to clear and append to output files 
    while (WaitForSingleObject(hLogSem, INFINITE) == WAIT_OBJECT_0) {
        EnterCriticalSection(&track_list_critsec);

        // prepend this new track to the list
        uint32_t deck_idx = deck_changes.front();
        deck_changes.pop();

        // the new track entry
        track_entry entry(deck_idx);
        tracks.push_front(entry);

        // make sure the list doesn't go beyond config.max_tracks
        if (tracks.size() > config.max_tracks) {
            // remove from the end
            tracks.pop_back();
        }

#ifdef _DEBUG
        info("Logging deck %u:", entry.idx);
        if (entry.title.length())        { info("\ttitle: %s", entry.title.c_str()); }
        if (entry.artist.length())       { info("\tartist: %s", entry.artist.c_str()); }
        if (entry.album.length())        { info("\talbum: %s", entry.album.c_str()); }
        if (entry.genre.length())        { info("\tgenre: %s", entry.genre.c_str()); }
        if (entry.label.length())        { info("\tlabel: %s", entry.label.c_str()); }
        if (entry.key.length())          { info("\tkey: %s", entry.key.c_str()); }
        if (entry.orig_artist.length())  { info("\torig artist: %s", entry.orig_artist.c_str()); }
        if (entry.remixer.length())      { info("\tremixer: %s", entry.remixer.c_str()); }
        if (entry.composer.length())     { info("\tcomposer: %s", entry.composer.c_str()); }
        if (entry.comment.length())      { info("\tcomment: %s", entry.comment.c_str()); }
        if (entry.mix_name.length())     { info("\tmix name: %s", entry.mix_name.c_str()); }
        if (entry.lyricist.length())     { info("\tlyricist: %s", entry.lyricist.c_str()); }
        if (entry.date_created.length()) { info("\tdate created: %s", entry.date_created.c_str()); }
        if (entry.date_added.length())   { info("\tdate added: %s", entry.date_added.c_str()); }
        if (entry.track_number.length()) { info("\ttrack number: %s", entry.track_number.c_str()); }
        if (entry.bpm.length())          { info("\tbpm: %s", entry.bpm.c_str()); }
#endif

        // update the last x file by iterating tracks and writing
        if (tracks.size() > 0) {
            // concatenate the tracks into a single string
            string tracks_str;
            for (auto it = tracks.begin(); it != tracks.end(); it++) {
                tracks_str += build_output(&it[0]) + "\r\n";
            }
            // rewrite the tracks file with all of the lines at once
            if (!rewrite_file(get_cur_tracks_file(), tracks_str)) {
                error("Failed to write to tracks file");
            }

            // rewrite the current track file with only the current track
            if (!rewrite_file(get_cur_track_file(), build_output(&tracks.at(0)))) {
                error("Failed to append to current track file");
            }
        }
        // rewrite the last track file with the previous track
        if (tracks.size() > 1) {
            if (!rewrite_file(get_last_track_file(), build_output(&tracks.at(1)))) {
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
        log_entry += build_output(&tracks.front()) + "\r\n";
        if (!append_file(get_log_file(), log_entry)) {
            error("Failed to log track to global log");
        }
        // end of atomic operations
        LeaveCriticalSection(&track_list_critsec);
    }
}

// updates the last_track, current_track, and current_tracks files
// this can be called from any thread safely
void update_output_files(uint32_t deck_idx)
{
    // make sure no two threads can simultaneously access the 
    // tracks global or any other globals, also make sure all 
    // the operations in this function are atomic so that all
    // of the output files stay in sync
    EnterCriticalSection(&track_list_critsec);

    // prepend this new track to the list
    deck_changes.push(deck_idx);

    // end of atomic operations
    LeaveCriticalSection(&track_list_critsec);

    // increment the semaphore to say we have a track to log
    ReleaseSemaphore(hLogSem, 1, NULL);
}

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
static string build_output(track_entry *entry)
{
    // start with out format
    string out = config.out_format;
    // replace various fields
    replace(out, "%title%", entry->title);
    replace(out, "%artist%", entry->artist);
    replace(out, "%album%", entry->album);
    replace(out, "%genre%", entry->genre);
    replace(out, "%label%", entry->label);
    replace(out, "%key%", entry->key);
    replace(out, "%orig_artist%", entry->orig_artist);
    replace(out, "%remixer%", entry->remixer);
    replace(out, "%composer%", entry->composer);
    replace(out, "%comment%", entry->comment);
    replace(out, "%mix_name%", entry->mix_name);
    replace(out, "%lyricist%", entry->lyricist);
    replace(out, "%date_created%", entry->date_created);
    replace(out, "%date_added%", entry->date_added);
    replace(out, "%track_number%", entry->track_number);
    replace(out, "%bpm%", entry->bpm);
    replace(out, "%time%", get_timestamp_since_start());
    return out;
}

// truncate a single file, only call this from the logger thread
static bool clear_file(string filename)
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
static bool append_file(string filename, string data)
{
    // open for append, create new or open existing
    HANDLE hFile = CreateFile(filename.c_str(), FILE_APPEND_DATA, 0, NULL, CREATE_NEW|OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        error("Failed to open file for append %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    // write data
    DWORD written = 0;
    if (!WriteFile(hFile, data.c_str(), (DWORD)data.length(), &written, NULL)) {
        error("Failed to write to %s (%d)", filename.c_str(), GetLastError());
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// tell logger thread to rewrite a file with data
static bool rewrite_file(string filename, string data)
{
    return clear_file(filename) && append_file(filename, data);
}


