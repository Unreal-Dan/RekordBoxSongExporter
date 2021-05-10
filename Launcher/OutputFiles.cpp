#include "OutputFiles.h"

using namespace std;

// global list of output files
vector<output_file> outputFiles;

// load output files from config
void loadOutputFiles()
{
    outputFiles.push_back(output_file("test"));
}
