#

/*LINTLIBRARY*/

/*
** Should there be any built-in rules other than GMT?
** In particular, should zones such as "EST5" (abbreviation is always "EST",
** GMT offset is always 5 hours) be built in?
*/

#include "tzfile.h"
#include "time.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

extern char *		asctime();
extern struct tm *	gmtime();
extern char *		strcpy();
extern char *		strcat();
extern char *		getenv();

static struct tzhead	h;
static long		ats[TZ_MAX_TIMES];
static unsigned char	types[TZ_MAX_TIMES];
static struct ttinfo	ttis[TZ_MAX_TYPES];
static char		chars[TZ_MAX_CHARS + 1];
static char		isset;

char *			tz_abbr;	/* set by localtime; available to all */

static
tzload(tzname)
register char *	tzname;
{
	register int	fid;
	register int	i;
	char		buf[256];

	fid = -1;
	if (tzname == 0)
		tzname = TZDEFAULT;
	if (tzname[0] != '/') {
		if ((strlen(buf) + strlen(tzname) + 2) > sizeof buf)
			goto oops;
		(void) strcpy(buf, TZDIR);
		(void) strcat(buf, "/");
		(void) strcat(buf, tzname);
		tzname = buf;
	}
	if ((fid = open(tzname, 0)) == -1)
		goto oops;
	if (read(fid, (char *) &h, sizeof h) != sizeof h)
		goto oops;
	if (h.tzh_timecnt > TZ_MAX_TIMES ||
		h.tzh_typecnt == 0 || h.tzh_typecnt > TZ_MAX_TYPES ||
		h.tzh_charcnt > TZ_MAX_CHARS)
			goto oops;
	i = h.tzh_timecnt * sizeof ats[0];
	if (read(fid, (char *) ats, i) != i)
		goto oops;
	i = h.tzh_timecnt * sizeof types[0];
	if (read(fid, (char *) types, i) != i)
		goto oops;
	i = h.tzh_typecnt * sizeof ttis[0];
	if (read(fid, (char *) ttis, i) != i)
		goto oops;
	i = h.tzh_charcnt * sizeof chars[0];
	if (read(fid, (char *) chars, i) != i)
		goto oops;
	chars[h.tzh_charcnt] = '\0';
	i = close(fid);
	fid = -1;
	if (i != 0)
		goto oops;
	for (i = 0; i < h.tzh_timecnt; ++i)
		if (types[i] > h.tzh_typecnt)
			goto oops;
	for (i = 0; i < h.tzh_typecnt; ++i)
		if (ttis[i].tt_abbrind > h.tzh_charcnt)
			goto oops;
	return 0;
oops:
	if (fid >= 0)
		(void) close(fid);
	/*
	** Clobber read-in information in case we're running set-user-id
	** and have been used to read a protected file.
	*/
	{
		struct tzhead	xxxh;

		h = xxxh;
		for (i = 0; i < TZ_MAX_TIMES; ++i) {
			ats[i] = 0;
			types[i] = 0;
		}
		for (i = 0; i < TZ_MAX_TYPES; ++i) {
			struct ttinfo	xxxtt;

			ttis[i] = xxxtt;
		}
		for (i = 0; i < TZ_MAX_CHARS; ++i)
			chars[i] = '\0';
	}
	return -1;
}

/*
** settz("")		Use built-in GMT.
** settz(0)		Use TZDEFAULT.
** settz(otherwise)	Use otherwise.
*/

settz(tzname)
char *	tzname;
{
	register int	answer;

	isset = TRUE;
	if (tzname != 0 && *tzname == '\0')
		answer = 0;			/* Use built-in GMT */
	else {
		if (tzload(tzname) == 0)
			return 0;
		/*
		** If we want to try for local time on errors. . .
		if (tzload((char *) 0) == 0)
			return -1;
		*/
		answer = -1;
	}
	h.tzh_timecnt = 0;
	ttis[0].tt_gmtoff = 0;
	ttis[0].tt_abbrind = 0;
	(void) strcpy(chars, "GMT");
	return answer;
}

struct tm *
newlocaltime(timep)
long *	timep;
{
	register struct ttinfo *	ttip;
	register struct tm *		ct;
	register int			i;
	long				t;

	t = *timep;
	if (!isset)
		(void) settz(getenv("TZ"));
	if (h.tzh_timecnt == 0 || t < ats[0])
		i = 0;
	else {
		for (i = 1; i < h.tzh_timecnt; ++i)
			if (t < ats[i])
				break;
		i = types[i - 1];
	}
	ttip = &ttis[i];
	t += ttip->tt_gmtoff;
	ct = gmtime(&t);
	ct->tm_isdst = ttip->tt_isdst;
	tz_abbr = &chars[ttip->tt_abbrind];
	return ct;
}

char *
newctime(timep)
long *	timep;
{
	return asctime(newlocaltime(timep));
}
