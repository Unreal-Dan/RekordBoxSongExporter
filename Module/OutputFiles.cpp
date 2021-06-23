#include <Windows.h>

#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <queue>

#include "LastTrackStorage.h"
#include "NetworkClient.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Log.h"

// the different ways an output file can be written
#define MODE_REPLACE    0
#define MODE_APPEND     1
#define MODE_PREPEND    2

using namespace std;

// simple local class to describe a track and it's metadata
class track_data
{
public:
    // initialize a new track with the index of the deck to load
    // the track information from
    track_data(uint32_t deck_idx);
   
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

// Bitflags to describe the various tags that can be used when a config is 
// loaded the format is scanned for tags and a field is set with each tag 
// to optimize replacements later on when the system has to replace fields 
// as a track changes
typedef enum out_tag_enum
{
    TAG_TITLE =         (1 << 0),
    TAG_ARTIST =        (1 << 1),
    TAG_ALBUM =         (1 << 2),
    TAG_GENRE =         (1 << 3),
    TAG_LABEL =         (1 << 4),
    TAG_KEY =           (1 << 5),
    TAG_ORIG_ARTIST =   (1 << 6),
    TAG_REMIXER =       (1 << 7),
    TAG_COMPOSER =      (1 << 8),
    TAG_COMMENT =       (1 << 9),
    TAG_MIX_NAME =      (1 << 10),
    TAG_LYRICIST =      (1 << 11),
    TAG_DATE_CREATED =  (1 << 12),
    TAG_DATE_ADDED =    (1 << 13),
    TAG_TRACK_NUMBER =  (1 << 14),
    TAG_BPM =           (1 << 15),
    TAG_TIME =          (1 << 16),
} out_tag_t;

// a class to describe an output file that will be written 
class output_file
{
public:
    // construct an output file by parsing a config line
    output_file(const string &line);

    // function to log a track to an output file
    void log_track(track_data *track);

    // the original configuration line
    string confline;
    // public info of output files
    string name;
    string format;
    // full path of the output file
    string path;
    // bitflags describing which %tags% are used
    uint32_t format_tags;
    uint32_t mode;
    uint32_t offset;
    uint32_t max_lines;

    // a unique ID for sending the track to server
    uint32_t id;

private:
    // a counter to give each module a unique incrementing ID
    static uint32_t id_counter;

    // cache previous lines to support offset
    // static because we want to hold onto the previous tracks so 
    // we can update the various multi-line logs and last_track
    deque<string> cached_lines;

    // helper routines

    // helper to build output line based on configured output format
    string build_output(const string &format, uint32_t format_tags, track_data *track);
    // update the various log files based on a track, only meant to be called
    // from the logging thread so making this static
    void update_output_file(const string &track_str);
    // read a file and append it to another in blocks
    bool block_copy(const string &source, const string &dest);
    // create and truncate a single file, only call this from the logger thread
    bool clear_file(const string &filename);
    // append data to a file, only call this from the logger thread
    bool append_file(const string &filename, const string &data);
    // just a wrapper around clear and append
    bool rewrite_file(const string &filename, const string &data);
    // helper for in-place replacements
    bool replace(string &str, const string &from, const string &to);
    // calc time since the first time this was called
    string get_timestamp_since_start();
};

// the ID counter for output files
uint32_t output_file::id_counter = 0;

// Pop the next deck change index out of the queue, only meant to
// be called from the logging thread so making this static
static uint32_t pop_deck_update();
// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to);
// calc time since the first time this was called
static string get_timestamp_since_start();

// queue of indexes of decks that changed
// For example if deck 1 changes then 1 gets pushed into this
queue<uint32_t> deck_changes;

// the list of output files loaded from the config
vector<output_file> output_files;

// critical section to protect track queue
CRITICAL_SECTION track_list_critsec;
// semaphore to signal when a deck has changed and there are
// entries in the deck_changes queue
HANDLE hLogSem;

// initialize all the output files
bool initialize_output_files()
{
    // critical section to ensure all output files update together
    InitializeCriticalSection(&track_list_critsec);
    
    // semaphore will signal when there was a deck change and
    // a track needs to be logged to the output files
    hLogSem = CreateSemaphore(NULL, 0, 10000, "LoggingSemaphore");
    if (!hLogSem) {
        error("Failed to create logging semaphore");
        return false;
    }

    // if not in server mode
    if (!config.use_server) {
        // create folder for output files
        string out_folder = get_dll_path() + "\\" OUTPUT_FOLDER;
        if (!CreateDirectory(out_folder.c_str(), NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                error("Failed to created output directory");
                return false;
            }
        }
    }

    success("Initialized output folder and files");

    return true;
}

// called by config loader when it's loading the config file
void load_output_files(ifstream &in)
{
    string line;
    while (getline(in, line)) {
        output_files.push_back(output_file(line));
    }
}

// the number of loaded output files
size_t num_output_files()
{
    return output_files.size();
}

// get the output file config string at index
std::string get_output_file_confline(size_t index)
{
    if (output_files.size() <= index) {
        return "";
    }
    return output_files[index].confline;
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
    // notifyMasterChange() will call push_deck_update(deck_idx) and that
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
        // iterate the list of output files and populate them
        for (auto outfile = output_files.begin(); outfile != output_files.end(); ++outfile) {
            // log the track to the output file
            outfile->log_track(&track);
        }
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

track_data::track_data(uint32_t deck_idx) : idx(deck_idx)
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

output_file::output_file(const string &line) 
{
    // format of a line is something like this:
    //  name=max_lines;offset;mode;format
    size_t pos = 0;
    string value;
    // grab the next id
    id = id_counter++;
    // the confline is the entire input line
    confline = line;
    // read out the name up till =
    pos = line.find_first_of("=");
    name = line.substr(0, pos);
    value = line.substr(pos + 1);
    // mode
    pos = value.find_first_of(";");
    mode = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // offset
    pos = value.find_first_of(";");
    offset = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // max lines
    pos = value.find_first_of(";");
    max_lines = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // the format is everything left over
    format = value;
    // check for tags and set bitflags so that performing 
    // replacements later will be optimized because we will
    // know exactly which tags need to be replaced
    if (format.find("%title%") != string::npos)         { format_tags |= TAG_TITLE; }
    if (format.find("%artist%") != string::npos)        { format_tags |= TAG_ARTIST; }
    if (format.find("%album%") != string::npos)         { format_tags |= TAG_ALBUM; }
    if (format.find("%genre%") != string::npos)         { format_tags |= TAG_GENRE; }
    if (format.find("%label%") != string::npos)         { format_tags |= TAG_LABEL; }
    if (format.find("%key%") != string::npos)           { format_tags |= TAG_KEY; }
    if (format.find("%orig_artist%") != string::npos)   { format_tags |= TAG_ORIG_ARTIST; }
    if (format.find("%remixer%") != string::npos)       { format_tags |= TAG_REMIXER; }
    if (format.find("%composer%") != string::npos)      { format_tags |= TAG_COMPOSER; }
    if (format.find("%comment%") != string::npos)       { format_tags |= TAG_COMMENT; }
    if (format.find("%mix_name%") != string::npos)      { format_tags |= TAG_MIX_NAME; }
    if (format.find("%lyricist%") != string::npos)      { format_tags |= TAG_LYRICIST; }
    if (format.find("%date_created%") != string::npos)  { format_tags |= TAG_DATE_CREATED; }
    if (format.find("%date_added%") != string::npos)    { format_tags |= TAG_DATE_ADDED; }
    if (format.find("%track_number%") != string::npos)  { format_tags |= TAG_TRACK_NUMBER; }
    if (format.find("%bpm%") != string::npos)           { format_tags |= TAG_BPM; }
    if (format.find("%time%") != string::npos)          { format_tags |= TAG_TIME; }
    // the full path of the output file
    path = get_dll_path() + "\\" OUTPUT_FOLDER "\\" + name + ".txt";
    // only clear the output file if not server mode
    if (!config.use_server && !clear_file(path)) {
        error("Failed to clear output file: %s", path.c_str());
    }
    info("Loaded output file: %s", name.c_str());
}

// function to log a track to an output file
void output_file::log_track(track_data *track)
{
    // build the string that will be logged to this output file
    string track_str = build_output(format, format_tags, track);

    // server mode?
    if (config.use_server) {
        // in server mode send the filename + track to the server for logging
        string message = to_string(id) + ":" + track_str;
        send_network_message(message.c_str());
        info("Sending track [%s] to file %s at server %s...",
            track_str.c_str(), name.c_str(), config.server_ip.c_str());
    } else {
        // otherwise in non-server mode just directly update the log 
        // files with this new track
        update_output_file(track_str);
    }
}

// helper to build output line based on configured output format
string output_file::build_output(const string &format, uint32_t format_tags, track_data *track)
{
    // start with the format
    string out = format;
    // only replace fields if the tag was previously detected in the format
    // this is a major optimization to avoid attempting to relace every tag
    if (format_tags & TAG_TITLE)        { replace(out, "%title%", track->title); }
    if (format_tags & TAG_ARTIST)       { replace(out, "%artist%", track->artist); }
    if (format_tags & TAG_ALBUM)        { replace(out, "%album%", track->album); }
    if (format_tags & TAG_GENRE)        { replace(out, "%genre%", track->genre); }
    if (format_tags & TAG_LABEL)        { replace(out, "%label%", track->label); }
    if (format_tags & TAG_KEY)          { replace(out, "%key%", track->key); }
    if (format_tags & TAG_ORIG_ARTIST)  { replace(out, "%orig_artist%", track->orig_artist); }
    if (format_tags & TAG_REMIXER)      { replace(out, "%remixer%", track->remixer); }
    if (format_tags & TAG_COMPOSER)     { replace(out, "%composer%", track->composer); }
    if (format_tags & TAG_COMMENT)      { replace(out, "%comment%", track->comment); }
    if (format_tags & TAG_MIX_NAME)     { replace(out, "%mix_name%", track->mix_name); }
    if (format_tags & TAG_LYRICIST)     { replace(out, "%lyricist%", track->lyricist); }
    if (format_tags & TAG_DATE_CREATED) { replace(out, "%date_created%", track->date_created); }
    if (format_tags & TAG_DATE_ADDED)   { replace(out, "%date_added%", track->date_added); }
    if (format_tags & TAG_TRACK_NUMBER) { replace(out, "%track_number%", track->track_number); }
    if (format_tags & TAG_BPM)          { replace(out, "%bpm%", track->bpm); }
    if (format_tags & TAG_TIME)         { replace(out, "%time%", get_timestamp_since_start()); }
    return out;
}

// update the various log files based on a track, only meant to be called
// from the logging thread so making this static
void output_file::update_output_file(const string &track_str)
{
    // the line being stored defaults to the current track
    string line = track_str;
    // but if there is an offset or max lines then we need to
    // utilize the cache and may need to access past tracks
    if (max_lines || offset) {
        // store the line in the cache
        cached_lines.push_front(line);
        // then trim the cache to the max lines + offset
        if (cached_lines.size() > (((size_t)max_lines + offset) + 1)) {
            cached_lines.pop_back();
        }
        // if we haven't cached up till the offset then return
        if (cached_lines.size() <= offset) {
            return;
        }
        // grab the line at offset out of the cache
        line = cached_lines.at(offset);
    }

    info("Logging [%s] to %s...", line.c_str(), name.c_str());

    // if the output file is in prepend mode
    switch (mode) {
    case MODE_REPLACE:
        if (!rewrite_file(path, line)) {
            error("Failed to rewrite track in %s", path.c_str());
        }
        break;
    case MODE_APPEND:
        if (!append_file(path, line + "\r\n")) {
            error("Failed to append track to %s", path.c_str());
        }
        break;
    case MODE_PREPEND:
        // Rewrite the entire file in prepend mode.
        if (cached_lines.size() > 0) {
            // If there is any cached lines we need to implode them and
            // rewrite the file with that instead
            auto it = cached_lines.begin() + offset;
            size_t lines = 0;
            string content;
            do {
                content += lines ? "\r\n" + it[0] : it[0];
            } while (++it != cached_lines.end() && ++lines < max_lines);
            // replace the 'line' with the multiline imploded cache
            line = content;
        }
        if (max_lines > 0) {
            // rewrite the file with the line which is actually many lines
            if (!rewrite_file(path, line)) {
                error("Failed to prepend track to %s", path.c_str());
            }
        } else {
            string temp_path = path + ".tmp";
            // otherwise if we're in unlimited prepend mode we need to use a temp file
            if (!rewrite_file(temp_path, line)) {
                error("Failed to prepend track to %s", path.c_str());
            }
            // append the old output file onto the temp file
            if (!block_copy(path, temp_path)) {
                error("Failed to block copy %s to %s", path.c_str(), temp_path.c_str());
            }
            // rename the temp file to the output file
            if (!MoveFileEx(temp_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                error("Failed to rename %s to %s (%d)", temp_path.c_str(), path.c_str(), GetLastError());
            }
        }
        break;
    }
}

// read a file and append it to another in blocks
bool output_file::block_copy(const string &source, const string &dest)
{
    char block[4096] = { 0 };
    bool newline = false;
    // open file
    fstream in;
    fstream out;
    in.open(source, ios::binary | ios::in);
    out.open(dest, ios::binary | ios::out | ios::app);

    size_t amt_read = 0;

    out.seekp(0, ios_base::end);

    // read blocks of data and append data to output
    do {
        // read block
        in.read(block, sizeof(block));
        amt_read = in.gcount();
        // if there is no content to append then bail out 
        // and don't add the newline or anything
        if (!amt_read) {
            break;
        }
        // need to append a newline before the new content
        if (!newline) {
            newline = true;
            out.write("\r\n", 2);
        }
        out.write(block, amt_read);
    } while (!in.eof());
    in.close();
    out.close();
    return true;
}

// create and truncate a single file, only call this from the logger thread
bool output_file::clear_file(const string &filename)
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
bool output_file::append_file(const string &filename, const string &data)
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
bool output_file::rewrite_file(const string &filename, const string &data)
{
    return clear_file(filename) && append_file(filename, data);
}

// helper for in-place replacements
bool output_file::replace(string& str, const string& from, const string& to) 
{
    size_t start_pos = str.find(from);
    if (start_pos == string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

// calc time since the first time this was called
string output_file::get_timestamp_since_start()
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
