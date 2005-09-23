/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _OBSERVATOR_H_
#define _OBSERVATOR_H_

enum S_TIME_FORMAT
{
	S_TIME_24H,
	S_TIME_12H,
	S_TIME_SYSTEM_DEFAULT
};

enum S_DATE_FORMAT
{
	S_DATE_MMDDYYYY,
	S_DATE_DDMMYYYY,
	S_DATE_SYSTEM_DEFAULT,
	S_DATE_YYYYMMDD
};

enum S_TZ_FORMAT
{
	S_TZ_CUSTOM,
	S_TZ_GMT_SHIFT,
	S_TZ_SYSTEM_DEFAULT
};

class Observator
{
public:
	Observator();
	~Observator();

	void save(const string& file, const string& section);
	void load(const string& file, const string& section);

	string get_printable_date_UTC(double JD) const;
	string get_printable_date_local(double JD) const;
	string get_printable_time_UTC(double JD) const;
	string get_printable_time_local(double JD) const;

	// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
	string get_ISO8601_time_UTC(double JD) const;
	string get_ISO8601_time_local(double JD) const;

	// Return the time zone name taken from system locale
	string get_time_zone_name_from_system(double JD) const;

	void set_GMT_shift(int t) {GMT_shift=t;}
	float get_GMT_shift(double JD = 0, bool _local=0) const;

	void set_custom_tz_name(const string& tzname);
	string get_custom_tz_name(void) const {return custom_tz_name;}

	S_TZ_FORMAT get_tz_format(void) const {return time_zone_mode;}

	string get_name(void);
	void set_latitude(double l) {latitude=l;}
	double get_latitude(void) const {return latitude;}
	void set_longitude(double l) {longitude=l;}
	double get_longitude(void) const {return longitude;}
	void set_altitude(int a) {altitude=a;}
	int get_altitude(void) const {return altitude;}
	void set_landscape_name(string s);
	string get_landscape_name(void) const {return landscape_name;}

	string get_time_format_str(void) const {return s_time_format_to_string(time_format);}
	void set_time_format_str(const string& tf) {time_format=string_to_s_time_format(tf);}
	string get_date_format_str(void) const {return s_date_format_to_string(date_format);}
	void set_date_format_str(const string& df) {date_format=string_to_s_date_format(df);}

	void move_to(double lat, double lon, double alt, int duration, const string& _name);  // duration in ms
	void update(int delta_time);  // for moving observing position 

private:
	string name;			// Position name
	double longitude;		// Longitude in degree
	double latitude;		// Latitude in degree
	int altitude;			// Altitude in meter
	string landscape_name;
	S_TIME_FORMAT time_format;
	S_DATE_FORMAT date_format;
	S_TZ_FORMAT time_zone_mode;		// Can be the system default or a user defined value
	string custom_tz_name;			// Something like "Europe/Paris"
	float GMT_shift;				// Time shift between GMT time and local time in hour. (positive for Est of GMT)

	unsigned int planet;	// Planet number : 0 floating, 1 Mercure - 9 pluton

	// Return the number of hours to add to gmt time to get the local time at tim JD
	// taking the parameters from system. This takes into account the daylight saving
	// time if there is. (positive for Est of GMT)
	float get_GMT_shift_from_system(double JD, bool _local=0) const;

	// Convert the time format enum to its associated string and reverse
	S_TIME_FORMAT string_to_s_time_format(const string&) const;
	string s_time_format_to_string(S_TIME_FORMAT) const;

	// Convert the date format enum to its associated string and reverse
	S_DATE_FORMAT Observator::string_to_s_date_format(const string& df) const;
	string Observator::s_date_format_to_string(S_DATE_FORMAT df) const;

	// for changing position
	bool flag_move_to;
	double start_lat, end_lat;
	double start_lon, end_lon;
	double start_alt, end_alt;
	float move_to_coef, move_to_mult;

};

time_t my_timegm (struct tm *tm);


#endif // _OBSERVATOR_H_
