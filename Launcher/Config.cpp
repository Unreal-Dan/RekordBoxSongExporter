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
static string getConfigPath()
{
    char path[MAX_PATH] = { 0 };
    string configPath;
    if (!GetModuleFileName(imageBase, path, sizeof(path)) || !PathRemoveFileSpec(path)) {
        return configPath;
    }
    configPath = string(path) + "\\config.ini";
    return configPath;
}

bool configLoad()
{
    ifstream in(getConfigPath());
    string line;
    bool is_legacy_config = false;
    while (getline(in, line)) {
        if (line[0] == '[') {
            string heading = line.substr(1, line.find_first_of("]") - 1);
            // Detect legacy config file
            if (heading == "RBSongExporterConfig") {
                is_legacy_config = true;
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
            config.version = value;
        } else if (key == "rbox_path") {
            config.path = value;
        } else if (key == "use_server") {
            config.use_server = (strtoul(value.c_str(), NULL, 10) != 0);
        } else if (key == "server_ip") {
            config.server_ip = value;
        }
    }
    if (is_legacy_config) {
        defaultOutputFiles();
    } else {
        loadOutputFiles(in);
    }
    return true;
}

bool configSave()
{
    ofstream of(getConfigPath(), ios_base::trunc);
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
    saveOutputFiles(of);
    return true;
}

//bool conf_save_version(const string &version)
//{
//    return (WritePrivateProfileString("RBSongExporterConfig", "rbox_version",
//        version.c_str(), get_config_path().c_str()) != 0);
//}
//
//bool conf_save_path(const string &path)
//{
//    return (WritePrivateProfileString("RBSongExporterConfig", "rbox_path",
//        path.c_str(), get_config_path().c_str()) != 0);
//}
//
//bool conf_save_use_server(bool use_server)
//{
//    return (WritePrivateProfileString("RBSongExporterConfig", "use_server",
//        use_server ? "1" : "0", get_config_path().c_str()) != 0);
//}
//
//bool conf_save_server_ip(const string &server_ip)
//{
//    return (WritePrivateProfileString("RBSongExporterConfig", "server_ip",
//        server_ip.c_str(), get_config_path().c_str()) != 0);
//}
//
//bool conf_save_output_files()
//{
//    return true;
//}