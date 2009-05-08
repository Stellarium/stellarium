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

#include "StelJsonParser.hpp"
#include <QDebug>
#include <stdexcept>

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
	Q_ASSERT(0);
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
	if (s=="null")
		return QVariant();
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
QVariant StelJsonParser::parse(QIODevice& input)
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

// Serialize the passed QVariant as JSON into the output QIODevice
void StelJsonParser::write(const QVariant& v, QIODevice& output, int indentLevel)
{
	switch (v.type())
	{
		case QVariant::Bool:
			output.write(v.toBool()==true ? "true" : "false");
			break;
		case QVariant::Invalid:
			output.write("null");
			break;
		case QVariant::String:
		{
			QString s(v.toString());
			s.replace('\"', "\\\"");
			//s.replace('\\', "\\\\");
			//s.replace('/', "\\/");
			s.replace('\b', "\\b");
			s.replace('\n', "\\n");
			s.replace('\f', "\\f");
			s.replace('\r', "\\r");
			s.replace('\t', "\\t");
			output.write(QString("\"%1\"").arg(s).toUtf8());
			break;
		}
		case QVariant::Int:
		case QVariant::Double:
			output.write(v.toString().toUtf8());
			break;
		case QVariant::List:
		{
			output.putChar('[');
			const QVariantList& l = v.toList();
			for (int i=0;i<l.size();++i)
			{
				// Break line if we start an JSON Object for nice looking
				if (l.at(i).type()==QVariant::Map)
					output.putChar('\n');
				write(l.at(i), output, indentLevel);
				if (i!=l.size()-1)
					output.write(", ");
			}
			output.putChar(']');
			break;
		}
		case QVariant::Map:
		{
			const QByteArray prepend(indentLevel, '\t');
			output.write(prepend);
			output.write("{\n");
			++indentLevel;
			const QVariantMap& m = v.toMap();
			int j =0;
			for (QVariantMap::ConstIterator i=m.begin();i!=m.end();++i)
			{
				output.write(prepend);
				output.write("\t\"");
				output.write(i.key().toUtf8());
				output.write("\": ");
				// Break line if we start an JSON Object for nice looking
				if (i.value().type()==QVariant::Map)
					output.putChar('\n');
				write(i.value(), output, indentLevel);
				if (++j!=m.size())
					output.putChar(',');
				output.putChar('\n');
			}
			output.write(prepend);
			output.write("}");
			--indentLevel;
			break;
		}
		default:
			qWarning() << "Cannot serialize QVariant of type " << v.typeName() << " in JSON";
			break;
	}
}


JsonListIterator::JsonListIterator(QIODevice& input) : input(input), startPos(input.pos())
{
	skipJson(input);
	if (!tryReadChar(input, '['))
	{
		reset();
		throw std::runtime_error("Expected '[' to start a list iterator");
	}
}

QVariant JsonListIterator::next() const
{
	skipJson(input);
	tryReadChar(input, ',');
	QVariant ret = StelJsonParser::parse(input);
	return ret;
}

bool JsonListIterator::hasNext()
{
	skipJson(input);
	return !tryReadChar(input, ']');
}

QVariant JsonListIterator::peekNext() const
{
	qint64 pos = input.pos();
	QVariant ret = next();
	input.seek(pos);
	return ret;
}

bool JsonListIterator::reset()
{
	return input.seek(startPos);
}

void JsonListIterator::toBack()
{
	while(hasNext())
		next();
	char c;
	input.getChar(&c);
}
void JsonListIterator::toFront()
{
	reset();
	if(!tryReadChar(input, '['))
		throw std::runtime_error("Expected '[' to start a list iterator");
	char c;
	input.getChar(&c);
}
