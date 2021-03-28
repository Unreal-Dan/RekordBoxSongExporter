#pragma once

#include <string>

// initialize the output files
bool init_output_files();

// log a song to files
void log_track(const std::string &song);

// These come from the client over the network
extern size_t config_max_tracks;
extern bool config_use_timestamps;

