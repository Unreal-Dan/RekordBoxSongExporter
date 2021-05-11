#include "OutputFiles.h"

using namespace std;

// global list of output files
vector<outputFile> outputFiles;

// constructor for an output file from a config line
outputFile::outputFile(string line)
{
    size_t pos = 0;
    string value;

    // format of a line is something like this:
    //  name=max_lines;offset;mode;format

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

// serialize the output file data to a single config line
string outputFile::toLine()
{
    return name + "=" +
        to_string(max_lines) + ";" +
        to_string(offset) + ";" +
        to_string(mode) + ";" +
        format;
}

const char *defaultOutputs[] = {
    "TrackTitle=0;0;0;%title%",
    "TrackArtist=0;0;0;%artist%",
    "LastTrackTitle=0;1;0;%title%",
    "LastTrackArtist=0;1;0;%artist%",
    "TrackList=0;0;1;%time% %artist% - %title%",
};

#define NUM_OUTPUTS sizeof(defaultOutputs) / sizeof(defaultOutputs[0])

// load in default output files
void defaultOutputFiles()
{
    for (size_t i = 0; i < NUM_OUTPUTS; ++i) {
        outputFiles.push_back(outputFile(defaultOutputs[i]));
    }
}

// load output files from config
void loadOutputFiles(ifstream &in)
{
    string line;
    while (getline(in, line)) {
        outputFiles.push_back(outputFile(line));
    }
}

// save output files to config
void saveOutputFiles(ofstream &out)
{
    for (auto it = outputFiles.begin(); it != outputFiles.end(); ++it) {
        out << (*it).toLine() << "\n";
    }
}
