#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <string>
#include <deque>

#include "OutputFiles.h"
#include "Server.h"
#include "Config.h"

using namespace std;

int main()
{
    // setup network stuff
    if (!init_server()) {
        return 1;
    }

    printf("Successfully Initialized Server\n");

    // reuse the server if the client disconnects
    while (1) {
        // start listening
        if (!start_listen()) {
            break;
        }

        printf("-----------------------------------------\n");
        printf("Listening on port " DEFAULT_PORT "...\n");

        // block till a client connects and clear the output files
        if (!accept_connection() || !init_output_files()) {
            break;
        }

        string message;
        // each message received will get passed into the logging system
        while (1) {
            if (!receive_message(message)) {
                break;
            }
            istringstream iss(message);
            string line;
            while (getline(iss, line)) {
                size_t pos = line.find_first_of(":");
                if (pos == string::npos) {
                    continue;
                }
                string file_id_str = line.substr(0, pos);
                string track = line.substr(pos + 1);
                uint32_t file_id = strtoul(file_id_str.c_str(), NULL, 0);
                log_track_to_output_file(file_id, track);
            }
        }

        printf("Connection closed\n");
    }

    // done
    cleanup_server();

    system("pause");

    return 0;
}