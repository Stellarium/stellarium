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

#ifndef QTJSONPARSER_H_
#define QTJSONPARSER_H_

#include <QIODevice>
#include <QVariant>

//! Qt-based simple JSON reader inspired by the one from Zoolib http://zoolib.sourceforge.net/
//! JSON is JavaScript Object Notation. See http://www.json.org/
//! The mapping with Qt types is done as following:
//! JSON            Qt
//! ----          -------
//! null          QVariant::Invalid
//! object        QVariantMap
//! array         QVariantList
//! boolean       QVariant::Bool
//! string        QVariant::String
//! number        QVariant::Int or QVariant::Double
class QtJsonParser
{
public:
	//! Parse the given input stream
	QVariant parse(QIODevice& input);
};

#endif /*QTJSONPARSER_H_*/
