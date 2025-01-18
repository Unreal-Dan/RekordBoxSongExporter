#include <Windows.h>
#include <stdio.h>
#include <string>

#include "Config.h"
#include "Hook.h"
#include "Log.h"

using namespace std;

// We'll track a pipe handle for our log output
static HANDLE pipe_handle = INVALID_HANDLE_VALUE;

// Try to connect to the named pipe that the launcher is waiting on
static bool connect_to_pipe()
{
  const char *PIPE_NAME = R"(\\.\pipe\rbse_log_pipe)";

  while (true) {
    pipe_handle = CreateFileA(
      PIPE_NAME,
      GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      0,
      nullptr
    );

    if (pipe_handle != INVALID_HANDLE_VALUE) {
      // Connected to the launcher
      return true;
    }

    if (GetLastError() == ERROR_PIPE_BUSY) {
      // Wait up to 5 seconds for pipe to become free
      if (!WaitNamedPipeA(PIPE_NAME, 5000))
        return false;
    } else {
      // Some other error (pipe not found, etc.)
      return false;
    }
  }
}

// Free the pipe handle if open
static void close_pipe()
{
  if (pipe_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe_handle);
    pipe_handle = INVALID_HANDLE_VALUE;
  }
}

// console handle (not used for a console in the target)
HANDLE out_handle = NULL;
// output file log (also optional if you want file logging)
HANDLE log_handle = NULL;
// path of outfile
string log_path;

// log a message with a prefix in brackets
void log_msg(const char *prefix, const char *func, const char *fmt, ...)
{
  char final_buffer[4096] = { 0 };
  char buffer[512] = { 0 };
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
  va_end(args);

  size_t len = snprintf(
    final_buffer,
    sizeof(final_buffer),
    "[%s] %s%s%s\n",
    prefix,
    func ? func : "",
    func ? "(): " : "",
    buffer
  );

  // If not connected to the launcher yet, try now.
  if (pipe_handle == INVALID_HANDLE_VALUE) {
    connect_to_pipe();
  }

  // If connected, write to the pipe
  if (pipe_handle != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(pipe_handle, final_buffer, (DWORD)len, &written, NULL);
  }
}

// initialize a console for logging
bool initialize_log()
{
#ifdef _DEBUG
  // Detach from any existing console in the target
  FreeConsole();

  // Force the target's stdout/stderr to go to NUL
  HANDLE hNullOut = CreateFile(
    "NUL:",
    GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  );
  if (hNullOut != INVALID_HANDLE_VALUE) {
    SetStdHandle(STD_OUTPUT_HANDLE, hNullOut);
    SetStdHandle(STD_ERROR_HANDLE, hNullOut);
  }
#endif

#ifdef _DEBUG
  // patch out AttachConsole it seems the logging library of rekordbox uses that
  // to attach to any existing console and direct logging to there -- this causes
  // the console we allocate for debug mode to get flooded with rbox output so we
  // patch out AttachConsole and rbox is unable to use our console for logging
  uint8_t *attachC = (uint8_t *)GetProcAddress(GetModuleHandle("kernelbase.dll"), "AttachConsole");
  DWORD oldProt;
  VirtualProtect(attachC, 1, PAGE_EXECUTE_READWRITE, &oldProt);
  *attachC = 0xc3;
#endif

  // Attempt to connect immediately so the first log message doesn't block
  connect_to_pipe();

  // For example:
  info("Injected logger initialized.");
  return true;
}

// On DLL unload, clean up the pipe
void shutdown_log()
{
  close_pipe();
}
