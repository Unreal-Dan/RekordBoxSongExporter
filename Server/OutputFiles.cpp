#include <Windows.h>

#include <deque>
#include <string>

#include "Config.h"

using namespace std;

// create and truncate a single file, only call this from the logger thread
static bool clear_file(string filename);
// append data to a file, only call this from the logger thread
static bool append_file(string filename, string data);
// just a wrapper around clear and append
static bool rewrite_file(string filename, string data);
// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to) ;
// calc time since the first time this was called
static string get_timestamp_since_start();

// initialize the output files
bool init_output_files()
{
    // clears all the track files (this initially creates them)
    if (!clear_file(LAST_TRACK_FILE) ||
        !clear_file(CUR_TRACKS_FILE) ||
        !clear_file(CUR_TRACK_FILE) ||
        !clear_file(TRACK_LOG_FILE)) {
        printf("Failed to clear output files");
        return false;
    }

    return true;
}

// log a song to files
void log_track(string song)
{
    // deque of tracks, deque instead of queue for iteration
    static deque<string> tracks;

    // store the track in the tracks list
    tracks.push_front(song);
    // make sure the list doesn't go beyond config.max_tracks
    if (tracks.size() > MAX_TRACKS) {
        tracks.pop_back();
    }

    // update the last x file by iterating tracks and writing
    if (tracks.size() > 0) {
        // concatenate the tracks into a single string
        string tracks_str;
        for (auto it = tracks.begin(); it != tracks.end(); it++) {
            tracks_str += it[0] + "\r\n";
        }
        // rewrite the tracks file with all of the lines at once
        if (!rewrite_file(CUR_TRACKS_FILE, tracks_str)) {
            printf("Failed to write to tracks file");
        }

        // rewrite the current track file with only the current track
        if (!rewrite_file(CUR_TRACK_FILE, tracks.at(0))) {
            printf("Failed to append to current track file");
        }
    }
    // rewrite the last track file with the previous track
    if (tracks.size() > 1) {
        if (!rewrite_file(LAST_TRACK_FILE, tracks.at(1))) {
            printf("Failed to write last track file");
        }
    }
    // append the artist and track to the global log
    string log_entry;
    if (USE_TIMESTAMPS) {
        // timestamp with a space after it
        log_entry += get_timestamp_since_start() + " ";
    }
    // the rest of the current track output
    log_entry += tracks.front() + "\r\n";
    if (!append_file(TRACK_LOG_FILE, log_entry)) {
        printf("Failed to log track to global log");
    }
}

// create and truncate a single file, only call this from the logger thread
static bool clear_file(string filename)
{
    // open with CREATE_ALWAYS to truncate any existing file and create any missing
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open for truncate %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// append data to a file, only call this from the logger thread
static bool append_file(string filename, string data)
{
    // open for append, create new, or open existing
    HANDLE hFile = CreateFile(filename.c_str(), FILE_APPEND_DATA, 0, NULL, CREATE_NEW|OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file for append %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(hFile, data.c_str(), (DWORD)data.length(), &written, NULL)) {
        printf("Failed to write to %s (%d)", filename.c_str(), GetLastError());
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// just a wrapper around clear and append
static bool rewrite_file(string filename, string data)
{
    return clear_file(filename) && append_file(filename, data);
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
