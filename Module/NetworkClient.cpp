#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>

#include "Config.h"
#include "Log.h"

using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "22345"

WSADATA wsaData;
SOCKET sock = INVALID_SOCKET;

// initialize the network client for server mode, thanks msdn for the code
bool init_network_client()
{
    struct addrinfo *addrs = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    int res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        error("WSAStartup failed with error: %d", res);
        return false;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    res = getaddrinfo(config.server_ip.c_str(), DEFAULT_PORT, &hints, &addrs);
    if (res != 0) {
        error("getaddrinfo failed with error: %", res);
        WSACleanup();
        return false;
    }
    info("Attempting to connect to %s", config.server_ip.c_str());
    // attempt to connect till one succeeds
    for (ptr = addrs; ptr != NULL; ptr = ptr->ai_next) {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == INVALID_SOCKET) {
            error("socket failed with error: %d", WSAGetLastError());
            freeaddrinfo(addrs);
            WSACleanup();
            return false;
        }
        if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR) {
            error("Connect failed");
            break;
        }
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    freeaddrinfo(addrs);
    if (sock == INVALID_SOCKET) {
        error("Unable to connect to server");
        WSACleanup();
        return false;
    }
    info("Connected to server %s", config.server_ip.c_str());
    return true;
}

// send a message
bool send_network_message(string message)
{
    info("Sending track to server: %s", message.c_str());
    if (send(sock, message.c_str(), (int)message.length(), 0 ) == SOCKET_ERROR) {
        error("send failed with error: %d", WSAGetLastError());
        return false;
    }
    return true;
}

void cleanup_network_client()
{
    if (shutdown(sock, SD_SEND) == SOCKET_ERROR) {
        error("shutdown failed with error: %d", WSAGetLastError());
    }
    closesocket(sock);
    WSACleanup();
}