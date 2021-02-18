#include <Windows.h>

#include <string>
#include <deque>
#include <queue>

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

// tell logger thread to clear a file
static bool clear(string filename);
// tell logger thread to append a line to a file
static bool append(string filename, string data);
// tell logger thread to rewrite a file with data
static bool rewrite(string filename, string data);
// truncate a single file, only call this from the main thread
static bool clear_file(string filename);
// append data to a file, only call this from the main thread
static bool append_file(string filename, string data);

// simple local class for entries in the track deque
class track_entry
{
public:
    track_entry(string track, string artist, string genre) :
        track(track), artist(artist), genre(genre) {}
    string track;
    string artist;
    string genre;
};

// deque of tracks, deque instead of queue for iteration
deque<track_entry> tracks;

// queue of filenames and messages
queue<string> fileNames;
queue<string> messages;

// filenames to clear
queue<string> clearFileNames;

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

// run the log listener loop which waits for messages to clear files or write to files
void run_log_listener()
{
    // only write to log files in our safe thread, for some reason windows 8.1
    // doesn't like when we call CreateFile inside the threads that we hook so 
    // we use a threadsafe queue of messages to clear and append to output files 
    while (WaitForSingleObject(hLogSem, INFINITE) == WAIT_OBJECT_0) {
        // if the clearFileNames has entries then we need to clear those files
        if (clearFileNames.size() > 0) {
            if (!clear_file(clearFileNames.front())) {
                error("Failed to clear file %s", clearFileNames.front().c_str());
            }
            clearFileNames.pop();
            continue;
        }
        // make sure there's filenames and messages to log
        if (!fileNames.size() || !messages.size()) {
            continue;
        }
        if (!append_file(fileNames.front(), messages.front())) {
            error("Failed to append [%s] to %s", messages.front().c_str(), 
                fileNames.front().c_str());
        }
        fileNames.pop();
        messages.pop();
    }
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

// helper to build output line based on configured output format
static string build_output(track_entry *entry)
{
    // start with out format
    string out = config.out_format;
    // replace various fields
    replace(out, "%artist%", entry->artist);
    replace(out, "%track%", entry->track);
    // maybe soon:
    //replace(out, "%genre%", entry->genre);
    return out;
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

// updates the last_track, current_track, and current_tracks files
// this can be called from any thread safely
void update_output_files(string track, string artist)
{
    // make sure no two threads can simultaneously access the 
    // tracks global or any other globals, also make sure all 
    // the operations in this function are atomic so that all
    // of the output files stay in sync
    EnterCriticalSection(&track_list_critsec);

    info("Logging [%s]", track.c_str());

    // prepend this new track to the list
    tracks.push_front(track_entry(track, artist, ""));
    // make sure the list doesn't go beyond config.max_tracks
    if (tracks.size() > config.max_tracks) {
        // remove from the end
        tracks.pop_back();
    }
 
    // update the last x file by iterating track_queue and writing
    if (tracks.size() > 0) {
        // concatenate the tracks into a single string
        string tracks_str;
        for (auto it = tracks.begin(); it != tracks.end(); it++) {
            tracks_str += build_output(&it[0]) + "\r\n";
        }
        // rewrite the tracks file with all of the lines at once
        if (!rewrite(get_cur_tracks_file(), tracks_str)) {
            error("Failed to write to tracks file");
        }

        // rewrite the current track file with only the current track
        if (!rewrite(get_cur_track_file(), build_output(&tracks.at(0)))) {
            error("Failed to append to current track file");
        }
    }
    // rewrite the last track file with the previous track
    if (tracks.size() > 1) {
        if (!rewrite(get_last_track_file(), build_output(&tracks.at(1)))) {
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
    if (!append(get_log_file(), log_entry)) {
        error("Failed to log track to global log");
    }

    // end of atomic operations
    LeaveCriticalSection(&track_list_critsec);
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

// tell logger thread to clear a file
static bool clear(string filename)
{
    clearFileNames.push(filename);
    ReleaseSemaphore(hLogSem, 1, NULL);
    return true;
}

// tell logger thread to append a line to a file
static bool append(string filename, string data)
{
    fileNames.push(filename);
    messages.push(data);
    ReleaseSemaphore(hLogSem, 1, NULL);
    return true;
}

// tell logger thread to rewrite a file with data
static bool rewrite(string filename, string data)
{
    return clear(filename) && append(filename, data);
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
