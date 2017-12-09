/* complete rewrite May 2017, Rumen G.Bogdanovski */
#include <time.h>

#ifdef __MACH__ /* Mac OSX prior Sierra is missing clock_gettime() */
#include <mach/clock.h>
#include <mach/mach.h>
void get_utc_time(struct timespec *ts)
{
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec  = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
}
#endif
