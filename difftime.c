/*
** This file is in the public domain, so clarified as of
** June 5, 1996 by Arthur David Olson (arthur_david_olson@nih.gov).
*/

#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "sys/types.h"	/* for time_t */
#include "private.h"	/* for TYPE_INTEGRAL and TYPE_SIGNED */

double
difftime(time1, time0)
const time_t	time1;
const time_t	time0;
{
	if (!TYPE_INTEGRAL(time_t)) {
		/*
		** time_t is floating.
		** We can't apply % to floats.
		** Do the math in whichever of time_t or double is wider.
		*/
		if (sizeof (time_t) >= sizeof (double))
			return time1 - time0;
		else	return (double) time1 - (double) time0;
	} else {
		/*
		** time_t is integral.
		** As elsewhere in the time zone package,
		** use modular arithmetic to avoid overflow.
		*/
		register time_t	lead;
		register time_t	trail;

		lead = time1 / 2 - time0 / 2;
		trail = time1 % 2 - time0 % 2;
		return 2 * ((double) lead) + trail;
	}
}
