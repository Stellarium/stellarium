/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains sample server classes (dummy, Meade LX200).

Author and Copyright of this file and of the stellarium telescope library:
Johannes Gajdosik, 2006, modified for NexStar telescopes by Michael Heinz.

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

#ifndef NEXSTARCONNECTION_HPP
#define NEXSTARCONNECTION_HPP

#include "common/SerialPort.hpp"
#include "common/TcpConnection.hpp"

#include <list>

class NexStarCommand;

//! Transport-independent interface to a Celestron NexStar or a compatible telescope.
//!
//! The NexStar protocol can be spoken either over a serial port or over a TCP/IP
//! connection. TelescopeClientDirectNexStar talks to the telescope through this
//! interface without needing to know which transport is in use.
class NexStarConnection
{
public:
	virtual ~NexStarConnection() {}
	virtual void sendGoto(unsigned int ra_int, int dec_int) = 0;
	virtual void sendSync(unsigned int ra_int, int dec_int) = 0;
	//! Delete pending commands and send abort signal.
	virtual void sendAbort() = 0;
	virtual void sendCommand(NexStarCommand *command) = 0;
	//! Returns true if the underlying connection is closed.
	virtual bool isClosed() const = 0;
};

//! Implements the NexStar command protocol over SerialPort or TcpConnection.
template <class Transport>
class NexStarConnectionImpl : public Transport, public NexStarConnection
{
public:
	//! Construct the underlying transport with its serial device or host and port.
	template <class... TransportArgs>
	NexStarConnectionImpl(Server &server, TransportArgs... transport_args)
		: Transport(server, transport_args...) {}
	//! Delete all pending commands before destroying the transport.
	~NexStarConnectionImpl() override { resetCommunication(); }
	void sendGoto(unsigned int ra_int, int dec_int) override;
	void sendSync(unsigned int ra_int, int dec_int) override;
	void sendAbort() override;
	void sendCommand(NexStarCommand *command) override;
	bool isClosed() const override {return Transport::isClosed();}

private:
	void dataReceived(const char *&p, const char *read_buff_end) override;
	void sendPosition(unsigned int ra_int, int dec_int, int status) override {Q_UNUSED(ra_int) Q_UNUSED(dec_int) Q_UNUSED(status)}
	void resetCommunication(void);
	
private:
	std::list<NexStarCommand*> command_list;
};

//! Serial port connection to a Celestron NexStar or a compatible telescope.
class NexStarSerialConnection : public NexStarConnectionImpl<SerialPort>
{
public:
	NexStarSerialConnection(Server &server, const char *serial_device)
		: NexStarConnectionImpl<SerialPort>(server, serial_device) {}
};

//! TCP/IP connection to a Celestron NexStar or a compatible telescope.
class NexStarTcpConnection : public NexStarConnectionImpl<TcpConnection>
{
public:
	NexStarTcpConnection(Server &server, const char *host, int port)
		: NexStarConnectionImpl<TcpConnection>(server, host, port) {}
};

#endif // NEXSTARCONNECTION_HPP
