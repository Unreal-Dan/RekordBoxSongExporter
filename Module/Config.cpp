#include <shlwapi.h>

#include <string>

#include "Config.h"
#include "Log.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

#define CONFIG_FILENAME "config.ini"

// handle to the current dll itself
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// strings corresponding to version numbers
const char *rbver_strings[RBVER_COUNT] = {
    "6.5.0", // RBVER_650
    "5.8.5", // RBVER_585
};

// global config object
config_t config;

bool initialize_config()
{
    // the config file path 
    char buf[2048] = { 0 };
    string configPath = get_dll_path() + "\\config.ini";
    if (!GetPrivateProfileString("RBSongExporterConfig", "rbox_version", "", buf, sizeof(buf), configPath.c_str())) {
        error("Failed to load rbox version from [%s]: %d", configPath.c_str(), GetLastError());
        return false;
    }
    // just the version part
    string rbox_version = buf;
    // determine numeric version id
    for (size_t ver = 0; ver < RBVER_COUNT; ver++) {
        if (rbox_version == rbver_strings[ver]) {
            config.rbox_version = (rbox_version_t)ver;
        }
    }

    // the output version
    if (!GetPrivateProfileString("RBSongExporterConfig", "out_format", "", buf, sizeof(buf), configPath.c_str())) {
        error("Failed to load out_format from [%s]: %d", configPath.c_str(), GetLastError());
        return false;
    }
    config.out_format = buf;

    // the num lines in cur tracks
    if (!GetPrivateProfileString("RBSongExporterConfig", "cur_tracks_count", "", buf, sizeof(buf), configPath.c_str())) {
        error("Failed to load out_format from [%s]: %d", configPath.c_str(), GetLastError());
        return false;
    }
    config.max_tracks = strtoul(buf, NULL, 10);

    // whether to use timestamps in global log
    if (!GetPrivateProfileString("RBSongExporterConfig", "use_timestamps", "", buf, sizeof(buf), configPath.c_str())) {
        error("Failed to load out_format from [%s]: %d", configPath.c_str(), GetLastError());
        return false;
    }
    config.use_timestamps = (strtoul(buf, NULL, 10) != 0);

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
