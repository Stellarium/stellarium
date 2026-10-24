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

#include "NexStarConnection.hpp"
#include "NexStarCommand.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#include "common/TelescopeLogging.hpp"

#include <iostream>

template <class Transport>
void NexStarConnectionImpl<Transport>::resetCommunication(void)
{
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}
	
	this->read_buff_end = this->read_buff;
	this->write_buff_end = this->write_buff;
	qCDebug(Telescopes) << this->getLogPrefix() << "NexStarConnection::resetCommunication";
}

template <class Transport>
void NexStarConnectionImpl<Transport>::sendGoto(unsigned int ra_int, int dec_int)
{
	sendCommand(new NexStarCommandGotoPosition(this->server, ra_int, dec_int));
}

template <class Transport>
void NexStarConnectionImpl<Transport>::sendSync(unsigned int ra_int, int dec_int)
{
	sendCommand(new NexStarCommandSync(this->server, ra_int, dec_int));
}

template <class Transport>
void NexStarConnectionImpl<Transport>::sendAbort()
{
	qCDebug(Telescopes) << this->getLogPrefix() << "NexStarConnection::sendAbort()";
	// Remove queued commands (pointers, so delete their objects!)
	while (!command_list.empty())
	{
		delete command_list.front();
		command_list.pop_front();
	}

	sendCommand(new NexStarCommandAbort(this->server));
}

template <class Transport>
void NexStarConnectionImpl<Transport>::dataReceived(const char *&p,const char *read_buff_end)
{
	if (this->isClosed())
	{
		qCWarning(Telescopes) << this->getLogPrefix()
		                      << "NexStarConnection::dataReceived: strange: fd is closed";
	}
	else if (command_list.empty())
	{
		qCWarning(Telescopes) << this->getLogPrefix()
		                      << "NexStarConnection::dataReceived: error: command_list is empty";
		resetCommunication();
		static_cast<TelescopeClientDirectNexStar*>(&this->server)->communicationResetReceived();
	}
	else if (command_list.front()->needsNoAnswer())
	{
		qCWarning(Telescopes).nospace() << this->getLogPrefix()
		                                << "NexStarConnection::dataReceived: "
		                                   "strange: command("
		                                << *command_list.front() << ") needs no answer";
	}
	else
	{
		while(true)
		{
			const int rc=command_list.front()->readAnswerFromBuffer(p, read_buff_end);
			/*
			qCDebug(Telescopes).nospace() << this->getLogPrefix() << "NexStarConnection::dataReceived: "
			                              << *command_list.front() << "->readAnswerFromBuffer returned "
			                              << rc;
			*/
			if (rc <= 0)
			{
				if (rc < 0)
				{
					resetCommunication();
					static_cast<TelescopeClientDirectNexStar*>(&this->server)
						->communicationResetReceived();
				}
				break;
			}
			delete command_list.front();
			command_list.pop_front();
			if (command_list.empty())
				break;
			if (!command_list.front()->writeCommandToBuffer(
			                                   this->write_buff_end,
			                                   this->write_buff+sizeof(this->write_buff)))
				break;
		}
	}
}

template <class Transport>
void NexStarConnectionImpl<Transport>::sendCommand(NexStarCommand *command)
{
	if (command)
	{
		qCDebug(Telescopes).nospace() << this->getLogPrefix() << "NexStarConnection::sendCommand("
		                              << *command << ")";
		/*
		// Uncomment with the end diagnostic below: processing may delete command.
		QString commandDescription;
		QDebug(&commandDescription).nospace() << *command;
		*/
		command_list.push_back(command);
		while (!command_list.front()->hasBeenWrittenToBuffer())
		{
			if (command_list.front()->writeCommandToBuffer(this->write_buff_end,
			                                             this->write_buff+sizeof(this->write_buff)))
			{
				/*
				qCDebug(Telescopes).nospace() << this->getLogPrefix()
				                              << "NexStarConnection::sendCommand: "
				                              << *command_list.front()
				                              << "::writeCommandToBuffer ok";
				*/
				if (command_list.front()->needsNoAnswer())
				{
					delete command_list.front();
					command_list.pop_front();
					if (command_list.empty())
						break;
				}
				else
				{
					break;
				}
			}
			else
			{
				/*
				qCDebug(Telescopes).nospace() << this->getLogPrefix()
				                              << "NexStarConnection::sendCommand: "
				                              << *command_list.front()
				                              << "::writeCommandToBuffer failed";
				*/
				break;
			}
		}
		/*
		qCDebug(Telescopes).nospace().noquote() << this->getLogPrefix() << "NexStarConnection::sendCommand("
		                                        << commandDescription << ") end";
		*/
	}
}

// Explicitly instantiate the serial and TCP/IP transports.
template class NexStarConnectionImpl<SerialPort>;
template class NexStarConnectionImpl<TcpConnection>;
