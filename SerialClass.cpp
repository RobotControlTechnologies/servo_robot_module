
//#define __STRICT_ANSI__

#define ERROR_VALUE -1

#include "SerialClass.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include <errno.h>
#include "error.h"
Serial::Serial(char *portName) {
  // We're not yet connected
  this->connected = false;

#ifdef _WIN32
  // Try to connect to the given port throuh CreateFile
  this->hSerial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  // Check if the connection was successfull
  if (this->hSerial == INVALID_HANDLE_VALUE) {
    // If not success full display an Error
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      // Print Error if neccessary
      throw new Error(ConsoleColor(ConsoleColor::red),"Error: Handle was not attached. Reason: %s not available.\n",
             portName);

    } else {
      throw new Error(ConsoleColor(ConsoleColor::red),"Unknown error on com port!");
    }
  } else {
    // If connected we try to set the comm parameters
    DCB dcbSerialParams = {0};

    // Try to get the current
    if (!GetCommState(this->hSerial, &dcbSerialParams)) {
      // If impossible, show an error
      printf("failed to get current serial parameters!");
    } else {
      // Define serial connection parameters for the arduino board
      dcbSerialParams.BaudRate = CBR_9600;
      dcbSerialParams.ByteSize = 8;
      dcbSerialParams.StopBits = ONESTOPBIT;
      dcbSerialParams.Parity = NOPARITY;
      dcbSerialParams.XonLim = 42;
      dcbSerialParams.XoffLim = 42;
      dcbSerialParams.fAbortOnError = TRUE;
      dcbSerialParams.fDtrControl = DTR_CONTROL_HANDSHAKE;
      dcbSerialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
      dcbSerialParams.fBinary = TRUE;
      dcbSerialParams.fParity = FALSE;
      dcbSerialParams.fInX = FALSE;
      dcbSerialParams.fOutX = FALSE;
      dcbSerialParams.XonChar = (unsigned char) 0x11;
      dcbSerialParams.XoffChar = (unsigned char) 0x13;
      dcbSerialParams.fErrorChar = FALSE;
      dcbSerialParams.fNull = FALSE;
      dcbSerialParams.fOutxCtsFlow = FALSE;
      dcbSerialParams.fOutxDsrFlow = FALSE;

      // Set the parameters and check for their proper application
      if (!SetCommState(hSerial, &dcbSerialParams)) {
        throw new Error(ConsoleColor(ConsoleColor::red),"Error: Could not set Serial Port parameters");
      } else {
        // If everything went fine we're connected
        this->connected = true;
        // Flush any remaining characters in the buffers
        PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
        // We wait 2s as the arduino board will be reseting
        Sleep(ARDUINO_WAIT_TIME);
      }
    }
  }
#else
  com = open(portName, O_RDWR | O_NOCTTY);
  memset(&tty, 0, sizeof tty);

  /* Error Handling */
  if (tcgetattr(com, &tty) != 0) {
    throw new Error(ConsoleColor(ConsoleColor::red),"Error handling  %d from tcgetattr: %s\n", errno, strerror(errno));
  }

  /* Save old tty parameters */
  tty_old = tty;

  /* Set Baud Rate */
  cfsetospeed(&tty, (speed_t)B9600);

  /* Setting other Port Stuff */
  tty.c_cflag &= ~PARENB;  // Make 8n1
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;

  tty.c_cflag &= ~CRTSCTS;        // no flow control
  tty.c_cc[VMIN] = 1;             // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  tty.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines

  /* Make raw */
  cfmakeraw(&tty);

  /* Flush Port, then applies attributes */
  tcflush(com, TCIFLUSH);
  if (tcsetattr(com, TCSANOW, &tty) != 0) {
    throw new Error(ConsoleColor(ConsoleColor::red), "Error apply attributes %d from tcsetattr\n", errno);
  }
  this->connected = true;
#endif
}

Serial::~Serial() {
  // Check if we are connected before trying to disconnect
  if (this->connected) {
    // We're no longer connected
    this->connected = false;
#ifdef _WIN32
    // Close the serial handler
    CloseHandle(this->hSerial);
#else
    close(com);
#endif
  }
}

bool Serial::WriteData(unsigned char *buffer, unsigned int nbChar) {
#ifdef _WIN32
  DWORD bytesSend;

  // Try to write the buffer on the Serial port
  if (!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0)) {
    // In case it don't work get comm error and return false
    ClearCommError(this->hSerial, &this->errors, &this->status);
    return false;
  } else
    ::Sleep(250);          // Sleep 250ms to ensure a good break
    FlushFileBuffers(this->hSerial);
    PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
#else
  if (write(com, buffer, nbChar) == ERROR_VALUE) {
    return false;
  };
  return true;
#endif
}

bool Serial::IsConnected() {
  // Simply return the connection status
  return this->connected;
}