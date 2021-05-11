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
class outputFile
{
public:
    // construct an output file by parsing a config line
    outputFile(std::string line); 

    // serialize the output file data to a single config line
    std::string toLine();

    // public info of output files
    std::string name;
    std::string format;
    uint32_t mode;
    uint32_t offset;
    uint32_t max_lines;
};

// the global list of output files
extern std::vector<outputFile> outputFiles;

// load in default output files
void defaultOutputFiles();

// load output files from config stream
void loadOutputFiles(std::ifstream &in);

// save output files to config stream
void saveOutputFiles(std::ofstream &out);
