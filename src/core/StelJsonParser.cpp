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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelJsonParser.hpp"
#include <QDebug>
#include <QBuffer>
#include <QDateTime>
#include <stdexcept>

class StelJsonParserInstance
{
public:
	StelJsonParserInstance(QIODevice* ain) : input(ain), hasNextChar(false)
	{
		cur = buffer;
		last = cur;
	}
	inline void skipJson();
	inline bool tryReadChar(char c);
	inline bool skipAndConsumeChar(char r);
	QByteArray readString();
	QVariant readOther();
	QVariant parse();

private:
	QIODevice* input;
	char nextChar;
	bool hasNextChar;

#define BUFSIZESTATIC 65536
	char buffer[BUFSIZESTATIC];
	char* last;
	char* cur;

	inline bool getNextFromBuffer(char *c)
	{
		if (cur==last)
		{
			if (cur==buffer-1) // End of file
				return false;

			last = buffer + input->read(buffer, BUFSIZESTATIC);
			if (last==buffer)
			{
				cur = buffer-1;
				last = cur;
				return false;
			}
			cur = buffer;
		}
		*c = *cur;
		++cur;
		return true;
	}

	inline bool getChar(char* c)
	{
		if (hasNextChar)
		{
			hasNextChar=false;
			*c = nextChar;
			return true;
		}
		return getNextFromBuffer(c);
	}

	inline void ungetChar(char c)
	{
		nextChar=c;
		hasNextChar=true;
	}

	inline void skipLine()
	{
		if (hasNextChar && nextChar=='\n')
		{
			hasNextChar=false;
			return;
		}
		hasNextChar=false;
		char c;
		while (getNextFromBuffer(&c) && c!='\n')
		{}
	}

	inline bool atEnd()
	{
		return cur==buffer-1;
	}
};

void StelJsonParserInstance::skipJson()
{
	// There is a weakness in this code -- it will cause any standalone '/' to be absorbed.
	char c;
	while (getChar(&c))
	{
		switch (c)
		{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				break;
			case '/':
				{
					if (!getChar(&c))
						return;

					if (c=='/')
						skipLine();
					else
					{
						// We have a problem, we removed a '/'.. This should never happen with properly formatted JSON though
						throw std::runtime_error(qPrintable(QString("Unexpected '/%1' in the JSON content").arg(c)));
					}
				}
				break;
			default:
				ungetChar(c);
				return;
		}
	}
}


bool StelJsonParserInstance::tryReadChar(char c)
{
	char r;
	if (!getChar(&r))
		return false;

	if (r == c)
		return true;

	ungetChar(r);
	return false;
}

bool StelJsonParserInstance::skipAndConsumeChar(char r)
{
	char c;
	while (getChar(&c))
	{
		switch (c)
		{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				break;
			case '/':
				{
					if (!getChar(&c))
						throw std::runtime_error(qPrintable(QString("Unexpected '/%1' in the JSON content").arg(c)));

					if (c=='/')
						skipLine();
					else
					{
						// We have a problem, we removed a '/'.. This should never happen with properly formatted JSON though
						throw std::runtime_error(qPrintable(QString("Unexpected '/%1' in the JSON content").arg(c)));
					}
				}
				break;
			default:
				if (r==c)
					return true;
				ungetChar(c);
				return false;
		}
	}
	return false;
}

// Read a string without the initial "
QByteArray StelJsonParserInstance::readString()
{
	QByteArray name;
	char c;
	while (getChar(&c))
	{
		switch (c)
		{
			case '"':
				return name;
			case '\\':
			{
				getChar(&c);
				if (c=='b') c='\b';
				if (c=='f') c='\f';
				if (c=='n') c='\n';
				if (c=='r') c='\r';
				if (c=='t') c='\t';
				if (c=='u') {qWarning() << "don't support \\uxxxx char"; continue;}
			}
			default:
				name+=c;
		}
	}
	if (atEnd())
		throw std::runtime_error(qPrintable(QString("End of file before end of string: "+name)));
	throw std::runtime_error(qPrintable(QString("Read error before end of string: "+name)));
	return "";
}

QVariant StelJsonParserInstance::readOther()
{
	QByteArray str;
	char c;
	while (getChar(&c))
	{
		if (c==' ' || c==',' || c=='\n' || c=='\r' || c==']' || c=='\t' || c=='}')
		{
			ungetChar(c);
			break;
		}
		str+=c;
	}
	bool ok;
	const int i = str.toInt(&ok);
	if (ok)
		return i;
	const double d = str.toDouble(&ok);
	if (ok)
		return d;
	if (str=="true")
		return QVariant(true);
	if (str=="false")
		return QVariant(false);
	if (str=="null")
		return QVariant();
	QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
	if (dt.isValid())
		return QVariant(dt);

	throw std::runtime_error(qPrintable(QString("Invalid JSON value: \"")+str+"\""));
}

// Parse the given input stream
QVariant StelJsonParserInstance::parse()
{
	skipJson();

	char r;
	if (!getChar(&r))
		return QVariant();

	switch (r)
	{
		case '{':
		{
			// We've got an object (a tuple)
			QVariantMap map;
			if (skipAndConsumeChar('}'))
				return map;
			for (;;)
			{
				if (!skipAndConsumeChar('\"'))
				{
					char cc=0;
					getChar(&cc);
					throw std::runtime_error(qPrintable(QString("Expected '\"' at beginning of string, found: '%1' (ASCII %2)").arg(cc).arg((int)(cc))));
				}
				const QByteArray& ar = readString();
				const QString& key = QString::fromUtf8(ar.constData(), ar.size());
				if (!skipAndConsumeChar(':'))
					throw std::runtime_error(qPrintable(QString("Expected ':' after a member name: ")+key));

				skipJson();
				map.insert(key, parse());
				if (!skipAndConsumeChar(','))
					break;
			}
			if (!skipAndConsumeChar('}'))
				throw std::runtime_error("Expected '}' to close an object");
			return map;
		}
		case '[':
		{
			// We've got an array (a vector)
			QVariantList list;
			if (skipAndConsumeChar(']'))
				return list;

			for (;;)
			{
				list.append(parse());
				if (!skipAndConsumeChar(','))
					break;
			}

			if (!skipAndConsumeChar(']'))
				throw std::runtime_error("Expected ']' to close an array");

			return list;
		}
		case '\"':
		{
			// We've got a string
			const QByteArray& ar = readString();
			return QString::fromUtf8(ar.constData(), ar.size());
		}
		default:
		{
			ungetChar(r);
			return readOther();
		}
	}
}

QHash<int, void (*)(const QVariant&, QIODevice*, int)> StelJsonParser::otherSerializer;

// Serialize the passed QVariant as JSON into the output QIODevice
void StelJsonParser::write(const QVariant& v, QIODevice* output, int indentLevel)
{
	switch (v.type())
	{
		case QVariant::Bool:
			output->write(v.toBool()==true ? "true" : "false");
			break;
		case QVariant::Invalid:
			output->write("null");
			break;
		case QVariant::ByteArray:
		{
			QByteArray s(v.toByteArray());
			s.replace('\\', "\\\\");
			s.replace('\"', "\\\"");
			s.replace('\b', "\\b");
			s.replace('\n', "\\n");
			s.replace('\f', "\\f");
			s.replace('\r', "\\r");
			s.replace('\t', "\\t");
			output->write("\"" + s + "\"");
			break;
		}
		case QVariant::String:
		{
			QString s(v.toString());
			s.replace('\\', "\\\\");
			s.replace('\"', "\\\"");
			s.replace('\b', "\\b");
			s.replace('\n', "\\n");
			s.replace('\f', "\\f");
			s.replace('\r', "\\r");
			s.replace('\t', "\\t");
			output->write(QString("\"%1\"").arg(s).toUtf8());
			break;
		}
		case QVariant::Int:
		case QVariant::Double:
			output->write(v.toString().toUtf8());
			break;
		case QVariant::DateTime:
		{
			output->write(v.toDateTime().toString(Qt::ISODate).toUtf8());
			break;
		}
		case QVariant::List:
		{
			output->putChar('[');
			const QVariantList& l = v.toList();
			for (int i=0;i<l.size();++i)
			{
				// Break line if we start an JSON Object for nice looking
				if (l.at(i).type()==QVariant::Map)
					output->putChar('\n');
				write(l.at(i), output, indentLevel);
				if (i!=l.size()-1)
					output->write(", ");
			}
			output->putChar(']');
			break;
		}
		case QVariant::Map:
		{
			const QByteArray prepend(indentLevel, '\t');
			output->write(prepend);
			output->write("{\n");
			++indentLevel;
			const QVariantMap& m = v.toMap();
			int j =0;
			for (QVariantMap::ConstIterator i=m.constBegin();i!=m.constEnd();++i)
			{
				output->write(prepend);
				output->write("\t\"");
				output->write(i.key().toUtf8());
				output->write("\": ");
				// Break line if we start an JSON Object for nice looking
				if (i.value().type()==QVariant::Map)
					output->putChar('\n');
				write(i.value(), output, indentLevel);
				if (++j!=m.size())
					output->putChar(',');
				output->putChar('\n');
			}
			output->write(prepend);
			output->write("}");
			--indentLevel;
			break;
		}
		default:
			QHash<int, void (*)(const QVariant&, QIODevice*, int)>::ConstIterator iter = otherSerializer.find(v.userType());
			if (iter!=otherSerializer.constEnd())
			{
				iter.value()(v, output, indentLevel);
			}
			else
				output->write("null");
			//qDebug() << v.type() << v.userType() << v.typeName();
			//qWarning() << "Cannot serialize QVariant of type " << v.typeName() << " in JSON";
			break;
	}
}

QByteArray StelJsonParser::write(const QVariant& jsonObject, int indentLevel)
{
	QByteArray ar;
	QBuffer buf(&ar);
	buf.open(QIODevice::WriteOnly);
	StelJsonParser::write(jsonObject, &buf, indentLevel);
	buf.close();
	return ar;
}

QVariant StelJsonParser::parse(QIODevice* input)
{
	StelJsonParserInstance parser(input);
	return parser.parse();
}

QVariant StelJsonParser::parse(const QByteArray& aar)
{
	QByteArray ar = aar;
	QBuffer buf(&ar);
	buf.open(QIODevice::ReadOnly);
	QVariant v = StelJsonParser::parse(&buf);
	buf.close();
	return v;
}

JsonListIterator::JsonListIterator(QIODevice* input)
{
	parser = new StelJsonParserInstance(input);
	if (!parser->skipAndConsumeChar('['))
	{
		throw std::runtime_error("Expected '[' to start a list iterator");
	}
	ahasNext = !parser->skipAndConsumeChar(']');
}

JsonListIterator::~JsonListIterator()
{
	Q_ASSERT(parser);
	delete parser;
	parser=NULL;
}

QVariant JsonListIterator::next()
{
	QVariant ret = parser->parse();
	ahasNext = parser->skipAndConsumeChar(',');
	if (!ahasNext)
	{
		if (!parser->skipAndConsumeChar(']'))
			throw std::runtime_error("Expected ']' to end a list iterator");
	}
	return ret;
}
