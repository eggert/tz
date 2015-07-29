/*
 * Copyright (c) 1985, 1987, 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "private.h"
#include "locale.h"
#if HAVE_UTMPX_H
#include "utmpx.h"
#endif

#ifndef OTIME_MSG
#define OTIME_MSG "old time"
#endif
#ifndef NTIME_MSG
#define NTIME_MSG "new time"
#endif
#if !defined WTMPX_FILE && defined _PATH_WTMPX
# define WTMPX_FILE _PATH_WTMPX
#endif

/*
** The two things date knows about time are. . .
*/

#ifndef TM_YEAR_BASE
#define TM_YEAR_BASE	1900
#endif /* !defined TM_YEAR_BASE */

#ifndef SECSPERMIN
#define SECSPERMIN	60
#endif /* !defined SECSPERMIN */

extern char **		environ;
extern char *		optarg;
extern int		optind;
extern char *		tzname[2];

static int		retval = EXIT_SUCCESS;

static void		checkfinal(char const *, bool, time_t, time_t);
static time_t		convert(const char *, bool, time_t);
static void		display(const char *, time_t);
static void		dogmt(void);
static void		errensure(void);
static void		iffy(time_t, time_t, const char *, const char *);
static void		oops(const char *);
static void		reset(time_t);
static void		timeout(FILE *, const char *, const struct tm *);
static void		usage(void);
static void		wildinput(const char *, const char *,
				const char *);

int
main(const int argc, char *argv[])
{
	register const char *	format;
	register const char *	value;
	register const char *	cp;
	register int		ch;
	register bool		dousg;
	register bool		rflag = false;
	time_t			now;
	time_t			t;
	intmax_t		secs;
	char *			endarg;

	INITIALIZE(dousg);
#ifdef LC_ALL
	setlocale(LC_ALL, "");
#endif /* defined(LC_ALL) */
#if HAVE_GETTEXT
#ifdef TZ_DOMAINDIR
	bindtextdomain(TZ_DOMAIN, TZ_DOMAINDIR);
#endif /* defined(TEXTDOMAINDIR) */
	textdomain(TZ_DOMAIN);
#endif /* HAVE_GETTEXT */
	t = now = time(NULL);
	format = value = NULL;
	while ((ch = getopt(argc, argv, "ucr:n")) != EOF && ch != -1) {
		switch (ch) {
		default:
			usage();
		case 'u':		/* do it in UT */
		case 'c':
			dogmt();
			break;
		case 'r':		/* seconds since 1970 */
			if (rflag) {
				fprintf(stderr,
					_("date: error: multiple -r's used"));
				usage();
			}
			rflag = true;
			errno = 0;
			secs = strtoimax (optarg, &endarg, 0);
			if (*endarg || optarg == endarg)
				errno = EINVAL;
			else if (! (time_t_min <= secs && secs <= time_t_max))
				errno = ERANGE;
			if (errno) {
				perror(optarg);
				errensure();
				exit(retval);
			}
			t = secs;
			break;
		case 'n':		/* don't set network (ignored) */
			break;
		}
	}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else {
				fprintf(stderr,
_("date: error: multiple formats in command line\n"));
				usage();
			}
		else	if (value == NULL && !rflag)
				value = cp;
			else {
				fprintf(stderr,
_("date: error: multiple values in command line\n"));
				usage();
			}
	}
	if (value != NULL) {
		/*
		** This order ensures that "reasonable" twelve-digit inputs
		** (such as 120203042006) won't be misinterpreted
		** even if time_t's range all the way back to the thirteenth
		** century.  Do not change the order.
		*/
		t = convert(value, (dousg = true), now);
		if (t == -1)
			t = convert(value, (dousg = false), now);
		if (t == -1) {
			/*
			** Out of range values,
			** or time that falls in a DST transition hole?
			*/
			if ((cp = strchr(value, '.')) != NULL) {
				/*
				** Ensure that the failure of
				**	TZ=America/New_York date 8712312359.60
				** doesn't get misdiagnosed.  (It was
				**	TZ=America/New_York date 8712311859.60
				** when the leap second was inserted.)
				** The normal check won't work since
				** the given time is valid in UTC.
				*/
				if (atoi(cp + 1) >= SECSPERMIN)
					wildinput(_("time"), value,
					    _("out of range seconds given"));
			}
			dogmt();
			t = convert(value, false, now);
			if (t == -1)
				t = convert(value, true, now);
			wildinput(_("time"), value,
				(t == -1) ?
				_("out of range value given") :
				_("time skipped when clock springs forward"));
		}
	}
	/*
	** Entire command line has now been checked.
	*/

	if (value) {
		reset(t);
		checkfinal(value, dousg, t, now);
		t = time(NULL);
	}

	display(format, t);
	return retval;
}

static void
dogmt(void)
{
	static char **	fakeenv;

	if (fakeenv == NULL) {
		register int	from;
		register int	to;
		register int	n;
		static char	tzegmt0[] = "TZ=GMT0";

		for (n = 0;  environ[n] != NULL;  ++n)
			continue;
		fakeenv = malloc((n + 2) * sizeof *fakeenv);
		if (fakeenv == NULL) {
			perror(_("Memory exhausted"));
			errensure();
			exit(retval);
		}
		to = 0;
		fakeenv[to++] = tzegmt0;
		for (from = 1; environ[from] != NULL; ++from)
			if (strncmp(environ[from], "TZ=", 3) != 0)
				fakeenv[to++] = environ[from];
		fakeenv[to] = NULL;
		environ = fakeenv;
	}
}

/*
** We assume we're on a POSIX-based system,
** should use clock_settime if CLOCK_REALTIME is defined,
** and should write utmp entries if HAVE_UTMPX_H,
** and don't have network notification to worry about.
*/

#include "fcntl.h"	/* for O_WRONLY, O_APPEND */

/*ARGSUSED*/
static void
reset(time_t newt)
{
#if HAVE_UTMPX_H
# if defined WTMPX_FILE && !SUPPRESS_WTMPX_FILE_UPDATE
	register int		fid;
# endif
	time_t oldt = time(NULL);
	static struct {
		struct utmpx	before;
		struct utmpx	after;
	} sx;
#endif

	/*
	** Wouldn't it be great if clock_settime returned the old time?
	*/
	int clock_settime_result;
#ifdef CLOCK_REALTIME
	struct timespec ts;
	ts.tv_sec = newt;
	ts.tv_nsec = 0;
	clock_settime_result = clock_settime(CLOCK_REALTIME, &ts);
#else
	clock_settime_result = -1;
	errno = EPERM;
#endif
	if (clock_settime_result != 0)
	  oops("clock_settime");

#if HAVE_UTMPX_H
	sx.before.ut_type = OLD_TIME;
	sx.before.ut_tv.tv_sec = oldt;
	strcpy(sx.before.ut_line, OTIME_MSG);
	sx.after.ut_type = NEW_TIME;
	sx.after.ut_tv.tv_sec = newt;
	strcpy(sx.after.ut_line, NTIME_MSG);
#if defined WTMPX_FILE && !SUPPRESS_WTMPX_FILE_UPDATE
	/* In Solaris 2.5 (and presumably other systems),
	   'date' does not update /var/adm/wtmpx.
	   This must be a bug.  If you'd like to reproduce the bug,
	   define SUPPRESS_WTMPX_FILE_UPDATE to be nonzero.  */
	fid = open(WTMPX_FILE, O_WRONLY | O_APPEND);
	if (fid < 0)
		oops(_("log file open"));
	if (write(fid, (char *) &sx, sizeof sx) != sizeof sx)
		oops(_("log file write"));
	if (close(fid) != 0)
		oops(_("log file close"));
# endif
	pututxline(&sx.before);
	pututxline(&sx.after);
#endif /* HAVE_UTMPX_H */
}

static void
wildinput(char const *item, char const *value, char const *reason)
{
	fprintf(stderr,
		_("date: error: bad command line %s \"%s\", %s\n"),
		item, value, reason);
	usage();
}

static void
errensure(void)
{
	if (retval == EXIT_SUCCESS)
		retval = EXIT_FAILURE;
}

static void
usage(void)
{
	fprintf(stderr,
		       _("date: usage: date [-u] [-c] [-r seconds]"
			 " [[yyyy]mmddhhmm[yyyy][.ss]] [+format]\n"));
	errensure();
	exit(retval);
}

static void
oops(char const *string)
{
	int		e = errno;

	fprintf(stderr, _("date: error: "));
	errno = e;
	perror(string);
	errensure();
	display(NULL, time(NULL));
	exit(retval);
}

static void
display(char const *format, time_t now)
{
	struct tm *tmp;

	tmp = localtime(&now);
	if (!tmp) {
		fprintf(stderr,
			_("date: error: time out of range\n"));
		errensure();
		return;
	}
	timeout(stdout, format ? format : "%+", tmp);
	putchar('\n');
	fflush(stdout);
	fflush(stderr);
	if (ferror(stdout) || ferror(stderr)) {
		fprintf(stderr,
			_("date: error: couldn't write results\n"));
		errensure();
	}
}

#define INCR	1024

static void
timeout(FILE *fp, char const *format, struct tm const *tmp)
{
	char *	cp;
	size_t	result;
	size_t	size;
	struct tm tm;

	if (*format == '\0')
		return;
	if (!tmp) {
		fprintf(stderr, _("date: error: time out of range\n"));
		errensure();
		return;
	}
	tm = *tmp;
	tmp = &tm;
	size = INCR;
	cp = malloc(size);
	for ( ; ; ) {
		if (cp == NULL) {
			fprintf(stderr,
				_("date: error: can't get memory\n"));
			errensure();
			exit(retval);
		}
		cp[0] = '\1';
		result = strftime(cp, size, format, tmp);
		if (result != 0 || cp[0] == '\0')
			break;
		size += INCR;
		cp = realloc(cp, size);
	}
	fwrite(cp, 1, result, fp);
	free(cp);
}

static bool
sametm(register const struct tm *const atmp,
       register const struct tm *const btmp)
{
	return atmp->tm_year == btmp->tm_year &&
		atmp->tm_mon == btmp->tm_mon &&
		atmp->tm_mday == btmp->tm_mday &&
		atmp->tm_hour == btmp->tm_hour &&
		atmp->tm_min == btmp->tm_min &&
		atmp->tm_sec == btmp->tm_sec;
}

/*
** convert --
**	convert user's input into a time_t.
*/

#define ATOI2(ar)	(ar[0] - '0') * 10 + (ar[1] - '0'); ar += 2;

static time_t
convert(char const *value, bool dousg, time_t t)
{
	register const char *	cp;
	register const char *	dotp;
	register int	cent, year_in_cent, month, hour, day, mins, secs;
	struct tm	tm, outtm, *tmp;
	time_t		outt;

	tmp = localtime(&t);
	if (!tmp)
		return -1;
	tm = *tmp;
#define DIVISOR	100
	year_in_cent = tm.tm_year % DIVISOR + TM_YEAR_BASE % DIVISOR;
	cent = tm.tm_year / DIVISOR + TM_YEAR_BASE / DIVISOR +
		year_in_cent / DIVISOR;
	year_in_cent %= DIVISOR;
	if (year_in_cent < 0) {
		year_in_cent += DIVISOR;
		--cent;
	}
	month = tm.tm_mon + 1;
	day = tm.tm_mday;
	hour = tm.tm_hour;
	mins = tm.tm_min;
	secs = 0;

	dotp = strchr(value, '.');
	for (cp = value; *cp != '\0'; ++cp)
		if (!is_digit(*cp) && cp != dotp)
			wildinput(_("time"), value, _("contains a nondigit"));

	if (dotp == NULL)
		dotp = strchr(value, '\0');
	else {
		cp = dotp + 1;
		if (strlen(cp) != 2)
			wildinput(_("time"), value,
				_("seconds part is not two digits"));
		secs = ATOI2(cp);
	}

	cp = value;
	switch (dotp - cp) {
		default:
			wildinput(_("time"), value,
				_("main part is wrong length"));
		case 12:
			if (!dousg) {
				cent = ATOI2(cp);
				year_in_cent = ATOI2(cp);
			}
			month = ATOI2(cp);
			day = ATOI2(cp);
			hour = ATOI2(cp);
			mins = ATOI2(cp);
			if (dousg) {
				cent = ATOI2(cp);
				year_in_cent = ATOI2(cp);
			}
			break;
		case 8:	/* mmddhhmm */
			month = ATOI2(cp);
			/* fall through to. . . */
		case 6:	/* ddhhmm */
			day = ATOI2(cp);
			/* fall through to. . . */
		case 4:	/* hhmm */
			hour = ATOI2(cp);
			mins = ATOI2(cp);
			break;
		case 10:
			if (!dousg) {
				year_in_cent = ATOI2(cp);
			}
			month = ATOI2(cp);
			day = ATOI2(cp);
			hour = ATOI2(cp);
			mins = ATOI2(cp);
			if (dousg) {
				year_in_cent = ATOI2(cp);
			}
			break;
	}

	tm.tm_year = cent * 100 + year_in_cent - TM_YEAR_BASE;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = mins;
	tm.tm_sec = secs;
	tm.tm_isdst = -1;
	outtm = tm;
	outt = mktime(&outtm);
	return sametm(&tm, &outtm) ? outt : -1;
}

/*
** Code from here on out is either based on code provided by UCB
** or is only called just before the program exits.
*/

/*
** Check for iffy input.
*/

static void
checkfinal(char const *value, bool didusg, time_t t, time_t oldnow)
{
	time_t		othert;
	struct tm	tm, *tmp;
	struct tm	othertm;
	register int	pass, offset;

	/*
	** See if there's both a USG and a BSD interpretation.
	*/
	othert = convert(value, !didusg, oldnow);
	if (othert != -1 && othert != t)
		iffy(t, othert, value, _("year could be at start or end"));
	/*
	** See if there's both a DST and a STD version.
	*/
	tmp = localtime(&t);
	if (!tmp)
		iffy(t, othert, value, _("time out of range"));
	othertm = tm = *tmp;
	othertm.tm_isdst = !tm.tm_isdst;
	othert = mktime(&othertm);
	if (othert != -1 && othertm.tm_isdst != tm.tm_isdst &&
		sametm(&tm, &othertm))
			iffy(t, othert, value,
			    _("both standard and summer time versions exist"));
/*
** Final check.
**
** If a jurisdiction shifts time *without* shifting whether time is
** summer or standard (as Hawaii, the United Kingdom, and Saudi Arabia
** have done), routine checks for iffy times may not work.
** So we perform this final check, deferring it until after the time has
** been set; it may take a while, and we don't want to introduce an unnecessary
** lag between the time the user enters their command and the time that
** clock_settime is called.
**
** We just check nearby times to see if any have the same representation
** as the time that convert returned.  We work our way out from the center
** for quick response in solar time situations.  We only handle common cases:
** offsets of at most a minute, and offsets of exact numbers of minutes
** and at most an hour.
*/
	for (offset = 1; offset <= 60; ++offset)
		for (pass = 1; pass <= 4; ++pass) {
			if (pass == 1)
				othert = t + offset;
			else if (pass == 2)
				othert = t - offset;
			else if (pass == 3)
				othert = t + 60 * offset;
			else	othert = t - 60 * offset;
			tmp = localtime(&othert);
			if (!tmp)
				iffy(t, othert, value,
					_("time out of range"));
			othertm = *tmp;
			if (sametm(&tm, &othertm))
				iffy(t, othert, value,
					_("multiple matching times exist"));
		}
}

static void
iffy(time_t thist, time_t thatt, char const *value, char const *reason)
{
	struct tm *tmp;
	bool dst;

	fprintf(stderr, _("date: warning: ambiguous time \"%s\", %s.\n"),
		value, reason);
	tmp = gmtime(&thist);
	/*
	** Avoid running afoul of SCCS!
	*/
	timeout(stderr, _("Time was set as if you used\n\tdate -u %m%d%H\
%M\
%Y.%S\n"), tmp);
	tmp = localtime(&thist);
	dst = tmp && tmp->tm_isdst;
	timeout(stderr, _("to get %c"), tmp);
	fprintf(stderr, _(" (%s).  Use\n"),
		dst ? _("summer time") : _("standard time"));
	tmp = gmtime(&thatt);
	timeout(stderr, _("\tdate -u %m%d%H\
%M\
%Y.%S\n"), tmp);
	tmp = localtime(&thatt);
	dst = tmp && tmp->tm_isdst;
	timeout(stderr, _("to get %c"), tmp);
	fprintf(stderr, _(" (%s).\n"),
		dst ? _("summer time") : _("standard time"));
	errensure();
	exit(retval);
}
