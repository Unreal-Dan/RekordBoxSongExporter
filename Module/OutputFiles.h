#pragma once
#include <string>

// initialize all the output files
bool initialize_output_files();

// updates the last_track, current_track, and current_tracks files
void update_output_files(std::string track, std::string artist);

// run the log listener loop which waits for messages to clear files or write to files
void run_log_listener();

// global log file path
std::string get_log_file();

// current track file path
std::string get_cur_track_file();

// last track file path
std::string get_last_track_file();

// current tracks file path
std::string get_cur_tracks_file();
