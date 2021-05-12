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

// the global list of output files
extern std::vector<output_file> g_output_files;

// load in default output files
void default_output_files();

// load output files from config stream
void load_output_files(std::ifstream &in);

// save output files to config stream
void save_output_files(std::ofstream &out);
