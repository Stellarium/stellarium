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

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <fcntl.h>

//FIXME: Remove these macro-redefinitions, we have no business messing with them
//ESPECIALLY if the change the values and are not commented
#ifdef _MSC_VER
//for now, this warning is disabled when this header is included
#pragma warning(disable: 4005)
#endif

#define ERRNO WSAGetLastError()
#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#undef EINTR
#define EINTR WSAEINTR
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
static inline int SetNonblocking(int s)
{
	u_long arg = 1;
	return ioctlsocket(s, FIONBIO, &arg);
}
#define SETNONBLOCK(s) SetNonblocking(s)
#define SOCKLEN_T int
#define close closesocket
#define IS_INVALID_SOCKET(fd) (fd==INVALID_SOCKET)
#define STRERROR(x) x
//  #define DEBUG5

#else

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> // strerror
#define ERRNO errno
#define SETNONBLOCK(s) fcntl(s,F_SETFL,O_NONBLOCK)
#define SOCKLEN_T socklen_t
#define SOCKET int
#define IS_INVALID_SOCKET(fd) (fd<0)
#define INVALID_SOCKET (-1)
#define STRERROR(x) strerror(x)

#endif //Q_OS_WIN

long long int GetNow(void);

class Server;

class Socket
{
public:
	virtual ~Socket() { hangup(); }
	void hangup();
	virtual void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max) = 0;
	virtual void handleSelectFds(const fd_set &read_fds, const fd_set &write_fds) = 0;
	virtual bool isClosed() const
	{
		return IS_INVALID_SOCKET(fd);
	}
	virtual bool isTcpConnection() const { return false; }
	virtual void sendPosition(unsigned int ra_int, int dec_int, int status) {Q_UNUSED(ra_int); Q_UNUSED(dec_int); Q_UNUSED(status);}
	
protected:
	Socket(Server &server, SOCKET fd) : server(server), fd(fd) {}
	Server & server;
	
#ifdef Q_OS_WIN
	virtual int readNonblocking(char *buf, int count)
	{
		return recv(fd, buf, count, 0);
	}
	virtual int writeNonblocking(const char *buf, int count)
	{
		return send(fd, buf, count, 0);
	}
#else
	int readNonblocking(void *buf, int count)
	{
		return read(fd, buf, count);
	}
	int writeNonblocking(const void *buf, int count) {
		return write(fd, buf, count);
	}
#endif

protected:
	SOCKET fd;
	
private:
	// no copying
	Socket(const Socket&);
	const Socket &operator=(const Socket&);
};

#endif
