#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
/*
** Based on the UCB version whose ID appears below.
** This is ANSIish only when time is treated identically in all locales and
** when "multibyte character == plain character".
*/
#endif /* !defined NOID */
#endif /* !defined lint */
/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strftime.c	5.4 (Berkeley) 3/14/89";
#endif /* LIBC_SCCS and not lint */

#include "sys/types.h"
#include "time.h"
#include "tzfile.h"

static char *afmt[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};
static char *Afmt[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	"Saturday",
};
static char *bfmt[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec",
};
static char *Bfmt[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December",
};

static char *_add();
static char *_conv();
static char *_fmt();

size_t
strftime(s, maxsize, format, t)
	char *s;
	size_t maxsize;
	char *format;
	struct tm *t;
{
	size_t gsize;

	gsize = maxsize;
	s = _fmt(format, t, &gsize, s);
	if (gsize <= 0)
		return 0;
	*s = '\0';
	return maxsize - gsize;
}

static char *
_fmt(format, t, gsizep, pt)
	char *format;
	struct tm *t;
	size_t *gsizep;
	char *pt;
{
	for (; *format; ++format) {
		if (*format == '%') {
label:
			switch(*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ?
					"?" : Afmt[t->tm_wday], gsizep, pt);
				continue;
			case 'a':
				pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ?
					"?" : afmt[t->tm_wday], gsizep, pt);
				continue;
			case 'B':
				pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ?
					"?" : Bfmt[t->tm_mon], gsizep, pt);
				continue;
			case 'b':
			case 'h':
				pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ?
					"?" : bfmt[t->tm_mon], gsizep, pt);
				continue;
			case 'c':
				pt = _fmt("%D %X", t, gsizep, pt);
				continue;
			case 'C':
				/*
				** %C used to do a...
				**	_fmt("%a %b %e %X %Y", t);
				** ...whereas now POSIX 1003.2 calls for
				** something completely different.
				** (ado, 5/24/93)
				*/
				pt = _conv((t->tm_year + TM_YEAR_BASE) / 100,
					2, '0', gsizep, pt);
				continue;
			case 'D':
			case 'x':
				/*
				** Version 3.0 of strftime from Arnold Robbins
				** (arnold@skeeve.atl.ga.us) does the
				** equivalent of...
				**	_fmt("%a %b %e %Y");
				** ...for %x; since the X3J11 C language
				** standard calls for "date, using locale's
				** date format," anything goes.  Using just
				** numbers (as here) makes Quakers happier.
				** (ado, 5/24/93)
				*/
				pt = _fmt("%m/%d/%y", t, gsizep, pt);
				continue;
			case 'd':
				pt = _conv(t->tm_mday, 2, '0', gsizep, pt);
				continue;
			case 'E':
			case 'O':
				/*
				** POSIX locale extensions, a la
				** Arnold Robbins' strftime version 3.0.
				** The sequences
				**	%Ec %EC %Ex %Ey %EY
				**	%Od %oe %OH %OI %Om %OM
				**	%OS %Ou %OU %OV %Ow %OW %Oy
				** are supposed to provide alternate
				** representations.
				** (ado, 5/24/93)
				*/
				goto label;
			case 'e':
				pt = _conv(t->tm_mday, 2, ' ', gsizep, pt);
				continue;
			case 'H':
				pt = _conv(t->tm_hour, 2, '0', gsizep, pt);
				continue;
			case 'I':
				pt = _conv((t->tm_hour % 12) ?
					(t->tm_hour % 12) : 12,
					2, '0', gsizep, pt);
				continue;
			case 'j':
				pt = _conv(t->tm_yday + 1, 3, '0', gsizep, pt);
				continue;
			case 'k':
				/*
				** This used to be...
				**	_conv(t->tm_hour % 12 ?
				**		t->tm_hour % 12 : 12, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbins'
				** strftime version 3.0.  That is, "%k" and
				** "%l" have been swapped.
				** (ado, 5/24/93)
				*/
				pt = _conv(t->tm_hour, 2, ' ', gsizep, pt);
				continue;
#ifdef KITCHEN_SINK
			case 'K':
				/*
				** After all this time, still unclaimed!
				*/
				pt = _add("kitchen sink", gsizep, pt);
				continue;
#endif /* defined KITCHEN_SINK */
			case 'l':
				/*
				** This used to be...
				**	_conv(t->tm_hour, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbin's
				** strftime version 3.0.  That is, "%k" and
				** "%l" have been swapped.
				** (ado, 5/24/93)
				*/
				pt = _conv((t->tm_hour % 12) ?
					(t->tm_hour % 12) : 12,
					2, ' ', gsizep, pt);
				continue;
			case 'M':
				pt = _conv(t->tm_min, 2, '0', gsizep, pt);
				continue;
			case 'm':
				pt = _conv(t->tm_mon + 1, 2, '0', gsizep, pt);
				continue;
			case 'n':
				pt = _add("\n", gsizep, pt);
				continue;
			case 'p':
				pt = _add(t->tm_hour >= 12 ? "PM" : "AM",
					gsizep, pt);
				continue;
			case 'R':
				pt = _fmt("%H:%M", t, gsizep, pt);
				continue;
			case 'r':
				pt = _fmt("%I:%M:%S %p", t, gsizep, pt);
				continue;
			case 'S':
				pt = _conv(t->tm_sec, 2, '0', gsizep, pt);
				continue;
			case 'T':
			case 'X':
				pt = _fmt("%H:%M:%S", t, gsizep, pt);
				continue;
			case 't':
				pt = _add("\t", gsizep, pt);
				continue;
			case 'U':
				pt = _conv((t->tm_yday + 7 - t->tm_wday) / 7,
					2, '0', gsizep, pt);
				continue;
			case 'u':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "ISO 8601: Weekday as a decimal number
				** [1 (Monday) - 7]"
				** (ado, 5/24/93)
				*/
				pt = _conv((t->tm_wday == 0) ? 7 : t->tm_wday,
					1, '0', gsizep, pt);
				continue;
			case 'V':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "the week number of the year (the first
				** Monday as the first day of week 1) as a
				** decimal number (01-53).  The method for
				** determining the week number is as specified
				** by ISO 8601 (to wit: if the week containing
				** January 1 has four or more days in the new
				** year, then it is week 1, otherwise it is
				** week 53 of the previous year and the next
				** week is week 1)."
				** (ado, 5/24/93)
				*/
				/*
				** XXX--If January 1 falls on a Friday,
				** January 1-3 are part of week 53 of the
				** previous year.  By analogy, if January
				** 1 falls on a Thursday, are December 29-31
				** of the PREVIOUS year part of week 1???
				** (ado 5/24/93)
				/*
				** You are understood not to expect this.
				*/
				{
					int i;

					i = (t->tm_yday + 10 - (t->tm_wday ?
						(t->tm_wday - 1) : 6)) / 7;
					pt = _conv((i == 0) ? 53 : i,
						2, '0', gsizep, pt);
				}
				continue;
			case 'v':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "date as dd-bbb-YYYY"
				** (ado, 5/24/93)
				*/
				pt = _fmt("%e-%b-%Y", t, gsizep, pt);
				continue;
			case 'W':
				pt = _conv((t->tm_yday + 7 -
					(t->tm_wday ?
					(t->tm_wday - 1) : 6)) / 7,
					2, '0', gsizep, pt);
				continue;
			case 'w':
				pt = _conv(t->tm_wday, 1, '0', gsizep, pt);
				continue;
			case 'y':
				pt = _conv((t->tm_year + TM_YEAR_BASE) % 100,
					2, '0', gsizep, pt);
				continue;
			case 'Y':
				pt = _conv(t->tm_year + TM_YEAR_BASE, 4, '0',
					gsizep, pt);
				continue;
			case 'Z':
#ifdef TM_ZONE
				if (t->TM_ZONE)
					pt = _add(t->TM_ZONE, gsizep, pt);
				else
#endif /* defined TM_ZONE */
				if (t->tm_isdst == 0 || t->tm_isdst == 1) {
					extern char *	tzname[2];

					pt = _add(tzname[t->tm_isdst],
						gsizep, pt);
				} else  pt = _add("?", gsizep, pt);
				continue;
			case '%':
			/*
			 * X311J/88-090 (4.12.3.5): if conversion char is
			 * undefined, behavior is undefined.  Print out the
			 * character itself as printf(3) also does.
			 */
			default:
				break;
			}
		}
		if (*gsizep <= 0)
			return pt;
		*pt++ = *format;
		--*gsizep;
	}
	return pt;
}

static char *
_conv(n, width, fill, gsizep, pt)
	int n, width, fill;
	size_t *gsizep;
	char *pt;
{
	char *p, *q, buf[12];
	static char digits[] = "0123456789";

	p = buf + sizeof buf;
	q = (width >= sizeof buf) ? buf : (p - width - 1);
	*--p = '\0';
	*--p = digits[n % 10];
	while (p > buf && (n /= 10) != 0)
		*--p = digits[n % 10];
	while (p > q)
		*--p = fill;
	return _add(p, gsizep, pt);
}

static char *
_add(str, gsizep, pt)
	char *str;
	size_t *gsizep;
	char *pt;
{
	while (*gsizep > 0 && (*pt = *str++) != '\0') {
		++pt;
		--*gsizep;
	}
	return pt;
}
