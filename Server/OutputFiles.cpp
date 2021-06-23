#include <Windows.h>

#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>

#include "Server.h"
#include "Config.h"

using namespace std;

// the different ways an output file can be written
#define MODE_REPLACE    0
#define MODE_APPEND     1
#define MODE_PREPEND    2

// a class to describe an output file that will be written 
class output_file
{
public:
    // construct an output file by parsing a config line
    output_file(const string &line);

    // function to log a track to an output file
    void log_track(const string &track_str);

    // public info of output files
    string name;
    // full path of the output file
    string path;
    uint32_t mode;
    uint32_t offset;
    uint32_t max_lines;

private:

    // cache previous lines to support offset
    // static because we want to hold onto the previous tracks so 
    // we can update the various multi-line logs and last_track
    deque<string> cached_lines;

    // helper routines

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

// map of name => output file
vector<output_file> output_files;

// initialize the output files
bool init_output_files()
{
    // create folder for output files
    if (!CreateDirectory(OUTPUT_FOLDER, NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            printf("Failed to created output directory\n");
            return false;
        }
    }

    // retrieve the output files
    string config_str;
    bool done_config = false;
    do {
        if (!receive_message(config_str)) {
            break;
        }
        istringstream iss(config_str);
        string line; 
        while (getline(iss, line)) {
            if (line == "config:end") {
                done_config = true;
                break;
            }
            output_files.push_back(output_file(line));
        }
    } while (!done_config);

    return done_config;
}

void log_track_to_output_file(uint32_t output_file_id, const string &track)
{
    if (output_files.size() <= output_file_id) {
        return;
    }
    output_files[output_file_id].log_track(track);
}

output_file::output_file(const string &line) 
{
    // format of a line is something like this:
    //  name=max_lines;offset;mode;format
    size_t pos = 0;
    string value;
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
    // the full path of the output file
    path = OUTPUT_FOLDER "\\" + name + ".txt";
    // clear the output file 
    if (!clear_file(path)) {
        printf("Failed to clear output file: %s\n", path.c_str());
    }
    printf("Loaded output file %s\n", name.c_str());
}

// log a song to file, stolen straight from the module code
void output_file::log_track(const string &track_str)
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

    printf("Logging [%s] to %s...\n", line.c_str(), name.c_str());

    // if the output file is in prepend mode
    switch (mode) {
    case MODE_REPLACE:
        if (!rewrite_file(path, line)) {
            printf("Failed to rewrite track in %s\n", path.c_str());
        }
        break;
    case MODE_APPEND:
        if (!append_file(path, line + "\r\n")) {
            printf("Failed to append track to %s\n", path.c_str());
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
            line = content;
        }
        if (max_lines > 0) {
            // rewrite the file with the line which is actually many lines
            if (!rewrite_file(path, line)) {
                printf("Failed to prepend track to %s", path.c_str());
            }
        } else {
            string temp_path = path + ".tmp";
            // otherwise if we're in unlimited prepend mode we need to use a temp file
            if (!rewrite_file(temp_path, line)) {
                printf("Failed to prepend track to %s", path.c_str());
            }
            // append the old output file onto the temp file
            if (!block_copy(path, temp_path)) {
                printf("Failed to block copy %s to %s", path.c_str(), temp_path.c_str());
            }
            // rename the temp file to the output file
            if (!MoveFileEx(temp_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                printf("Failed to rename %s to %s (%d)", temp_path.c_str(), path.c_str(), GetLastError());
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
        printf("Failed to open for truncate %s (%d)\n", filename.c_str(), GetLastError());
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
        printf("Failed to open file for append %s (%d)\n", filename.c_str(), GetLastError());
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(hFile, data.c_str(), (DWORD)data.length(), &written, NULL)) {
        printf("Failed to write to %s (%d)\n", filename.c_str(), GetLastError());
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
    if (start_pos == std::string::npos) {
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
