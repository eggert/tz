#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
/*
** Modified from the UCB version whose sccsid appears below.
*/
#endif /* !defined NOID */
#endif /* !defined lint */

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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1987, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)date.c	4.23 (Berkeley) 9/20/88";
#endif /* not lint */

#include "sys/param.h"
#include "sys/time.h"
#include "sys/file.h"
#include "errno.h"
#include "syslog.h"
#include "utmp.h"
#include "stdio.h"
#include "ctype.h"
#include "strings.h"
#include "tzfile.h"

#ifndef NO_SOCKETS
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#define TSPTYPES
#include "protocols/timed.h"
#endif /* !defined NO_SOCKETS */

/*
** What options should be provided?
** We normally provide everything we can, and "#define X_OPTION"
** for each option provided.  If "NO_X_OPTION" is defined for some case,
** we leave the option out.
**
** "-u" is provided unconditionally.
*/

/*
** Nothing special needed for BSD's "-n" option to not notify networked systems
** of a time change--on non-networked systems, "date -n" == "date".
*/

#ifndef N_OPTION
#ifndef NO_N_OPTION
#define N_OPTION
#endif /* !defined NO_N_OPTION */
#endif /* !defined N_OPTION */

/*
** BSD's "-d dst" and "-t min-west" options, for setting the kernel's
** notion of dst and offset, require the "settimeofday" system call;
** its availability is deduced from "DST_NONE" being defined.
*/

#ifndef D_OPTION
#ifndef NO_D_OPTION
#ifdef DST_NONE
#define D_OPTION
#endif /* defined DST_NONE */
#endif /* !defined NO_D_OPTION */
#endif /* !defined D_OPTION */

#ifndef T_OPTION
#ifndef NO_T_OPTION
#ifdef DST_NONE
#define T_OPTION
#endif /* defined DST_NONE */
#endif /* !defined NO_T_OPTION */
#endif /* !defined T_OPTION */

/*
** Sun's "-a sss.fff" option to slowly adjust the time requires the
** "adjtime" system call.  We assume it's available if DST_NONE is defined.
*/

#ifndef A_OPTION
#ifndef NO_A_OPTION
#ifdef DST_NONE
#define A_OPTION
#endif /* defined DST_NONE */
#endif /* !defined NO_A_OPTION */
#endif /* !defined A_OPTION */

/*
** So much for options.  One "combination" setting makes life easier.
*/

#ifdef D_OPTION
#define D_OR_T_OPTION
#else /* !defined D_OPTION */
#ifdef T_OPTION
#define D_OR_T_OPTION
#endif /* defined T_OPTION */
#endif /* !defined D_OPTION */

/*
** Finally, features.
*/

#ifndef USG_INPUT
#ifndef NO_USG_INPUT
#define USG_INPUT
#endif /* !defined NO_USG_INPUT */
#endif /* !defined USG_INPUT */

#ifndef USG_OUTPUT
#ifndef NO_USG_OUTPUT
#define USG_OUTPUT
#endif /* !defined NO_USG_OUTPUT */
#endif /* !defined USG_OUTPUT */

/*
** So much for features.
*/

#ifndef TIME_USER
#ifdef OTIME_MSG
#define TIME_USER	username
#else /* !defined OTIME_MSG */
#define TIME_USER	"date"
#endif /* !defined OTIME_MSG */
#endif /* !defined OTIME_USER */

#ifndef OTIME_MSG
#define OTIME_MSG	"|"
#endif /* !defined OTIME_MSG */

#ifndef NTIME_MSG
#define NTIME_MSG	"{"
#endif /* !defined NTIME_MSG */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	0
#endif /* !defined EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	1
#endif /* !defined EXIT_FAILURE */

extern char **		environ;
extern char *		getlogin();
extern int		logwtmp();
extern time_t		mktime();
extern char *		optarg;
extern int		optind;
extern time_t		time();
extern char *		tzname[2];

static time_t		now;

static int		retval = EXIT_SUCCESS;

static void		ambiguous();
static void		display();
static void		finalcheck();
static time_t		parse();
#ifdef TSP_SETDATE
int			netsettime();
#endif /* defined TSP_SETDATE */
static char *		nondigit();
static void		oops();
static void		timeout();
static void		usage();
static time_t		xtime();

static char *	options[132];
static char *	usemes[132];

int
main(argc, argv)
int	argc;
char *	argv[];
{
	register char *		format;
	register char *		value;
	register char *		cp;
	register char *		username;
	register int		ch;
	time_t			t;
#ifdef DST_NONE
	static struct timeval	tv;	/* static so tv_usec is 0 */
#endif /* defined DST_NONE */
#ifdef N_OPTION
#ifdef TSP_SETDATE
	register int		nflag = 0;
#endif /* defined TSP_SETDATE */
#endif /* defined N_OPTION */
#ifdef D_OPTION
	register int		dflag = 0;
#endif /* defined D_OPTION */ 
#ifdef T_OPTION
	register int		tflag = 0;
#endif /* defined T_OPTION */ 
#ifdef D_OR_T_OPTION
	struct timezone		tz;
#endif /* defined D_OR_T_OPTION */
#ifdef A_OPTION
	register int		aflag = 0;
	struct timeval		atv;
#endif /* defined A_OPTION */

#ifdef D_OR_T_OPTION
	if (gettimeofday((struct timeval *) NULL, &tz) != 0) {
		perror("date: error: gettimeofday");
		(void) exit(EXIT_FAILURE);
	}
#endif /* defined D_OR_T_OPTION */
	(void) time(&now);
	format = value = NULL;
	/*
	** "-u" is available everywhere.
	*/
	(void) strcpy(options, "u");
	(void) strcpy(usemes, "[-u");
#ifdef N_OPTION
	(void) strcat(options, "n");
	(void) strcat(usemes, "n");
#endif /* defined N_OPTION */
	(void) strcat(usemes, "]");
#ifdef D_OPTION
	(void) strcat(options, "d:");
	(void) strcat(usemes, " [-d dst]");
#endif /* defined D_OPTION */
#ifdef T_OPTION
	(void) strcat(options, "t:");
	(void) strcat(usemes, " [-t min-west]");
#endif /* defined T_OPTION */
#ifdef A_OPTION
	(void) strcat(options, "a:");
	(void) strcat(usemes, " [-a sss.fff]");
#endif /* defined A_OPTION */
	/*
	** Two time argument variants.
	*/
#ifdef USG_INPUT
	(void) strcat(usemes, " [[yyyy]mmddhhmm[yy][.ss]]");
#else /* !defined USG_INPUT */
	(void) strcat(usemes, " [[yyyy]mmddhhmm[.ss]]");
#endif /* !defined USG_INPUT */
	/*
	** "+format" is available everywhere.
	*/
	(void) strcat(usemes, " [+format]");
	while ((ch = getopt(argc, argv, options)) != EOF) {
		switch (ch) {
		default:
			usage();
		case 'u':		/* do it in GMT */
			{
				register char **	saveenv;
				static char *		fakeenv[] = {
								"TZ=GMT0",
								NULL
							};

				saveenv = environ;
				environ = fakeenv;
				tzset();
				environ = saveenv;
			}
			break;
#ifdef N_OPTION
		case 'n':		/* don't set network */
#ifdef TSP_SETDATE
			nflag = 1;
#endif /* defined TSP_SETDATE */
			break;
#endif /* defined N_OPTION */
#ifdef D_OPTION
		case 'd':		/* daylight savings time */
			if (dflag) {
				(void) fprintf(stderr,
					"date: error: multiple -d's used");
				usage();
			}
			dflag = 1;
			tz.tz_dsttime = atoi(optarg);
			if (*optarg == '\0')
				usage();
			if (*nondigit(optarg) != '\0')
				usage();
			break;
#endif /* defined D_OPTION */
#ifdef T_OPTION
		case 't':		/* minutes west of GMT */
			if (tflag) {
				(void) fprintf(stderr,
					"date: error: multiple -t's used");
				usage();
			}
			tflag = 1;
			tz.tz_minuteswest = atoi(optarg);
			if (*optarg == '+' || *optarg == '-')
				++optarg;
			if (*optarg == '\0')
				usage();
			if (*nondigit(optarg) != '\0')
				usage();
			break;
#endif /* defined T_OPTION */
#ifdef A_OPTION
		case 'a':		/* adjustment */
			if (aflag) {
				(void) fprintf(stderr,
					"date: error: multiple -a's used");
				usage();
			}
			aflag = 1;
			{
				float		f;
				extern double	atof();

				f = atof(optarg);
				if (*optarg == '+' || *optarg == '-')
					++optarg;
				if (*optarg == '\0' || strcmp(optarg, ".") == 0)
					usage();
				optarg = nondigit(optarg);
				if (*optarg == '.')
					++optarg;
				if (*nondigit(optarg) != '\0')
					usage();
				atv.tv_sec = (int) f;
				atv.tv_usec =
					(int) ((f - atv.tv_sec) * 1000000);
			}
			break;
#endif /* defined A_OPTION */
		}
	}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else {
				(void) fprintf(stderr, 
"date: error: multiple formats in command line\n");
				usage();
			}
		else	if (value == NULL)
				value = cp;
			else {
				(void) fprintf(stderr,
"date: error: multiple values in command line\n");
				usage();
			}
	}
	if (value != NULL) {
		t = parse(value, -1);
		if (t == -1)
			usage();
	}
	/*
	** Entire command line has now been checked.
	*/
#ifdef A_OPTION
	/*
	** Just what order *should* we do things in if the user asks
	** for both adjustment and time setting?
	*/
	if (aflag)
		if (adjtime(&atv, (struct timeval *) NULL) != 0)
			oops("date: error: adjtime");
#endif /* defined A_OPTION */
#ifdef D_OR_T_OPTION
	if ((tflag || dflag) &&
		settimeofday((struct timeval *) NULL, &tz) != 0)
			oops("date: error: settimeofday");
#endif /* defined D_OR_T_OPTION */
	if (value == NULL)
		display(format);
	username = getlogin();
	if (username == NULL || *username == '\0') /* single-user or no tty */
		username = "root";
	/*
	** You could argue that we shouldn't put the "before" entry into wtmp
	** until we've determined that the time-setting call has succeeded.
	** We'll continue to put the entry in unconditionally for compatibility.
	*/
#ifdef DST_NONE
#ifdef TSP_SETDATE
	tv.tv_sec = t;
#ifdef N_OPTION
	if (!nflag)
#endif /* defined N_OPTION */
		if (netsettime(tv) != 1)
			retval = EXIT_FAILURE;
#endif /* defined TSP_SETDATE */
	logwtmp(OTIME_MSG, TIME_USER, "");
	if (settimeofday(&tv, (struct timezone *) NULL) == 0) {
		logwtmp(NTIME_MSG, TIME_USER, "");
		syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", username);
	} else 	oops("date: error: settimeofday");
#else /* !defined DST_NONE */
	logwtmp(OTIME_MSG, TIME_USER, "");
	if (stime(&t) == 0)
		logwtmp(NTIME_MSG, TIME_USER, "");
	else	oops("date: error: stime");
#endif /* !defined DST_NONE */

	finalcheck(t);

	display(format);
	for ( ; ; )
		;
}

static char *
nondigit(cp)
register char *	cp;
{
	while (isdigit(*cp))
		++cp;
	return cp;
}

static void
usage()
{
	(void) fprintf(stderr, "date: usage is date %s\n", usemes);
	retval = EXIT_FAILURE;
	display((char *) NULL);
}

static void
oops(string)
char *	string;
{
	(void) perror(string);
	retval = EXIT_FAILURE;
	display((char *) NULL);
}

static void
ambiguous(thist, thatt, was_set)
time_t	thist;
time_t	thatt;
int	was_set;
{
	struct tm	tm;

	(void) fprintf(stderr, "date: error: ambiguous time.  ");
	tm = *gmtime(&thist);
	if (was_set)
		(void) fprintf(stderr, "Time was set as if you used\n");
	else	(void) fprintf(stderr, "Use\n");
	/*
	** Avoid running afoul of SCCS!
	*/
	timeout(stderr, "\tdate -u %Y", &tm);
	timeout(stderr, "%m%d%H", &tm);
	timeout(stderr, "%M.%S\n", &tm);
	tm = *localtime(&thist);
	timeout(stderr, "to get %c", &tm);
	(void) fprintf(stderr, " (%s time)",
		tm.tm_isdst ? "summer" : "standard");
	if (was_set)
		(void) fprintf(stderr, ".  Use\n");
	else	(void) fprintf(stderr, ", or\n");
	tm = *gmtime(&thatt);
	timeout(stderr, "\tdate -u %Y", &tm);
	timeout(stderr, "%m%d%H", &tm);
	timeout(stderr, "%M.%S\n", &tm);
	tm = *localtime(&thatt);
	timeout(stderr, "to get %c", &tm);
	(void) fprintf(stderr, " (%s time)",
		tm.tm_isdst ? "summer" : "standard");
	(void) fprintf(stderr, ".\n");
	(void) exit(EXIT_FAILURE);
}

static void
display(format)
char *	format;
{
	struct tm	tm;

	(void) time(&now);
	tm = *localtime(&now);
	timeout(stdout, (format == NULL) ? "%c" : format, &tm);
	(void) putchar('\n');
	(void) fflush(stdout);
	(void) fflush(stderr);
	if (ferror(stdout) || ferror(stderr)) {
		(void) fprintf(stderr, "date: error: couldn't write results\n");
		if (retval == EXIT_SUCCESS)
			retval = EXIT_FAILURE;
	}
	(void) exit(retval);
	for ( ; ; )
		;
}

static char *	wday_names[] = {
	"Sunday",	"Monday",	"Tuesday",	"Wednesday",
	"Thursday",	"Friday",	"Saturday"
};

static char *	mon_names[] = {
	"January",	"February",	"March",	"April",
	"May",		"June",		"July",		"August",
	"September",	"October",	"November",	"December"
};

static void
timeout(fp, format, tmp)
register FILE *	fp;
register char *	format;
struct tm *	tmp;
{
	register int	c;
	register int	wday;

	for ( ; ; ) {
		c = *format++;
		if (c == '\0')
			return;
		if (c != '%') {
			(void) putc(c, fp);
			continue;
		}
/*
** Format characters below come from
** December 7, 1988 version of X3J11's description of the strftime function.
*/
		switch (c = *format++) {
		default:
			/*
			** Clear out possible partial output.
			*/
			(void) putc('\n', fp);
			(void) fflush(fp);
			(void) fprintf(stderr,
				"date: error: bad format character - %c\n", c);
			retval = EXIT_FAILURE;
			display((char *) NULL);
		case 'a':
			(void) fprintf(fp, "%.3s", wday_names[tmp->tm_wday]);
			break;
		case 'A':
			(void) fprintf(fp, "%s", wday_names[tmp->tm_wday]);
			break;
		case 'b':
			(void) fprintf(fp, "%.3s", mon_names[tmp->tm_mon]);
			break;
		case 'B':
			(void) fprintf(fp, "%s", mon_names[tmp->tm_mon]);
			break;
		case 'c':
			timeout(fp, "%x %X %Z %Y", tmp);
			break;
		case 'd':
			(void) fprintf(fp, "%02.2d", tmp->tm_mday);
			break;
		case 'H':
			(void) fprintf(fp, "%02.2d", tmp->tm_hour);
			break;
		case 'I':
			(void) fprintf(fp, "%02.2d",
				((tmp->tm_hour % 12) == 0) ?
				12 : (tmp->tm_hour % 12));
			break;
		case 'j':
			(void) fprintf(fp, "%03.3d", tmp->tm_yday + 1);
			break;
#ifdef KITCHEN_SINK
		case 'k':
			(void) fprintf(fp, "kitchen sink");
			break;
#endif /* defined KITCHEN_SINK */
		case 'm':
			(void) fprintf(fp, "%02.2d", tmp->tm_mon + 1);
			break;
		case 'M':
			(void) fprintf(fp, "%02.2d", tmp->tm_min);
			break;
		case 'p':
			(void) fprintf(fp, "%cM",
				(tmp->tm_hour >= 12) ? 'P' : 'A');
			break;
		case 'S':
			(void) fprintf(fp, "%02.2d", tmp->tm_sec);
			break;
		case 'U':
			/* How many Sundays fall on or before this day? */
			wday = tmp->tm_wday;
			(void) fprintf(fp, "%02.2d",
				(tmp->tm_yday + 7 - wday) / 7);
			break;
		case 'w':
			(void) fprintf(fp, "%d", tmp->tm_wday);
			break;
		case 'W':
			/* How many Mondays fall on or before this day? */
			/* Transform it to the Sunday problem and solve that */
			wday = tmp->tm_wday;
			if (--wday < 0)
				wday = 6;
			(void) fprintf(fp, "%02.2d",
				(tmp->tm_yday + 7 - wday) / 7);
			break;
		case 'x':
			timeout(fp, "%a %b ", tmp);
			(void) fprintf(fp, "%2d", tmp->tm_mday);
			break;
		case 'X':
			timeout(fp, "%H:%M:%S", tmp);
			break;
		case 'y':
			(void) fprintf(fp, "%02.2d",
				(tmp->tm_year + TM_YEAR_BASE) % 100);
			break;
		case 'Y':
			(void) fprintf(fp, "%d", tmp->tm_year + TM_YEAR_BASE);
			break;
		case 'Z':
			(void) fprintf(fp, "%s", tzname[tmp->tm_isdst]);
			break;
		case '%':
			(void) putc('%', fp);
			break;
#ifdef USG_OUTPUT
/*
** Format characters below from:
** the System V Release 2.0 description of the date command;
** and the System V Release 3.1 description of the ascftime function.
*/
		case 'D':
			timeout(fp, "%m/%d/%y", tmp);
			break;
		case 'h':
			timeout(fp, "%b", tmp);
			break;
		case 'n':
			(void) putc('\n', fp);
			break;
		case 'r':
			timeout(fp, "%I:%M:%S %p", tmp);
			break;
		case 'R':
			timeout(fp, "%H:%M", tmp);
			break;
		case 't':
			(void) putc('\t', fp);
			break;
		case 'T':
			timeout(fp, "%X", tmp);
			break;
#endif /* defined USG_OUTPUT */
		}
	}
}

/*
** If a jurisdiction shifts time *without* shifting whether time is
** summer or standard (as Hawaii, the United Kingdom, and Saudi Arabia
** have done), routine checks for ambiguous times may not work.
** So we perform this final check, deferring it until after the time has
** been set--it may take a while, and we don't want to introduce an unnecessary
** lag between the time the user enters their command and the time that
** stime/settimeofday is called.
** 
** We just check nearby times to see if any of them have the same representation
** as the time that parse returned.  We work our way out from the center
** for quick response in solar time situations.  We only handle common cases--
** offsets of at most a minute, and offsets of exact numbers of minutes
** and at most an hour.
*/

static void
finalcheck(t)
time_t	t;
{
	struct tm	tm;
	register int	pass;
	register long	offset;
	time_t		othert;
	struct tm	othertm;
	
	tm = *localtime(&t);
	for (offset = 1; offset <= 60; ++offset)
		for (pass = 1; pass <= 4; ++pass) {
			if (pass == 1)
				othert = t + offset;
			else if (pass == 2)
				othert = t - offset;
			else if (pass == 3)
				othert = t + 60 * offset;
			else	othert = t - 60 * offset;
			othertm = *localtime(&othert);
			if (tm.tm_year == othertm.tm_year &&
				tm.tm_mon == othertm.tm_mon &&
				tm.tm_hour == othertm.tm_hour &&
				tm.tm_min == othertm.tm_min &&
				tm.tm_sec == othertm.tm_sec &&
				tm.tm_isdst == othertm.tm_isdst)
					ambiguous(t, othert, 1);
		}
}

/*
** parse --
**	convert user's input into a time_t.
*/

static int
pair(cp)
register char * cp;
{
	if (!isdigit(cp[0]) || !isdigit(cp[1]))
		return -1;
	return (cp[0] - '0') * 10 + cp[1] - '0';
}

static time_t
xtime(intmp)
register struct tm * intmp;
{
	struct tm	outtm;
	time_t		outt;

	outtm = *intmp;
	outt = mktime(&outtm);
	return (outtm.tm_isdst == intmp->tm_isdst &&
		outtm.tm_sec == intmp->tm_sec &&
		outtm.tm_min == intmp->tm_min &&
		outtm.tm_hour == intmp->tm_hour &&
		outtm.tm_mday == intmp->tm_mday &&
		outtm.tm_mon == intmp->tm_mon &&
		outtm.tm_year == intmp->tm_year) ?
			outt : -1;
}

static time_t
parse(cp, isdst)
register char *	cp;
int		isdst;
{
	register int	i;
	register int	year;
	struct tm	tm;
	int		pairs[6];
	time_t		thist;
	time_t		thatt;

	if (isdst < 0) {
		thist = parse(cp, 0);
		thatt = parse(cp, 1);
		if (thist == -1)
			if (thatt == -1)
				return -1;
			else	return thatt;
		else	if (thatt == -1)
				return thist;
			else	ambiguous(thist, thatt, 0);
	}
	tm = *localtime(&now);
	tm.tm_isdst = isdst;
	tm.tm_sec = 0;
	for (i = 0; ; ++i) {
		if (*cp == '\0')
			break;
		if (*cp == '.') {
			++cp;
			tm.tm_sec = pair(cp);
			if (tm.tm_sec < 0)
				return -1;
			if (*(cp + 2) != '\0')
				return -1;
			break;
		}
		if (i >= (sizeof pairs / sizeof pairs[0]))
			return -1;
		pairs[i] = pair(cp);
		if (pairs[i] < 0)
			return -1;
		cp += 2;
	}
	switch (i) {
		default:
			break;
		case 2:	/* hhmm */
			tm.tm_hour = pairs[0];
			tm.tm_min = pairs[1];
			return xtime(&tm);
		case 3:	/* ddhhmm */
			tm.tm_mday = pairs[0];
			tm.tm_hour = pairs[1];
			tm.tm_min = pairs[2];
			return xtime(&tm);
		case 4:	/* mmddhhmm */
			tm.tm_mon = pairs[0] - 1;
			tm.tm_mday = pairs[1];
			tm.tm_hour = pairs[2];
			tm.tm_min = pairs[3];
			return xtime(&tm);

		case 5:	/* Ulp! yymmddhhmm or mmddhhmmyy */
			year = tm.tm_year + TM_YEAR_BASE;
			year -= year % 100;
			year = year + pairs[0];
			tm.tm_year = year - TM_YEAR_BASE;

			tm.tm_mon = pairs[1] - 1;
			tm.tm_mday = pairs[2];
			tm.tm_hour = pairs[3];
			tm.tm_min = pairs[4];

			thist = xtime(&tm);
		
#ifndef USG_INPUT
			return thist;
#else /* defined USG_INPUT */
			tm.tm_mon = pairs[0] - 1;
			tm.tm_mday = pairs[1];
			tm.tm_hour = pairs[2];
			tm.tm_min = pairs[3];

			year = tm.tm_year + TM_YEAR_BASE;
			year -= year % 100;
			year = year + pairs[4];
			tm.tm_year = year - TM_YEAR_BASE;

			thatt = xtime(&tm);

			if (thist == -1)
				if (thatt == -1)
					break;
				else	return thatt;
			else	if (thatt == -1)
					return thist;
				else	ambiguous(thist, thatt, 0);
#endif /* defined USG_INPUT */

		case 6:	/* yyyymmddhhmm--BSD finally wins in the 21st century */
			year = pairs[0] * 100 + pairs[1];
			tm.tm_year = year - TM_YEAR_BASE;
			tm.tm_mon = pairs[2] - 1;
			tm.tm_mday = pairs[3];
			tm.tm_hour = pairs[4];
			tm.tm_min = pairs[5];
			return xtime(&tm);
	}
	return -1;
}

#ifdef TSP_SETDATE
#define	WAITACK		2	/* seconds */
#define	WAITDATEACK	5	/* seconds */

extern	int errno;
/*
 * Set the date in the machines controlled by timedaemons
 * by communicating the new date to the local timedaemon. 
 * If the timedaemon is in the master state, it performs the
 * correction on all slaves.  If it is in the slave state, it
 * notifies the master that a correction is needed.
 * Returns 1 on success, 0 on failure.
 */
netsettime(ntv)
	struct timeval ntv;
{
	int s, length, port, timed_ack, found, err;
	long waittime;
	fd_set ready;
	char hostname[MAXHOSTNAMELEN];
	struct timeval tout;
	struct servent *sp;
	struct tsp msg;
	struct sockaddr_in sin, dest, from;

	sp = getservbyname("timed", "udp");
	if (sp == 0) {
		fputs("udp/timed: unknown service\n", stderr);
		retval = 2;
		return (0);
	}
	dest.sin_port = sp->s_port;
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl((u_long)INADDR_ANY);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno != EPROTONOSUPPORT)
			perror("date: socket");
		goto bad;
	}
	bzero((char *)&sin, sizeof (sin));
	sin.sin_family = AF_INET;
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED / 2; port--) {
		sin.sin_port = htons((u_short)port);
		if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			break;
		if (errno != EADDRINUSE) {
			if (errno != EADDRNOTAVAIL)
				perror("date: bind");
			goto bad;
		}
	}
	if (port == IPPORT_RESERVED / 2) {
		fputs("date: All ports in use\n", stderr);
		goto bad;
	}
	msg.tsp_type = TSP_SETDATE;
	msg.tsp_vers = TSPVERSION;
	if (gethostname(hostname, sizeof (hostname))) {
		perror("date: gethostname");
		goto bad;
	}
	(void) strncpy(msg.tsp_name, hostname, sizeof (hostname));
	msg.tsp_seq = htons((u_short)0);
	msg.tsp_time.tv_sec = htonl((u_long)ntv.tv_sec);
	msg.tsp_time.tv_usec = htonl((u_long)ntv.tv_usec);
	length = sizeof (struct sockaddr_in);
	if (connect(s, &dest, length) < 0) {
		perror("date: connect");
		goto bad;
	}
	if (send(s, (char *)&msg, sizeof (struct tsp), 0) < 0) {
		if (errno != ECONNREFUSED)
			perror("date: send");
		goto bad;
	}
	timed_ack = -1;
	waittime = WAITACK;
loop:
	tout.tv_sec = waittime;
	tout.tv_usec = 0;
	FD_ZERO(&ready);
	FD_SET(s, &ready);
	found = select(FD_SETSIZE, &ready, (fd_set *)0, (fd_set *)0, &tout);
	length = sizeof(err);
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &length) == 0
	    && err) {
		errno = err;
		if (errno != ECONNREFUSED)
			perror("date: send (delayed error)");
		goto bad;
	}
	if (found > 0 && FD_ISSET(s, &ready)) {
		length = sizeof (struct sockaddr_in);
		if (recvfrom(s, (char *)&msg, sizeof (struct tsp), 0, &from,
		    &length) < 0) {
			if (errno != ECONNREFUSED)
				perror("date: recvfrom");
			goto bad;
		}
		msg.tsp_seq = ntohs(msg.tsp_seq);
		msg.tsp_time.tv_sec = ntohl(msg.tsp_time.tv_sec);
		msg.tsp_time.tv_usec = ntohl(msg.tsp_time.tv_usec);
		switch (msg.tsp_type) {

		case TSP_ACK:
			timed_ack = TSP_ACK;
			waittime = WAITDATEACK;
			goto loop;

		case TSP_DATEACK:
			(void)close(s);
			return (1);

		default:
			(void) fprintf(stderr,
			    "date: Wrong ack received from timed: %s\n", 
			    tsptype[msg.tsp_type]);
			timed_ack = -1;
			break;
		}
	}
	if (timed_ack == -1)
		fputs("date: Can't reach time daemon, time set locally.\n",
		    stderr);
bad:
	(void)close(s);
	retval = 2;
	return (0);
}
#endif /* defined TSP_SETDATE */
