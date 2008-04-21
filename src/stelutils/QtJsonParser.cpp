/*
 * Copyright (C) 2008 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "QtJsonParser.hpp"
#include <QDebug>
#include <stdexcept>
#include <cassert>

void skipJson(QIODevice& input)
{
	// There is a weakness in this code -- it will cause any standalone '/' to be absorbed.
	char c;
	while (input.getChar(&c))
	{
		if (QChar(c).isSpace() || c=='\n')
		{
			continue;
		}
		else
		{
			if (c!='/')
			{
				input.ungetChar(c);
				return;
			}

			if (!input.getChar(&c))
				return;

			if (c=='/')
			{
				input.readLine();
			}
			else
			{
				// We have a problem, we removed an '/'..
				qWarning() << "I removed a '/' while parsing JSON file.";
				input.ungetChar(c);
				return;
			}
		}
	}
}

bool tryReadChar(QIODevice& input, char c)
{
	char r;
	if (!input.getChar(&r))
		return false;

	if (r == c)
		return true;
		
	input.ungetChar(r);
	return false;
}

QString readString(QIODevice& input)
{
	QByteArray name;
	char c;
	input.getChar(&c);
	if (c!='\"')
		throw std::runtime_error("Expected '\"' at beginning of string");
	for (;;)
	{
		input.getChar(&c);
		if (c=='\\')
		{
			input.getChar(&c);
	// 			case '\"': break;
	// 			case '\\': break;
	// 			case '/': break;
			if (c=='b') c='\b';
			if (c=='f') c='\f'; break;
			if (c=='n') c='\n'; break;
			if (c=='r') c='\r'; break;
			if (c=='t') c='\t'; break;
			if (c=='u') qWarning() << "don't support \\uxxxx char"; break;
		}
		if (c=='\"')
			return QString(name);
		name+=c;
		if (input.atEnd())
			throw std::runtime_error(qPrintable(QString("End of file before end of string: "+name)));
	}
	assert(0);
	return "";
}

QVariant readOther(QIODevice& input)
{
	QByteArray str;
	char c;
	while (input.getChar(&c))
	{
		if (QChar(c).isSpace() || c==']' || c=='}' || c==',')
		{
			input.ungetChar(c);
			break;
		}
		str+=c;
	}
	QTextStream ts(&str);
	QString s;
	ts >> s;
	if (s=="true")
		return QVariant(true);
	if (s=="false")
		return QVariant(false);
	QVariant v(s);
	bool ok=false;
	int i = v.toInt(&ok);
	if (ok) return i;
	double d = v.toDouble(&ok);
	if (ok)
		return d;
	return v;
}

// Parse the given input stream
QVariant QtJsonParser::parse(QIODevice& input)
{
	skipJson(input);
	
	if (tryReadChar(input, '{'))
	{
		// We've got an object (a tuple)
		QVariantMap map;
		skipJson(input);
		if (tryReadChar(input, '}'))
			return map;
		for (;;)
		{
			skipJson(input);
			QString key = readString(input);
			skipJson(input);
			if (!tryReadChar(input, ':'))
				throw std::runtime_error(qPrintable(QString("Expected ':' after a member name: ")+key));
			
			skipJson(input);
			map.insert(key, parse(input));
			skipJson(input);
		
			if (!tryReadChar(input, ','))
				break;
		}
	
		skipJson(input);
	
		if (!tryReadChar(input, '}'))
			throw std::runtime_error("Expected '}' to close an object");
		return map;
	}
	else if (tryReadChar(input, '['))
	{
		// We've got an array (a vector)
		QVariantList list;
		skipJson(input);
		if (tryReadChar(input, ']'))
			return list;
		
		for (;;)
		{
			list.append(parse(input));
			skipJson(input);
			if (!tryReadChar(input, ','))
				break;
		}
	
		skipJson(input);
	
		if (!tryReadChar(input, ']'))
			throw std::runtime_error("Expected ']' to close an array");
		
		return list;
	}
	else if (tryReadChar(input, '\"'))
	{
		// We've got a string
		input.ungetChar('\"');
		return readString(input);
	}
	return readOther(input);
}
