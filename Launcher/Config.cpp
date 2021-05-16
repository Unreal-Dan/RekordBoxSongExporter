#include <Windows.h>
#include <shlwapi.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "OutputFiles.h"
#include "Config.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// module base
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE imageBase = (HINSTANCE)&__ImageBase;

// currently loaded config
Config config;

// helper to get the config file path
static string get_config_path()
{
    char path[MAX_PATH] = { 0 };
    string configPath;
    if (!GetModuleFileName(imageBase, path, sizeof(path)) || !PathRemoveFileSpec(path)) {
        return configPath;
    }
    configPath = string(path) + "\\config.ini";
    return configPath;
}

bool config_default()
{
    // the only config that really needs to be initialized
    // for a default setup is the server ip, the rest will
    // automatically be filled with good default values
    // because the version checkbox will select the latest
    // version which will populate the path textbox
    config.server_ip = "127.0.0.1";

    // default the output files
    default_output_files();
    return true;
}

bool config_load()
{
    ifstream in(get_config_path());
    // if the config is missing just default the configs
    if (!in.is_open()) {
        return config_default();
    }
    string line;
    bool is_legacy_config = false;
    while (getline(in, line)) {
        if (line[0] == '[') {
            string heading = line.substr(1, line.find_first_of("]") - 1);
            // Detect new config file
            if (heading == "RBSongExporterConfig") {
                is_legacy_config = true;
            } 
            // start of output files section
            if (heading == "Output Files") {
                break;
            }
            continue;
        }
        // split each config key=value
        size_t pos = line.find_first_of("=");
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        if (key == "version") {
            // make sure the config was generate by the same utility
            if (value != RBSE_VERSION) {
                // TODO: Do something when loading old version?
                break;
            }
        } else if (key == "rbox_version") {
            config.version = value;
        } else if (key == "rbox_path") {
            config.path = value;
        } else if (key == "use_server") {
            config.use_server = (strtoul(value.c_str(), NULL, 10) != 0);
        } else if (key == "server_ip") {
            config.server_ip = value;
        }
    }
    // if we loaded an outdated config file then fill defaults
    if (is_legacy_config) {
        default_output_files();
    } else {
        load_output_files(in);
    }
    return true;
}

bool config_save()
{
    ofstream of(get_config_path(), ios_base::trunc);
    if (!of.is_open()) {
        return false;
    }
    string line;
    of << "[RBSE Config]\n"
       << "version=" RBSE_VERSION "\n"
       << "rbox_version=" << config.version << "\n"
       << "rbox_path=" << config.path << "\n"
       << "use_server=" << (config.use_server ? "1" : "0") << "\n"
       << "server_ip=" << config.server_ip << "\n"
       << "\n[Output Files]\n";
    save_output_files(of);
    return true;
}