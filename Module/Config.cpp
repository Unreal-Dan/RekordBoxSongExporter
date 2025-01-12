#include <shlwapi.h>

#include <fstream>
#include <string>

#include "OutputFiles.h"
#include "Config.h"
#include "Log.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

#define CONFIG_FILENAME "config.ini"

// handle to the current dll itself
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// strings corresponding to version numbers
const char *rbver_strings[RBVER_COUNT] = {
    "5.8.5", // RBVER_585
    "6.5.0", // RBVER_650
    "6.5.1", // RBVER_651
    "6.5.2", // RBVER_652
    "6.5.3", // RBVER_653
    "6.6.1", // RBVER_661
    "6.6.2", // RBVER_662
    "6.6.3", // RBVER_663
    "6.6.4", // RBVER_664
    "6.6.10", // RBVER_6610
    "6.6.11", // RBVER_6611
    "6.7.0", // RBVER_670
    "6.7.5", // RBVER_675
    "7.0.1", // RBVER_701
};

// global config object
Config config;

static rbox_version_t version_parse(const string &rbox_version)
{
    // determine numeric version id
    for (size_t ver = 0; ver < RBVER_COUNT; ver++) {
        if (rbox_version == rbver_strings[ver]) {
            return (rbox_version_t)ver;
        }
    }
    return RBVER_UNK;
}

bool initialize_config()
{
    string config_path = get_dll_path() + "\\config.ini";
    ifstream in(config_path);
    string line;
    while (getline(in, line)) {
        if (line[0] == '[') {
            string heading = line.substr(1, line.find_first_of("]") - 1);
            // Detect legacy config file
            if (heading == "RBSongExporterConfig") {
                return false;
            }
            // start of output filessection
            if (heading == "Output Files") {
                break;
            }
            continue;
        }
        size_t pos = line.find_first_of("=");
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        if (key == "rbox_version") {
            config.version = version_parse(value);
        } else if (key == "use_server") {
            config.use_server = (strtoul(value.c_str(), NULL, 10) != 0);
        } else if (key == "server_ip") {
            config.server_ip = value;
        } else if (key == "update_rate") {
            config.update_rate = strtoul(value.c_str(), NULL, 10);
        }
    }
    // read out all the output file config strings into a vector
    while (getline(in, line)) {
        config.output_files.push_back(line);
    }
    return true;
}

// path/folder where the module exists
string get_dll_path()
{
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileName((HINSTANCE)&__ImageBase, path, sizeof(path));
    if (!len || !PathRemoveFileSpec(path)) {
        error("Failed to process file path");
    }
    return string(path);
}

// rekordbox.exe base address
uintptr_t rb_base()
{
    static uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    return base;
}
