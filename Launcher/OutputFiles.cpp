#include "OutputFiles.h"
#include "Config.h"

using namespace std;

// global list of output files
vector<output_file> g_output_files;

// constructor for an output file from a config line
output_file::output_file(const string &line)
{
    // format of a line is something like this:
    //  name=max_lines;offset;mode;format
    size_t pos = 0;
    string value;
    // read out the name up till =
    pos = line.find_first_of("=");
    name = line.substr(0, pos);
    value = line.substr(pos + 1);
    // max lines
    pos = value.find_first_of(";");
    max_lines = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // offset
    pos = value.find_first_of(";");
    offset = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // mode
    pos = value.find_first_of(";");
    mode = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // the format is everything left over
    format = value;
}

output_file::output_file() : 
    // call line constructor with default output file format
    output_file(DEFAULT_OUTPUT_FILE)
{
}

// serialize the output file data to a single config line
string output_file::to_line()
{
    return name + "=" +
        to_string(max_lines) + ";" +
        to_string(offset) + ";" +
        to_string(mode) + ";" +
        format;
}

// load in default output files
void default_output_files()
{
    static const char *default_files[] = {
        "TrackTitle=0;0;0;%title%",
        "TrackArtist=0;0;0;%artist%",
        "LastTrackTitle=0;1;0;%title%",
        "LastTrackArtist=0;1;0;%artist%",
        "TrackList=0;0;1;%time% %artist% - %title%",
    };

    for (size_t i = 0; i < sizeof(default_files) / sizeof(default_files[0]); ++i) {
        g_output_files.push_back(output_file(default_files[i]));
    }
}

// load output files from config
void load_output_files(ifstream &in)
{
    string line;
    while (getline(in, line)) {
        g_output_files.push_back(output_file(line));
    }
}

// save output files to config
void save_output_files(ofstream &out)
{
    for (auto it = g_output_files.begin(); it != g_output_files.end(); ++it) {
        out << (*it).to_line() << "\n";
    }
}
