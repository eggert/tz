#

/*LINTLIBRARY*/

#include "tzfile.h"
#include "time.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

extern char *		asctime();
extern struct tm *	gmtime();
extern char *		strcpy();
extern char *		strcat();
extern char *		getenv();

static struct tzinfo	tzinfo;

char *			tz_abbr;	/* set by localtime; available to all */

static
tzload(tzname)
register char *	tzname;
{
	register struct tzinfo *	tzp;
	register struct dsinfo *	dsp;
	register int			fid;
	register int			i, j, ok;
	char				buf[256];

	if (tzname == 0)
		tzname = TZDEFAULT;
	tzp = &tzinfo;
	if (tzname[0] != '/') {
		(void) strcpy(buf, TZDIR);
		(void) strcat(buf, "/");
		if ((strlen(buf) + strlen(tzname) + 1) > sizeof buf)
			goto oops;
		(void) strcat(buf, tzname);
		tzname = buf;
	}
	if ((fid = open(tzname, 0)) == -1)
		goto oops;
	ok = read(fid, (char *) tzp, sizeof *tzp) == sizeof *tzp;
	if (close(fid) != 0 || !ok)
		goto oops;
	/*
	** Check for errors that could cause core dumps.
	** Note:  all tz_dsinfo elements are checked even if they aren't used.
	** Note that a zero-length time zone abbreviation is *not* considered
	** to be an error.
	*/
	if (tzp->tz_timecnt < 0 || tzp->tz_timecnt > TZ_MAX_TIMES)
		goto oops;
	for (i = 0; i < tzp->tz_timecnt; ++i)
		if (tzp->tz_types[i] > TZ_MAX_TYPES)
			goto oops;
	for (i = 0; i < TZ_MAX_TYPES; ++i) {
		dsp = tzp->tz_dsinfo + i;
		j = 0;
		while (dsp->ds_abbr[j] != '\0')
			if (++j > TZ_ABBR_LEN)
				goto oops;
	}
	return 0;
oops:
	/*
	** Clobber tzinfo (in case we're running set-user-id and have been
	** used to read a protected file).
	*/
	{
		struct tzinfo	nonsense;

		*tzp = nonsense;
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
	tzinfo.tz_timecnt = 0;
	tzinfo.tz_dsinfo[0].ds_gmtoff = 0;
	(void) strcpy(tzinfo.tz_dsinfo[0].ds_abbr, "GMT");
	return answer;
}

struct tm *
newlocaltime(timep)
long *timep;
{
	register struct tzinfo *	tzp;
	register struct dsinfo *	dsp;
	register struct tm *		ct;
	register int			i;
	long				t;

	tzp = &tzinfo;
	t = *timep;
	if (tzp->tz_dsinfo[0].ds_abbr[0] == '\0')
		(void) settz(getenv("TZ"));
	if (tzp->tz_timecnt == 0 || t < tzp->tz_times[0])
		dsp = tzp->tz_dsinfo;
	else {
		for (i = 0; i < tzp->tz_timecnt; ++i)
			if (t < tzp->tz_times[i])
				break;
		dsp = tzp->tz_dsinfo + tzp->tz_types[i - 1];
	}
	t += dsp->ds_gmtoff;
	ct = gmtime(&t);
	ct->tm_isdst = dsp->ds_isdst;
	tz_abbr = dsp->ds_abbr;
	return ct;
}

char *
newctime(timep)
long *	timep;
{
	register char *	cp;
	register char *	dp;
	static char	buf[26 + TZ_ABBR_LEN + 1];

	(void) strcpy(buf, asctime(newlocaltime(timep)));
	dp = &buf[24];
	*dp++ = ' ';
	cp = tz_abbr;
	while ((*dp = *cp++) != '\0')
		++dp;
	*dp++ = '\n';
	*dp++ = '\0';
	return buf;
}
