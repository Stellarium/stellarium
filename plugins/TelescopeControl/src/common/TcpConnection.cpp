/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

Author and Copyright of this file:
Rumen Bogdanovski, 2026

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

#include "TcpConnection.hpp"
#include "LogFile.hpp"
#include "StelUtils.hpp"

#ifdef Q_OS_WIN
  #include <ws2tcpip.h> // getaddrinfo, freeaddrinfo
#else
  #include <netdb.h>    // getaddrinfo, freeaddrinfo
#endif

#include <cstring> // memset
#include <string>

namespace
{
//! Number of seconds we are willing to wait for the connection to be
//! established before giving up.
const int CONNECT_TIMEOUT_SECONDS = 5;

#ifdef Q_OS_WIN
//! Make sure the Windows Sockets library is initialised before we use it.
//! WSAStartup() is reference counted, so this cooperates with Qt (which
//! initialises WinSock too) and the matching WSACleanup() is intentionally
//! omitted for the lifetime of the process.
void ensureWinSock(void)
{
	static bool initialised = false;
	if (!initialised)
	{
		WSADATA wsa_data;
		WSAStartup(MAKEWORD(2, 2), &wsa_data);
		initialised = true;
	}
}
#endif
} // anonymous namespace

TcpConnection::TcpConnection(Server &server, const char *host, int port)
	: Connection(server, INVALID_SOCKET)
{
#ifdef Q_OS_WIN
	ensureWinSock();
#endif

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;   // allow both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP

	const std::string port_string = std::to_string(port);
	struct addrinfo *address_list = nullptr;
	const int gai_rc = getaddrinfo(host, port_string.c_str(), &hints, &address_list);
	if (gai_rc != 0 || address_list == nullptr)
	{
		*log_file << Now() << "TcpConnection::TcpConnection(" << host << ':' << port << "): "
		             "cannot resolve host: " << gai_strerror(gai_rc) << StelUtils::getEndLineChar();
		return;
	}

	// Try each resolved address in turn until one connects.
	for (struct addrinfo *ai = address_list; ai != nullptr; ai = ai->ai_next)
	{
		const SOCKET sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (IS_INVALID_SOCKET(sock))
			continue;

		if (SETNONBLOCK(sock) < 0)
		{
			close(sock);
			continue;
		}

		const int connect_rc = connect(sock, ai->ai_addr, static_cast<SOCKLEN_T>(ai->ai_addrlen));
		bool connected = (connect_rc == 0);
		if (!connected)
		{
#ifdef Q_OS_WIN
			const bool in_progress = (ERRNO == WSAEWOULDBLOCK || ERRNO == WSAEINPROGRESS);
#else
			const bool in_progress = (ERRNO == EINPROGRESS || ERRNO == EAGAIN || ERRNO == EINTR);
#endif
			if (in_progress)
			{
				// Non-blocking connect in progress: wait (with a timeout) for
				// the socket to become writable, then verify it really connected.
				fd_set write_fds;
				FD_ZERO(&write_fds);
				FD_SET(sock, &write_fds);
				struct timeval tv;
				tv.tv_sec  = CONNECT_TIMEOUT_SECONDS;
				tv.tv_usec = 0;
				const int select_rc = select(static_cast<int>(sock) + 1, nullptr, &write_fds, nullptr, &tv);
				if (select_rc > 0 && FD_ISSET(sock, &write_fds))
				{
					int so_error = 0;
					SOCKLEN_T len = sizeof(so_error);
					if (getsockopt(sock, SOL_SOCKET, SO_ERROR,
					               reinterpret_cast<char *>(&so_error), &len) == 0 && so_error == 0)
						connected = true;
				}
			}
		}

		if (connected)
		{
			fd = sock;
			break;
		}

		close(sock);
	}

	freeaddrinfo(address_list);

	if (IS_INVALID_SOCKET(fd))
	{
		// Note: after a non-blocking connect ERRNO is not a reliable indicator
		// of the failure reason, so we do not print it here.
		*log_file << Now() << "TcpConnection::TcpConnection(" << host << ':' << port << "): "
		             "cannot connect to host" << StelUtils::getEndLineChar();
	}
}
