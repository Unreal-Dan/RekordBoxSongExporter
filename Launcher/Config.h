#pragma once
#include <Windows.h>
#include <inttypes.h>

#include <string>

// the image base for various stuff
extern HINSTANCE imageBase;

// the file path of the config.ini file
std::string get_config_path();

// the version string from config
std::string conf_load_version();

// the path of rekordbox from config
std::string conf_load_path();

// the output format string from config
std::string conf_load_out_format();

// the number of lines in cur tracks
std::string conf_load_cur_tracks_count();

// whether to emit timetsamps in global og
bool conf_load_use_timestamps();

// whether to use the server
bool conf_load_use_server();

// the remote server ip
std::string conf_load_server_ip();

// ==========================
// saving functions

// save the version
bool conf_save_version(std::string version);

// save the rbox path
bool conf_save_path(std::string path);

// save the output format
bool conf_save_out_format(std::string out_format);

// save the cur tracks line count
bool conf_save_cur_tracks_count(std::string num_tracks);

// save the use timestamps boolean
bool conf_save_use_timestamps(bool use_timestamps);

// whether to use the server
bool conf_save_use_server(bool use_server);

// save the server ip string
bool conf_save_server_ip(std::string server_ip);

