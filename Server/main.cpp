#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

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

    // start listening
    if (!start_listen()) {
        return 1;
    }

    printf("Listening...\n");

    // block till a client connects
    if (!accept_connection()) {
        return 1;
    }

    printf("Received connection!\n");

    string message;
    // each message received will get passed into the logging system
    while (receive_message(message)) {
        log_track(message);
    }

    // done
    cleanup_server();

    return 0;
}