#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "stdio.h"
#include "time.h"
#include "tzfile.h"
#include "nonstd.h"

/*
** A la X3J11
*/

char *
asctime(timeptr)
register const struct tm *	timeptr;
{
	static const char	wday_name[DAYSPERWEEK][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[MONSPERYEAR][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char	result[26];

	(void) sprintf(result, "%.3s %.3s%3d %02.2d:%02.2d:%02.2d %d\n",
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
	return result;
}
