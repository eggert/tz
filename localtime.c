#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "tzfile.h"
#include "time.h"
#include "string.h"
#include "nonstd.h"

#ifdef __TURBOC__
#include "io.h"			/* to pick up prototypes for open and such */
#include "fcntl.h"		/* to pick up O_BINARY and O_RDONLY bits */
#endif /* defined __TURBOC__ */

#ifdef unix
#include "fcntl.h"
#endif /* defined unix */

#ifdef O_BINARY
#ifdef O_RDONLY
#define OPEN_MODE	O_BINARY | O_RDONLY
#endif /* defined O_RDONLY */
#endif /* defined O_BINARY */

#ifndef OPEN_MODE
#define OPEN_MODE	0
#endif /* !defined OPEN_MODE */

#ifdef O_RDONLY
#define ACCESS_MODE	O_RDONLY
#else /* !defined O_RDONLY */
#define ACCESS_MODE	4
#endif /* !defined O_RDONLY */

#ifdef __STDC__

#include "time.h"

#include "stdlib.h"

#ifdef FILENAME_MAX
#define MAXPATHLEN	FILENAME_MAX
#endif /* defined FILENAME_MAX */

#else /* !defined __STDC__ */

#include "sys/types.h"		/* to get time_t */

extern char *	getenv();

#ifndef MAXPATHLEN
#include "sys/param.h"
#endif /* !defined MAXPATHLEN */

#endif /* !defined __STDC__ */

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif /* !defined MAXPATHLEN */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !defined TRUE */

static long		detzcode P((const char * codep));
#ifdef STD_INSPIRED
struct tm *     	offtime P((const time_t * clockp, long offset));
#else /* !defined STD_INSPIRED */
static struct tm *      offtime P((const time_t * clockp, long offset));
#endif /* !defined STD_INSPIRED */
static struct tm *	timesub P((const time_t * clockp, long offset,
				struct state * sp));
static int		tzload P((const char * name, struct state * sp));
static void		tzsetgmt P((struct state * sp));
void			tzsetwall P((void));

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

#ifdef TZA_COMPAT
char *			tz_abbr;	/* compatibility w/older versions */
#endif /* defined TZA_COMPAT */

static long
detzcode(codep)
const char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static int
tzload(name, sp)
register const char *		name;
register struct state *	sp;
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
		if (doaccess && access(name, ACCESS_MODE) != 0)
			return -1;
		if ((fid = open(name, OPEN_MODE)) == -1)
			return -1;
	}
	{
		register char *			p;
		register struct tzhead *	tzhp;
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
	if (sp == &lclstate) {
		tzname[0] = tzname[1] = &sp->chars[0];
#ifdef USG_COMPAT
		timezone = -sp->ttis[0].tt_gmtoff;
		daylight = 0;
#endif /* defined USG_COMPAT */
		for (i = 1; i < sp->typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &sp->ttis[i];
			if (ttisp->tt_isdst) {
				tzname[1] = &sp->chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
				daylight = 1;
#endif /* defined USG_COMPAT */
			} else {
				tzname[0] = &sp->chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
				timezone = -ttisp->tt_gmtoff;
#endif /* defined USG_COMPAT */
			}
		}
	}
	return 0;
}

static void
tzsetgmt(sp)
register struct state *	sp;
{
	sp->leapcnt = 0;		/* so, we're off a little */
	sp->timecnt = 0;
	sp->ttis[0].tt_gmtoff = 0;
	sp->ttis[0].tt_abbrind = 0;
	(void) strcpy(sp->chars, "GMT");
	if (sp == &lclstate) {
		tzname[0] = tzname[1] = sp->chars;
#ifdef USG_COMPAT
		timezone = 0;
		daylight = 0;
#endif /* defined USG_COMPAT */
	}
}

void
tzset()
{
	register char *	name;

	lcl_is_set = TRUE;
	name = getenv("TZ");
	if (name != 0 && *name == '\0')
		tzsetgmt(&lclstate);		/* GMT by request */
	else if (tzload(name, &lclstate) != 0)
		tzsetgmt(&lclstate);
}

void
tzsetwall()
{
	lcl_is_set = TRUE;
	if (tzload((char *) 0, &lclstate) != 0)
		tzsetgmt(&lclstate);
}

struct tm *
localtime(timep)
const time_t *	timep;
{
	register struct state *		sp;
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	time_t				t;

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
	**	tmp = timesub(&t, 0L, sp);
	*/
	tmp = timesub(&t, ttisp->tt_gmtoff, sp);
	tmp->tm_isdst = ttisp->tt_isdst;
	tzname[tmp->tm_isdst] = &sp->chars[ttisp->tt_abbrind];
#ifdef KRE_COMPAT
	tmp->tm_zone = &sp->chars[ttisp->tt_abbrind];
#endif /* defined KRE_COMPAT */
#ifdef TZA_COMPAT
	tz_abbr = &sp->chars[ttisp->tt_abbrind];
#endif /* defined TZA_COMPAT */
	return tmp;
}

struct tm *
gmtime(clock)
const time_t *	clock;
{
	register struct tm *	tmp;

	tmp = offtime(clock, 0L);
	tzname[0] = "GMT";
#ifdef KRE_COMPAT
	tmp->tm_zone = "GMT";		/* UCT ? */
#endif /* defined KRE_COMPAT */
	return tmp;
}

#ifdef STD_INSPIRED
struct tm *
#else /* !defined STD_INSPIRED */
static struct tm *
#endif /* !defined STD_INSPIRED */
offtime(clock, offset)
const time_t *	clock;
long		offset;
{
	if (!gmt_is_set) {
		gmt_is_set = TRUE;
		if (tzload("GMT", &gmtstate) != 0)
			tzsetgmt(&gmtstate);
	}
	return timesub(clock, offset, &gmtstate);
}

static int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

static struct tm *
timesub(clock, offset, sp)
const time_t *		clock;
long			offset;
register struct state *	sp;
{
	register struct lsinfo *	lp;
	register struct tm *		tmp;
	register long			days;
	register long			rem;
	register int			y;
	register int			yleap;
	register int *			ip;
	register long			corr;
	register int			hit;
	static struct tm		tm;

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
	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += (offset - corr);
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	if (hit)
		/*
		 * A positive leap second requires a special
		 * representation.  This uses "... ??:59:60".
		 */
		 tmp->tm_sec += 1;
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
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
#ifdef KRE_COMPAT
	tmp->tm_zone = "";
	tmp->tm_gmtoff = offset;
#endif /* defined KRE_COMPAT */
	return tmp;
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
time_t *	timep;
{
	return asctime(localtime(timep));
}

#endif /* defined BSD_COMPAT */

/*
** UNIX is a registered trademark of AT&T.
*/
