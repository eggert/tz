/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson (arthur_david_olson@nih.gov).
*/

#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "private.h"
#include "tzfile.h"

#define STANDARD_BUFFER_SIZE	26

/*
** A la ISO/IEC 9945-1, ANSI/IEEE Std 1003.1, 2004 Edition.
*/

char *
asctime_r(timeptr, buf)
register const struct tm *	timeptr;
char *				buf;
{
	static const char	wday_name[][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	register const char *	wn;
	register const char *	mn;
	register int		result;

	if (timeptr->tm_wday < 0 || timeptr->tm_wday >= DAYSPERWEEK)
		wn = "???";
	else	wn = wday_name[timeptr->tm_wday];
	if (timeptr->tm_mon < 0 || timeptr->tm_mon >= MONSPERYEAR)
		mn = "???";
	else	mn = mon_name[timeptr->tm_mon];
	/*
	** The format used in the (2004) standard is
	**	"%.3s %.3s%3d %.2d:%.2d:%.2d %d\n"
	** Use "%02d", as it is a bit more portable than "%.2d".
	*/
	result = snprintf(buf, STANDARD_BUFFER_SIZE,
		"%.3s %.3s%3d %02d:%02d:%02d %ld\n",
		wn, mn,
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		timeptr->tm_year + (long) TM_YEAR_BASE);
	if (result < 0 || result >= STANDARD_BUFFER_SIZE) {
		errno = EOVERFLOW;
		return NULL;
	}
	return buf;
}

/*
** A la ISO/IEC 9945-1, ANSI/IEEE Std 1003.1, 2004 Edition,
** with core dump avoidance.
*/

char *
asctime(timeptr)
register const struct tm *	timeptr;
{
	static char		result[STANDARD_BUFFER_SIZE];

	return asctime_r(timeptr, result);
}
