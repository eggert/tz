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

struct state {
	struct tzhead	h;
	long		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[TZ_MAX_CHARS + 1];
};

static struct state	s;

static char		isset;

char *			tz_abbr;	/* set by localtime; available to all */

#define	GETSHORT(val, p) { \
	register int shortval; \
	shortval = *p++; \
	shortval = (shortval << 8) | *p++; \
	val = shortval; \
	}

#define GETLONG(val, p) { \
	register long longval; \
	longval = *p++; \
	longval = (longval << 8) | *p++; \
	longval = (longval << 8) | *p++; \
	longval = (longval << 8) | *p++; \
	val = longval; \
	}
	
static
tzload(tzname, sp)
register char *		tzname;
register struct state *	sp;
{
	register int	i;
	register int	fid;

	if (tzname == 0 && (tzname = TZDEFAULT) == 0)
		return -1;
	{
		register char *	p;
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		doaccess = tzname[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(tzname) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, tzname);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			while (*tzname != '\0')
				if (*tzname++ == '.')
					doaccess = TRUE;
			tzname = fullname;
		}
		if (doaccess && access(tzname, 4) != 0)
			return -1;
		if ((fid = open(tzname, 0)) == -1)
			return -1;
	}
	{
		register unsigned char *	p;
		register struct ttinfo *	ttisp;
		unsigned char			buf[sizeof s];

		p = buf;
		i = read(fid, (char *) p, sizeof buf);
		if (close(fid) != 0 || i < sizeof sp->h)
			return -1;
		p += sizeof h.tzh_reserved;
		GETSHORT(sp->h.tzh_timecnt, p);
		GETSHORT(sp->h.tzh_typecnt, p);
		GETSHORT(sp->h.tzh_charcnt, p);
		if (sp->h.tzh_timecnt > TZ_MAX_TIMES ||
			sp->h.tzh_typecnt == 0 ||
			sp->h.tzh_typecnt > TZ_MAX_TYPES ||
			sp->h.tzh_charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof h.tzh_reserved + 3 * sizeof (short) +
			sp->h.tzh_timecnt * (sizeof (long) + sizeof (char)) +
			sp->h.tzh_typecnt * (sizeof (long) + 2*sizeof (char)) +
			sp->h.tzh_charcnt * sizeof (char))
				return -1;
		for (i = 0; i < sp->h.tzh_timecnt; ++i)
			GETLONG(sp->ats[i], p);
		for (i = 0; i < sp->h.tzh_timecnt; ++i)
			sp->types[i] = *p++;
		for (i = 0, ttisp = ttis; i < sp->h.tzh_typecnt; ++i, ++ttisp) {
			GETLONG(ttisp->tt_gmtoff, p);
			ttisp->tt_abbrind = *p++;
			ttisp->tt_abbrind = *p++;
		}
		for (i = 0; i < sp->h.tzh_charcnt; ++i)
			sp->chars[i] = *p++;
		sp->chars[sp->h.tzh_charcnt] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < sp->h.tzh_timecnt; ++i)
		if (sp->types[i] >= sp->h.tzh_typecnt)
			return -1;
	/*
	** Check that all the abbreviation indices are valid.
	*/
	for (i = 0; i < sp->h.tzh_typecnt; ++i)
		if (sp->ttis[i].tt_abbrind >= sp->h.tzh_charcnt)
			return -1;
	return 0;
}

/*
** settz("")		Use built-in GMT.
** settz((char *) 0)	Use TZDEFAULT.
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
		if (tzload(tzname, &s) == 0)
			return 0;
		/*
		** If we want to try for local time on errors. . .
		if (tzload((char *) 0, &s) == 0)
			return -1;
		*/
		answer = -1;
	}
	s.h.tzh_timecnt = 0;
	s.ttis[0].tt_gmtoff = 0;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, "GMT");
	return answer;
}

static struct tm *
newsub(timep, sp)
register long *		timep;
register struct state *	sp;
{
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	long				t;

	t = *timep;
	if (sp->h.tzh_timecnt == 0 || t < sp->ats[0])
		i = 0;
	else {
		for (i = 1; i < sp->h.tzh_timecnt; ++i)
			if (t < sp->ats[i])
				break;
		i = sp->types[i - 1];
	}
	ttisp = &sp->ttis[i];
	t += ttisp->tt_gmtoff;
	tmp = gmtime(&t);
	tmp->tm_isdst = ttisp->tt_isdst;
	tz_abbr = &sp->chars[ttisp->tt_abbrind];
	return tmp;
}

struct tm *
newlocaltime(timep)
long *	timep;
{
	if (!isset)
		(void) settz(getenv("TZ"));
	return newsub(timep, &s);
}

struct tm *
zonetime(timep, zone)
long *	timep;
char *	zone;
{
	struct state	st;

	return (tzload(zone, &st) == 0) ? newsub(timep, &st) : 0;
}

char *
newctime(timep)
long *	timep;
{
	return asctime(newlocaltime(timep));
}
