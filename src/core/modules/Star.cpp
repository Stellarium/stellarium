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

#include <QDebug>
#include <QString>

const QString STAR_TYPE = QStringLiteral("Star");

QString Star1::getNameI18n(void) const
{
	QStringList starNames;
	StarId star_id = getHip() ? getHip() : 	getGaia();
	starNames << StarMgr::getCommonNameI18n(star_id) << getDesignation();
	starNames.removeAll(QString(""));
	if (starNames.count()>0)
		return starNames.first();
	else
		return QString();
}

// Return modern name only if Skyculture allows this!
QString Star1::getScreenNameI18n(const bool withCommonNameI18n) const
{
	QStringList starNames;
	StarId star_id = getHip() ? getHip() : 	getGaia();
	if (!StarMgr::getDesignationUsage())
	{
		const QString culturalLabel=StarMgr::getCulturalScreenLabel(star_id);
		if (culturalLabel.isEmpty() && withCommonNameI18n)
			starNames << StarMgr::getCommonNameI18n(star_id);
		else
			starNames << culturalLabel;
	}
	if (StarMgr::getFlagSciNames()) // The scientific designations can be used for western sky cultures only
	{
		starNames << StarMgr::getSciDesignation(star_id).split(" - ");
		if (StarMgr::getFlagDblStarsDesignation()) // append the traditional designations of double stars
			starNames << StarMgr::getSciExtraDesignation(star_id).split(" - ");
		if (StarMgr::getFlagVarStarsDesignation()) // append the designations of variable stars (from GCVS)
			starNames << StarMgr::getGcvsDesignation(star_id);
		if (StarMgr::getFlagHIPDesignation()) // append the HIP numbers of stars
			starNames << QString("HIP %1").arg(star_id);
	}
	starNames.removeDuplicates();
	starNames.removeAll(QString(""));
	starNames.removeAll(QString());
	if (starNames.count()>0)
		return starNames.first();
	else
		return QString();
}

QString Star1::getDesignation() const
{
	QStringList starNames;
	StarId star_id = getHip() ? getHip() : 	getGaia();
	starNames << StarMgr::getSciDesignation(star_id).split(" - ");
	starNames << StarMgr::getSciExtraDesignation(star_id).split(" - ");
	starNames << StarMgr::getGcvsDesignation(star_id);
	if (getHip())
		starNames << QString("HIP %1").arg(star_id);
	else
		starNames << QString("Gaia DR3 %1").arg(star_id);
	starNames.removeAll(QString(""));
	starNames.removeAll(QString());
	if (starNames.count()>0)
		return starNames.first();
	else
		return QString();
}

int Star1::hasComponentID(void) const
{
	if (getComponentIds())
		return getComponentIds();

	return 0;
}
