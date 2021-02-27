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
    GetPrivateProfileString("RBSongExporterConfig", "out_format", "%artist% - %title%", 
        buf, sizeof(buf), get_config_path().c_str());
    // return string(buf); // uncomment me later
    // --------------- 
    // TODO: Remove this later once nobody is upgrading from old version.
    // This replaces %track% with %title%
    string result = string(buf);
    size_t start_pos = result.find("%track%");
    if (start_pos != std::string::npos) {
        result.replace(start_pos, 7, "%title%");
    }
    // -------------- end upgrade contingency
    return result;
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

bool conf_load_use_server()
{
    return (bool)GetPrivateProfileInt("RBSongExporterConfig", "use_server", 0,
        get_config_path().c_str());
}

string conf_load_server_ip()
{
    char buf[2048] = { 0 };
    GetPrivateProfileString("RBSongExporterConfig", "server_ip", "127.0.0.1", 
        buf, sizeof(buf), get_config_path().c_str());
    return string(buf);
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

bool conf_save_use_server(bool use_server)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "use_server",
        use_server ? "1" : "0", get_config_path().c_str()) != 0);
}

bool conf_save_server_ip(string server_ip)
{
    return (WritePrivateProfileString("RBSongExporterConfig", "server_ip",
        server_ip.c_str(), get_config_path().c_str()) != 0);
}