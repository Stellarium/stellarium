/*
The stellarium telescope library helps building
telescope server programs, that can communicate with stellarium
by means of the stellarium TCP telescope protocol.
It also contains smaple server classes (dummy, Meade LX200).

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
#include "common/LogFile.hpp"
#include "StelUtils.hpp"

#include <cmath>

using namespace std;

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
	#ifdef DEBUG5
	char *b = p;
	#endif
	
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
	#ifdef DEBUG5
	*log_file << Now() << "NexStarCommandGotoPosition::writeCommandToBuffer:"
		  << b << StelUtils::getEndLineChar();
	#endif
	
	return true;
}

int NexStarCommandGotoPosition::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;
	
	if (*buff=='#')
	{
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandGotoPosition::readAnswerFromBuffer: slew ok"
			  << StelUtils::getEndLineChar();
		#endif
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandGotoPosition::readAnswerFromBuffer: slew failed." << StelUtils::getEndLineChar();
		#endif
	}
	buff++;
	return 1;
}

void NexStarCommandGotoPosition::print(QTextStream &o) const
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
	#ifdef DEBUG5
	char *b = p;
	#endif

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
	#ifdef DEBUG5
	*log_file << Now() << "NexStarCommandSync::writeCommandToBuffer:"
		  << b << StelUtils::getEndLineChar();
	#endif

	return true;
}

int NexStarCommandSync::readAnswerFromBuffer(const char *&buff, const char *end) const
{
	if (buff >= end)
		return 0;

	if (*buff=='#')
	{
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandSync::readAnswerFromBuffer: sync ok"
			  << StelUtils::getEndLineChar();
		#endif
	}
	else
	{
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandSync::readAnswerFromBuffer: sync failed." << StelUtils::getEndLineChar();
		#endif
	}
	buff++;
	return 1;
}

void NexStarCommandSync::print(QTextStream &o) const
{
	o << "NexStarCommandSync("
	  << ra << "," << dec <<')';
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
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandGetRaDec::readAnswerFromBuffer: "
				      "error: ',' expected" << StelUtils::getEndLineChar();
		#endif
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
		#ifdef DEBUG4
		*log_file << Now() << "NexStarCommandGetRaDec::readAnswerFromBuffer: "
				      "error: '#' expected" << StelUtils::getEndLineChar();
		#endif
		return -1;
	}
	
	
	#ifdef DEBUG4
	*log_file << Now() << "NexStarCommandGetRaDec::readAnswerFromBuffer: "
	                      "ra = " << ra << ", dec = " << dec
		  << StelUtils::getEndLineChar();
	#endif
	buff = p;

	server.raReceived(static_cast<unsigned int>(ra));
	server.decReceived(static_cast<unsigned int>(dec));
	return 1;
}

void NexStarCommandGetRaDec::print(QTextStream &o) const
{
	o << "NexStarCommandGetRaDec";
}

