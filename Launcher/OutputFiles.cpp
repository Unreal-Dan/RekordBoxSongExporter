#include "OutputFiles.h"
#include "Config.h"

using namespace std;

// global list of output files
vector<output_file> output_files;

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
    // mode
    pos = value.find_first_of(";");
    mode = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // offset
    pos = value.find_first_of(";");
    offset = strtoul(value.substr(0, pos).c_str(), NULL, 10);
    value = value.substr(pos + 1);
    // max lines
    pos = value.find_first_of(";");
    max_lines = strtoul(value.substr(0, pos).c_str(), NULL, 10);
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
        to_string(mode) + ";" +
        to_string(offset) + ";" +
        to_string(max_lines) + ";" +
        format;
}

size_t num_output_files()
{
    return output_files.size();
}

// add a default output file at index
void add_output_file(int index)
{
    // generate default output file
    if (index < 0) {
        output_files.push_back(output_file());
    } else {
        output_files.insert(output_files.begin() + index, 1, output_file());
    }
}

// remove the output file at the index
void remove_output_file(int index)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files.erase(output_files.begin() + index);
}

const char *get_outfile_name(uint32_t index)
{
    if (output_files.size() <= index) {
        return "";
    }
    return output_files[index].name.c_str();
}

const char *get_outfile_format(uint32_t index)
{
    if (output_files.size() <= index) {
        return "";
    }
    return output_files[index].format.c_str();
}

uint32_t get_outfile_mode(uint32_t index)
{
    if (output_files.size() <= index) {
        return MODE_REPLACE;
    }
    return output_files[index].mode;
}

const char *get_outfile_offset(uint32_t index)
{
    static char offset_text[16] = { 0 };
    if (output_files.size() <= index) {
        return MODE_REPLACE;
    }
    snprintf(offset_text, sizeof(offset_text), "%u", output_files[index].offset);
    return offset_text;
}

const char *get_outfile_max_lines(uint32_t index)
{
    static char maxlines_text[16] = { 0 };
    if (output_files.size() <= index) {
        return MODE_REPLACE;
    }
    snprintf(maxlines_text, sizeof(maxlines_text), "%u", output_files[index].max_lines);
    return maxlines_text;
}

void set_outfile_name(uint32_t index, const string &name)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files[index].name = name;
}

void set_outfile_format(uint32_t index, const string &format)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files[index].format = format;
}

void set_outfile_mode(uint32_t index, uint32_t mode)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files[index].mode = mode;
}

void set_outfile_offset(uint32_t index, uint32_t offset)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files[index].offset = offset;
}

void set_outfile_max_lines(uint32_t index, uint32_t max_lines)
{
    if (output_files.size() <= index) {
        return;
    }
    output_files[index].max_lines = max_lines;
}

// load in default output files
void default_output_files()
{
    static const char *default_files[] = DEFAULT_OUTPUT_FILES;
    for (size_t i = 0; i < sizeof(default_files) / sizeof(default_files[0]); ++i) {
        output_files.push_back(output_file(default_files[i]));
    }
}

// load output files from config
void load_output_files(ifstream &in)
{
    string line;
    while (getline(in, line)) {
        output_files.push_back(output_file(line));
    }
}

// save output files to config
void save_output_files(ofstream &out)
{
    for (auto it = output_files.begin(); it != output_files.end(); ++it) {
        out << (*it).to_line() << "\n";
    }
}
