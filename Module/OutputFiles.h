#pragma once
#include <string>

// initialize all the output files
bool initialize_output_files();

// load output files from config stream
void load_output_files(std::ifstream &in);

// Push the index of a deck which has changed into the queue 
// for the logging thread to log the track of that deck
void push_deck_update(uint32_t deck_idx);

// run the listener loop which waits for messages to update te output files
void run_listener();