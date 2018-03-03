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

#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <list>
using namespace std;

class Socket;

//! Base class for telescope server classes. A true telescope server class
//! should inherit Server and implement device-specific functions.
//! The class makes heavy use of polymorphism to perform all its work
//! by maintaining a list of connections in the form of Socket pointers.
//! The list actually contains objects of classes that inherit Socket, including
//! one Listener object, created in the constructor, and one or more
//! Connection objects, each representing a TCP/IP connection to a client.
//! Classes that inherit Server (such as ServerLx200) also have a special
//! device-specific connection object (such as Lx200Connection) that represents
//! a serial connection to the device. The step() method calls
//! Socket::prepareSelectFds() and Socket::handleSelectFds() for each connection
//! in the list. These methods are reimplemented for each class.
class Server
{
public:
	Server(void) {}
	Server(int port);
	virtual ~Server(void) {}
	virtual void step(long long int timeout_micros);
	
protected:
	void sendPosition(unsigned int ra_int, int dec_int, int status);
	//! Adds this object to the list of connections maintained by this server.
	//! This method is called by Listener.
	//! @param s can be anything that inherits Socket, including Listener,
	//! Connection or any custom class that implements a serial port
	//! connection (such as Lx200Connection and NexStarConnection).
	void addConnection(Socket *s)
	{
		if (s)
			socket_list.push_back(s);
	}
	void closeAcceptedConnections(void);
	friend class Listener;
	
private:
	  // called by Connection:
	virtual void gotoReceived(unsigned int ra_int, int dec_int) = 0;
	friend class Connection;
	
	class SocketList : public list<Socket*>
	{
		public:
		~SocketList(void) { clear(); }
		void clear(void);
	};
	//! A list of the connections maintained by the server.
	SocketList socket_list;
};

#endif
