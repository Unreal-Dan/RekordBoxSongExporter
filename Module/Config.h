#pragma once
#include <string>

// Pick one of these, either 585 or 650
#define REKORDBOX_650
//#define REKORDBOX_585

// This is the number of songs that will be kept in current_tracks.txt
#define MAX_TRACKS       3

// path/folder where the module exists
std::string get_dll_path();