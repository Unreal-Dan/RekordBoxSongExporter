#pragma once
#include <string>

typedef enum deck_update_type_enum {
    UPDATE_TYPE_NORMAL = 0, // a track changed and the master switched
    UPDATE_TYPE_BPM,        // the bpm changed
    UPDATE_TYPE_ROUNDED_BPM,// the rounded bpm changed
    UPDATE_TYPE_TIME,       // the time changed
    UPDATE_TYPE_TOTAL_TIME, // the total time changed
    UPDATE_TYPE_MASTER,     // the master switched (track may not have changed)
} deck_update_type_t;

// initialize all the output files
bool initialize_output_files();

// load output files from config stream
void load_output_files();

// the number of loaded output files
size_t num_output_files();

// get the output file config string at index
std::string get_output_file_confline(size_t index);

// Push the index of a deck which has changed into the queue
// for the logging thread to log the track of that deck
void push_deck_update(uint32_t deck_idx, deck_update_type_t type);

// run the listener loop which waits for messages to update te output files
void run_listener();