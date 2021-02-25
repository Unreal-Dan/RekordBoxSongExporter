#pragma once
#include <inttypes.h>

#include <string>

// must call this to initialize the storage
bool initialize_last_track_storage();

// whether this track has been logged
void set_logged(uint32_t deck, bool logged);
bool get_logged(uint32_t deck);

// the current master index
void set_master(uint32_t deck_idx);
uint32_t get_master();

// the current master index
void set_track_id(uint32_t deck_idx, uint32_t id);
uint32_t get_track_id(uint32_t deck_idx);


