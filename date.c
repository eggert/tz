#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
/*
** Modified from the UCB version whose sccsid appears below.
** Add -a option for benefit of Sun?
*/
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Is this next right????  There's a TSP_SETTIME but no TSP_SETDATE in
** Sun's "protocols/timed.h".  Are the two synonymous?
*/

#ifdef sun
#define TSP_SETDATE	TSP_SETTIME
#endif /* defined sun */

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
extern time_t		mktime();
extern char *		optarg;
extern int		optind;
extern time_t		time();
extern char *		tzname[2];

static time_t		now;

static int		retval = EXIT_SUCCESS;

static void		display();
static time_t		gettime();
int			netsettime();
static void		timeout();
static void		usage();
static time_t		xtime();

#ifdef DST_NONE
#define OPTIONS	"uDSnd:t:"
#else /* !defined DST_NONE */
#define OPTIONS	"uDSn"
#endif /* !defined DST_NONE */

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
	register int		isdst;
	register int		nflag;
	time_t			t;
#ifdef DST_NONE
	register int		tflag, dflag;
	struct timezone		tz;
	static struct timeval	tv;	/* static so tv_usec is 0 */

	if (gettimeofday((struct timeval *) NULL, &tz) != 0) {
		perror("date: error: gettimeofday");
		(void) exit(EXIT_FAILURE);
	}
	tflag = dflag = 0;
#endif /* defined DST_NONE */
	(void) time(&now);
	format = value = NULL;
	isdst = -1;
	nflag = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != EOF) {
		switch (ch) {
		default:
			usage();
		case 'S':		/* take time to be Standard time */
		case 'D':		/* take time to be Deviant time */
			if (isdst != -1) {
				(void) fprintf(stderr,
					"date: error: multiple -S/-D's used");
				usage();
			}
			isdst = (ch == 'S') ? 0 : 1;
			break;
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
		case 'n':		/* don't set network */
			nflag = 1;
			break;
#ifdef DST_NONE
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
			while (*optarg != '\0')
				if (!isdigit(*optarg))
					usage();
			break;
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
			while (*optarg != '\0')
				if (!isdigit(*optarg))
					usage();
			break;
#endif /* defined DST_NONE */
		}
	}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else {
				(void) fprintf(stderr, 
					"date: error: multiple formats given\n");
				usage();
			}
		else	if (value == NULL)
				value = cp;
			else {
				(void) fprintf(stderr,
					"date: error: multiple values given\n");
				usage();
			}
	}
	if (value != NULL) {
		t = gettime(value, isdst);
		if (t == -1)
			usage();
	}
	/*
	** Entire command line has now been checked.
	*/
#ifdef DST_NONE
	if ((tflag || dflag) &&
		settimeofday((struct timeval *) NULL, &tz) != 0) {
			perror("date: error: settimeofday");
			retval = 1;
			display(format);
	}
#endif /* defined DST_NONE */
	if (value == NULL)
		display(format);
	username = getlogin();
	if (username == NULL || *username == '\0') /* single-user or no tty */
		username = "root";
	/*
	** XXX--shouldn't put the "before" entry into wtmp until we've
	** determined that the time-setting call has succeeded--but to
	** do that, we'd need to add a new parameter to logwtmp.
	**
	** Partial workaround would be to do a uid check before the first
	** write to wtmp.
	*/
#ifdef DST_NONE
	if (!nflag) {
		tv.tv_sec = t;
		if (netsettime(tv) != 1)
			exit(EXIT_FAILURE);
	}
	logwtmp(OTIME_MSG, TIME_USER, "");
	if (settimeofday(&tv, (struct timezone *) NULL) == 0) {
		logwtmp(NTIME_MSG, TIME_USER, "");
		syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", username);
	} else {
		perror("date: error: settimeofday");
		retval = EXIT_FAILURE;
	}
#else /* !defined DST_NONE */
	logwtmp(OTIME_MSG, TIME_USER, "");
	if (stime(&t) == 0)
		logwtmp(NTIME_MSG, TIME_USER, "");
	else {
		perror("date: error: stime");
		retval = EXIT_FAILURE;
	}
#endif /* !defined DST_NONE */

	display(format);
	for ( ; ; )
		;
}

#ifdef DST_NONE
static char	usemes[] = "\
date: usage is date [-uDSn][-d dst][-t mins_west] [[yy]mmddhhmm[yy][.ss]] [+fmt]\
";
#else /* !defined DST_NONE */
static char	usemes[] = "\
date: usage is date [-uDSn] [[yy]mmddhhmm[yy][.ss]] [+format]";
#endif /* !defined DST_NONE */

static void
usage()
{
	(void) fprintf(stderr, usemes);
	retval = EXIT_FAILURE;
	display((char *) NULL);
}

static void
display(format)
char *	format;
{
	struct tm	tm;

	tm = *localtime(&now);
	timeout((format == NULL) ? "%c" : format, &tm);
	(void) putchar('\n');
	(void) fflush(stdout);
	(void) fflush(stderr);
	if (ferror(stdout) || ferror(stderr)) {
		(void) fprintf(stderr, "date: error: couldn't write results\n");
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
timeout(format, tmp)
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
			(void) putchar(c);
			continue;
		}
		switch (c = *format++) {
		default:
			(void) fprintf(stderr,
				"date: error: bad format character - %c\n", c);
			retval = EXIT_FAILURE;
			display((char *) NULL);
		case 'a':
			(void) printf("%.3s", wday_names[tmp->tm_mon]);
			break;
		case 'A':
			(void) printf("%s", wday_names[tmp->tm_mon]);
			break;
		case 'b':
		case 'h':
			(void) printf("%.3s", mon_names[tmp->tm_mon]);
			break;
		case 'B':
			(void) printf("%s", mon_names[tmp->tm_mon]);
			break;
		case 'c':
			timeout("%x %X %Z %Y", tmp);
			break;
		case 'd':
			(void) printf("%02.2d", tmp->tm_mday);
			break;
		case 'D':
			timeout("%m/%d/%y", tmp);
			break;
		case 'H':
			(void) printf("%02.2d", tmp->tm_hour);
			break;
		case 'I':
			(void) printf("%02.2d", ((tmp->tm_hour - 1) % 12) + 1);
			break;
		case 'j':
			(void) printf("%03.3d", tmp->tm_yday + 1);
			break;
		case 'm':
			(void) printf("%02.2d", tmp->tm_mon + 1);
			break;
		case 'M':
			(void) printf("%02.2d", tmp->tm_min);
			break;
		case 'n':
			(void) putchar('\n');
			break;
		case 'p':
			(void) printf("%cM", (tmp->tm_hour >= 12) ? 'A' : 'P');
			break;
		case 'r':
			timeout("%I:%M:%S %p", tmp);
			break;
		case 'S':
			(void) printf("%02.2d", tmp->tm_sec);
			break;
		case 't':
			(void) putchar('\t');
			break;
		case 'T':
			timeout("%H:%M:%S", tmp);
			break;
		case 'U':
			/* How many Sundays fall on or before this day? */
			wday = tmp->tm_wday;
			(void) printf("%02.2d", (tmp->tm_yday + 7 - wday) / 7);
			break;
		case 'w':
			(void) printf("%d", tmp->tm_wday);
			break;
		case 'W':
			/* How many Mondays fall on or before this day? */
			/* Transform it to the Sunday problem and solve that */
			wday = tmp->tm_wday;
			if (--wday < 0)
				wday = 6;
			(void) printf("%02.2d",
				(tmp->tm_yday + 7 - wday) / 7);
			break;
		case 'x':
			timeout("%a %b %d", tmp);
			break;
		case 'X':
			timeout("%H:%M:%S", tmp);
			break;
		case 'y':
			(void) printf("%02.2d",
				(tmp->tm_year + TM_YEAR_BASE) % 100);
			break;
		case 'Y':
			(void) printf("%d", tmp->tm_year + TM_YEAR_BASE);
			break;
		case 'Z':
			(void) printf("%s", tzname[tmp->tm_isdst]);
			break;
		case '%':
			(void) putchar('%');
			break;
		}
	}
}

/*
** gettime --
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

#define YEAR_THIS_WAS_WRITTEN	1989

static time_t
gettime(cp, isdst)
register char *	cp;
int		isdst;
{
	register int	i;
	struct tm	tm;
	int		pairs[5];
	time_t		thist;
	time_t		thatt;

	if (isdst < 0) {
		thist = gettime(cp, 0);
		thatt = gettime(cp, 1);
		if (thist == -1)
			if (thatt == -1)
				return -1;
			else	return thatt;
		else	if (thatt == -1)
				return thist;
			else {
				(void) fprintf(stderr,
"date: error: ambiguous time--use -S/-D to tell if it's Daylight or Standard\n"
					);
				return -1;
			}
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
			tm.tm_mday = pairs[0] - 1;
			tm.tm_hour = pairs[1];
			tm.tm_min = pairs[2];
			return xtime(&tm);
		case 4:	/* mmddhhmm */
			tm.tm_mon = pairs[0] - 1;
			tm.tm_mday = pairs[1] - 1;
			tm.tm_hour = pairs[2];
			tm.tm_min = pairs[3];
			return xtime(&tm);
		case 5:	/* Ulp! yymmddhhmm or mmddhhmmyy */
			tm.tm_year = pairs[0];
			/*
			** Looking ten years down the road. . .
			*/
			if (tm.tm_year + TM_YEAR_BASE < YEAR_THIS_WAS_WRITTEN)
				tm.tm_year += 100;
			tm.tm_mon = pairs[1] - 1;
			tm.tm_mday = pairs[2] - 1;
			tm.tm_hour = pairs[3];
			tm.tm_min = pairs[4];
			thist = xtime(&tm);
			tm.tm_mon = pairs[0] - 1;
			tm.tm_mday = pairs[1] - 1;
			tm.tm_hour = pairs[2];
			tm.tm_min = pairs[3];
			tm.tm_year = pairs[4];
			/*
			** . . .makes for decadent logic.
			*/
			if (tm.tm_year + TM_YEAR_BASE < YEAR_THIS_WAS_WRITTEN)
				tm.tm_year += 100;
			thatt = xtime(&tm);
			if (thist == -1)
				if (thatt == -1)
					break;
				else	return thatt;
			else	if (thatt == -1)
					return thist;
				else {
					(void) fprintf(stderr,
"date: GOLLY: WHAT THE DICKENS DO I TELL THE USER AT THIS POINT?\n");
					display((char *) NULL);
				}
	}
	return -1;
}

#ifdef DST_NONE
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define TSPTYPES
#include <protocols/timed.h>

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
#endif /* defined DST_NONE */
