/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 *
 * Thanks go to Nigel Kerr for ideas and testing of BE/LE star repacking
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

#include "Star.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QString>

const QString STAR_TYPE = QStringLiteral("Star");

QString Star1::getNameI18n(void) const
{
	if (getHip())
	{
		const QString commonNameI18 = StarMgr::getCommonName(getHip());
		if (!commonNameI18.isEmpty()) return commonNameI18;
		if (StarMgr::getFlagSciNames())
		{
			const QString sciName = StarMgr::getSciName(getHip());
			if (!sciName.isEmpty()) return sciName;
			const QString varSciName = StarMgr::getGcvsName(getHip());
			if (!varSciName.isEmpty() && varSciName!=sciName) return varSciName;			
			return QString("HIP %1").arg(getHip());
		}
	}
	return QString();
}

QString Star1::getDesignation() const
{
	if (getHip())
	{
		const QString sciName = StarMgr::getSciName(getHip());
		if (!sciName.isEmpty()) return sciName;
		const QString varSciName = StarMgr::getGcvsName(getHip());
		if (!varSciName.isEmpty() && varSciName!=sciName) return varSciName;
		return QString("HIP %1").arg(getHip());
	}
	return QString();
}

int Star1::hasComponentID(void) const
{
	if (getComponentIds())
	{
		return getComponentIds();
	}
	return 0;
}

void Star1::print(void) const
{
	qDebug() << "hip: " << getHip()
		 << ", componentIds: " << getComponentIds()
		 << ", x0: " << getX0()
		 << ", x1: " << getX1()
		 << ", bV: " << (static_cast<unsigned int>(getBV()))
		 << ", mag: " << (static_cast<unsigned int>(getMag()))
		 << ", spInt: " << getSpInt()
		 << ", dx0: " << getDx0()
		 << ", dx1: " << getDx1()
		 << ", plx: " << getPlx();
}

void Star2::print(void) const
{
	qDebug() << "x0: " << getX0()
		 << ", x1: " << getX1()
		 << ", dx0: " << getDx0()
		 << ", dx1: " << getDx1()
		 << ", bV: " << getBV()
		 << ", mag: " << getMag();
}

