#

/*LINTLIBRARY*/

#include "timezone.h"
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

settz(tzname)
char *	tzname;
{
	register struct tzinfo *	tzp;
	register struct dsinfo *	dsp;
	register int			fid;
	register int			i, j;
	char				buf[256];

	tzp = &tzinfo;
	fid = -1;
	if (tzname == 0 && tzname[0] == '\0')
		goto gmt;
	if (tzname[0] == '/')
		buf[0] = '\0';
	else {
		(void) strcpy(buf, TZDIR);
		(void) strcat(buf, "/");
	}
	if ((strlen(buf) + strlen(tzname) + 1) > sizeof buf)
		goto gmt;
	(void) strcat(buf, tzname);
	/*
	** We might be running set-user-ID, so. . .
	*/
	if (access(buf, 4) != 0)
		goto gmt;
	if ((fid = open(buf, 0)) == -1)
		goto gmt;
	if (read(fid, (char *) tzp, sizeof *tzp) != sizeof *tzp)
		goto gmt;
	if (close(fid) != 0)
		goto gmt;
	fid = -1;
	/*
	** Check the information.
	*/
	dsp = tzp->tz_dsinfo;
	if (dsp->ds_abbr[0] == '\0' || dsp->ds_abbr[TZ_ABBR_LEN] != '\0')
		goto gmt;
	for (i = 0; i < tzp->tz_rulecnt; ++i) {
		if (i > 0 && tzp->tz_times[i] <= tzp->tz_times[i - 1])
			goto gmt;
		j = tzp->tz_types[i];
		if (j < 0 || j >= TZ_MAX_TYPES)
			goto gmt;
		dsp = tzp->tz_dsinfo +  j;
		if (dsp->ds_abbr[0] == '\0' ||
			dsp->ds_abbr[TZ_ABBR_LEN] != '\0')
				goto gmt;
	}
	return 0;
gmt:
	(void) close(fid);
	tzp->tz_rulecnt = 0;
	dsp = tzp->tz_dsinfo;
	dsp->ds_gmtoff = 0;
	(void) strcpy(dsp->ds_abbr, "GMT");
	return (tzname[0] == 0) ? 0 : -1;
}

static struct dsinfo *	getdsp(t)
register long		t;
{
	register struct tzinfo *	tzp;
	register int			i;

	tzp = &tzinfo;
	if (tzp->tz_dsinfo[0].ds_abbr[0] == '\0')
		(void) settz(getenv("TZ"));
	if (tzp->tz_rulecnt == 0 || t < tzp->tz_times[0])
		return tzp->tz_dsinfo;
	for (i = 0; i < tzp->tz_rulecnt; ++i)
		if (t < tzp->tz_times[i])
			break;
	return tzp->tz_dsinfo + tzp->tz_types[i - 1];
}

char *
newctime(timep)
long *timep;
{
	register struct dsinfo *	dsp;
	register char *			cp;
	register char *			dp;
	long				copyt;
	static char			buf[26 + TZ_ABBR_LEN + 1];

	dsp = getdsp(*timep);
	copyt = *timep + dsp->ds_gmtoff;
	(void) strcpy(buf, asctime(gmtime(&copyt)));
	dp = &buf[24];
	*dp++ = ' ';
	cp = dsp->ds_abbr;
	while ((*dp = *cp++) != '\0')
		++dp;
	*dp++ = '\n';
	*dp++ = '\0';
	return buf;
}

struct tm *
newlocaltime(timep)
long *timep;
{
	register struct dsinfo *	dsp;
	long				copyt;
	register struct tm *		ct;

	dsp = getdsp(*timep);
	copyt = *timep + dsp->ds_gmtoff;
	ct = gmtime(&copyt);
	ct->tm_isdst = dsp->ds_isdst != 0;
	return ct;
}
