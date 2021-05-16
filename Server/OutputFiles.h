#pragma once

#include <string>

// initialize the output files
bool init_output_files();

// log a song to files
void log_track_to_output_file(uint32_t output_file_id, const std::string &track);


