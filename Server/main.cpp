#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <deque>

#pragma comment (lib, "Ws2_32.lib")

// This is the file containing the last config.max_tracks songs
#define CUR_TRACKS_FILE "current_tracks.txt"
// This file contains the current track only
#define CUR_TRACK_FILE  "current_track.txt"
// This file contains the last track only
#define LAST_TRACK_FILE "last_track.txt"
// this is the logfile for all songs played
#define TRACK_LOG_FILE  "played_tracks.txt"

// hardcoded configurations
#define MAX_TRACKS      3
#define USE_TIMESTAMPS  1
#define DEFAULT_PORT    "22345"

using namespace std;

// initialize the server
static bool init_server();
// begin the listen server
static bool start_listen();
// wait for a connection from client
static bool accept_connection();
// receive a message from client
static bool receive_message(string &out_message);
// cleanup networking stuff
static void cleanup_server();
// log a song to files
static void log_track(string song);
// create and truncate a single file, only call this from the logger thread
static bool clear_file(string filename);
// append data to a file, only call this from the logger thread
static bool append_file(string filename, string data);
// just a wrapper around clear and append
static bool rewrite_file(string filename, string data);
// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to) ;
// calc time since the first time this was called
static string get_timestamp_since_start();

// global network stuff
WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

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

// initialize the server
static bool init_server() 
{
    // clears all the track files (this initially creates them)
    if (!clear_file(LAST_TRACK_FILE) ||
        !clear_file(CUR_TRACKS_FILE) ||
        !clear_file(CUR_TRACK_FILE) ||
        !clear_file(TRACK_LOG_FILE)) {
        printf("Failed to clear output files");
        return false;
    }

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
    if (res == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }
    freeaddrinfo(result);
    return true;
}

// begin the listen server
static bool start_listen()
{
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }
    return true;
}

// wait for a connection from client
static bool accept_connection()
{
    // Wait for a client connection and accept it
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    // No longer need server socket
    closesocket(ListenSocket);
    return true;
}

// receive a message from client
static bool receive_message(string &out_message)
{
    char recvbuf[2048];
    memset(recvbuf, 0, sizeof(recvbuf));
    int res = recv(ClientSocket, recvbuf, sizeof(recvbuf), 0);
    if (res > 0) {
        printf("Received: \n%s\n", recvbuf);
        out_message = recvbuf;
        return true;
    }
    return false;
}

// cleanup networking stuff
static void cleanup_server()
{
    // shutdown the connection since we're done
    if (shutdown(ClientSocket, SD_SEND) == SOCKET_ERROR) {
        printf("Shutdown failed: %d\n", WSAGetLastError());
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
}

// log a song to files
static void log_track(string song)
{
    // deque of tracks, deque instead of queue for iteration
    static deque<string> tracks;

    // store the track in the tracks list
    tracks.push_front(song);
    // make sure the list doesn't go beyond config.max_tracks
    if (tracks.size() > MAX_TRACKS) {
        tracks.pop_back();
    }

    // update the last x file by iterating tracks and writing
    if (tracks.size() > 0) {
        // concatenate the tracks into a single string
        string tracks_str;
        for (auto it = tracks.begin(); it != tracks.end(); it++) {
            tracks_str += it[0] + "\r\n";
        }
        // rewrite the tracks file with all of the lines at once
        if (!rewrite_file(CUR_TRACKS_FILE, tracks_str)) {
            printf("Failed to write to tracks file");
        }

        // rewrite the current track file with only the current track
        if (!rewrite_file(CUR_TRACK_FILE, tracks.at(0))) {
            printf("Failed to append to current track file");
        }
    }
    // rewrite the last track file with the previous track
    if (tracks.size() > 1) {
        if (!rewrite_file(LAST_TRACK_FILE, tracks.at(1))) {
            printf("Failed to write last track file");
        }
    }
    // append the artist and track to the global log
    string log_entry;
    if (USE_TIMESTAMPS) {
        // timestamp with a space after it
        log_entry += get_timestamp_since_start() + " ";
    }
    // the rest of the current track output
    log_entry += tracks.front() + "\r\n";
    if (!append_file(TRACK_LOG_FILE, log_entry)) {
        printf("Failed to log track to global log");
    }
}

// create and truncate a single file, only call this from the logger thread
static bool clear_file(string filename)
{
    // open with CREATE_ALWAYS to truncate any existing file and create any missing
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open for truncate %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// append data to a file, only call this from the logger thread
static bool append_file(string filename, string data)
{
    // open for append, create new, or open existing
    HANDLE hFile = CreateFile(filename.c_str(), FILE_APPEND_DATA, 0, NULL, CREATE_NEW|OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file for append %s (%d)", filename.c_str(), GetLastError());
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(hFile, data.c_str(), (DWORD)data.length(), &written, NULL)) {
        printf("Failed to write to %s (%d)", filename.c_str(), GetLastError());
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);
    return true;
}

// just a wrapper around clear and append
static bool rewrite_file(string filename, string data)
{
    return clear_file(filename) && append_file(filename, data);
}

// helper for in-place replacements
static bool replace(string& str, const string& from, const string& to) 
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

// calc time since the first time this was called
static string get_timestamp_since_start()
{
    static DWORD start_timestamp = 0;
    if (!start_timestamp) {
        start_timestamp = GetTickCount();
        return string("(00:00:00)");
    }
    DWORD elapsed = GetTickCount() - start_timestamp;
    // grab hours
    DWORD hr = elapsed / 3600000;
    elapsed -= (3600000 * hr);
    // grab minutes
    DWORD min = elapsed / 60000;
    elapsed -= (60000 * min);
    // grab seconds
    DWORD sec = elapsed / 1000;
    elapsed -= (1000 * sec);
    // return the timestamp
    char buf[256] = { 0 };
    if (snprintf(buf, sizeof(buf), "(%02u:%02u:%02u)", hr, min, sec) < 1) {
        return string("(00:00:00)");
    }
    return string(buf);
}