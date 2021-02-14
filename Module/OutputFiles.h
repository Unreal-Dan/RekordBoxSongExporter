#pragma once
#include <string>

// initialize all the output files
bool initialize_output_files();

// truncate a single file
bool clear_file(std::string filename);

// append data to a file
bool append_file(std::string filename, std::string data);

// updates the last_track, current_track, and current_tracks files
void update_track_list(std::string track, std::string artist);

// global log file path
std::string get_log_file();

// current track file path
std::string get_cur_track_file();

// last track file path
std::string get_last_track_file();

// current tracks file path
std::string get_cur_tracks_file();
