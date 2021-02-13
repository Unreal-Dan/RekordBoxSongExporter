#include <shlwapi.h>

#include <string>

#include "Config.h"
#include "Log.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// handle to the current dll itself
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// path/folder where the module exists
string get_dll_path()
{
    char path[MAX_PATH];
    DWORD len = GetModuleFileName((HINSTANCE)&__ImageBase, path, sizeof(path));
    if (!len || !PathRemoveFileSpec(path)) {
        error("Failed to process file path");
    }
    return string(path);
}