/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

#include <string>
#include <ctime>
#include "stellarium.h"
#include "stel_utility.h"
#include "stellastro.h"
#include "init_parser.h"
#include "observator.h"

// Use to remove a boring warning
size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	return strftime(s, max, fmt, tm);
}

////////////////////////////////////////////////////////////////////////////////
Observator::Observator() : longitude(0.), latitude(0.), altitude(0), GMT_shift(0), planet(3)
{
	name = "Anonymous_Location";

	// Set the program global intern C timezones variables from the system locale
	tzset();

	// Set the time format global intern C variables from the system locale
	setlocale(LC_TIME, "");
}

Observator::~Observator()
{
}

void Observator::load(const string& file, const string& section)
{
	init_parser conf;
	conf.load(file);

	if (!conf.find_entry(section))
	{
		cout << "ERROR : Can't find observator section " << section << " in file " << file << endl;
		exit(-1);
	}

	name = conf.get_str(section, "name");
	latitude  = get_dec_angle(conf.get_str(section, "latitude"));
    longitude = get_dec_angle(conf.get_str(section, "longitude"));
	altitude = conf.get_int(section, "altitude");
	landscape_name = conf.get_str(section, "landscape_name", "sea");

	string tzmodestr = conf.get_str(section, "time_zone");
	if (tzmodestr == "system_default")
	{
		time_zone_mode = S_TZ_SYSTEM_DEFAULT;
	}
	else
	{
		if (tzmodestr == "custom")
		{
			time_zone_mode = S_TZ_CUSTOM;
		}
		else
		{
			cout << "ERROR : unrecognized timezone mode " << tzmodestr << endl;
			exit(-1);
		}
	}

	string tfstr = conf.get_str(section, "time_display_format");
	if (tzmodestr == "system_default")
	{
		time_format = S_TIME_SYSTEM_DEFAULT;
	}
	else
	{
		cout << "ERROR : only \"system_default\" time_display_format is recognized yet." << endl;
		exit(-1);
	}

	tfstr = conf.get_str(section, "date_display_format");
	if (tzmodestr == "system_default")
	{
		date_format = S_DATE_SYSTEM_DEFAULT;
	}
	else
	{
		cout << "ERROR : only \"system_default\" date_display_format is recognized yet." << endl;
		exit(-1);
	}

}

void Observator::save(const string& file, const string& section)
{
	cout << "Saving location " << name << " to file " << file << endl;

	init_parser conf;
	conf.load(file);

	conf.set_str(section + ":name", name);
	conf.set_str(section + ":latitude", print_angle_dms(latitude));
    conf.set_str(section + ":longitude", print_angle_dms(longitude));
	conf.set_int(section + ":altitude", altitude);
	conf.set_str(section + ":landscape_name", landscape_name);

	if (time_zone_mode == S_TZ_CUSTOM) conf.set_str(section + ":time_zone", "custom");
	else conf.set_str(section + ":time_zone", "system_default");

	conf.set_str(section + ":time_display_format", "system_default");
	conf.set_str(section + ":date_display_format", "system_default");

	conf.save(file);
}

int Observator::get_GMT_shift(double JD) const
{
	if (time_zone_mode == S_TZ_CUSTOM) return GMT_shift;
	else return get_GMT_shift_from_system(JD);
}

// Return the time zone name taken from system locale
string Observator::get_time_zone_name_from_system(double JD) const
{
	// The timezone name depends on the day because of the summer time
	time_t rawtime = get_time_t_from_julian(JD);

	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	static char timez[255];
	timez[0] = 0;
	my_strftime(timez, 254, "%z", timeinfo);
	return timez;
}



// Return the number of hours to add to gmt time to get the local time in day JD
// taking the parameters from system. This takes into account the daylight saving
// time if there is. (positive for Est of GMT)
int Observator::get_GMT_shift_from_system(double JD) const
{
	time_t rawtime = get_time_t_from_julian(JD);

	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	char heure[20];
	heure[0] = 0;
	my_strftime(heure, 19, "%Z", timeinfo);
	heure[3] = '\0';
	return atoi(heure);
}

// OK Verified
string Observator::get_printable_date_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char date[255];
	my_strftime(date, 254, "%x", &time_utc);
	return date;
}

// OK Verified
string Observator::get_printable_time_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char heure[255];
	my_strftime(heure, 254, "%X", &time_utc);
	return heure;
}

string Observator::get_printable_date_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	else
		get_tm_from_julian(JD + GMT_shift, &time_local);

	static char date[255];
	my_strftime(date, 254, "%x", &time_local);
	return date;
}

string Observator::get_printable_time_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	else
		get_tm_from_julian(JD + GMT_shift, &time_local);

	static char heure[255];
	my_strftime(heure, 254, "%X", &time_local);
	return heure;
}
