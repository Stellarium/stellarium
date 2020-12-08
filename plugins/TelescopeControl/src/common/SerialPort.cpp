/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "SerialPort.hpp"
#include "LogFile.hpp"
#include "StelUtils.hpp"

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#include <cstring> // memset

using namespace std;

SerialPort::SerialPort(Server &server, const char *serial_device)
	: Connection(server, INVALID_SOCKET)
	#ifndef Q_OS_WIN
	, termios_original(termios())
	#endif
{
#ifdef Q_OS_WIN
	handle = CreateFile(serial_device, GENERIC_READ|GENERIC_WRITE, 0, Q_NULLPTR, OPEN_EXISTING, 0, Q_NULLPTR);
	if (handle == INVALID_HANDLE_VALUE)
	{
		*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
				      "CreateFile() failed: " << GetLastError() << StelUtils::getEndLineChar();
	}
	else
	{
		COMMTIMEOUTS timeouts;
		timeouts.ReadIntervalTimeout = MAXDWORD; 
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;
		if (!SetCommTimeouts(handle, &timeouts))
		{
			*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
					      "SetCommTimeouts() failed: " << GetLastError() << StelUtils::getEndLineChar();
		}
		else
		{
			if (!GetCommState(handle, &dcb_original))
			{
				*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
						      "GetCommState() failed: " << GetLastError() << StelUtils::getEndLineChar();
			}
			else
			{
				DCB dcb;
				memset(&dcb, 0, sizeof(dcb));
				dcb.DCBlength = sizeof(dcb);
				if (!BuildCommDCB("9600,n,8,1", &dcb))
				{
					*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
							      "BuildCommDCB() failed: " << GetLastError() << StelUtils::getEndLineChar();
				}
				else
				{
					if (!SetCommState(handle,&dcb))
					{
						*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
								      "SetCommState() failed: " << GetLastError() << StelUtils::getEndLineChar();
					}
					else
					{
						// success
						return;
					}
				}
			}
		}
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}
#else
	fd = open(serial_device, O_RDWR|O_NOCTTY);
	if (fd < 0)
	{
		*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
				      "open() failed: " << strerror(errno) << StelUtils::getEndLineChar();
	}
	else
	{
		if (SETNONBLOCK(fd) < 0)
		{
			*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
					      "fcntl(O_NONBLOCK) failed: " << STRERROR(ERRNO) << StelUtils::getEndLineChar();
		}
		else
		{
			if (tcgetattr(fd,&termios_original) < 0)
			{
				*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
						      "tcgetattr failed: " << strerror(errno) << StelUtils::getEndLineChar();
			}
			else
			{
				struct termios termios_new;
				memset(&termios_new, 0, sizeof(termios_new));
				termios_new.c_cflag = CS8    |  // 8 data bits
				                      // no parity because PARENB is not set
				                      CLOCAL |  // Ignore modem control lines
				                      CREAD;    // Enable receiver
				cfsetospeed(&termios_new, B9600);
				termios_new.c_lflag = 0;
				termios_new.c_cc[VTIME] = 0;
				termios_new.c_cc[VMIN] = 1;
				if (tcsetattr(fd,TCSAFLUSH,&termios_new) < 0)
				{
					*log_file << Now() << "SerialPort::SerialPort(" << serial_device << "): "
							      "tcsetattr failed: " << strerror(errno) << StelUtils::getEndLineChar();
				}
				else
				{
					// success
					return;
				}
			}
		}
		close(fd);
		fd = -1;
	}
#endif //Q_OS_WIN
}

SerialPort::~SerialPort(void)
{
#ifdef Q_OS_WIN
	if (handle != INVALID_HANDLE_VALUE)
	{
		// restore original settings
		SetCommState(handle, &dcb_original);
		CloseHandle(handle);
	}
#else
	if (fd >= 0)
	{
		// restore original settings
		tcsetattr(fd, TCSANOW, &termios_original);
		close(fd);
		fd=INVALID_SOCKET; // fix Coverity 48939?
	}
#endif
}


#ifdef Q_OS_WIN

int SerialPort::readNonblocking(char *buf, int count)
{
	DWORD rval;
	if (ReadFile(handle, buf, static_cast<DWORD>(count), &rval, Q_NULLPTR))
		return static_cast<int>(rval);
	if (GetLastError() == ERROR_IO_PENDING)
		return 0;
	return -1;
}

int SerialPort::writeNonblocking(const char *buf, int count)
{
	DWORD rval;
	if (WriteFile(handle, buf, static_cast<DWORD>(count), &rval, Q_NULLPTR))
		return static_cast<int>(rval);
	if (GetLastError() == ERROR_IO_PENDING)
		return 0;
	return -1;
}

#endif

void SerialPort::prepareSelectFds(fd_set &read_fds,
                                  fd_set &write_fds,
                                  int &fd_max)
{
#ifdef Q_OS_WIN
	Q_UNUSED(read_fds)
	Q_UNUSED(write_fds)
	Q_UNUSED(fd_max)
	// handle all IO here
	if (write_buff_end > write_buff)
		performWriting();
	performReading();
#else
	Connection::prepareSelectFds(read_fds, write_fds, fd_max);
#endif //Q_OS_WIN
}
