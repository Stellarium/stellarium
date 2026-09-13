/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

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

#include "Server.hpp"
#include "Socket.hpp"
//#include "Listener.hpp"

void Server::SocketList::clear(void)
{
	for (auto* socket : *this)
	{
		delete socket;
	}
	list<Socket*>::clear();
}

Server::Server(int)
{
	//Socket *listener = new Listener(*this, port);
	//socket_list.push_back(listener);
}

void Server::sendPosition(unsigned int ra_int, int dec_int, int status)
{
	for (auto* socket : socket_list)
	{
		socket->sendPosition(ra_int, dec_int, status);
	}
}

void Server::step(long long int timeout_micros)
{
	fd_set read_fds, write_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	int fd_max = -1;
	
	for (auto* socket : socket_list)
	{
		socket->prepareSelectFds(read_fds, write_fds, fd_max);
	}
	
	struct timeval tv;
	if (timeout_micros < 0)
		timeout_micros = 0;
	tv.tv_sec = static_cast<long>(timeout_micros / 1000000);
	tv.tv_usec = static_cast<long>(timeout_micros % 1000000);
	const int select_rc = select(fd_max+1, &read_fds, &write_fds, nullptr, &tv);
	if (select_rc > 0)
	{
		// Note: a connection that has become closed is intentionally NOT
		// deleted here. A device client (e.g. TelescopeClientDirectLx200)
		// keeps a pointer to its connection for its whole lifetime; deleting
		// the object here would leave that pointer dangling and cause a
		// use-after-free (e.g. a crash after a TCP connection drops). Closed
		// connections stay in the list (their prepareSelectFds()/
		// handleSelectFds() become no-ops once the socket is invalid) and are
		// freed when the Server itself is destroyed.
		for (auto* socket : socket_list)
		{
			socket->handleSelectFds(read_fds, write_fds);
		}
	}
}

void Server::closeAcceptedConnections(void)
{
	for (auto* socket : socket_list)
	{
		if (socket->isTcpConnection())
		{
			socket->hangup();
		}
	}
}


