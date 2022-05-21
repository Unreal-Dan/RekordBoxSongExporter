#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// the different ways an output file can be written
#define MODE_REPLACE    0
#define MODE_APPEND     1
#define MODE_PREPEND    2

// an output file object
class output_file
{
public:
    // construct an output file by parsing a config line
    output_file(const std::string &line);
    // construct a default output file
    output_file();

    // serialize the output file data to a single config line
    std::string to_line();

    // public info of output files
    std::string name;
    std::string format;
    uint32_t mode;
    uint32_t offset;
    uint32_t max_lines;
};

// the number of output files
size_t num_output_files();

// add a default output file at index
void add_output_file(int index);
// remove the output file at the index
void remove_output_file(int index);

// getters for the list of output files
const char *get_outfile_name(uint32_t index);
const char *get_outfile_format(uint32_t index);
uint32_t get_outfile_mode(uint32_t index);
const char *get_outfile_offset(uint32_t index);
const char *get_outfile_max_lines(uint32_t index);

// setters for the list of output files
void set_outfile_name(uint32_t index, const std::string &name);
void set_outfile_format(uint32_t index, const std::string &format);
void set_outfile_mode(uint32_t index, uint32_t mode);
void set_outfile_offset(uint32_t index, uint32_t offset);
void set_outfile_max_lines(uint32_t index, uint32_t max_lines);

// load in default output files
void default_output_files();

// load output files from config stream
void load_output_files(std::ifstream &in);

// save output files to config stream
void save_output_files(std::ofstream &out);
