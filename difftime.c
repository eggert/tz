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
		** Do the math in whichever of time_t or double is wider
		** (assuming that the wider type has more precision).
		*/
		if (sizeof (time_t) >= sizeof (double))
			return time1 - time0;
		else	return (double) time1 - (double) time0;
	} else if (!TYPE_SIGNED(time_t)) {
		/*
		** time_t is integral and unsigned.
		** The difference of two unsigned values can't overflow
		** if the minuend is greater than or equal to the subtrahend.
		*/
		if (time1 >= time0)
			return time1 - time0;
		else	return -((double) (time0 - time1));
	} else {
		/*
		** time_t is integral and signed.
		** Handle cases where both time1 and time0 have the same sign
		** (meaning that their difference cannot overflow).
		** Handle the more common case (both non-negative) first.
		*/
		if (time1 >= 0 && time0 >= 0)
			return time1 - time0;
		if (time1 < 0 && time0 < 0)
			return time1 - time0;
		/*
		** Punt everything else.
		** Note that we can't use the % operator on time_t values;
		** doing so is invalid on systems where time_t isn't integral.
		*/
		return (double) time1 - (double) time0;
	}
}
