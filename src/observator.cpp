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
#include <clocale>
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

	// Set the program global intern timezones variables from the system locale
	tzset();

	// Set the time format global intern variables from the system locale
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

	cout << "Loading location " << name << " from file " << file << endl;

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

	time_format = string_to_s_time_format(conf.get_str(section, "time_display_format"));
	date_format = string_to_s_date_format(conf.get_str(section, "date_display_format"));

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

	conf.set_str(section + ":time_display_format", get_time_format_str());
	conf.set_str(section + ":date_display_format", get_date_format_str());

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
	my_strftime(timez, 254, "%Z", timeinfo);
	return timez;
}



// Return the number of hours to add to gmt time to get the local time in day JD
// taking the parameters from system. This takes into account the daylight saving
// time if there is. (positive for Est of GMT)
// TODO : %z in strftime only works on GNU compiler
int Observator::get_GMT_shift_from_system(double JD) const
{
	time_t rawtime = get_time_t_from_julian(JD);

	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	static char heure[20];
	heure[0] = 0;
	my_strftime(heure, 19, "%z", timeinfo);
	heure[3] = '\0';
	return atoi(heure);
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string Observator::get_ISO8601_time_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char isotime[255];
	my_strftime(isotime, 254, "%Y-%m-%d %H:%M:%S", &time_utc);
	return isotime;
}

// Return a string with the UTC date formated according to the date_format variable
string Observator::get_printable_date_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : my_strftime(date, 254, "%x", &time_utc); break;
		case S_DATE_MMDDYYYY : my_strftime(date, 254, "%m/%d/%Y", &time_utc); break;
		case S_DATE_DDMMYYYY : my_strftime(date, 254, "%d/%m/%Y", &time_utc); break;
	}
	return date;
}

// Return a string with the UTC time formated according to the time_format variable
// TODO : for some locales (french) the %p returns nothing
string Observator::get_printable_time_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char heure[255];
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT : my_strftime(heure, 254, "%X", &time_utc); break;
		case S_TIME_24H : my_strftime(heure, 254, "%H:%M:%S", &time_utc); break;
		case S_TIME_12H : my_strftime(heure, 254, "%I:%M:%S %p", &time_utc); break;
	}
	return heure;
}

// Return a string with the local date formated according to the date_format variable
string Observator::get_printable_date_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	else
		get_tm_from_julian(JD + GMT_shift, &time_local);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : my_strftime(date, 254, "%x", &time_local); break;
		case S_DATE_MMDDYYYY : my_strftime(date, 254, "%m/%d/%Y", &time_local); break;
		case S_DATE_DDMMYYYY : my_strftime(date, 254, "%d/%m/%Y", &time_local); break;
	}

	return date;
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
string Observator::get_printable_time_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	else
		get_tm_from_julian(JD + GMT_shift, &time_local);

	static char heure[255];
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT : my_strftime(heure, 254, "%X", &time_local); break;
		case S_TIME_24H : my_strftime(heure, 254, "%H:%M:%S", &time_local); break;
		case S_TIME_12H : my_strftime(heure, 254, "%I:%M:%S %p", &time_local); break;
	}
	return heure;
}

// Convert the time format enum to its associated string and reverse
S_TIME_FORMAT Observator::string_to_s_time_format(const string& tf) const
{
	if (tf == "system_default") return S_TIME_SYSTEM_DEFAULT;
	if (tf == "24h") return S_TIME_24H;
	if (tf == "12h") return S_TIME_12H;
	cout << "ERROR : unrecognized time_display_format : " << tf << " system_default used." << endl;
	return S_TIME_SYSTEM_DEFAULT;
}

string Observator::s_time_format_to_string(S_TIME_FORMAT tf) const
{
	if (tf == S_TIME_SYSTEM_DEFAULT) return "system_default";
	if (tf == S_TIME_24H) return "24h";
	if (tf == S_TIME_12H) return "12h";
	cout << "ERROR : unrecognized time_display_format value : " << tf << " system_default used." << endl;
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
S_DATE_FORMAT Observator::string_to_s_date_format(const string& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	cout << "ERROR : unrecognized date_display_format : " << df << " system_default used." << endl;
	return S_DATE_SYSTEM_DEFAULT;
}

string Observator::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	cout << "ERROR : unrecognized date_display_format value : " << df << " system_default used." << endl;
	return "system_default";
}
