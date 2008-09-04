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

#include "PlanetLocation.hpp"
#include "StelLocaleMgr.hpp"
#include <QStringList>

// Output the location as a string ready to be stored in the user_location file
QString PlanetLocation::serializeToLine() const
{
}

// Parse a lcoation from a line serialization
PlanetLocation PlanetLocation::createFromLine(const QString& rawline)
{
	PlanetLocation loc;
	const QStringList& splitline = rawline.split("\t");
	loc.name    = splitline[0];
	loc.state   = splitline[1];
	loc.country = StelLocaleMgr::countryCodeToString(splitline[2]);
	if (loc.country.isEmpty())
		loc.country = splitline[2];
					
	loc.role    = splitline[3].at(0);
	loc.population = (int) ( 1000 * splitline[4].toFloat() );

	const QString& lngstring = splitline[6];
	loc.longitude = lngstring.left(lngstring.size() - 1).toDouble();
	if (lngstring.contains("W"))
		loc.longitude=-loc.longitude;
	
	const QString& latstring = splitline[5];
	loc.latitude = latstring.left(latstring.size() - 1).toDouble();
	if (latstring.contains("S"))
		loc.latitude=-loc.latitude;
	
	loc.altitude = (splitline[7]).toInt();
	bool ok;
	loc.bortleScaleIndex = (splitline[8]).toInt(&ok);
	if (ok==false)
		loc.bortleScaleIndex = 2;
		
	// Reserve for TimeZone
	// if (splitline.size()>9) {}
		
	if (splitline.size()>10)
	{
		// Parse planet name
		loc.planetName = splitline[10];
	}
	else
	{
		// Earth by default
		loc.planetName = "Earth";
	}
		
	if (splitline.size()>11)
	{
		// Parse optional associated landscape key
		loc.landscapeKey = splitline[11];
	}
	return loc;
}

