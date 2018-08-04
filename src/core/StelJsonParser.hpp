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

#ifndef STELJSONPARSER_HPP
#define STELJSONPARSER_HPP

#include <QIODevice>
#include <QVariant>
#include <QByteArray>


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
	//! Parse the given input stream.
	static QVariant parse(QIODevice* input);
	static QVariant parse(const QByteArray& input);

	//! Serialize the passed QVariant as JSON into the output QIODevice.
	static void write(const QVariant& jsonObject, QIODevice* output, int indentLevel=0);

	//! Serialize the passed QVariant as JSON in a QByteArray.
	static QByteArray write(const QVariant& jsonObject, int indentLevel=0);
	
	// static void registerSerializerForType(int t, void (*func)(const QVariant&, QIODevice*, int)) {otherSerializer.insert(t, func);}
};

#endif // STELJSONPARSER_HPP
