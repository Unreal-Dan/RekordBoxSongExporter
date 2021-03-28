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

        printf("Received connection!\n");

        string config_str;
        if (!receive_message(config_str)) {
            printf("Failed to receive config\n");
            break;
        }

        // quickly parse out the first message which is always the config
        istringstream config_ss(config_str);
        string token;
        getline(config_ss, token, '|');
        config_use_timestamps = (strtoul(token.c_str(), NULL, 10) == 1);
        getline(config_ss, token, '|');
        config_max_tracks = strtoul(token.c_str(), NULL, 10);

        // dump config to console
        printf("Timestamps are %s\n", config_use_timestamps ? "enabled" : "disabled");
        printf("Max track count is %zu\n", config_max_tracks);

        string track;
        // each message received will get passed into the logging system
        while (receive_message(track)) {
            printf("Logging track [%s]\n", track.c_str());
            log_track(track);
        }

        printf("Connection closed\n");
    }

    // done
    cleanup_server();

    system("pause");

    return 0;
}