#pragma once

#include <vector>
#include <string>

// the different ways an output file can be written
enum class output_mode : int
{
    MODE_REPLACE = 0,
    MODE_APPEND = 1,
    MODE_PREPEND = 2
};

// an output file object
class output_file
{
public:
    output_file(std::string name) : 
        name(name),
        format("%title%"),
        mode(output_mode::MODE_REPLACE),
        offset(0),
        max_lines(3) {};
    std::string name;
    std::string format;
    output_mode mode;
    uint32_t offset;
    uint32_t max_lines;
};

// the global list of output files
extern std::vector<output_file> outputFiles;

// load output files from config
void loadOutputFiles();
