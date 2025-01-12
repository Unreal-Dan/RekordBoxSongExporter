#include <windows.h>
#include <cstdio>
#include <string>
#include <thread>
#include <iostream>

// Pipe name shared with the injected DLL
static const char* PIPE_NAME = R"(\\.\pipe\rbse_log_pipe)";

// Continuously read from the pipe in a separate thread
void ModuleLoggerThread()
{
  HANDLE hPipe = CreateNamedPipeA(
    PIPE_NAME,
    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
    1,       // max instances
    4096,    // out buffer size
    4096,    // in buffer size
    0,       // default timeout
    nullptr
  );

  if (hPipe == INVALID_HANDLE_VALUE) {
    std::cerr << "CreateNamedPipe failed: " << GetLastError() << std::endl;
    return;
  }

  BOOL connected = ConnectNamedPipe(hPipe, nullptr)
    ? TRUE
    : (GetLastError() == ERROR_PIPE_CONNECTED);
  if (!connected) {
    std::cerr << "ConnectNamedPipe failed: "
      << GetLastError() << std::endl;
    CloseHandle(hPipe);
    return;
  }

  std::cout << "[PipeReader] Client connected. Ready to receive logs." << std::endl;

  // We'll read up to 511 bytes at a time (leaving 1 byte for null-terminator)
  char buffer[512];
  DWORD bytesRead = 0;

  // We'll need a handle to the console for color manipulation
  HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  while (true) {
    BOOL success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
    if (!success || bytesRead == 0) {
      std::cerr << "[PipeReader] ReadFile failed or no data: "
        << GetLastError() << std::endl;
      break;
    }

    // Null-terminate so we can treat it as a C-string
    buffer[bytesRead] = '\0';

#ifdef _DEBUG
    // Capture the current console text attributes
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(out_handle, &info);

    // By convention, if the message starts with "[+]" or "[-]" or "[*]",
    // we parse out buffer[1] to decide color
    char colorChar = '\0';
    if (bytesRead >= 4 && buffer[0] == '[' && buffer[2] == ']') {
      colorChar = buffer[1];
      switch (colorChar) {
      case '+':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        break;
      case '-':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY | FOREGROUND_RED);
        break;
      case '*':
        SetConsoleTextAttribute(out_handle, FOREGROUND_INTENSITY);
        break;
      default:
        // leave as is
        break;
      }
    }

    // Print the message
    std::cout << buffer;

    // Restore original attributes
    SetConsoleTextAttribute(out_handle, info.wAttributes);
#else
    // If we're not in debug mode, just print directly
    std::cout << buffer;
#endif
  }

  DisconnectNamedPipe(hPipe);
  CloseHandle(hPipe);
}

void InitModuleLogger()
{
  // Create our own console window
  AllocConsole();

  FILE *newStdout = nullptr;
  FILE *newStderr = nullptr;
  freopen_s(&newStdout, "CONOUT$", "w", stdout);
  freopen_s(&newStderr, "CONOUT$", "w", stderr);

  std::cout << "[Launcher] Console initialized." << std::endl;

  // Start a thread to handle pipe connections / reading
  std::thread reader(ModuleLoggerThread);
  reader.detach();

  std::cout << "[Launcher] Now you would inject your DLL into the target process." << std::endl;
  std::cout << "[Launcher] Press Enter to exit..." << std::endl;
}
