/*
** TO DO:  FIGURE OUT WHAT TO SET TZNAME AND TM_ZONE TO FOR OUTRE CASES.
*/

#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Leap second handling from Bradley White (bww@k.gp.cs.cmu.edu).
** POSIX-format TZ environment variable handling from Guy Harris
** (guy@auspex.uucp).
*/

/*LINTLIBRARY*/

#include "tzfile.h"
#include "time.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"	/* for FILENAME_MAX */
#include "fcntl.h"	/* for O_RDONLY */
#include "nonstd.h"

#ifdef __TURBOC__
#include "io.h"		/* for open et al. prototypes */
#endif /* defined __TURBOC__ */

#define ACCESS_MODE	O_RDONLY

#ifdef O_BINARY
#define OPEN_MODE	O_RDONLY | O_BINARY
#else /* !defined O_BINARY */
#define OPEN_MODE	O_RDONLY
#endif /* !defined O_BINARY */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !defined TRUE */

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
};

struct lsinfo {				/* leap second information */
	time_t		ls_trans;	/* transition time */
	long		ls_corr;	/* correction to apply */
};

struct state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[TZ_MAX_CHARS + 1];
	struct lsinfo	lsis[TZ_MAX_LEAPS];
};

struct rule {
	int	r_type;		/* type of rule--see below */
	int	r_day;		/* day number of rule */
	int	r_week;		/* week number of rule */
	int	r_mon;		/* month number of rule */
	long	r_time;		/* transition time of rule */
};

#define	JULIAN_DAY		0	/* Jn - Julian day */
#define	DAY_OF_YEAR		1	/* n - day of year */
#define	MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

static long		detzcode P((const char * codep));
static void		settzname P((const struct state *sp));
static const char *	getzname P((const char *strp));
static const char *	getnum P((const char *strp, int *nump, int min,
				int max));
static const char *	gettime P((const char *strp, long *timep));
static const char *	getoffset P((const char *strp, long *offsetp));
static const char *	getrule P((const char *strp, struct rule *rulep));
static time_t		transtime P((time_t janfirst, int year,
				const struct rule *rulep, long offset));
static int		tzparse P((const char *name, struct state *sp,
				const int dotzname));
#ifdef STD_INSPIRED
struct tm *		offtime P((const time_t * clockp, long offset));
#endif /* !defined STD_INSPIRED */
static void		timesub P((const time_t * clockp, long offset,
				const struct state * sp, struct tm * tmp));
static int		tzload P((const char * name, struct state * sp,
				const int dotzname));
static void		tzsetgmt P((struct state * const sp,
				const int dotzname));
void			tzsetwall P((void));

static struct state	lclstate;
static struct state	gmtstate;

static int		lcl_is_set;
static int		gmt_is_set;

char *			tzname[2] = {
	"GMT",
	"GMT"
};

#ifdef USG_COMPAT
time_t			timezone = 0;
int			daylight = 0;
#endif /* defined USG_COMPAT */

static long
detzcode(codep)
const char * const	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static void
settzname(sp)
register const struct state * const	sp;
{
	register int	i;

	/*
	** Avoid using stuff left over from previously set zone (if any).
	*/
	tzname[0] = tzname[1] = "   ";
#ifdef USG_COMPAT
	timezone = -sp->ttis[0].tt_gmtoff;
	daylight = 0;
#endif /* defined USG_COMPAT */
	for (i = 0; i < sp->typecnt; ++i) {
		register const struct ttinfo *	ttisp;

		ttisp = &sp->ttis[i];
		if (ttisp->tt_isdst) {
			tzname[1] = (char *) &sp->chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			daylight = 1;
#endif /* defined USG_COMPAT */
		} else {
			tzname[0] = (char *) &sp->chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			timezone = -ttisp->tt_gmtoff;
#endif /* defined USG_COMPAT */
		}
	}
}

static int
tzload(name, sp, dotzname)
register const char *		name;
register struct state * const	sp;
const int			dotzname;
{
	register const char *	p;
	register int		i;
	register int		fid;

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register int 	doaccess;
		char		fullname[FILENAME_MAX + 1];

		if (name[0] == ':')
			name++;
		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == NULL)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			if (strchr(name, '.') != NULL)
				doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, ACCESS_MODE) != 0)
			return -1;
		if ((fid = open(name, OPEN_MODE)) == -1)
			return -1;
	}
	{
		register const struct tzhead *	tzhp;
		char				buf[sizeof *sp];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		sp->leapcnt = (int) detzcode(tzhp->tzh_leapcnt);
		sp->timecnt = (int) detzcode(tzhp->tzh_timecnt);
		sp->typecnt = (int) detzcode(tzhp->tzh_typecnt);
		sp->charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (sp->leapcnt > TZ_MAX_LEAPS ||
			sp->timecnt > TZ_MAX_TIMES ||
			sp->typecnt == 0 ||
			sp->typecnt > TZ_MAX_TYPES ||
			sp->charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			sp->timecnt * (4 + sizeof (char)) +
			sp->typecnt * (4 + 2 * sizeof (char)) +
			sp->charcnt * sizeof (char) +
			sp->leapcnt * 2 * 4)
				return -1;
		p = buf + sizeof *tzhp;
		for (i = 0; i < sp->timecnt; ++i) {
			sp->ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < sp->timecnt; ++i)
			sp->types[i] = (unsigned char) *p++;
		for (i = 0; i < sp->typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &sp->ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < sp->charcnt; ++i)
			sp->chars[i] = *p++;
		sp->chars[i] = '\0';	/* ensure '\0' at end */
		for (i = 0; i < sp->leapcnt; ++i) {
			register struct lsinfo *	lsisp;

			lsisp = &sp->lsis[i];
			lsisp->ls_trans = detzcode(p);
			p += 4;
			lsisp->ls_corr = detzcode(p);
			p += 4;
		}
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < sp->timecnt; ++i)
		if (sp->types[i] >= sp->typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < sp->typecnt; ++i)
		if (sp->ttis[i].tt_abbrind >= sp->charcnt)
			return -1;
	/*
	** Set tzname elements to initial values.
	*/
	if (dotzname)
		settzname(sp);
	return 0;
}

static const int	mon_lengths[2][MONSPERYEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const int	year_lengths[2] = {
	DAYSPERNYEAR, DAYSPERLYEAR
};

/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found.  Return a pointer to that
** character.
*/

static const char *
getzname(strp)
register const char *	strp;
{
	register char	c;

	while ((c = *strp) != '\0' && !isdigit(c) && c != ',' && c != '-' &&
		c != '+')
			++strp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/

static const char *
getnum(strp, nump, min, max)
register const char *	strp;
int * const		nump;
const int		min;
const int		max;
{
	register char	c;
	register int	num;

	num = 0;
	while ((c = *strp) != '\0' && isdigit(c)) {
		num = num * 10 + (c - '0');
		if (num > max)
			return NULL;	/* illegal value */
		++strp;
	}
	if (num < min)
		return NULL;		/* illegal value */
	*nump = num;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a time, in hh[:mm[:ss]]
** form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/

static const char *
gettime(strp, timep)
register const char *	strp;
long * const		timep;
{
	int	num;

	strp = getnum(strp, &num, 0, HOURSPERDAY / 2);
	if (strp == NULL)
		return NULL;
	*timep = num * SECSPERHOUR;
	if (*strp == ':') {
		++strp;
		strp = getnum(strp, &num, 0, MINSPERHOUR - 1);
		if (strp == NULL)
			return NULL;
		*timep += num * SECSPERMIN;
		if (*strp == ':') {
			++strp;
			strp = getnum(strp, &num, 0, SECSPERMIN - 1);
			if (strp == NULL)
				return NULL;
			*timep += num;
		}
	}
	return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/

static const char *
getoffset(strp, offsetp)
register const char *	strp;
long * const		offsetp;
{
	register int	neg;

	if (*strp == '-') {
		neg = 1;
		++strp;
	} else if (*strp == '+' || isdigit(*strp))
		neg = 0;
	else	return NULL;		/* illegal offset */
	strp = gettime(strp, offsetp);
	if (strp == NULL)
		return NULL;		/* illegal time */
	if (neg)
		*offsetp = -*offsetp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time].  See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/

static const char *
getrule(strp, rulep)
const char *			strp;
register struct rule * const	rulep;
{
	if (*strp == 'J') {
		/*
		** Julian day.
		*/
		rulep->r_type = JULIAN_DAY;
		++strp;
		strp = getnum(strp, &rulep->r_day, 1, DAYSPERNYEAR);
	} else if (*strp == 'M') {
		/*
		** Month, week, day.
		*/
		rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
		++strp;
		strp = getnum(strp, &rulep->r_mon, 1, MONSPERYEAR);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_week, 1, 5);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERWEEK - 1);
	} else if (isdigit(*strp)) {
		/*
		** Day of year.
		*/
		rulep->r_type = DAY_OF_YEAR;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERLYEAR - 1);
	} else	return NULL;		/* invalid format */
	if (strp == NULL)
		return NULL;
	if (*strp == '/') {
		/*
		** Time specified.
		*/
		++strp;
		strp = gettime(strp, &rulep->r_time);
		if (strp == NULL)
			return NULL;
	} else	rulep->r_time = 2 * SECSPERHOUR;	/* default = 2:00:00 */
	return strp;
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 GMT, in a year, the
** year, a rule, and the offset from GMT at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/

static time_t
transtime(janfirst, year, rulep, offset)
const time_t				janfirst;
const int				year;
register const struct rule * const	rulep;
const long				offset;
{
	register int	leapyear;
	register time_t	value;
	register int	i;
	int		d, m1, yy0, yy1, yy2, dow;

	leapyear = isleap(year);
	switch (rulep->r_type) {

	case JULIAN_DAY:
		/*
		** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
		** years.
		** In non-leap years, or if the day number is 59 or less, just
		** add SECSPERDAY times the day number-1 to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + (rulep->r_day - 1) * SECSPERDAY;
		if (leapyear && rulep->r_day >= 60)
			value += SECSPERDAY;
		break;

	case DAY_OF_YEAR:
		/*
		** n - day of year.
		** Just add SECSPERDAY times the day number to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + rulep->r_day * SECSPERDAY;
		break;

	case MONTH_NTH_DAY_OF_WEEK:
		/*
		** Mm.n.d - nth "dth day" of month m.
		*/
		value = janfirst;
		for (i = 0; i < rulep->r_mon - 1; ++i)
			value += mon_lengths[leapyear][i] * SECSPERDAY;

		/*
		** Use Zeller's Congruence to get day-of-week of first day of
		** month.
		*/
		m1 = (rulep->r_mon + 9) % 12 + 1;
		yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
		yy1 = yy0 / 100;
		yy2 = yy0 % 100;
		dow = ((26 * m1 - 2) / 10 +
			1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
		if (dow < 0)
			dow += DAYSPERWEEK;

		/*
		** "dow" is the day-of-week of the first day of the month.  Get
		** the day-of-month (zero-origin) of the first "dow" day of the
		** month.
		*/
		d = rulep->r_day - dow;
		if (d < 0)
			d += DAYSPERWEEK;
		for (i = 1; i < rulep->r_week; ++i) {
			if (d + DAYSPERWEEK >=
				mon_lengths[leapyear][rulep->r_mon - 1])
					break;
			d += DAYSPERWEEK;
		}

		/*
		** "d" is the day-of-month (zero-origin) of the day we want.
		*/
		value += d * SECSPERDAY;
		break;
	}

	/*
	** "value" is the Epoch-relative time of 00:00:00 GMT on the day in
	** question.  To get the Epoch-relative time of the specified local
	** time on that day, add the transition time and the current offset
	** from GMT.
	*/
	return value + rulep->r_time + offset;
}

/*
** Given a POSIX section 8-style TZ string, fill in the rule tables as
** appropriate.
*/

static int
tzparse(name, sp, dotzname)
const char *			name;
register struct state * const	sp;
const int			dotzname;
{
	const char *			stdname;
	const char *			dstname;
	int				stdlen;
	int				dstlen;
	long				stdoffset;
	long				dstoffset;
	register time_t *		atp;
	register unsigned char *	typep;
	register char *			cp;
	register int			load_result;

	stdname = name;
	name = getzname(name);
	stdlen = name - stdname;	/* length of standard zone name */
	if (stdlen == 0)
		return -1;
	name = getoffset(name, &stdoffset);
	if (name == NULL)
		return -1;
	load_result = tzload(TZDEFRULES, sp, dotzname);
	if (load_result != 0)
		sp->leapcnt = 0;		/* so, we're off a little */
	if (*name != '\0') {
		dstname = name;
		name = getzname(name);
		dstlen = name - dstname;	/* length of DST zone name */
		if (dstlen == 0)
			return -1;
		if (*name != '\0' && *name != ',' && *name != ';') {
			name = getoffset(name, &dstoffset);
			if (name == NULL)
				return -1;
		} else
			dstoffset = stdoffset - 1 * SECSPERHOUR;
		if (*name == ',' || *name == ';') {
			struct rule	start;
			struct rule	end;
			register int	year;
			register time_t	janfirst;
			time_t		starttime;
			time_t		endtime;

			++name;
			if ((name = getrule(name, &start)) == NULL)
				return -1;
			if (*name++ != ',')
				return -1;
			if ((name = getrule(name, &end)) == NULL)
				return -1;
			if (*name != '\0')
				return -1;
			sp->typecnt = 2;	/* standard time and DST */
			/*
			** Two transitions per year, from 1970 to 2038.
			*/
			sp->timecnt = 2 * (2038 - 1970 + 1);
			if (sp->timecnt > TZ_MAX_TIMES)
				return -1;
			sp->ttis[0].tt_gmtoff = -dstoffset;
			sp->ttis[0].tt_isdst = 1;
			sp->ttis[0].tt_abbrind = stdlen + 1;
			sp->ttis[1].tt_gmtoff = -stdoffset;
			sp->ttis[1].tt_isdst = 0;
			sp->ttis[1].tt_abbrind = 0;
			atp = sp->ats;
			typep = sp->types;
			for (year = 1970, janfirst = 0; year <= 2038; year++) {
				starttime = transtime(janfirst, year, &start,
					stdoffset);
				endtime = transtime(janfirst, year, &end,
					dstoffset);
				if (starttime > endtime) {
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
				} else {
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
				}
				janfirst +=
					year_lengths[isleap(year)] * SECSPERDAY;
			}
		} else {
			int		sawstd;
			int		sawdst;
			long		stdfix;
			long		dstfix;
			long		oldfix;
			int		isdst;
			register int	i;

			if (*name != '\0')
				return -1;
			if (load_result != 0)
				return -1;
			/*
			** Compute the difference between the real and
			** prototype standard and summer time offsets
			** from GMT, and put the real standard and summer
			** time offsets into the rules in place of the
			** prototype offsets.
			*/
			sawstd = FALSE;
			sawdst = FALSE;
			stdfix = 0;
			dstfix = 0;
			for (i = 0; i < sp->typecnt; ++i) {
				if (sp->ttis[i].tt_isdst) {
					oldfix = dstfix;
					dstfix =
					    sp->ttis[i].tt_gmtoff + dstoffset;
					if (sawdst && (oldfix != dstfix))
						return -1;
					sp->ttis[i].tt_gmtoff = -dstoffset;
					sp->ttis[i].tt_abbrind = stdlen + 1;
					sawdst = TRUE;
				} else {
					oldfix = stdfix;
					stdfix =
					    sp->ttis[i].tt_gmtoff + stdoffset;
					if (sawstd && (oldfix != stdfix))
						return -1;
					sp->ttis[i].tt_gmtoff = -stdoffset;
					sp->ttis[i].tt_abbrind = 0;
					sawstd = TRUE;
				}
			}
			/*
			** Make sure we have both standard and summer time.
			*/
			if (!sawdst || !sawstd)
				return -1;
			/*
			** Now correct the transition times by shifting
			** them by the difference between the real and
			** prototype offsets.  Note that this difference
			** can be different in standard and summer time;
			** the prototype probably has a 1-hour difference
			** between standard and summer time, but a different
			** difference can be specified in TZ.
			*/
			isdst = FALSE;	/* we start in standard time */
			for (i = 0; i < sp->timecnt; ++i) {
				/*
				** XXX - if the DST end time was specified
				** as "standard" rather than "local" time,
				** all transition times should be shifted
				** by "stdfix".  Unfortunately, we don't
				** know how they were specified....
				*/
				sp->ats[i] += isdst ? dstfix : stdfix;
				isdst = sp->ttis[sp->types[i]].tt_isdst;
			}
		}
	} else {
		dstlen = 0;
		sp->typecnt = 1;		/* only standard time */
		sp->timecnt = 0;
		sp->ttis[0].tt_gmtoff = -stdoffset;
		sp->ttis[0].tt_isdst = 0;
		sp->ttis[0].tt_abbrind = 0;
	}
	sp->charcnt = stdlen + 1;
	if (dstlen != 0)
		sp->charcnt += dstlen + 1;
	if (sp->charcnt > sizeof sp->chars)
		return -1;
	cp = sp->chars;
	(void) strncpy(cp, stdname, stdlen);
	cp += stdlen;
	*cp++ = '\0';
	if (dstlen != 0) {
		(void) strncpy(cp, dstname, dstlen);
		*(cp + dstlen) = '\0';
	}
	if (dotzname)
		settzname(sp);
	return 0;
}

static void
tzsetgmt(sp, dotzname)
register struct state * const	sp;
const int			dotzname;
{
	sp->leapcnt = 0;		/* so, we're off a little */
	sp->timecnt = 0;
	sp->ttis[0].tt_gmtoff = 0;
	sp->ttis[0].tt_abbrind = 0;
	(void) strcpy(sp->chars, "GMT");
	if (dotzname)
		settzname(sp);
}

void
tzset()
{
	register const char *	name;

	lcl_is_set = TRUE;
	name = getenv("TZ");
	if (name != 0 && *name == '\0')
		tzsetgmt(&lclstate, TRUE);		/* GMT by request */
	else if (tzload(name, &lclstate, TRUE) != 0) {
		if (name[0] == ':' || tzparse(name, &lclstate, TRUE) != 0)
			tzsetgmt(&lclstate, TRUE);
	}
}

void
tzsetwall()
{
	lcl_is_set = TRUE;
	if (tzload((char *) 0, &lclstate, TRUE) != 0)
		tzsetgmt(&lclstate, TRUE);
}

struct tm *
localtime(timep)
const time_t * const	timep;
{
	register const struct state *	sp;
	register const struct ttinfo *	ttisp;
	register int			i;
	time_t				t;
	static struct tm		tm;

	if (!lcl_is_set)
		tzset();
	sp = &lclstate;
	t = *timep;
	if (sp->timecnt == 0 || t < sp->ats[0]) {
		i = 0;
		while (sp->ttis[i].tt_isdst)
			if (++i >= sp->typecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < sp->timecnt; ++i)
			if (t < sp->ats[i])
				break;
		i = sp->types[i - 1];
	}
	ttisp = &sp->ttis[i];
	/*
	** To get (wrong) behavior that's compatible with System V Release 2.0
	** you'd replace the statement below with
	**	t += ttisp->tt_gmtoff;
	**	timesub(&t, 0L, sp, &tm);
	*/
	timesub(&t, ttisp->tt_gmtoff, sp, &tm);
	tm.tm_isdst = ttisp->tt_isdst;
	tzname[tm.tm_isdst] = (char *) &sp->chars[ttisp->tt_abbrind];
#ifdef TM_ZONE
	tm.TM_ZONE = &sp->chars[ttisp->tt_abbrind];
#endif /* defined TM_ZONE */
	return &tm;
}

struct tm *
gmtime(clock)
const time_t * const	clock;
{
	static struct tm	tm;

	if (!gmt_is_set) {
		gmt_is_set = TRUE;
		if (tzload("GMT", &gmtstate, FALSE) != 0)
			tzsetgmt(&gmtstate, FALSE);
	}
	timesub(clock, 0L, &gmtstate, &tm);
#ifdef TM_ZONE
	tm.TM_ZONE = "GMT";
#endif /* defined TM_ZONE */
	return &tm;
}

#ifdef STD_INSPIRED

struct tm *
offtime(clock, offset)
const time_t * const	clock;
const long		offset;
{
	static struct tm	tm;

	if (!gmt_is_set) {
		gmt_is_set = TRUE;
		if (tzload("GMT", &gmtstate, FALSE) != 0)
			tzsetgmt(&gmtstate, FALSE);
	}
	timesub(clock, offset, &gmtstate, &tm);
	return &tm;
}

#endif /* defined STD_INSPIRED */

static void
timesub(clock, offset, sp, tmp)
const time_t * const			clock;
const long				offset;
register const struct state * const	sp;
register struct tm * const		tmp;
{
	register const struct lsinfo *	lp;
	register long			days;
	register long			rem;
	register int			y;
	register int			yleap;
	register const int *		ip;
	register long			corr;
	register int			hit;

	corr = 0;
	hit = FALSE;
	y = sp->leapcnt;
	while (--y >= 0) {
		lp = &sp->lsis[y];
		if (*clock >= lp->ls_trans) {
			if (*clock == lp->ls_trans)
				hit = ((y == 0 && lp->ls_corr > 0) ||
					lp->ls_corr > sp->lsis[y-1].ls_corr);
			corr = lp->ls_corr;
			break;
		}
	}
	days = *clock / SECSPERDAY;
	rem = *clock % SECSPERDAY;
#ifdef mc68k
	if (*clock == 0x80000000) {
		/*
		** A 3B1 muffs the division on the most negative number.
		*/
		days = -24855;
		rem = -11648;
	}
#endif /* mc68k */
	rem += (offset - corr);
	while (rem < 0) {
		rem += SECSPERDAY;
		--days;
	}
	while (rem >= SECSPERDAY) {
		rem -= SECSPERDAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECSPERHOUR);
	rem = rem % SECSPERHOUR;
	tmp->tm_min = (int) (rem / SECSPERMIN);
	tmp->tm_sec = (int) (rem % SECSPERMIN);
	if (hit)
		/*
		** A positive leap second requires a special
		** representation.  This uses "... ??:59:60".
		*/
		tmp->tm_sec += 1;
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYSPERWEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYSPERWEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
#ifdef TM_ZONE
	tmp->TM_ZONE = "";
#endif /* defined TM_ZONE */
#ifdef TM_GMTOFF
	tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
}

#ifdef BSD_COMPAT

/*
** If ctime and localtime aren't in the same file on 4.3BSD systems,
** you can run into compilation problems--take
**	cc date.c -lz
** (please).
*/

char *
ctime(timep)
const time_t * const	timep;
{
	return asctime(localtime(timep));
}

#endif /* defined BSD_COMPAT */

/*
** Adapted from code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago. (mtxinu!kridle now).
**	It does a binary search of the time_t space.  Since time_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
**
** This code does handle "out of bounds" values in the way described
** for "mktime" in the October, 1986 draft of the proposed ANSI C Standard;
** though this is an accident of the implementation and *cannot* be made to
** work correctly for the purposes there described.
**
** A warning applies if you try to use these functions with a version of
** "localtime" that has overflow problems (such as System V Release 2.0
** or 4.3 BSD localtime).
** If you're not using GMT and feed a value to localtime
** that's near the minimum (or maximum) possible time_t value, localtime
** may return a struct that represents a time near the maximum (or minimum)
** possible time_t value (because of overflow).  If such a returned struct tm
** is fed to timelocal, it will not return the value originally feed to
** localtime.
*/

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

static void
normalize(tensptr, unitsptr, base)
int * const	tensptr;
int * const	unitsptr;
{
	if (*unitsptr >= base) {
		*tensptr += *unitsptr / base - 1;
		*unitsptr %= base;
	} else if (*unitsptr < 0) {
		--*tensptr;
		*unitsptr += base;
		if (*unitsptr < 0) {
			*tensptr -= 1 + (-*unitsptr) / base;
			*unitsptr = base - (-*unitsptr) % base;

		}
	}
}

static int
tmcomp(atmp, btmp)
register const struct tm * const atmp;
register const struct tm * const btmp;
{
	register int	result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
		(result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
		(result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
		(result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
		(result = (atmp->tm_min - btmp->tm_min)) == 0)
			result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

#define BREAKDOWN(t)	(funcp == localtime || funcp == gmtime) ? \
				*((*funcp)(&(t))) : *((*funcp)(&(t), offset));

static time_t
time2(timeptr, funcp, offset, okayp)
struct tm * const	timeptr;
struct tm * (*		funcp)();
const long		offset;
int * const		okayp;
{
	register int	dir;
	register int	bits;
	register int	i;
	register int	saved_seconds;
	time_t		t;
	struct tm	yourtm, mytm;

	*okayp = FALSE;
	yourtm = *timeptr;
	normalize(&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR);
	normalize(&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY);
	normalize(&yourtm.tm_year, &yourtm.tm_mon, MONSPERYEAR);
	while (yourtm.tm_mday <= 0) {
		--yourtm.tm_year;
		yourtm.tm_mday +=
			year_lengths[isleap(yourtm.tm_year + TM_YEAR_BASE)];
	}
	for ( ; ; ) {
		i = mon_lengths[isleap(yourtm.tm_year +
			TM_YEAR_BASE)][yourtm.tm_mon];
		if (yourtm.tm_mday <= i)
			break;
		yourtm.tm_mday -= i;
		if (++yourtm.tm_mon >= MONSPERYEAR) {
			yourtm.tm_mon = 0;
			++yourtm.tm_year;
		}
	}
	saved_seconds = yourtm.tm_sec;
	yourtm.tm_sec = 0;
	/*
	** Calculate the number of magnitude bits in a time_t
	** (this works regardless of whether time_t is
	** signed or unsigned, though lint complains if unsigned).
	*/
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1)
		;
	/*
	** If time_t is signed, then 0 is the median value,
	** if time_t is unsigned, then 1 << bits is median.
	*/
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for ( ; ; ) {
		mytm = BREAKDOWN(t);
		dir = tmcomp(&mytm, &yourtm);
		if (dir != 0) {
			if (bits-- < 0)
				return WRONG;
			if (bits < 0)
				--t;
			else if (dir > 0)
				t -= (time_t) 1 << bits;
			else	t += (time_t) 1 << bits;
			continue;
		}
		if (yourtm.tm_isdst >= 0 && mytm.tm_isdst != yourtm.tm_isdst) {
			/*
			** Right time, wrong type.
			** Hunt for right time, right type.
			** It's okay to guess wrong since the guess
			** gets checked.
			*/
			register const struct state *	sp;
			register int			j;
			time_t				newt;

			sp = (const struct state *)
				((funcp == localtime) ? &lclstate : &gmtstate);
			for (i = 0; i < sp->typecnt; ++i) {
				for (j = 0; j < sp->typecnt; ++j) {
					newt = t + sp->ttis[i].tt_gmtoff -
						sp->ttis[j].tt_gmtoff;
					mytm = BREAKDOWN(newt);
					if (tmcomp(&mytm, &yourtm) != 0)
						continue;
					if (mytm.tm_isdst != yourtm.tm_isdst)
						continue;
					/*
					** We have a match.
					*/
					t = newt;
					goto label;
				}
			}
			/*
			** Failed to find a match.
			*/
			return WRONG;
		}
label:
		t += saved_seconds;
		*timeptr = BREAKDOWN(t);
		*okayp = TRUE;
		return t;
	}
}

static time_t
time1(timeptr, funcp, offset)
struct tm * const	timeptr;
struct tm * (* const	funcp)();
const long		offset;
{
	register time_t			t;
	register const struct state *	sp;
	register int			samei, otheri;
	int				okay;

	if (timeptr->tm_isdst > 1)
		return WRONG;
	t = time2(timeptr, funcp, offset, &okay);
	if (okay || timeptr->tm_isdst < 0)
		return t;
	/*
	** We're supposed to assume that somebody took a time of one type,
	** and did some math on it that yielded a "struct tm" that's bad.
	** We try to divine the type they started from and adjust to the
	** type they need.
	*/
	sp = (const struct state *) 
		((funcp == localtime) ? &lclstate : &gmtstate);
	for (samei = 0; samei < sp->typecnt; ++samei) {
		if (sp->ttis[samei].tt_isdst != timeptr->tm_isdst)
			continue;
		for (otheri = 0; otheri < sp->typecnt; ++otheri) {
			if (sp->ttis[otheri].tt_isdst == timeptr->tm_isdst)
				continue;
			timeptr->tm_sec += sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff;
			timeptr->tm_isdst = !timeptr->tm_isdst;
			t = time2(timeptr, funcp, offset, &okay);
			if (okay)
				return t;
			timeptr->tm_sec -= sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff;
			timeptr->tm_isdst = !timeptr->tm_isdst;
		}
	}
	return WRONG;
}

time_t
mktime(timeptr)
struct tm * const	timeptr;
{
	return time1(timeptr, localtime, 0L);
}

#ifdef STD_INSPIRED

time_t
timelocal(timeptr)
struct tm * const	timeptr;
{
	return mktime(timeptr);
}

time_t
timegm(timeptr)
struct tm * const	timeptr;
{
	return time1(timeptr, gmtime, 0L);
}

extern struct tm *	offtime P((const time_t * clock, long offset));

time_t
timeoff(timeptr, offset)
struct tm * const	timeptr;
const long		offset;
{

	return time1(timeptr, offtime, offset);
}

#endif /* defined STD_INSPIRED */

#ifdef CMUCS

/*
** The following is supplied for compatibility with
** previous versions of the CMUCS runtime library.
*/

long
gtime(tm)
struct tm * const	tmp;
{
	time_t	t;

	if ((t = mktime(tmp)) == WRONG)
		return -1L;
	return (long) t;
}

#endif /* defined CMUCS */
