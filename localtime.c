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

static int	tzload(tzname)
register char *	tzname;
{
	register struct tzinfo *	tzp;
	register struct dsinfo *	dsp;
	register int			fid;
	register int			i, j, ok;
	char				buf[256];

	if (tzname == 0 || *tzname == '\0')
		return -1;
	tzp = &tzinfo;
	if (tzname[0] != '/') {
		(void) strcpy(buf, TZDIR);
		(void) strcat(buf, "/");
		if ((strlen(buf) + strlen(tzname) + 1) > sizeof buf)
			return -1;
		(void) strcat(buf, tzname);
		tzname = buf;
	}
	/*
	** We might be running set-user-ID, so. . .
	*/
	if (access(tzname, 4) != 0)
		return -1;
	if ((fid = open(tzname, 0)) == -1)
		return -1;
	ok = read(fid, (char *) tzp, sizeof *tzp) == sizeof *tzp;
	if (close(fid) != 0 || !ok)
		return -1;
	/*
	** Check the information.
	*/
	dsp = tzp->tz_dsinfo;
	if (dsp->ds_abbr[0] == '\0' || dsp->ds_abbr[TZ_ABBR_LEN] != '\0')
		return -1;
	for (i = 0; i < tzp->tz_rulecnt; ++i) {
		if (i > 0 && tzp->tz_times[i] <= tzp->tz_times[i - 1])
			return -1;
		j = tzp->tz_types[i];
		if (j < 0 || j >= TZ_MAX_TYPES)
			return -1;
		dsp = tzp->tz_dsinfo +  j;
		if (dsp->ds_abbr[0] == '\0' ||
			dsp->ds_abbr[TZ_ABBR_LEN] != '\0')
				return -1;
	}
	return 0;
}

/*
** settz("")			Use built-in GMT.
** settz(0)			Use TZDEFAULT.
** settz(otherwise)		Use otherwise.
*/

settz(tzname)
char *	tzname;
{
	register int	answer;

	if (tzname == 0)
		tzname = TZDEFAULT;
	if (*tzname == '\0')
		answer = 0;			/* Use built-in GMT */
	else {
		if (tzload(tzname) == 0)
			return 0;
		/*
		** Do the next two lines of code really belong here?
		*/
		if (tzload(TZDEFAULT) == 0)
			return -1;
		answer = -1;
	}
	tzinfo.tz_rulecnt = 0;
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
	if (tzp->tz_rulecnt == 0 || t < tzp->tz_times[0])
		dsp = tzp->tz_dsinfo;
	else {
		for (i = 0; i < tzp->tz_rulecnt; ++i)
			if (t < tzp->tz_times[i])
				break;
		dsp = tzp->tz_dsinfo + tzp->tz_types[i - 1];
	}
	t += dsp->ds_gmtoff;
	ct = gmtime(&t);
	ct->tm_isdst = dsp->ds_isdst != 0;
	tz_abbr = dsp->ds_abbr;
	return ct;
}

char *
newctime(timep)
long *timep;
{
	register char *			cp;
	register char *			dp;
	static char			buf[26 + TZ_ABBR_LEN + 1];

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
