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
#include <QJsonDocument>
#include <stdexcept>

void StelJsonParser::write(const QVariant& v, QIODevice* output, int indentLevel)
{
	QByteArray json = write(v, indentLevel);
	output->write(json);
}

QByteArray StelJsonParser::write(const QVariant& jsonObject, int indentLevel)
{
	Q_UNUSED(indentLevel)
	QJsonDocument doc = QJsonDocument::fromVariant(jsonObject);
	return doc.toJson();
}

QVariant StelJsonParser::parse(QIODevice* input)
{
	QByteArray data = input->readAll();
	return parse(data);
}

QVariant StelJsonParser::parse(const QByteArray& aar)
{
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(aar, &error);
	if (error.error != QJsonParseError::NoError)
	{
		throw std::runtime_error((QString("%1 at offset %2").arg(error.errorString()).arg(error.offset)).toLatin1().constData());
	}
	return doc.toVariant();
}
