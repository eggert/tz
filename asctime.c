#

/*LINTLIBRARY*/

#include "stdio.h"

#ifndef lint
#ifndef NOID
static char	sccsid[] = "%W%";
#endif /* !NOID */
#endif /* !lint */

#include "time.h"

#ifndef TM_YEAR_BASE
#define TM_YEAR_BASE	1970
#endif /* !TM_YEAR_BASE */

#ifndef USG
extern char *	sprintf();
#endif /* !USG */

/*
** A la X3J11
*/

char *
asctime(timeptr)
register struct tm *	timeptr;
{
	static char	wday_name[7][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char	mon_name[12][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char	result[26];

	(void) sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
	return result;
}
