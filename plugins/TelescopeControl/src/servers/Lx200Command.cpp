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

#include "Lx200Command.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "LogFile.hpp"

#include <math.h>
#include <stdlib.h> // abs

#include <QByteArray>

using namespace std;

Lx200Command::Lx200Command(Server &server)
             : server(*static_cast<TelescopeClientDirectLx200*>(&server)),
               has_been_written_to_buffer(false)
{
}


bool Lx200CommandToggleFormat::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 4)
		return false;
	*p++ = '#';
	*p++ = ':';
	*p++ = 'U';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

void Lx200CommandToggleFormat::print(QTextStream &o) const
{
	o << "Lx200CommandToggleFormat";
}


bool Lx200CommandStopSlew::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 4)
		return false;
	*p++ = '#';
	*p++ = ':';
	*p++ = 'Q';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

void Lx200CommandStopSlew::print(QTextStream &o) const {
	o << "Lx200CommandStopSlew";
}


bool Lx200CommandSetSelectedRa::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 13) return false;
	  // set object ra:
	*p++ = ':';
	*p++ = 'S';
	*p++ = 'r';
	*p++ = ' ';
	int x = ra;
	p += 8;
	p[-1] = '0' + (x % 10); x /= 10;
	p[-2] = '0' + (x %  6); x /=  6;
	p[-3] = ':';
	p[-4] = '0' + (x % 10); x /= 10;
	p[-5] = '0' + (x %  6); x /=  6;
	p[-6] = ':';
	p[-7] = '0' + (x % 10); x /= 10;
	p[-8] = '0' + x;
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandSetSelectedRa::readAnswerFromBuffer(const char *&buff,
                                                    const char *end)
{
	if (buff < end && *buff=='#')
		buff++; // ignore silly byte
	
	if (buff >= end)
		return 0;
	
	switch (buff[0])
	{
		case '0':
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedRa::readAnswerFromBuffer:"
			             "ra invalid"
			          << endl;
			#endif
			buff++;
			break;
		
		case '1':
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedRa::readAnswerFromBuffer:"
			             "ra valid"
			          << endl;
			#endif
			buff++;
			break;
		
		default:
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedRa::readAnswerFromBuffer:"
			             "strange: unexpected char"
			          << endl;
			#endif
			break;
	}
	
	return 1;
}

void Lx200CommandSetSelectedRa::print(QTextStream &o) const
{
	o << "Lx200CommandSetSelectedRa("
	  << (ra/3600) << ':' << ((ra/60)%60) << ':' << (ra%60) << ')';
}


bool Lx200CommandSetSelectedDec::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 13)
		return false;
	
	  // set object dec:
	*p++ = ':';
	*p++ = 'S';
	*p++ = 'd';
	*p++ = ' ';
	int x = dec;
	if (x < 0)
	{
		*p++ = '-';
		x = -x;
	}
	else
	{
		*p++ = '+';
	}
	p += 8;
	p[-1] = '0' + (x % 10); x /= 10;
	p[-2] = '0' + (x %  6); x /=  6;
	p[-3] = ':';
	p[-4] = '0' + (x % 10); x /= 10;
	p[-5] = '0' + (x %  6); x /=  6;
	p[-6] = 223; // degree symbol
	p[-7] = '0' + (x % 10); x /= 10;
	p[-8] = '0' + x;
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandSetSelectedDec::readAnswerFromBuffer(const char *&buff,
                                                     const char *end)
{
	if (buff < end && *buff=='#')
		buff++; // ignore silly byte
	
	if (buff >= end)
		return 0;
	
	switch (buff[0])
	{
		case '0':
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedDec::readAnswerFromBuffer:"
			             "dec invalid"
			          << endl;
			#endif
			buff++;
			break;
		
		case '1':
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedDec::readAnswerFromBuffer:"
			             "dec valid"
			          << endl;
			#endif
			buff++;
			break;
		
		default:
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandSetSelectedDec::readAnswerFromBuffer:"
			             "strange: unexpected char"
			          << endl;
			#endif
			break;
	}
	
	return 1;
}

void Lx200CommandSetSelectedDec::print(QTextStream &o) const
{
	const int d = abs(dec);
	o << "Lx200CommandSetSelectedDec("
	  << ((dec<0)?'-':'+')
	  << (d/3600) << ':' << ((d/60)%60) << ':' << (d%60) << ')';
}


bool Lx200CommandGotoSelected::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 4)
		return false;
	
	  // slew to current object coordinates
	*p++ = ':';
	*p++ = 'M';
	*p++ = 'S';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandGotoSelected::readAnswerFromBuffer(const char *&buff,
                                                   const char *end)
{
	if (buff < end && *buff=='#')
		buff++; // ignore silly byte
	
	if (buff >= end)
		return 0;
	
	const char *p = buff;
	if (first_byte == 256)
	{
		first_byte = buff[0];
		p++;
	}
	
	switch (first_byte)
	{
		case '0':
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
			             "slew ok"
			          << endl;
			#endif
			buff++;
			return 1;
		
		case '1':
		case '2':
		{
			if (p == end)
			{
				// the AutoStar 494 returns just '1', nothing else
				#ifdef DEBUG4
				*log_file << Now()
				          << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
				             "slew failed ("
				          << ((char)first_byte)
				          << "), "
				             "but no complete answer yet"
				          << endl;
				#endif
				buff++;
				return 0;
			}
			
			for (;;p++)
			{
				if (p >= end)
				{
					return 0;
				}
				if (*p == '#')
					break;
			}
			#ifdef DEBUG4
			*log_file << Now()
			<< "Lx200CommandGotoSelected::readAnswerFromBuffer: "
			   "slew failed ("
			<< ((char)first_byte)
			<< "): '"
			<< QByteArray(buff + 1, p - buff - 1)
			<< '\''
			<< endl;
			#endif
			buff = p+1;
			return 1;
		}
		
		default:
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
			             "slew returns something weird"
			          << endl;
			#endif
			break;
	}
	
	return -1;
}

void Lx200CommandGotoSelected::print(QTextStream &o) const
{
	o << "Lx200CommandGotoSelected";
}




bool Lx200CommandGetRa::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 5)
		return false;
	
	  // get RA:
	*p++ = '#';
	*p++ = ':';
	*p++ = 'G';
	*p++ = 'R';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandGetRa::readAnswerFromBuffer(const char *&buff,
                                            const char *end)
{
	if (buff < end && *buff=='#')
		buff++; // ignore silly byte
		
	if (end-buff < 8)
		return 0;
	
	bool long_format = true;
	int ra;
	const char *p = buff;
	ra = ((*p++) - '0');
	ra *= 10;
	ra += ((*p++) - '0');
	if (*p++ != ':')
	{
		#ifdef DEBUG4
		*log_file << Now()
		          << "Lx200CommandGetRa::readAnswerFromBuffer: "
		             "error: ':' expected"
		          << endl;
		#endif
		return -1;
	}
	
	ra *=  6; ra += ((*p++) - '0');
	ra *= 10; ra += ((*p++) - '0');
	switch (*p++)
	{
		case ':':
			ra *=  6; ra += ((*p++) - '0');
			ra *= 10; ra += ((*p++) - '0');
			if (end-buff < 9)
				return 0;
			break;
		
		case '.':
			ra *= 10; ra += ((*p++) - '0');
			ra *= 6;
			long_format = false;
			break;
		
		default:
			*log_file << Now()
			          << "Lx200CommandGetRa::readAnswerFromBuffer: "
			             "error: '.' or ':' expected"
			          << endl;
			return -1;
	}
	
	if (*p++ != '#')
	{
		*log_file << Now()
		          << "Lx200CommandGetRa::readAnswerFromBuffer: "
		             "error: '#' expected"
		          << endl;
		return -1;
	}
	
	#ifdef DEBUG4
	*log_file << Now()
	          << "Lx200CommandGetRa::readAnswerFromBuffer: "
	          << "RA = "
	          << qSetPadChar('0')
	          << qSetFieldWidth(2)  << (ra/3600)
	          << qSetFieldWidth(0) << ':'
	          << qSetFieldWidth(2) << ((ra/60)%60)
	          << qSetFieldWidth(0) << ':'
	          << qSetFieldWidth(2) << (ra%60)
	          << qSetFieldWidth(0) << qSetPadChar(' ')
	          << endl;
	#endif
	
	buff = p;
	server.longFormatUsedReceived(long_format);
	server.raReceived((unsigned int)floor(ra * (4294967296.0/86400.0)));
	return 1;
}

void Lx200CommandGetRa::print(QTextStream &o) const
{
	o << "Lx200CommandGetRa";
}




bool Lx200CommandGetDec::writeCommandToBuffer(char *&p,char *end)
{
	if (end-p < 5)
		return false;
	
	  // get Dec:
	*p++ = '#';
	*p++ = ':';
	*p++ = 'G';
	*p++ = 'D';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandGetDec::readAnswerFromBuffer(const char *&buff,
                                             const char *end)
{
	if (buff < end && *buff=='#')
		buff++; // ignore silly byte
		
	if (end-buff < 7)
		return 0;
		
	bool long_format = true;
	int dec;
	const char *p = buff;
	bool sign_dec = false;
	switch (*p++)
	{
		case '+':
			break;
		
		case '-':
			sign_dec = true;
			break;
		
		default:
			#ifdef DEBUG4
			*log_file << Now()
			          << "Lx200CommandGetDec::readAnswerFromBuffer: "
			             "error: '+' or '-' expected"
			          << endl;
			#endif
			return -1;
	}
	
	dec = ((*p++) - '0');
	dec *= 10; dec += ((*p++) - '0');
	if (*p++ != ((char)223))
	{
		*log_file << Now()
		          << "Lx200CommandGetDec::readAnswerFromBuffer: "
		             "error: degree sign expected"
		          << endl;
	}
	
	dec *=  6; dec += ((*p++) - '0');
	dec *= 10; dec += ((*p++) - '0');
	switch (*p++)
	{
		case '#':
			long_format = false;
			dec *= 60;
			break;
		
		case ':':
			if (end-buff < 10)
				return 0;
			dec *=  6; dec += ((*p++) - '0');
			dec *= 10; dec += ((*p++) - '0');
			if (*p++ != '#')
			{
				*log_file << Now()
				          << "Lx200CommandGetDec::readAnswerFromBuffer: "
				             "error: '#' expected"
				          << endl;
				return -1;
			}
			break;
		
		default:
			*log_file << Now()
			          << "Lx200CommandGetDec::readAnswerFromBuffer: "
			             "error: '#' or ':' expected"
			          << endl;
			return -1;
	}
	#ifdef DEBUG4
	*log_file << Now()
	          << "Lx200CommandGetDec::readAnswerFromBuffer: "
	          << "Dec = " << (sign_dec?'-':'+')
	          << qSetPadChar('0')
	          << qSetFieldWidth(2) << (dec/3600)
	          << qSetFieldWidth(0) << ':'
	          << qSetFieldWidth(2) << ((dec/60)%60)
	          << qSetFieldWidth(0) << ':'
	          << qSetFieldWidth(2) << (dec%60)
	          << qSetFieldWidth(0) << qSetPadChar(' ')
	          << endl;
	#endif
	
	if (sign_dec)
		dec = -dec;
	buff = p;
	server.longFormatUsedReceived(long_format);
	server.decReceived((int)floor(dec* (4294967296.0/(360*3600.0))));
	return 1;
}

void Lx200CommandGetDec::print(QTextStream &o) const
{
	o << "Lx200CommandGetDec";
}


