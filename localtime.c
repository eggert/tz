#

/*LINTLIBRARY*/

#include "sys/param.h"
#include "tzfile.h"
#include "time.h"

#ifndef lint
#ifndef NOID
static char	sccsid[] = "%W%";
#endif /* !NOID */
#endif /* !lint */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !TRUE */

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif /* !MAXPATHLEN */

extern char *		getenv();
extern struct tm *	gmtime();
extern char *		strcpy();
extern char *		strcat();

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
};

struct state {
	int		timecnt;
	int		typecnt;
	int		charcnt;
	long		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[TZ_MAX_CHARS + 1];
};

static struct state	s;

static int		tz_is_set;

#ifdef USG_COMPAT
long			timezone = 0;
int			daylight = 0;
#endif /* USG_COMPAT */

char *			tzname[2] = {
	"GMT",
	"GMT"
};

#ifdef TZA_COMPAT
char *			tz_abbr;	/* compatibility w/older versions */
#endif /* TZA_COMPAT */

static long
detzcode(codep)
char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static
tzload(name)
register char *	name;
{
	register int	i;
	register int	fid;

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register char *	p;
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			while (*name != '\0')
				if (*name++ == '.')
					doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register char *			p;
		register struct tzhead *	tzhp;
		char				buf[sizeof s];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		s.timecnt = detzcode(tzhp->tzh_timecnt);
		s.typecnt = detzcode(tzhp->tzh_typecnt);
		s.charcnt = detzcode(tzhp->tzh_charcnt);
		if (s.timecnt > TZ_MAX_TIMES ||
			s.typecnt == 0 ||
			s.typecnt > TZ_MAX_TYPES ||
			s.charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			s.timecnt * (4 + sizeof (char)) +
			s.typecnt * (4 + 2 * sizeof (char)) +
			s.charcnt * sizeof (char))
				return -1;
		p = buf + sizeof *tzhp;
		for (i = 0; i < s.timecnt; ++i) {
			s.ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s.timecnt; ++i)
			s.types[i] = (unsigned char) *p++;
		for (i = 0; i < s.typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &s.ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s.charcnt; ++i)
			s.chars[i] = *p++;
		s.chars[i] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < s.timecnt; ++i)
		if (s.types[i] >= s.typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < s.typecnt; ++i)
		if (s.ttis[i].tt_abbrind >= s.charcnt)
			return -1;
	/*
	** Set tzname elements to initial values.
	*/
#ifdef USG_COMPAT
	timezone = s.ttis[0].tt_gmtoff;
	daylight = 0;
#endif /* USG_COMPAT */
	tzname[0] = tzname[1] = &s.chars[0];
	for (i = 1; i < s.typecnt; ++i) {
		register struct ttinfo *	ttisp;

		ttisp = &s.ttis[i];
		if (ttisp->tt_isdst) {
#ifdef USG_COMPAT
			daylight = 1;
#endif /* USG_COMPAT */
			tzname[1] = &s.chars[ttisp->tt_abbrind];
		} else {
#ifdef USG_COMPAT
			timezone = ttisp->tt_gmtoff;
#endif /* USG_COMPAT */
			tzname[0] = &s.chars[ttisp->tt_abbrind];
		}
	}
	return 0;
}

static
tzgmt()
{
	s.timecnt = 0;
#ifdef USG_COMPAT
	timezone = 0;
#endif /* USG_COMPAT */
	s.ttis[0].tt_gmtoff = 0;
#ifdef USG_COMPAT
	daylight = 0;
#endif /* USG_COMPAT */
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, "GMT");
	tzname[0] = tzname[1] = s.chars;
}

void
tzset()
{
	register char *	name;

	tz_is_set = TRUE;
	name = getenv("TZ");
	if (name != 0 && *name == '\0')
		tzgmt();		/* GMT by request */
	else if (tzload(name) != 0)
		tzgmt();
}

void
tzsetwall()
{
	tz_is_set = TRUE;
	if (tzload((char *) 0) != 0)
		tzgmt();
}

struct tm *
localtime(timep)
long *	timep;
{
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	long				t;

	if (!tz_is_set)
		(void) tzset();
	t = *timep;
	if (s.timecnt == 0 || t < s.ats[0]) {
		i = 0;
		while (s.ttis[i].tt_isdst)
			if (++i >= s.timecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < s.timecnt; ++i)
			if (t < s.ats[i])
				break;
		i = s.types[i - 1];
	}
	ttisp = &s.ttis[i];
	t += ttisp->tt_gmtoff;
	tmp = gmtime(&t);
	tmp->tm_isdst = ttisp->tt_isdst;
#ifdef TZA_COMPAT
	tz_abbr =
#endif /* TZA_COMPAT */
	tzname[tmp->tm_isdst] = &s.chars[ttisp->tt_abbrind];
	return tmp;
}

/*
********************************************************************************
*/

#define SECS_PER_MIN	60
#define MINS_PER_HOUR	60
#define HOURS_PER_DAY	24
#define DAYS_PER_WEEK	7
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)

#define TM_THURSDAY	4

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

static int	mon_lengths[2][12] = {	/* ". . .knuckles are 31. . ." */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int	year_lengths[2] = {
	365, 366
};

/*
** Since the times representable in a 32-bit signed integer
** run from 1901 to 2037, the define below works--2000 IS a leap year.
*/

#define isleap(y) (((y) % 4) == 0)

time_t
mktime(timeptr)
register struct tm *	timeptr;
{
	register int			i, year, month;
	register time_t			loctimevalue;
	time_t				gmtimevalue;
	register struct ttinfo *	ttisp;

	if (!tz_is_set)
		(void) tzset();

	/*
	** First, check that the time structure passed to us contains
	** a valid time.
	** SPECIFICATION ERROR: what if it isn't?  Do we bitch, or do
	** we correct it somehow - if we correct it, how do we do so?
	** The only thing I [guy@sun] can think of is that it calls "gmtime"
	** when it's got a time value, and stuffs the returned structure
	** back into "*timeptr".  Let's try that.
	*/
	month = timeptr->tm_mon;
	if (month < 0 || month > 11)
		return -1;
	if (timeptr->tm_mday < 1 || timeptr->tm_mday > 31)
		return -1;
	if (timeptr->tm_min < 0 || timeptr->tm_min > 59)
		return -1;
	if (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)
		return -1;

	loctimevalue = 0;
	year = timeptr->tm_year + TM_YEAR_BASE;
	if (year < EPOCH_YEAR) {
		for (i = year; i < EPOCH_YEAR; i++)
			loctimevalue -= year_lengths[isleap(i)];
	} else {
		for (i = EPOCH_YEAR; i < year; i++)
			loctimevalue += year_lengths[isleap(i)];
	}
	while (month--)
		loctimevalue += mon_lengths[isleap(year)][month];
	loctimevalue += timeptr->tm_mday - 1;
	loctimevalue = HOURS_PER_DAY * loctimevalue + timeptr->tm_hour;
	loctimevalue = MINS_PER_HOUR * loctimevalue + timeptr->tm_min;
	loctimevalue = SECS_PER_MIN * loctimevalue + timeptr->tm_sec;

	/*
	** "timevalue" is now the number of seconds since January 1, 1970,
	** 00:00 local time.  Convert it to GMT.
	** SPECIFICATION ERROR: should it always use the value of tm_isdst,
	** or should it compute it?  We could use it only to handle local
	** times that can map to two GMT times, and recompute it when the
	** smoke clears.  The nasty part of that is figuring out what those
	** times are.
	*/

	if (s.timecnt == 0 || loctimevalue < s.ats[0]) {
		i = 0;
		while (s.ttis[i].tt_isdst)
			if (++i >= s.timecnt) {
				i = 0;
				break;
			}
		ttisp = &s.ttis[i];
		gmtimevalue = loctimevalue - ttisp->tt_gmtoff;
	} else {
		for (i = 1; i < s.timecnt; ++i) {
			ttisp = &s.ttis[s.types[i - 1]];
			gmtimevalue = loctimevalue - ttisp->tt_gmtoff;
			if (gmtimevalue < s.ats[i])
				break;
		}
	}

	/*
	** Fill in the rest of the structure.
	*/

	*timeptr = *localtime(&gmtimevalue);
	return gmtimevalue;
}
