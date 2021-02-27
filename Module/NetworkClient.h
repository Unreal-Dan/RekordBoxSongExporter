#pragma once

#include <string>

// initialize the network client for server mode, thanks msdn for the code
bool init_network_client();

// send a message
bool send_network_message(std::string message);

// cleanup network stuff
void cleanup_network_client();