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

#if STRICTLY_STANDARD_ASCTIME
#define ASCTIME_FMT	"%.3s %.3s%3d %.2d:%.2d:%.2d %ld\n"
#define ASCTIME_FMT_B	ASCTIME_FMT
#else /* !STRICTLY_STANDARD_ASCTIME */
/*
** Some systems only handle "%.2d"; others only handle "%02d";
** "%02.2d" makes (most) everybody happy.
** At least some versions of gcc warn about the %02.2d; ignore the warning.
*/
/*
** All years associated with 32-bit time_t values are exactly four digits long;
** some years associated with 64-bit time_t values are not.
** Vintage programs are coded for years that are always four digits long
** and may assume that the newline always lands in the same place.
** For years that are less than four digits, we pad the output with
** spaces before the newline to get the newline in the traditional place.
*/
#define ASCTIME_FMT	"%.3s %.3s%3d %02.2d:%02.2d:%02.2d %-4ld\n"
/*
** For years that are more than four digits we put extra spaces before the year
** so that code trying to overwrite the newline won't end up overwriting
** a digit within a year and truncating the year (operating on the assumption
** that no output is better than wrong output).
*/
#define ASCTIME_FMT_B	"%.3s %.3s%3d %02.2d:%02.2d:%02.2d     %ld\n"
#endif /* !STRICTLY_STANDARD_ASCTIME */

#define STD_ASCTIME_BUF_SIZE	26
/*
** Big enough for something such as
** ??? ???-2147483648 -2147483648:-2147483648:-2147483648     -2147483648\n
** (two three-character abbreviations, five strings denoting integers,
** seven explicit spaces, two explicit colons, a newline,
** and a trailing ASCII nul).
** The values above are for systems where an int is 32 bits and are provided
** as an example; the define below calculates the maximum for the system at
** hand.
*/
#define MAX_ASCTIME_BUF_SIZE	(2*3+5*INT_STRLEN_MAXIMUM(int)+7+2+1+1)

static char	buf_asctime[MAX_ASCTIME_BUF_SIZE];

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
	long			year;
	char			result[MAX_ASCTIME_BUF_SIZE];

	if ((unsigned) timeptr->tm_wday >= DAYSPERWEEK)
		wn = "???";
	else	wn = wday_name[timeptr->tm_wday];
	if ((unsigned) timeptr->tm_mon >= MONSPERYEAR)
		mn = "???";
	else	mn = mon_name[timeptr->tm_mon];
	year = timeptr->tm_year + (long) TM_YEAR_BASE;
	/*
	** We avoid using snprintf since it's not available on all systems.
	*/
	(void) sprintf(result,
		((year >= -999 && year <= 9999) ? ASCTIME_FMT : ASCTIME_FMT_B),
		wn, mn,
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		year);
	if (strlen(result) < STD_ASCTIME_BUF_SIZE || buf == buf_asctime) {
		(void) strcpy(buf, result);
		return buf;
	} else {
#ifdef EOVERFLOW
		errno = EOVERFLOW;
#else /* !defined EOVERFLOW */
		errno = EINVAL;
#endif /* !defined EOVERFLOW */
		return NULL;
	}
}

/*
** A la ISO/IEC 9945-1, ANSI/IEEE Std 1003.1, 2004 Edition.
*/

char *
asctime(timeptr)
register const struct tm *	timeptr;
{
	return asctime_r(timeptr, buf_asctime);
}
