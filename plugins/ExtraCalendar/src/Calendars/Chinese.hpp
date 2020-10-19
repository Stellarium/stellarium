#ifndef __CHINESE_CALENDAR__
#define __CHINESE_CALENDAR__

#include "Gregorian.hpp"

#include <QString>

typedef struct ChineseCalendar
{
	bool isleap;
	int day;
	int month;
	int year;
} ChineseCalendar;

QString getChineseCalendarDateString(Gregorian* g);

#endif /* defined(__CHINESE_CALENDAR__) */
