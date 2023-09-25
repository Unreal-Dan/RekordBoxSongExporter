#include "Versions.h"

using namespace std;

// list of supported versions
version_path versions[] = {
    // Friendly name     Default installation path
    { "Rekordbox 6.7.5", "C:\\Program Files\\Pioneer\\rekordbox 6.7.5\\rekordbox.exe" },
    { "Rekordbox 6.7.0", "C:\\Program Files\\Pioneer\\rekordbox 6.7.0\\rekordbox.exe" },
    { "Rekordbox 6.6.11", "C:\\Program Files\\Pioneer\\rekordbox 6.6.11\\rekordbox.exe" },
    { "Rekordbox 6.6.10", "C:\\Program Files\\Pioneer\\rekordbox 6.6.10\\rekordbox.exe" },
    { "Rekordbox 6.6.4", "C:\\Program Files\\Pioneer\\rekordbox 6.6.4\\rekordbox.exe" },
    // Less than 6.6.4 no longer supported in this build
    //{ "Rekordbox 6.6.3", "C:\\Program Files\\Pioneer\\rekordbox 6.6.3\\rekordbox.exe" },
    //{ "Rekordbox 6.6.2", "C:\\Program Files\\Pioneer\\rekordbox 6.6.2\\rekordbox.exe" },
    //{ "Rekordbox 6.6.1", "C:\\Program Files\\Pioneer\\rekordbox 6.6.1\\rekordbox.exe" },
    //{ "Rekordbox 6.5.3", "C:\\Program Files\\Pioneer\\rekordbox 6.5.3\\rekordbox.exe" },
    //{ "Rekordbox 6.5.2", "C:\\Program Files\\Pioneer\\rekordbox 6.5.2\\rekordbox.exe" },
    //{ "Rekordbox 6.5.1", "C:\\Program Files\\Pioneer\\rekordbox 6.5.1\\rekordbox.exe" },
    //{ "Rekordbox 6.5.0", "C:\\Program Files\\Pioneer\\rekordbox 6.5.0\\rekordbox.exe" },
    //{ "Rekordbox 5.8.5", "C:\\Program Files\\Pioneer\\rekordbox 5.8.5\\rekordbox.exe" },
};

#define NUM_VERSIONS (sizeof(versions) / sizeof(versions[0]))

// get the name of a rekordbox version in the supported versions table (Ex: Rekordbox 6.5.0)
const char *get_version_name(size_t version_index) {
    if (version_index >= NUM_VERSIONS) {
        return "Unknown Version";
    }
    return versions[version_index].name;
}

// get the version number of a rekordbox version in the supported versions table (Ex: 6.5.0)
const char *get_version_number(size_t version_index) {
    if (version_index >= NUM_VERSIONS) {
        return "Unknown Version";
    }
    return versions[version_index].name + sizeof("Rekordbox");
}

// get the path of a rekordbox version in the supported versions table
const char *get_version_path(size_t version_index) {
    if (version_index >= NUM_VERSIONS) {
        return "Unknown Version";
    }
    return versions[version_index].path;
}

// the latest version name of rekordbox (ex: Rekordbox 6.5.2)
const char *get_latest_version_name() {
    return versions[0].name;
}

// the latest version number of rekordbox (ex: 6.5.2)
const char *get_latest_version_number() {
    return versions[0].name + sizeof("Rekordbox");
}

// the latest version path of rekordbox
const char *get_latest_version_path() {
    return versions[0].path;
}

// get the number of version objects
size_t num_versions() {
    return NUM_VERSIONS;
}
