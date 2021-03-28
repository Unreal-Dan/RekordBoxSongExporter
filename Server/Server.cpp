#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <string>

#include "Server.h"
#include "Config.h"

#pragma comment (lib, "Ws2_32.lib")

using namespace std;

// global network stuff
WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

// initialize the server
bool init_server() 
{
    // Initialize Winsock
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        printf("WSAStartup failed with error: %d\n", res);
        return false;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    struct addrinfo *result = NULL;
    res = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failed with error: %d\n", res);
        WSACleanup();
        return false;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    // Setup the TCP listening socket
    res = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (res == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }
    return true;
}

// begin the listen server
bool start_listen()
{
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

// wait for a connection from client
bool accept_connection()
{
    // Wait for a client connection and accept it
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        return 1;
    }
    return true;
}

// receive a message from client
bool receive_message(string &out_message)
{
    char recvbuf[2048];
    memset(recvbuf, 0, sizeof(recvbuf));
    int res = recv(ClientSocket, recvbuf, sizeof(recvbuf), 0);
    if (res > 0) {
        out_message = recvbuf;
        return true;
    }
    return false;
}

// cleanup networking stuff
void cleanup_server()
{
    // shutdown the connection since we're done
    if (shutdown(ClientSocket, SD_SEND) == SOCKET_ERROR) {
        printf("Shutdown failed: %d\n", WSAGetLastError());
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
}
