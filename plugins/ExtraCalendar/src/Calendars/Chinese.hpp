#ifndef __CHINESE_CALENDAR__
#define __CHINESE_CALENDAR__

#include "Gregorian.hpp"

#include <QString>

#define SUPPORT_YEAR_MIN (1901)
#define SUPPORT_YEAR_MAX (2100)
// Only support years from 1901 to 2100 because we have to find values from a table, and the current table we have only supports years between 1901 to 2100

typedef struct ChineseCalendar
{
	bool isleap;
	int day;
	int month;
	int year;
} ChineseCalendar;

QString getChineseCalendarDateString(Gregorian* g);

#endif /* defined(__CHINESE_CALENDAR__) */
