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

#include "Lx200Command.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "common/TelescopeLogging.hpp"

#include <cmath>

#include <QByteArray>

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

void Lx200CommandToggleFormat::print(QDebug &o) const
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

void Lx200CommandStopSlew::print(QDebug &o) const
{
	o << "Lx200CommandStopSlew";
}


bool Lx200CommandSetSelectedRa::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 13) return false;
	  // set object ra:
	*p++ = ':';
	*p++ = 'S';
	*p++ = 'r';
//	*p++ = ' ';  // GZ seems this space is wrong?
	int x = ra;
	p += 8;
	p[-1] = '0' + (x % 10); x /= 10;
	p[-2] = '0' + (x %  6); x /=  6;
	p[-3] = ':';
	p[-4] = '0' + (x % 10); x /= 10;
	p[-5] = '0' + (x %  6); x /=  6;
	p[-6] = ':';
	p[-7] = '0' + (x % 10); x /= 10;
	p[-8] = '0' + static_cast<char>(x);
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
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandSetSelectedRa::readAnswerFromBuffer: ra invalid";
			buff++;
			break;
		
		case '1':
			qCDebug(Telescopes) << server.getLogPrefix()
			                    << "Lx200CommandSetSelectedRa::readAnswerFromBuffer: ra valid";
			buff++;
			break;
		
		default:
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandSetSelectedRa::readAnswerFromBuffer: "
			                         "strange: unexpected char";
			break;
	}
	
	return 1;
}

void Lx200CommandSetSelectedRa::print(QDebug &o) const
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
//	*p++ = ' ';  // GZ seems this space is wrong?
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
	p[-6] = '\xDF'; // = 223, degree symbol. This is always printed as asterisk in the specs, very confusing.
	p[-7] = '0' + (x % 10); x /= 10;
	p[-8] = '0' + static_cast<char>(x);
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
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandSetSelectedDec::readAnswerFromBuffer: dec invalid";
			buff++;
			break;
		
		case '1':
			qCDebug(Telescopes) << server.getLogPrefix()
			                    << "Lx200CommandSetSelectedDec::readAnswerFromBuffer: dec valid";
			buff++;
			break;
		
		default:
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandSetSelectedDec::readAnswerFromBuffer: "
			                         "strange: unexpected char";
			break;
	}
	
	return 1;
}

void Lx200CommandSetSelectedDec::print(QDebug &o) const
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
			qCDebug(Telescopes) << server.getLogPrefix()
			                    << "Lx200CommandGotoSelected::readAnswerFromBuffer: slew ok";
			buff++;
			return 1;
		
		case '1':
		case '2':
		{
			if (p == end)
			{
				// the AutoStar 494 returns just '1', nothing else
				qCWarning(Telescopes).nospace() << server.getLogPrefix()
				                                << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
				                                   "slew failed ("
				                                << static_cast<char>(first_byte)
				                                << "), but no complete answer yet";
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
			qCWarning(Telescopes).nospace() << server.getLogPrefix()
			                                << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
			                                   "slew failed ("
			                                << static_cast<char>(first_byte) << "): "
			                                << QByteArray(buff + 1, static_cast<int>(p - buff - 1));
			buff = p+1;
			return 1;
		}
		
		default:
			qCDebug(Telescopes) << server.getLogPrefix()
			                    << "Lx200CommandGotoSelected::readAnswerFromBuffer: "
			                       "slew returns something weird";
			break;
	}
	
	return -1;
}

void Lx200CommandGotoSelected::print(QDebug &o) const
{
	o << "Lx200CommandGotoSelected";
}

bool Lx200CommandSyncSelected::writeCommandToBuffer(char *&p, char *end)
{
	if (end-p < 4)
		return false;

	  // slew to current object coordinates
	*p++ = ':';
	*p++ = 'C';
	*p++ = 'M';
	*p++ = '#';
	has_been_written_to_buffer = true;
	return true;
}

int Lx200CommandSyncSelected::readAnswerFromBuffer(const char *&buff,
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

	// Unlike ":MS#" (goto), the Meade ":CM#" (sync) command does not return a
	// numeric status. On success it replies with a '#'-terminated string - the
	// name of the object the telescope was synchronised to (e.g. "M31#"), a
	// message such as "Coordinates matched.#", or simply "OK#" as sent by
	// INDIGO's LX200 emulation. Accept any '#'-terminated response as success
	// and consume it. A bare '0' (used by some controllers) is still accepted
	// as a single-byte answer.
	if (first_byte == '0')
	{
		qCDebug(Telescopes) << server.getLogPrefix()
		                    << "Lx200CommandSyncSelected::readAnswerFromBuffer: sync ok";
		buff++;
		return 1;
	}

	for (;;p++)
	{
		if (p >= end)
			return 0; // wait for the rest of the answer
		if (*p == '#')
			break;
	}
	qCDebug(Telescopes) << server.getLogPrefix()
	                    << "Lx200CommandSyncSelected::readAnswerFromBuffer: sync acknowledged:"
	                    << QByteArray(buff, static_cast<int>(p - buff));
	buff = p+1;
	return 1;
}

void Lx200CommandSyncSelected::print(QDebug &o) const
{
	o << "Lx200CommandSyncSelected";
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
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "Lx200CommandGetRa::readAnswerFromBuffer: error: ':' expected";
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
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandGetRa::readAnswerFromBuffer: error: '.' or ':' expected";
			return -1;
	}
	
	if (*p++ != '#')
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "Lx200CommandGetRa::readAnswerFromBuffer: error: '#' expected";
		return -1;
	}

	qCDebug(Telescopes).noquote() << server.getLogPrefix() << "Lx200CommandGetRa::readAnswerFromBuffer: RA ="
	                              << QStringLiteral("%1:%2:%3")
	                                         .arg(ra / 3600, 2, 10, QLatin1Char('0'))
	                                         .arg((ra / 60) % 60, 2, 10, QLatin1Char('0'))
	                                         .arg(ra % 60, 2, 10, QLatin1Char('0'));
	
	buff = p;
	server.longFormatUsedReceived(long_format);
	server.raReceived(static_cast<unsigned int>(std::floor(ra * (4294967296.0/86400.0))));
	return 1;
}

void Lx200CommandGetRa::print(QDebug &o) const
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
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandGetDec::readAnswerFromBuffer: "
			                         "error: '+' or '-' expected";
			return -1;
	}
	
	dec = ((*p++) - '0');
	dec *= 10; dec += ((*p++) - '0');
	if (*p++ != (static_cast<char>(223)))
	{
		qCWarning(Telescopes) << server.getLogPrefix()
		                      << "Lx200CommandGetDec::readAnswerFromBuffer: "
		                         "error: degree sign expected"; // not a star, really 0xDF
	}
	
	dec *=  6; dec += ((*p++) - '0');
	dec *= 10; dec += ((*p++) - '0');
	switch (*p++)
	{
		case '#':
			long_format = false;
			dec *= 60;
			break;

		// High-precision declination is "sDD*MM'SS#": the canonical Meade
		// LX200 separator between arcminutes and arcseconds is an apostrophe
		// (as sent by real mounts and by INDIGO's LX200 emulation). Some
		// implementations use a colon instead, so accept both.
		case ':':
		case '\'':
			if (end-buff < 10)
				return 0;
			dec *=  6; dec += ((*p++) - '0');
			dec *= 10; dec += ((*p++) - '0');
			if (*p++ != '#')
			{
				qCWarning(Telescopes) << server.getLogPrefix()
				                      << "Lx200CommandGetDec::readAnswerFromBuffer: "
				                         "error: '#' expected";
				return -1;
			}
			break;

		default:
			qCWarning(Telescopes) << server.getLogPrefix()
			                      << "Lx200CommandGetDec::readAnswerFromBuffer: "
			                         "error: '#', ':' or ''' expected";
			return -1;
	}
	qCDebug(Telescopes).nospace().noquote() << server.getLogPrefix() << "Lx200CommandGetDec::readAnswerFromBuffer:"
	                                        << "Dec = " << (sign_dec ? '-' : '+')
	                                        << QStringLiteral("%1:%2:%3")
	                                                   .arg(dec / 3600, 2, 10, QLatin1Char('0'))
	                                                   .arg((dec / 60) % 60, 2, 10, QLatin1Char('0'))
	                                                   .arg(dec % 60, 2, 10, QLatin1Char('0'));
	
	if (sign_dec)
		dec = -dec;
	buff = p;
	server.longFormatUsedReceived(long_format);
	server.decReceived(static_cast<unsigned int>(std::floor(dec* (4294967296.0/(360*3600.0)))));
	return 1;
}

void Lx200CommandGetDec::print(QDebug &o) const
{
	o << "Lx200CommandGetDec";
}


