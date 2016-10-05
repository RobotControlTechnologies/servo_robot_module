#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#define ARDUINO_WAIT_TIME 2000

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>   // UNIX standard function definitions
#include <fcntl.h>    // File control definitions
#include <errno.h>    // Error number definitions
#include <termios.h>  // POSIX terminal control definitions
#endif
#include <stdio.h>
#include <stdlib.h>

class Serial {
 private:
  // Connection status
  bool connected;

#ifdef _WIN32
  // Serial comm handler
  HANDLE hSerial;

  // Get various information about the connection
  COMSTAT status;
  // Keep track of last error
  DWORD errors;
#else
  int com;
  termios tty;
  termios tty_old;
#endif

 public:
  // Initialize Serial communication with the given COM port
  explicit Serial(char *portName);
  // Close the connection
  ~Serial();
  // Writes data from a buffer through the Serial connection
  // return true on success.
  bool WriteData(unsigned char *buffer, unsigned int nbChar);
  // Check if we are actually connected
  bool IsConnected();
};

#endif  // SERIALCLASS_H_INCLUDED