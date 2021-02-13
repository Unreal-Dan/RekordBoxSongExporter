#pragma once
#include <string>

// must call this to initialize the storage
bool initialize_last_track_storage();

// store info in global threadsafe storage
void set_last_track(std::string track);
void set_last_artist(std::string artist);

// grab info out of global threadsafe storage
std::string get_last_track();
std::string get_last_artist();

