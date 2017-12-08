/*
 * Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

   Base on the following works:
	* Firmata GUI example. http://www.pjrc.com/teensy/firmata_test/
	  Copyright 2010, Paul Stoffregen (paul at pjrc.com)

	* firmataplus: http://sourceforge.net/projects/firmataplus/
	  Copyright (c) 2008 - Scott Reid, dataczar.com

   Firmata C++ library. 
*/

#ifndef ARDUINO_H
#define ARDUINO_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <termios.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define ARDUINO_DEFAULT_BAUD   19200
#define ARDUINO_DIGITAL_PINS   0x0E      // # of digital pins
#define ARDUINO_ANALOG         PINS 0x16 // # of analog pins
#define ARDUINO_N_PORTS        0x03      // total # of ports for the board
#define ARDUINO_DIG_PORTS      0x03      // # of digital ports on the board
#define ARDUINO_ANALOG_PORT    0x02      // port# of analog used as digital
#define ARDUINO_HIGH           0x01      // digital output pin 5V command
#define ARDUINO_LOW            0x00      // digital output pin 0V command
#define ARDUINO_MAX_DATA_BYTES 256

using namespace std;

class Arduino
{
  public:
    Arduino();
    ~Arduino();
    int destroy();
    int sendUchar(const unsigned char);
    int sendString(const string);
    int readPort(void *buff, int count);
    int openPort(const char *_serialPort);
    int openPort(const char *_serialPort, int _baud);
    int closePort();
    int flushPort();

  protected:
    /* Serial port to which the arduino is connected */
    char serialPort[PATH_MAX];
    int baud;
    struct termios oldterm;
    struct termios term;
    int flags;
    /* File descriptor associated with serial connection (-1 if no valid
		* connection) */
    int fd;
};

#endif // ARDUINO_H
