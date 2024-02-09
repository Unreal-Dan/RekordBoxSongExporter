#pragma once
#include <vector>
#include <string>

// don't think this can be anything else
// if you change this things will probably break
#define NUM_DECKS       4

// port for network comms
#define DEFAULT_PORT    "22345"

// name of output folder
#define OUTPUT_FOLDER   "OutputFiles"

// the number of server updates that can be sent per second
#define DEFAULT_UPDATE_RATE 500

// enum of versions we support
typedef enum rbox_version_enum
{
    RBVER_UNK = -1,

    // We retain entries for these version but they are no longer supported
    // in this version of the software, only legacy
    RBVER_585, // 5.8.5
    RBVER_650, // 6.5.0
    RBVER_651, // 6.5.1
    RBVER_652, // 6.5.2
    RBVER_653, // 6.5.3
    RBVER_661, // 6.6.1
    RBVER_662, // 6.6.2
    RBVER_663, // 6.6.3

    // only supporting this version+
    RBVER_664, // 6.6.4
    RBVER_6610, // 6.6.10
    RBVER_6611, // 6.6.11
    RBVER_670, // 6.7.0
    RBVER_675, // 6.7.5

    RBVER_COUNT // the number of versions supported
} rbox_version_t;

// configuration options
class Config
{
public:
    Config() : version(RBVER_UNK), use_server(false), server_ip(), output_files(), update_rate(500) {}
    // the version loaded from config
    rbox_version_t version;
    // whether to use server mode
    bool use_server;
    // a server ip if there is one
    std::string server_ip;
    // a vector of output file config strings
    std::vector<std::string> output_files;
    // a secret update_rate config for maxing out the server command update rate
    uint32_t update_rate;
};

// global config object
extern Config config;

// load configuration from the config file
bool initialize_config();

// path/folder where the module exists
std::string get_dll_path();

// rekordbox.exe base address
uintptr_t rb_base();
