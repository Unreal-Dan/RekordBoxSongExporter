#pragma once
#include <inttypes.h>

#include <string>

// must call this to initialize the storage
bool initialize_last_track_storage();

// store info in global threadsafe storage
void set_last_track(std::string track, uint32_t deck);
void set_last_artist(std::string artist, uint32_t deck);
// whether this track has been logged
void set_logged(uint32_t deck, bool logged);

// grab info out of global threadsafe storage
std::string get_last_track(uint32_t deck);
std::string get_last_artist(uint32_t deck);
// whether this track has been logged
bool get_logged(uint32_t deck);

// the current master index
void set_master(uint32_t);
uint32_t get_master();
