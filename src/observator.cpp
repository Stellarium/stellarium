/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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
#include <cstdlib>
#include <clocale>

#include "stellarium.h"
#include "stel_utility.h"
#include "stellastro.h"
#include "init_parser.h"
#include "observator.h"
#include "translator.h"

// Use to remove a boring warning
size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	return strftime(s, max, fmt, tm);
}

Observator::Observator() : longitude(0.), latitude(0.), altitude(0), GMT_shift(0), planet(3)
{
	name = L"Anonymous_Location";
	flag_move_to = 0;
}

Observator::~Observator()
{
}

void Observator::load(const string& file, const string& section)
{
	InitParser conf;
	conf.load(file);

	if (!conf.find_entry(section))
	{
		cerr << "ERROR : Can't find observator section " << section << " in file " << file << endl;
		exit(-1);
	}

	name = _(conf.get_str(section, "name").c_str());

	printf("Loading location: \"%s\", ", StelUtility::wstringToString(name).c_str());

	for (string::size_type i=0;i<name.length();++i)
	{
		if (name[i]=='_') name[i]=' ';
	}

	latitude  = get_dec_angle(conf.get_str(section, "latitude"));
	longitude = get_dec_angle(conf.get_str(section, "longitude"));
	altitude = conf.get_int(section, "altitude");
	set_landscape_name(conf.get_str(section, "landscape_name", "sea"));

	printf("(landscape is: \"%s\")\n", landscape_name.c_str());

	string tzstr = conf.get_str(section, "time_zone");
	if (tzstr == "system_default")
	{
		time_zone_mode = S_TZ_SYSTEM_DEFAULT;
		// Set the program global intern timezones variables from the system locale
		tzset();
	}
	else
	{
		if (tzstr == "gmt+x") // TODO : handle GMT+X timezones form
		{
			time_zone_mode = S_TZ_GMT_SHIFT;
			// GMT_shift = x;
		}
		else
		{
			// We have a custom time zone name
			time_zone_mode = S_TZ_CUSTOM;
			set_custom_tz_name(tzstr);
		}
	}

	time_format = string_to_s_time_format(conf.get_str(section, "time_display_format"));
	date_format = string_to_s_date_format(conf.get_str(section, "date_display_format"));
}

void Observator::set_landscape_name(string s) {

	// need to lower case name because config file parser lowercases section names
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	landscape_name = s;
}

void Observator::save(const string& file, const string& section)
{
	printf("Saving location %s to file %s\n",StelUtility::wstringToString(name).c_str(), file.c_str());

	InitParser conf;
	conf.load(file);

	conf.set_str(section + ":name", StelUtility::wstringToString(name));
	conf.set_str(section + ":latitude", StelUtility::wstringToString(StelUtility::printAngleDMS(latitude, true, true)));
    conf.set_str(section + ":longitude", StelUtility::wstringToString(StelUtility::printAngleDMS(longitude, true, true)));

	conf.set_int(section + ":altitude", altitude);
	conf.set_str(section + ":landscape_name", landscape_name);
	
	if (time_zone_mode == S_TZ_CUSTOM)
	{
		conf.set_str(section + ":time_zone", custom_tz_name);
	}
	if (time_zone_mode == S_TZ_SYSTEM_DEFAULT)
	{
		conf.set_str(section + ":time_zone", "system_default");
	}
	if (time_zone_mode == S_TZ_GMT_SHIFT)
	{
		conf.set_str(section + ":time_zone", "gmt+x");
	}

	conf.set_str(section + ":time_display_format", get_time_format_str());
	conf.set_str(section + ":date_display_format", get_date_format_str());

	conf.save(file);
}

void Observator::set_custom_tz_name(const string& tzname)
{
	custom_tz_name = tzname;
	time_zone_mode = S_TZ_CUSTOM;

	if( custom_tz_name != "")
	{
		// set the TZ environement variable and update c locale stuff
		putenv(strdup((string("TZ=") + custom_tz_name).c_str()));
		tzset();
	}
}

float Observator::get_GMT_shift(double JD, bool _local) const
{
	if (time_zone_mode == S_TZ_GMT_SHIFT) return GMT_shift;
	else return get_GMT_shift_from_system(JD,_local);
}

// Return the time zone name taken from system locale
wstring Observator::get_time_zone_name_from_system(double JD) const
{

	// Windows will crash if date before 1970
	// And no changes on Linux before that year either
	// TODO: ALSO, on Win XP timezone never changes anyway??? 
	if(JD < 2440588 ) JD = 2440588;

	// The timezone name depends on the day because of the summer time
	time_t rawtime = get_time_t_from_julian(JD);

	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	static char timez[255];
	timez[0] = 0;
	my_strftime(timez, 254, "%Z", timeinfo);
	return StelUtility::stringToWstring(timez);
}


// Return the number of hours to add to gmt time to get the local time in day JD
// taking the parameters from system. This takes into account the daylight saving
// time if there is. (positive for Est of GMT)
// TODO : %z in strftime only works on GNU compiler
// Fixed 31-05-2004 Now use the extern variables set by tzset()
float Observator::get_GMT_shift_from_system(double JD, bool _local) const
{
	/* Doesn't seem like MACOSX is a special case... ??? rob
    #if defined( MACOSX ) || defined(WIN32)
	struct tm *timeinfo;
	time_t rawtime; time(&rawtime);
	timeinfo = localtime(&rawtime);
	return (float)timeinfo->tm_gmtoff/3600 + (timeinfo->tm_isdst!=0); 
	#else */

#if !defined(WIN32)

	struct tm * timeinfo;

	if(!_local)
	{
		// JD is UTC
		struct tm rawtime;
		get_tm_from_julian(JD, &rawtime);
		
#ifdef HAVE_TIMEGM
		time_t ltime = timegm(&rawtime);
#else
		// This does not work
		time_t ltime = my_timegm(&rawtime);
#endif
		
		timeinfo = localtime(&ltime);
	} else {
	  time_t rtime;
	  rtime = get_time_t_from_julian(JD);
	  timeinfo = localtime(&rtime);
	}

	static char heure[20];
	heure[0] = '\0';

	my_strftime(heure, 19, "%z", timeinfo);
	//	cout << heure << endl;

	//cout << timezone << endl;
	
	heure[5] = '\0';
	float min = 1.f/60.f * atoi(&heure[3]);
	heure[3] = '\0';
	return min + atoi(heure);
#else
     struct tm *timeinfo;
     time_t rawtime; time(&rawtime);
     timeinfo = localtime(&rawtime);
     return -(float)timezone/3600 + (timeinfo->tm_isdst!=0);
#endif

	 //#endif
	
}

// for platforms without built in timegm function
// taken from the timegm man page
time_t my_timegm (struct tm *tm) {
	time_t ret;
	char *tz;
	char tmpstr[255];
	tz = getenv("TZ");
	putenv("TZ=");
	tzset();
	ret = mktime(tm);
	if (tz)
	{
 snprintf(tmpstr, 255, "TZ=%s", tz);
		putenv(tmpstr);
   }
    else
		putenv("");
	tzset();
	return ret;
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
wstring Observator::get_printable_date_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : my_strftime(date, 254, "%x", &time_utc); break;
		case S_DATE_MMDDYYYY : my_strftime(date, 254, "%m/%d/%Y", &time_utc); break;
		case S_DATE_DDMMYYYY : my_strftime(date, 254, "%d/%m/%Y", &time_utc); break;
		case S_DATE_YYYYMMDD : my_strftime(date, 254, "%Y-%m-%d", &time_utc); break;
	}
	return StelUtility::stringToWstring(date);
}

// Return a string with the UTC time formated according to the time_format variable
// TODO : for some locales (french) the %p returns nothing
wstring Observator::get_printable_time_UTC(double JD) const
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
	return StelUtility::stringToWstring(heure);
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string Observator::get_ISO8601_time_local(double JD) const
{
	struct tm time_local;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char isotime[255];
	my_strftime(isotime, 254, "%Y-%m-%d %H:%M:%S", &time_local);
	return isotime;
}


// Return a string with the local date formated according to the date_format variable
wstring Observator::get_printable_date_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char date[255];
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT : my_strftime(date, 254, "%x", &time_local); break;
		case S_DATE_MMDDYYYY : my_strftime(date, 254, "%m/%d/%Y", &time_local); break;
		case S_DATE_DDMMYYYY : my_strftime(date, 254, "%d/%m/%Y", &time_local); break;
		case S_DATE_YYYYMMDD : my_strftime(date, 254, "%Y-%m-%d", &time_local); break;
	}

	return StelUtility::stringToWstring(date);
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
wstring Observator::get_printable_time_local(double JD) const
{
	struct tm time_local;

	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);

	static char heure[255];
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT : my_strftime(heure, 254, "%X", &time_local); break;
		case S_TIME_24H : my_strftime(heure, 254, "%H:%M:%S", &time_local); break;
		case S_TIME_12H : my_strftime(heure, 254, "%I:%M:%S %p", &time_local); break;
	}
	return StelUtility::stringToWstring(heure);
}

// Convert the time format enum to its associated string and reverse
Observator::S_TIME_FORMAT Observator::string_to_s_time_format(const string& tf) const
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
Observator::S_DATE_FORMAT Observator::string_to_s_date_format(const string& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	if (df == "yyyymmdd") return S_DATE_YYYYMMDD;  // iso8601
	cout << "ERROR : unrecognized date_display_format : " << df << " system_default used." << endl;
	return S_DATE_SYSTEM_DEFAULT;
}

string Observator::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	if (df == S_DATE_YYYYMMDD) return "yyyymmdd";
	cout << "ERROR : unrecognized date_display_format value : " << df << " system_default used." << endl;
	return "system_default";
}


// move gradually to a new observation location
void Observator::move_to(double lat, double lon, double alt, int duration, const wstring& _name)
{
  flag_move_to = 1;

  start_lat = latitude;
  end_lat = lat;

  start_lon = longitude;
  end_lon = lon;

  start_alt = altitude;
  end_alt = alt;

  move_to_coef = 1.0f/duration;
  move_to_mult = 0;

	name = _name;
  //  printf("coef = %f\n", move_to_coef);
}

wstring Observator::get_name(void)
{
	return name;
}

// for moving observator position gradually
// TODO need to work on direction of motion...
void Observator::update(int delta_time) {
  if(flag_move_to) {
    move_to_mult += move_to_coef*delta_time;

    if( move_to_mult >= 1) {
      move_to_mult = 1;
      flag_move_to = 0;
    }

    latitude = start_lat - move_to_mult*(start_lat-end_lat);
    longitude = start_lon - move_to_mult*(start_lon-end_lon);
    altitude = int(start_alt - move_to_mult*(start_alt-end_alt));

  }
}

