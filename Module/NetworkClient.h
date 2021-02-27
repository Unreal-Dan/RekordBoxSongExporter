#pragma once

#include <string>

bool init_network_client();
bool send_network_message(std::string message);
void cleanup_network_client();
