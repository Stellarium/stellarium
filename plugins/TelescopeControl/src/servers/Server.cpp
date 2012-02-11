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

#include "Server.hpp"
#include "Socket.hpp"
//#include "Listener.hpp"
#include "LogFile.hpp"

void Server::SocketList::clear(void)
{
	for (const_iterator it(begin()); it != end(); it++)
	{
		delete (*it);
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
	for (SocketList::const_iterator it(socket_list.begin());
	     it != socket_list.end();
	     it++)
	{
		(*it)->sendPosition(ra_int, dec_int, status);
	}
}

void Server::step(long long int timeout_micros)
{
	fd_set read_fds, write_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	int fd_max = -1;
	
	for (SocketList::const_iterator it(socket_list.begin());
	     it != socket_list.end();
	     it++)
	{
		(*it)->prepareSelectFds(read_fds, write_fds, fd_max);
	}
	
	struct timeval tv;
	if (timeout_micros < 0)
		timeout_micros = 0;
	tv.tv_sec = timeout_micros / 1000000;
	tv.tv_usec = timeout_micros % 1000000;
	const int select_rc = select(fd_max+1, &read_fds, &write_fds, 0, &tv);
	if (select_rc > 0)
	{
		SocketList::iterator it(socket_list.begin());
		while (it != socket_list.end())
		{
			(*it)->handleSelectFds(read_fds, write_fds);
			if ((*it)->isClosed())
			{
				SocketList::iterator tmp(it);
				it++;
				delete (*tmp);
				socket_list.erase(tmp);
			}
			else
			{
				it++;
			}
		}
	}
}

void Server::closeAcceptedConnections(void)
{
	for (SocketList::iterator it(socket_list.begin());
	     it != socket_list.end();
	     it++)
	{
		if ((*it)->isTcpConnection())
		{
			(*it)->hangup();
		}
	}
}


