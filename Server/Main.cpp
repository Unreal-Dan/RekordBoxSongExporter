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

        string file_track;
        // each message received will get passed into the logging system
        while (receive_message(file_track)) {
            size_t pos = file_track.find_first_of("=");
            if (pos == string::npos) {
                continue;
            }
            string file = file_track.substr(0, pos);
            string track = file_track.substr(pos +1);
            printf("Logging track [%s] to %s\n", track.c_str(), file.c_str());
            log_track_to_output_file(file, track);
        }

        printf("Connection closed\n");
    }

    // done
    cleanup_server();

    system("pause");

    return 0;
}