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
#define TRUE		1
#define FALSE		0
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
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

#define TZ_MAX_TOTAL	(sizeof h + sizeof ats + sizeof types + \
				sizeof ttis + sizeof chars)

static char		isset;

char *			tz_abbr;	/* set by localtime; available to all */

/*
** Not available west of the Rockies. . .
*/

static char *
memcpy(to, from, n)
char *	to;
char *	from;
{
	register int	i;

	for (i = 0; i < n; ++i)
		to[i] = from[i];
	return to;
}

static
tzload(tzname)
register char *	tzname;
{
	register char *	p;
	register int	fid;
	register int	i;
	register int	doaccess;
	char		buf[(TZ_MAX_TOTAL>MAXPATHLEN)?TZ_MAX_TOTAL:MAXPATHLEN];

	if (tzname == 0 && (tzname = TZDEFAULT) == 0)
		return -1;
	doaccess = tzname[0] == '/';
	if (!doaccess) {
		if ((p = TZDIR) == 0)
			return -1;
		if ((strlen(p) + strlen(tzname) + 1) >= sizeof buf)
			return -1;
		(void) strcpy(buf, p);
		(void) strcat(buf, "/");
		(void) strcat(buf, tzname);
		/*
		** Set doaccess if '.' (as in "../") shows up in name.
		*/
		while (*tzname != '\0')
			if (*tzname++ == '.')
				doaccess = TRUE;
		tzname = buf;
	}
	if (doaccess && access(tzname, 4) != 0)
		return -1;
	if ((fid = open(tzname, 0)) == -1)
		return -1;
	p = buf;
	i = read(fid, p, sizeof buf);
	if (close(fid) != 0 || i < sizeof h)
		return -1;
	(void) memcpy((char *) &h, p, sizeof h);
	if (h.tzh_timecnt > TZ_MAX_TIMES ||
		h.tzh_typecnt == 0 || h.tzh_typecnt > TZ_MAX_TYPES ||
		h.tzh_charcnt > TZ_MAX_CHARS)
			return -1;
	if (i < sizeof h +
		h.tzh_timecnt * (sizeof ats[0] + sizeof types[0]) +
		h.tzh_typecnt * sizeof ttis[0] +
		h.tzh_charcnt * sizeof chars[0])
			return -1;
	p += sizeof h;
	if ((i = h.tzh_timecnt) > 0) {
		(void) memcpy((char *) ats, p, i * sizeof ats[0]);
		p += i * sizeof ats[0];
		(void) memcpy((char *) types, p, i * sizeof types[0]);
		p += i * sizeof types[0];
	}
	if ((i = h.tzh_typecnt) > 0) {
		(void) memcpy((char *) ttis, p, i * sizeof ttis[0]);
		p += i * sizeof ttis[0];
	}
	if ((i = h.tzh_charcnt) > 0)
		(void) memcpy((char *) chars, p, i * sizeof chars[0]);
	chars[h.tzh_charcnt] = '\0';	/* ensure '\0'-termination */
	for (i = 0; i < h.tzh_timecnt; ++i)
		if (types[i] > h.tzh_typecnt)
			return -1;
	for (i = 0; i < h.tzh_typecnt; ++i)
		if (ttis[i].tt_abbrind > h.tzh_charcnt)
			return -1;
	return 0;
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
