#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "private.h"
#include "tzfile.h"

/*
** A la X3J11, with core dump avoidance.
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
	/*
	** Big enough for
	** ??? ???-2147483648 -2147483648:-2147483648:-2147483648 -2147483648\n
	** (two three-character abbreviations, five eleven-character numbers,
	** three explicit spaces, two explicit colons, a newline,
	** and a trailing ASCII nul.)
	*/
	static char		result[3 * 2 + 5 * 11 + 3 + 2 + 1 + 1];
	register const char *	wn;
	register const char *	mn;

	if (timeptr->tm_wday < 0 || timeptr->tm_wday >= DAYSPERWEEK)
		wn = "???";
	else	wn = wday_name[timeptr->tm_wday];
	if (timeptr->tm_mon < 0 || timeptr->tm_mon >= MONSPERYEAR)
		mn = "???";
	else	mn = mon_name[timeptr->tm_mon];
	/*
	** The X3J11-suggested format is
	**	"%.3s %.3s%3d %02.2d:%02.2d:%02.2d %d\n"
	** but, given the way we've set wn and mn, the .3's are superfluous.
	** Meanwhile, the .2 in 02.2d is ignored, so we drop it.
	*/
	(void) sprintf(result, "%s %s%3d %02d:%02d:%02d %d\n",
		wn, mn,
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
	return result;
}
