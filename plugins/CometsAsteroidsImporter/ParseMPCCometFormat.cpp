/*
Code for converting comet orbital elements in MPC's one-line format to Stellarium's ssystem.ini format
Copyright (C) 2010  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//The code has been cut from another file and will not compile unless
//included in a function. The necessary header files are not included, too.

	QString orbitalElementsMPC("0002P         2010 08  6.5102  0.336152  0.848265  186.5242  334.5718   11.7843  20100104  11.5  6.0  2P/Encke                                                 MPC 59600");
	QSettings solarSystemFile(StelFileMgr::getUserDir() + "/data/ssystem.ini", StelIniFormat, this);
	QRegExp mpcParser("^\\s*(\\d{4})?([A-Z])(\\w{7})?\\s+(\\d{4})\\s+(\\d{2})\\s+(\\d{1,2}\\.\\d{4})\\s+(\\d{1,2}\\.\\d{6})\\s+(\\d\\.\\d{6})\\s+(\\d{1,3}\\.\\d{4})\\s+(\\d{1,3}\\.\\d{4})\\s+(\\d{1,3}\\.\\d{4})\\s+((\\d{4})(?:\\d\\d)(\\d\\d))?\\s+(\\d{1,2}\\.\\d)\\s+(\\d{1,2}\\.\\d)\\s+(\\S.{55})\\s+(\\S.*)$");//

	int match = mpcParser.indexIn(orbitalElementsMPC);
	//qDebug() << "RegExp captured:" << match << mpcParser.capturedTexts();

	if (match < 0)
	{
		qWarning() << "No match";
		return;
	}

	QString name = mpcParser.cap(17).trimmed();
	QString sectionName(name);
	sectionName.remove('\\');
	sectionName.remove('/');
	sectionName.remove('#');
	solarSystemFile.beginGroup(sectionName);

	if (mpcParser.cap(1).isEmpty() && mpcParser.cap(3).isEmpty())
	{
		qWarning() << "Comet is missing both comet number AND provisional designation.";
		return;
	}
	solarSystemFile.setValue("name", name);
	solarSystemFile.setValue("parent", "Sun");
	solarSystemFile.setValue("coord_func","comet_orbit");

	solarSystemFile.setValue("lighting", false);
	solarSystemFile.setValue("color", "1.0, 1.0, 1.0");//TODO
	solarSystemFile.setValue("tex_map", "nomap.png");

	bool ok = false;

	int year	= mpcParser.cap(4).toInt();
	int month	= mpcParser.cap(5).toInt();
	double dayFraction	= mpcParser.cap(6).toDouble(&ok);
	int day = (int) dayFraction;
	QDate datePerihelionPassage(year, month, day);
	int fraction = (int) ((dayFraction - day) * 24 * 60 * 60);
	int seconds = fraction % 60; fraction /= 60;
	int minutes = fraction % 60; fraction /= 60;
	int hours = fraction % 24;
	//qDebug() << hours << minutes << seconds << fraction;
	QTime timePerihelionPassage(hours, minutes, seconds, 0);
	QDateTime dtPerihelionPassage(datePerihelionPassage, timePerihelionPassage, Qt::UTC);
	double jdPerihelionPassage = StelUtils::qDateTimeToJd(dtPerihelionPassage);
	solarSystemFile.setValue("orbit_TimeAtPericenter", jdPerihelionPassage);

	double perihelionDistance = mpcParser.cap(7).toDouble(&ok);//AU
	solarSystemFile.setValue("orbit_PericenterDistance", perihelionDistance);

	double eccentricity = mpcParser.cap(8).toDouble(&ok);//degrees
	qDebug() << "excentr:" << eccentricity;
	solarSystemFile.setValue("orbit_Eccentricity", eccentricity);

	double argumentOfPerihelion = mpcParser.cap(9).toDouble(&ok);//J2000.0, degrees
	solarSystemFile.setValue("orbit_ArgOfPericenter", argumentOfPerihelion);

	double longitudeOfTheAscendingNode = mpcParser.cap(10).toDouble(&ok);//J2000.0, degrees
	solarSystemFile.setValue("orbit_AscendingNode", longitudeOfTheAscendingNode);

	double inclination = mpcParser.cap(11).toDouble(&ok);
	qDebug() << "inclunation:" << inclination;
	solarSystemFile.setValue("orbit_Inclination", inclination);

	//Albedo doesn't work at all
	//TODO: Make sure comets don't display albedo
	double absoluteMagnitude = mpcParser.cap(15).toDouble(&ok);
	double radius = 5; //Fictitious
	solarSystemFile.setValue("radius", radius);
	qDebug() << 1329 * pow(10, (absoluteMagnitude/-5));
	double albedo = pow(( (1329 * pow(10, (absoluteMagnitude/-5))) / (2 * radius)), 2);//from http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
	solarSystemFile.setValue("albedo", albedo);

	solarSystemFile.endGroup();
