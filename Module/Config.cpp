#include <shlwapi.h>

#include <string>

#include "Config.h"
#include "Log.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

#define CONFIG_FILENAME "config.ini"

// handle to the current dll itself
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

bool initialize_config()
{
    // the config file path 
    string configPath = get_dll_path() + CONFIG_FILENAME;

    

    // open for append, create new or open existing
    HANDLE hFile = CreateFile(configPath.c_str(), FILE_GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        error("Failed to open config file %s (%d)", configPath.c_str(), GetLastError());
        return false;
    }



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