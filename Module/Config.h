#pragma once
#include <string>

// don't think this can be anything else
// if you change this things will probably break
#define NUM_DECKS       4

// port for network comms
#define DEFAULT_PORT    "22345"

// name of output folder
#define OUTPUT_FOLDER   "OutputFiles"

// enum of versions we support
typedef enum rbox_version_enum
{
    RBVER_UNK = -1,

    RBVER_585, // 5.8.5
    RBVER_650, // 6.5.0
    RBVER_651, // 6.5.1
    RBVER_652, // 6.5.2

    RBVER_COUNT // the number of versions supported
} rbox_version_t;

// configuration options
class Config
{
public:
    // the version loaded from config
    rbox_version_t version;
    // whether to use server mode
    bool use_server;
    // a server ip if there is one
    std::string server_ip;
};

// global config object
extern Config config;

// load configuration from the config file
bool initialize_config();

// path/folder where the module exists
std::string get_dll_path();

// rekordbox.exe base address
uintptr_t rb_base();
