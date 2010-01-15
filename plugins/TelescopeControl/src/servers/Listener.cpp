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

#include "Listener.hpp"
#include "Connection.hpp"
#include "Server.hpp"
#include "LogFile.hpp"

#include <iostream>

#include <stdlib.h> // exit

using namespace std;

void Listener::prepareSelectFds(fd_set &read_fds,
                                fd_set &write_fds,
                                int &fd_max)
{
	if (IS_INVALID_SOCKET(fd))
	{
		struct sockaddr_in sock_addr;
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (IS_INVALID_SOCKET(fd))
		{
			*log_file << Now()
			          << "socket() failed: "
			          << STRERROR(ERRNO)
			          << endl;
			exit(127);
		}
		
		int yes = -1; // all bits set to 1
		if (0 != setsockopt(fd,
		                    SOL_SOCKET,
		                    SO_REUSEADDR,
		                    reinterpret_cast<const char*>(&yes),
		                    sizeof(int)))
		{
			*log_file << Now()
			          << "setsockopt(SO_REUSEADDR) failed: "
			          << STRERROR(ERRNO)
			          << endl;
			exit(127);
		}
		
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = INADDR_ANY;
		sock_addr.sin_port = htons(port);
		if (bind(fd, (struct sockaddr*)(&sock_addr), sizeof(sock_addr)))
		{
			*log_file << Now()
			          << "bind(...) failed: "
			          << STRERROR(ERRNO)
			          << endl;
			exit(127);
		}
		
		if (listen(fd, 10))
		{
			*log_file << Now()
			          << "listen(...) failed: "
			          << STRERROR(ERRNO)
			          << endl;
			exit(127);
		}
		
		*log_file << Now() << "listening on port " << port << endl;
	}
	else
	{
		if (fd_max < (int)fd)
			fd_max = (int)fd;
		FD_SET(fd, &read_fds);
	}
}

void Listener::handleSelectFds(const fd_set &read_fds,
                               const fd_set &write_fds)
{
	if (!IS_INVALID_SOCKET(fd)
	    && FD_ISSET(fd, const_cast<fd_set *>(&read_fds)))
	{
		struct sockaddr_in client_addr;
		SOCKLEN_T length = sizeof(client_addr);
		const SOCKET client_sock = accept(fd,
		                                  (struct sockaddr*)&client_addr,
		                                  &length);
		
		if (IS_INVALID_SOCKET(client_sock))
		{
			*log_file << Now()
			          << "accept(...) failed: "
			          << STRERROR(ERRNO)
			          << endl;
			close(client_sock);
			return;
		}
		
		*log_file << Now() << "connection accepted" << endl;
		if (0 != SETNONBLOCK(client_sock))
		{
			*log_file << Now()
			          << "SETNONBLOCK(...) failed: "
			          << STRERROR(ERRNO)
			          << endl;
			close(client_sock);
			return;
		}
		server.addConnection(new Connection(server, client_sock));
	}
}
