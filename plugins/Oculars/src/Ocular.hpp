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

#ifndef OCULAR_HPP_
#define OCULAR_HPP_

#include <QObject>
#include <QString>
#include <QSqlRecord>

class Telescope;

class Ocular : public QObject
{
	Q_OBJECT
public:
	Ocular(QSqlRecord record);
	virtual ~Ocular();
	const QString getName();
	int getOcularID();
	double getAppearentFOV();
	double getEffectiveFocalLength();
	double getFieldStop();
	
	double getAcutalFOV(Telescope *telescope);
	double getMagnification(Telescope *telescope);
	double getExitCircle(Telescope *telescope);
		
private:
	int ocularID;
	QString name;
	double appearentFOV;
	double effectiveFocalLength;
	double fieldStop;
};
#endif /*OCULAR_HPP_*/
