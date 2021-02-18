#include <Windows.h>
#include <shlwapi.h>

#include <string>

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// module base
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE imageBase = (HINSTANCE)&__ImageBase;

string get_config_path()
{
    char path[MAX_PATH] = { 0 };
    string configPath;
    if (!GetModuleFileName(imageBase, path, sizeof(path)) || !PathRemoveFileSpec(path)) {
        return configPath;
    }
    configPath = string(path) + "\\config.ini";
    return configPath;
}

string conf_load_version()
{
    char buf[2048] = { 0 };
    GetPrivateProfileString("RBSongExporterConfig", "rbox_version", "", 
        buf, sizeof(buf), get_config_path().c_str());
    return string(buf);
}

string conf_load_path()
{
    char buf[2048] = { 0 };
    GetPrivateProfileString("RBSongExporterConfig", "rbox_path", "", 
        buf, sizeof(buf), get_config_path().c_str());
    return string(buf);
}

string conf_load_out_format()
{
    char buf[2048] = { 0 };
    GetPrivateProfileString("RBSongExporterConfig", "out_format", "%artist% - %track%", 
        buf, sizeof(buf), get_config_path().c_str());
    return string(buf);
}

string conf_load_cur_tracks_count()
{
    char buf[2048] = { 0 };
    GetPrivateProfileString("RBSongExporterConfig", "cur_tracks_count", "3", 
        buf, sizeof(buf), get_config_path().c_str());
    return string(buf);
}

bool conf_load_use_timestamps()
{
    return (bool)GetPrivateProfileInt("RBSongExporterConfig", "use_timestamps", 1,
        get_config_path().c_str());
}

// ==========================
// saving functions

bool conf_save_version(string version)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "rbox_version",
        version.c_str(), get_config_path().c_str()) != 0);
}

bool conf_save_path(string path)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "rbox_path",
        path.c_str(), get_config_path().c_str()) != 0);
}

bool conf_save_out_format(string out_format)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "out_format",
        out_format.c_str(), get_config_path().c_str()) != 0);
}

bool conf_save_cur_tracks_count(string num_tracks)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "cur_tracks_count",
        num_tracks.c_str(), get_config_path().c_str()) != 0);
}

bool conf_save_use_timestamps(bool use_timestamps)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "use_timestamps",
        use_timestamps ? "1" : "0", get_config_path().c_str()) != 0);
}