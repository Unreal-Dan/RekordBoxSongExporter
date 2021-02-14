#include <Windows.h>

#include <string>
#include <deque>

#include "OutputFiles.h"
#include "Config.h"
#include "Log.h"

// This is the file containing the last MAX_TRACKS songs
#define CUR_TRACKS_FILE "current_tracks.txt"

// This file contains the current track only
#define CUR_TRACK_FILE  "current_track.txt"

// This file contains the last track only
#define LAST_TRACK_FILE "last_track.txt"

// this is the logfile for all songs played
#define TRACK_LOG_FILE  "played_tracks.txt"

using namespace std;

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

    return true;
}

// truncate a single file
bool clear_file(string filename)
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

// append data to a file
bool append_file(string filename, string data)
{
    // open for append, create new or open existing
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

    info("Logging Track: %s - %s", track.c_str(), artist.c_str());

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
            tracks.append(it->artist).append(" - ").append(it->track).append( "\n");
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

    // log to the global log
    if (!append_file(get_log_file(), artist + " - " + track + "\n")) {
        error("Failed to log track to global log");
    }
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