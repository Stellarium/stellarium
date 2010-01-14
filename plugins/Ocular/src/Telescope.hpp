/*
 * Copyright (C) 2009 Timothy Reaves
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

#ifndef TELESCOPE_HPP_
#define TELESCOPE_HPP_

#include <QObject>
#include <QString>
#include <QSqlRecord>

class Telescope : public QObject
{
	Q_OBJECT
public:
	Telescope(QSqlRecord record);
	virtual ~Telescope();
	double getDiameter();
	double getFocalLength();
	int getTelescopeID();
	const QString getName();
	bool isHFlipped();
	bool isVFlipped();
	
private:
	int telescopeID;
	QString name;
	double diameter;
	double focalLength;
	bool hFlipped;
	bool vFlipped;
};
#endif /*TELESCOPE_HPP_*/
