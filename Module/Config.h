#pragma once
#include <string>

// don't think this can be anything else
// if you change this things will probably break
#define NUM_DECKS 4

// port for network comms
#define DEFAULT_PORT "22345"

// enum of versions we support
typedef enum rbox_version_enum
{
    RBVER_UNK = -1,

    RBVER_585, // 5.8.5
    RBVER_650, // 6.5.0
    RBVER_651, // 6.5.1

    RBVER_COUNT // the number of versions supported
} rbox_version_t;

// configuration options
typedef struct config_struct
{
    // the version loaded from config
    rbox_version_t rbox_version;

    // the output format
    std::string out_format;

    // the max tracks to log in current tracks
    uint32_t max_tracks;

    // whether to use timestamps in global log
    bool use_timestamps;

    // whether to use server mode
    bool use_server;

    // a server ip if there is one
    std::string server_ip;
} config_t;

// global config object
extern config_t config;

bool initialize_config();

// path/folder where the module exists
std::string get_dll_path();

// rekordbox.exe base address
uintptr_t rb_base();
