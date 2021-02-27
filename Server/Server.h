#pragma once

#include <string>

// initialize the server
bool init_server();

// begin the listen server
bool start_listen();

// wait for a connection from client
bool accept_connection();

// receive a message from client
bool receive_message(std::string &out_message);

// cleanup networking stuff
void cleanup_server();
