/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#ifdef WIN32

#include <winsock2.h>
#include <fcntl.h>

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

#endif //WIN32

long long int GetNow(void);

class Server;

class Socket
{
public:
	virtual ~Socket(void) { hangup(); }
	void hangup(void);
	virtual void prepareSelectFds(fd_set &read_fds, fd_set &write_fds, int &fd_max) = 0;
	virtual void handleSelectFds(const fd_set &read_fds, const fd_set &write_fds) = 0;
	virtual bool isClosed(void) const
	{
		return IS_INVALID_SOCKET(fd);
	}
	virtual bool isTcpConnection(void) const { return false; }
	virtual void sendPosition(unsigned int ra_int, int dec_int, int status) {}
	
protected:
	Socket(Server &server, SOCKET fd) : server(server), fd(fd) {}
	Server & server;
	
#ifdef WIN32
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
