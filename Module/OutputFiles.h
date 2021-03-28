#pragma once
#include <string>

// global log file path
std::string get_log_file();

// current track file path
std::string get_cur_track_file();

// last track file path
std::string get_last_track_file();

// current tracks file path
std::string get_cur_tracks_file();

// initialize all the output files
bool initialize_output_files();

// Push the index of a deck which has changed into the queue 
// for the logging thread to log the track of that deck
void push_deck_update(uint32_t deck_idx);

// run the listener loop which waits for messages to update te output files
void run_listener();