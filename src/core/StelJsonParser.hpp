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

#ifndef _STELJSONPARSER_HPP_
#define _STELJSONPARSER_HPP_

#include <QIODevice>
#include <QVariant>
#include <QByteArray>


//! Qt-style iterator over a JSON array. An actual list is not kept in memory,
//! so only forward iteration is supported and all methods, including the constructor,
//! involve read() calls on the QIODevice. Because of this, do not modify the
//! QIODevice between calls to JsonListIterator methods. Also, the toFront()
//! method has a special function and reset() is provided for convenience. Only
//! peekNext() is guaranteed not to modify the QIODevice.
class JsonListIterator
{
public:
	//! Sets up JsonListIterator to read an array. Swallows all whitespace
	//! up to a beginning '[' character. If '[' is not the first non-whitespace
	//! character encountered, reset() is called and an exception is thrown.
	JsonListIterator(QIODevice* input);
	~JsonListIterator();

	//! Reads and parses the next object from input. Advances QIODevice to
	//! just after the object.
	//! @return the next object from the array
	QVariant next();

	//! Returns true if the next non-whitespace character is not a ']' character.
	bool hasNext() const {return ahasNext;}

private:
	bool ahasNext;
	class StelJsonParserInstance* parser;
};

//! @class StelJsonParser
//! Qt-based simple JSON reader inspired by the one from <a href='http://zoolib.sourceforge.net/'>Zoolib</a>.
//! JSON is JavaScript Object Notation. See http://www.json.org/
/*! <p>The mapping with Qt types is done as following:
@verbatim
JSON            Qt
----          -------
null          QVariant::Invalid
object        QVariantMap (QVariant::Map)
array         QVariantList (QVariant::List)
boolean       QVariant::Bool
string        QVariant::String
number        QVariant::Int or QVariant::Double
@endverbatim */
class StelJsonParser
{
public:
	//! Create a JsonListIterator from the given input device.
	static JsonListIterator initListIterator(QIODevice* in) {return JsonListIterator(in);}

	//! Parse the given input stream.
	static QVariant parse(QIODevice* input);
	static QVariant parse(const QByteArray& input);

	//! Serialize the passed QVariant as JSON into the output QIODevice.
	static void write(const QVariant& jsonObject, QIODevice* output, int indentLevel=0);

	//! Serialize the passed QVariant as JSON in a QByteArray.
	static QByteArray write(const QVariant& jsonObject, int indentLevel=0);
	
	static void registerSerializerForType(int t, void (*func)(const QVariant&, QIODevice*, int)) {otherSerializer.insert(t, func);}
	
private:
	static QHash<int, void (*)(const QVariant&, QIODevice*, int)> otherSerializer;
};

#endif // _STELJSONPARSER_HPP_
