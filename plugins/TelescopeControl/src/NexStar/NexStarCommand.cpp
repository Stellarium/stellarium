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

#include "NexStarCommand.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#include "common/TelescopeLogging.hpp"

#include <cmath>

NexStarCommand::NexStarCommand(Server &server) : server(*static_cast<TelescopeClientDirectNexStar*>(&server)), has_been_written_to_buffer(false)
{
}

NexStarCommandGotoPosition::NexStarCommandGotoPosition(Server &server, unsigned int ra_int, int dec_int) : NexStarCommand(server)
{
	dec = dec_int;
	ra = static_cast<int>(ra_int);
}

#define NIBTOASCII(x) (((x)<10)?('0'+(x)):('A'+(x)-10))
#define ASCIITONIB(x) (((x)<'A')?((x)-'0'):((x)-'A'+10))

bool NexStarCommandGotoPosition::writeCommandToBuffer(char *&p,char *end)
{
	char *b = p;
	
	if (end-p < 18)
		return false;
	// high-precision aiming:
	*p++ = 'r';
	
	// set the RA
	int x = ra;
	*p++ = NIBTOASCII ((x>>28) & 0x0f); 
	*p++ = NIBTOASCII ((x>>24) & 0x0f); 
	*p++ = NIBTOASCII ((x>>20) & 0x0f); 
	*p++ = NIBTOASCII ((x>>16) & 0x0f); 
	*p++ = NIBTOASCII ((x>>12) & 0x0f); 
	*p++ = NIBTOASCII ((x>>8) & 0x0f); 
	*p++ = NIBTOASCII ((x>>4) & 0x0f); 
	*p++ = NIBTOASCII (x & 0x0f); 
	*p++ = ',';

	// set object dec:
	x = dec;
	*p++ = NIBTOASCII ((x>>28) & 0x0f); 
	*p++ = NIBTOASCII ((x>>24) & 0x0f); 
	*p++ = NIBTOASCII ((x>>20) & 0x0f); 
	*p++ = NIBTOASCII ((x>>16) & 0x0f); 
	*p++ = NIBTOASCII ((x>>12) & 0x0f); 
	*p++ = NIBTOASCII ((x>>8) & 0x0f); 
	*p++ = NIBTOASCII ((x>>4) & 0x0f); 
	*p++ = NIBTOASCII (x & 0x0f); 
	*p = 0;

	has_been_written_to_buffer = true;
	qCDebug(Telescopes) << server.getLogPrefix() << "NexStarCommandGotoPosition::writeCommandToBuffer:"
	                    << QByteArray(b, static_cast<int>(p - b));
	
	return true;
}

int NexStarCommandGotoPosition::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;
	
	if (*buff=='#')
	{
		qCDebug(Telescopes) << server.getLogPrefix()
		                    << "NexStarCommandGotoPosition::readAnswerFromBuffer: slew ok";
	}
	else
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "NexStarCommandGotoPosition::readAnswerFromBuffer: slew failed.";
	}
	buff++;
	return 1;
}

void NexStarCommandGotoPosition::print(QDebug &o) const
{
	o << "NexStarCommandGotoPosition("
	  << ra << "," << dec <<')';
}

NexStarCommandSync::NexStarCommandSync(Server &server, unsigned int ra_int, int dec_int) : NexStarCommand(server)
{
	dec = dec_int;
	ra = static_cast<int>(ra_int);
}

bool NexStarCommandSync::writeCommandToBuffer(char *&p,char *end)
{
	char *b = p;

	if (end-p < 18)
		return false;
	// high-precision aiming:
	*p++ = 's';

	// set the RA
	int x = ra;
	*p++ = NIBTOASCII ((x>>28) & 0x0f);
	*p++ = NIBTOASCII ((x>>24) & 0x0f);
	*p++ = NIBTOASCII ((x>>20) & 0x0f);
	*p++ = NIBTOASCII ((x>>16) & 0x0f);
	*p++ = NIBTOASCII ((x>>12) & 0x0f);
	*p++ = NIBTOASCII ((x>>8) & 0x0f);
	*p++ = NIBTOASCII ((x>>4) & 0x0f);
	*p++ = NIBTOASCII (x & 0x0f);
	*p++ = ',';

	// set dec:
	x = dec;
	*p++ = NIBTOASCII ((x>>28) & 0x0f);
	*p++ = NIBTOASCII ((x>>24) & 0x0f);
	*p++ = NIBTOASCII ((x>>20) & 0x0f);
	*p++ = NIBTOASCII ((x>>16) & 0x0f);
	*p++ = NIBTOASCII ((x>>12) & 0x0f);
	*p++ = NIBTOASCII ((x>>8) & 0x0f);
	*p++ = NIBTOASCII ((x>>4) & 0x0f);
	*p++ = NIBTOASCII (x & 0x0f);
	*p = 0;

	has_been_written_to_buffer = true;
	qCDebug(Telescopes) << server.getLogPrefix() << "NexStarCommandSync::writeCommandToBuffer:"
	                    << QByteArray(b, static_cast<int>(p - b));

	return true;
}

int NexStarCommandSync::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;

	if (*buff=='#')
	{
		qCDebug(Telescopes) << server.getLogPrefix() << "NexStarCommandSync::readAnswerFromBuffer: sync ok";
	}
	else
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "NexStarCommandSync::readAnswerFromBuffer: sync failed.";
	}
	buff++;
	return 1;
}

void NexStarCommandSync::print(QDebug &o) const
{
	o << "NexStarCommandSync("
	  << ra << "," << dec <<')';
}

bool NexStarCommandAbort::writeCommandToBuffer(char *&p,char *end)
{
	char *b = p;

	if (end-p < 1)
		return false;
	// Only one char:
	*p++ = 'M';

	has_been_written_to_buffer = true;
	qCDebug(Telescopes) << server.getLogPrefix() << "NexStarCommandAbort::writeCommandToBuffer:"
	                    << QByteArray(b, static_cast<int>(p - b));

	return true;
}

int NexStarCommandAbort::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;

	if (*buff=='#')
		qCDebug(Telescopes) << server.getLogPrefix() << "NexStarCommandAbort::readAnswerFromBuffer: answer ok";
	else
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "NexStarCommandAbort::readAnswerFromBuffer: abort failed.";
	buff++;
	return 1;
}

void NexStarCommandAbort::print(QDebug &o) const
{
	o << "NexStarCommandAbort()";
}

bool NexStarCommandGetRaDec::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 1)
		return false;
	// get RA:
	*p++ = 'e';
	has_been_written_to_buffer = true;
	return true;
}

int NexStarCommandGetRaDec::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (end-buff < 18)
		return 0;

	int ra, dec;
	const char *p = buff;

	// Next 8 bytes are RA.
	ra = 0;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++; 
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); ra <<= 4; p++;
	ra += ASCIITONIB(*p); p++;

	if (*p++ != ',')
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "NexStarCommandGetRaDec::readAnswerFromBuffer: error: ',' expected";
		return -1;
	}

	// Next 8 bytes are DEC.
	dec = 0;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); dec <<= 4; p++;
	dec += ASCIITONIB(*p); p++;

	if (*p++ != '#')
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "NexStarCommandGetRaDec::readAnswerFromBuffer: error: '#' expected";
		return -1;
	}

	qCDebug(Telescopes).nospace() << server.getLogPrefix()
	                              << "NexStarCommandGetRaDec::readAnswerFromBuffer: ra = "
	                              << ra << ", dec = " << dec;
	buff = p;

	server.raReceived(static_cast<unsigned int>(ra));
	server.decReceived(static_cast<unsigned int>(dec));
	return 1;
}

void NexStarCommandGetRaDec::print(QDebug &o) const
{
	o << "NexStarCommandGetRaDec";
}

