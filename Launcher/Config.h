#pragma once
#include <Windows.h>
#include <inttypes.h>

#include <vector>
#include <string>

// The current version of RBSE that appears in the titlebar
#define RBSE_VERSION            "3.0"
// maximum length of output file names
#define MAX_OUTFILE_NAME_LEN    64
// maximum length of output formats
#define MAX_OUTFILE_FORMAT_LEN  512
// the default entry that is added when clicking 'add'
#define DEFAULT_OUTPUT_FILE     "OutputFile=0;0;0;%title%"
// the list of default output files if the config is missing
#define DEFAULT_OUTPUT_FILES                                    \
    {                                                           \
        "TrackTitle=0;0;0;%title%",                             \
        "TrackArtist=0;0;0;%artist%",                           \
        "LastTrackTitle=0;1;0;%title%",                         \
        "LastTrackArtist=0;1;0;%artist%",                       \
        "TrackList=1;0;0;%time% %artist% - %title%",            \
        "RotatingTrackList=2;2;5;%time% %artist% - %title%",    \
    }

// the image base for various stuff
extern HINSTANCE imageBase;

// describes the contents of the config files
class Config
{
public:
    // the version of rekordbox being loaded
    std::string version;
    std::string path;
    bool use_server;
    std::string server_ip;
};

// the currently loaded config
extern Config config; 

// load the global config from file
bool config_load();

// save the global config to file
bool config_save();
