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

#ifndef _QTJSONPARSER_HPP_
#define _QTJSONPARSER_HPP_

#include <QIODevice>
#include <QVariant>
#include <QByteArray>

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
	//! Parse the given input stream
	QVariant parse(QIODevice& input) const;
	
	//! Serialize the passed QVariant as JSON into the output QIODevice
	void write(const QVariant& jsonObject, QIODevice& output, int indentLevel=0) const;
};

#endif // _QTJSONPARSER_HPP_
