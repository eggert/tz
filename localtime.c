#

/*LINTLIBRARY*/

#include "timezone.h"
#include "time.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

extern char *	strcpy();
extern char *	strcat();
extern char *	getenv();

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

/*
 * This routine converts time as follows.
 * The epoch is 0000 Jan 1 1970 GMT.
 * The argument time is in seconds since then.
 * The localtime(t) entry returns a pointer to an array
 * containing
 *  seconds (0-59)
 *  minutes (0-59)
 *  hours (0-23)
 *  day of month (1-31)
 *  month (0-11)
 *  year-1970
 *  weekday (0-6, Sun is 0)
 *  day of the year
 *  daylight savings flag
 *
 * asctime(tvec))
 * where tvec is produced by localtime
 * returns a ptr to a character string
 * that has the ascii time in the form
 *	Thu Jan 01 00:00:00 1970n0\\
 *	01234567890123456789012345
 *	0	  1	    2
 *
 */

static	char	cbuf[26 + TZ_ABBR_LEN + 1];
static	int	dmsize[12] =
{
	31,
	28,
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31
};

struct tm	*gmtime();
char		*ct_numb();
struct tm	*localtime();
char	*ctime();
char	*ct_num();
char	*asctime();

char *
ctime(t)
long *t;
{
	return(asctime(localtime(t)));
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
newctime(tim)
long *tim;
{
	register struct dsinfo *	dsp;
	register char *			cp;
	register char *			dp;
	long				copyt;

	dsp = getdsp(*tim);
	copyt = *tim + dsp->ds_gmtoff;
	(void) asctime(gmtime(&copyt));
	dp = &cbuf[24];
	*dp++ = ' ';
	cp = dsp->ds_abbr;
	while ((*dp = *cp++) != '\0')
		++dp;
	*dp++ = '\n';
	*dp++ = '\0';
	return cbuf;
}

struct tm *
localtime(tim)
long *tim;
{
	register struct dsinfo *	dsp;
	long				copyt;
	register struct tm *		ct;

	dsp = getdsp(*tim);
	copyt = *tim + dsp->ds_gmtoff;
	ct = gmtime(&copyt);
	ct->tm_isdst = dsp->ds_isdst != 0;
	return ct;
}

/*
** Things are unchanged from here down!
*/

struct tm *
gmtime(tim)
long *tim;
{
	register int d0, d1;
	long hms, day;
	register int *tp;
	static struct tm xtime;

	/*
	 * break initial number into days
	 */
	hms = *tim % 86400;
	day = *tim / 86400;
	if (hms<0) {
		hms += 86400;
		day -= 1;
	}
	tp = (int *)&xtime;

	/*
	 * generate hours:minutes:seconds
	 */
	*tp++ = hms%60;
	d1 = hms/60;
	*tp++ = d1%60;
	d1 /= 60;
	*tp++ = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	xtime.tm_wday = (day+7340036)%7;

	/*
	 * year number
	 */
	if (day>=0) for(d1=70; day >= dysize(d1); d1++)
		day -= dysize(d1);
	else for (d1=70; day<0; d1--)
		day += dysize(d1-1);
	xtime.tm_year = d1;
	xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if (dysize(d1)==366)
		dmsize[1] = 29;
	for(d1=0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	*tp++ = d0+1;
	*tp++ = d1;
	xtime.tm_isdst = 0;
	return(&xtime);
}

char *
asctime(t)
struct tm *t;
{
	register char *cp, *ncp;
	register int *tp;

	cp = cbuf;
	for (ncp = "Day Mon 00 00:00:00 1900\n"; *cp++ = *ncp++;);
	ncp = &"SunMonTueWedThuFriSat"[3*t->tm_wday];
	cp = cbuf;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp++;
	tp = &t->tm_mon;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[(*tp)*3];
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp = ct_numb(cp, *--tp);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	if (t->tm_year>=100) {
		cp[1] = '2';
		cp[2] = '0';
	}
	cp += 2;
	cp = ct_numb(cp, t->tm_year+100);
	return(cbuf);
}

dysize(y)
{
	if((y%4) == 0)
		return(366);
	return(365);
}

static char *
ct_numb(cp, n)
register char *cp;
{
	cp++;
	if (n>=10)
		*cp++ = (n/10)%10 + '0';
	else
		*cp++ = ' ';
	*cp++ = n%10 + '0';
	return(cp);
}
