/* complete rewrite May 2017, Rumen G.Bogdanovski */

#ifndef mach_time_h
#define mach_time_h

#ifdef __MACH__ /* Mac OSX prior Sierra is missing clock_gettime() */
#include <mach/clock.h>
#include <mach/mach.h>
void get_utc_time(struct timespec *ts);
#else
#define get_utc_time(ts) clock_gettime(CLOCK_MONOTONIC, ts)
#endif

#endif
