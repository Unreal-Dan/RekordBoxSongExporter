#pragma once

#include <string>

// describes a supported version and it's path
struct version_path
{
    const char *name;
    const char *path;
};

// get the name of a rekordbox version in the supported versions table (Ex: Rekordbox 6.5.0)
const char *get_version_name(size_t version_index);

// get the version number of a rekordbox version in the supported versions table (Ex: 6.5.0)
const char *get_version_number(size_t version_index);

// get the path of a rekordbox version in the supported versions table
const char *get_version_path(size_t version_index);

// the latest version name of rekordbox (ex: Rekordbox 6.5.2)
const char *get_latest_version_name();

// the latest version number of rekordbox (ex: 6.5.2)
const char *get_latest_version_number();

// the latest version path of rekordbox
const char *get_latest_version_path();

// get the number of version objects
size_t num_versions();