#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
/*
** Modified from the UCB version whose sccsid appears below.
*/
#endif /* !defined NOID */
#endif /* !defined lint */

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1

#define BEFORE		"|"
#define AFTER		"{"

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

/*
 * Date - print and set date
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <errno.h>
#include <syslog.h>
#include <utmp.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "tzfile.h"

/*
** TO DO:  provide a way to disambiguate 1:30 a.m. on the day you "fall back"?
** 	   also:  provide a way to disambiguate between multiple noons
**	   in Riyadh, both of which are standard time.
*/

extern int	optind;
static int	retval = EXIT_SUCCESS;

static void	display();

#ifndef USG

static struct timeval	tv;

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
	struct timezone tz;
	char *ap, *tzn;
	int ch, nflag;
	int	isdst, newdst;
	char * cp;
	char *username, *getlogin();
	time_t time();
	char *	format;
	char *	value;

	format = value = NULL;
	nflag = 0;
	tz.tz_dsttime = tz.tz_minuteswest = 0;
	isdst = -1;
	format = NULL;
	while ((ch = getopt(argc, argv, "d:nut:DS")) != EOF)
		switch (ch) {
		case 'D':
		case 'S':
			newdst = (ch == 'D') ? 1 : 0;
			if (isdst >= 0 && newdst != isdst)
				usage();
			isdst = newdst;
			break;
		case 'd':		/* daylight savings time */
			tz.tz_dsttime = atoi(optarg) ? 1 : 0;
			break;
		case 'n':		/* don't set network */
			nflag = 1;
			break;
		case 'u':		/* do it in GMT */
			setgmt();
			break;
		case 't':		/* minutes west of GMT */
					/* error check; we can't allow "PST" */
			if (isdigit(*optarg)) {
				tz.tz_minuteswest = atoi(optarg);
				break;
			}
			/*FALLTHROUGH*/
		default:
			usage();
		}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else	usage();
		else	if (value == NULL)
				value = cp;
			else	usage();
	}
	if ((tz.tz_minuteswest || tz.tz_dsttime) &&
	    settimeofday((struct timeval *)NULL, &tz)) {
		perror("settimeofday");
		retval = 1;
		display(format);
	}
	if (value == NULL)
		display(format);

	if (gettimeofday(&tv, &tz)) {
		perror("gettimeofday");
		(void) exit(EXIT_FAILURE);
	}

	tv.tv_sec = gtime(value, (time_t) tv.tv_sec, isdst);
	if (tv.tv_sec == -1) {
		usage();
		retval = 1;
		display(format);
	}

	if (nflag || !netsettime(tv)) {
		logwtmp(BEFORE, "date", "");
		if (settimeofday(&tv, (struct timezone *)NULL)) {
			perror("settimeofday");
			retval = 1;
			display(format);
		}
		logwtmp(AFTER, "date", "");
	}

	username = getlogin();
	if (!username || *username == '\0')	/* single-user or no tty */
		username = "root";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", username);

	display(format);
	for ( ; ; )
		;
}

usage()
{
	fputs("usage: date [-nu] [-d dst] [-D] [-S] [-t minutes_west] [yymmddhhmm[.ss]]\n", stderr);
	retval = EXIT_FAILURE;
}

#else /* defined USG */

int
main(argc, argv)
int	argc;
char *	argv[];
{
	char *	cp;
	int	ch;
	time_t	time();
	time_t	t;
	int	isdst;
	char *	format;
	char *	value;

	format = value = NULL;
	isdst = -1;
	while ((ch = getopt(argc, argv, "uDS")) != EOF)
		switch (ch) {
			case 'D':
			case 'S':
				newdst = (ch == 'D') ? 1 : 0;
				if (isdst >= 0 && newdst != isdst)
					usage();
				isdst = newdst;
				break;
			case 'u':
				setgmt():
				break;
			default:
				usage();
		}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else	usage();
		else	if (value == NULL)
				value = cp;
			else	usage();
	}
	if (value == NULL)
		display(format);
	(void) time(&t);
	t = gtime(value, t, isdst);
	if (t == -1)
		usage();
	logwtmp(BEFORE, "date", "");
	if (stime(&t) == 0)
		logwtmp(AFTER, "date", "");
	else {
		perror("stime");
		retval = 1;
	}
	display(format);
}

usage()
{
	(void) fprintf(stderr, "date: usage is date [-u] yymmddhhmm[.ss]\n");
	retval = EXIT_FAILURE;
	display(format);
}

#endif /* defined USG */

static void	timeout();

static void
display(format)
char *	format;
{
	register struct tm *	tp;
	register char *		cp;
	time_t			t;
	struct tm		tm;
	extern char *		tzname[2];

	(void) time(&t);
	tm = *localtime(&t);
	timeout((format == NULL) ? "%c" : format, &tm);
	(void) putchar('\n');
	/*
	** Should check stdout/stderr here.
	*/
	(void) exit(retval);
	for ( ; ; )
		;
}

static char *	wday_names[] = {
	"Sunday",	"Monday",	"Tuesday",	"Wednesday",
	"Thursday",	"Friday",	"Saturday"
};

static char *	month_names[] = {
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
				"date: bad format character - %c\n", c);
			(void) exit(EXIT_FAILURE);
		case 'a':
			(void) printf("%.3s", wday_names[tmp->tm_mon]);
			break;
		case 'A':
			(void) printf("%s", wday_names[tmp->tm_mon]);
			break;
		case 'b':
		case 'h':
			(void) printf("%.3s", month_names[tmp->tm_mon]);
			break;
		case 'B':
			(void) printf("%s", month_names[tmp->tm_mon]);
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
			(void) printf("%02.2d",
				(tmp->tm_yday + DAYSPERWEEK - wday) /
				DAYSPERWEEK);
			break;
		case 'w':
			(void) printf("%d", tmp->tm_wday);
			break;
		case 'W':
			/* How many Mondays fall on or before this day? */
			/* Transform it to the Sunday problem and solve that */
			wday = tmp->tm_wday;
			if (--wday < 0)
				wday = DAYSPERWEEK - 1;
			(void) printf("%02.2d",
				(tmp->tm_yday + DAYSPERWEEK - wday) /
				DAYSPERWEEK);
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

setgmt()
{
	register char **	saveenv;
	extern char **		environ;
	char *			fakeenv[2];
	int			wday;

	fakeenv[0] = "TZ=GMT0";
	fakeenv[1] = NULL;
	saveenv = environ;
	environ = fakeenv;
	tzset();
	environ = saveenv;
}

/*
 * gtime --
 *	convert user's input into a time_t.
 * Track the BSD behavior of treating
 * 	8903062415
 * as March 7, 1989, 12:15 a.m. rather than
 *    March 6, 1989, 12:15 a.m.
 */

#define PAIR(cp)	((*(cp) - '0') * 10 + *((cp) + 1) - '0')

static time_t
gtime(ap, t, isdst)
register char *	ap;
time_t		t;	/* time to use for filling in unspecified information */
int		isdst;
{
	register char *	cp;
	struct tm	intm, outtm;
	time_t		outt;
	int		saw24;
	int		okay;

	if (isdst < 0) {
		time_t	thist, thatt;

		thist = gtime(ap, t, 0);
		thatt = gtime(ap, t, 1);
		if (thist == -1)
			if (thatt == -1)
				return -1;
			else	return thatt;
		else	if (thatt == -1)
				return thist;
			else {
				(void) fprintf(stderr, "ambiguous time\n");
				return -1;
			}
	}
	for (cp = ap; *cp != '\0'; ++cp)
		if (!isdigit(*cp) && *cp != '.')
			return -1;
	intm = *localtime(&t);
	intm.tm_sec = 0;
	saw24 = 0;
	cp = ap;
	switch (strlen(cp)) {
		case 13:			/* yymmddhhmm.ss */
			if (cp[10] != '.')
				return -1;
			intm.tm_sec = PAIR(&cp[11]);
		case 10:			/* yymmddhhmm */
			intm.tm_year = PAIR(cp);
			cp += 2;
		case 8:				/* mmddhhmm */
			intm.tm_mon = PAIR(cp);
			cp += 2;
			--intm.tm_mon;
		case 6:				/* ddhhmm */
			intm.tm_mday = PAIR(cp);
			cp += 2;
		case 4:				/* hhmm */
			intm.tm_hour = PAIR(cp);
			cp += 2;
			if (intm.tm_hour == 24) {
				saw24 = 1;
				intm.tm_hour = 0;
				++intm.tm_mday;
			}
			intm.tm_min = PAIR(cp);
			break;
		default:
			return -1;
	}
	intm.tm_isdst = isdst;
	outtm = intm;
	outt = mktime(&outtm);
	if (outtm.tm_isdst != intm.tm_isdst ||
		outtm.tm_sec != intm.tm_sec ||
		outtm.tm_min != intm.tm_min ||
		outtm.tm_hour != intm.tm_hour)
			okay = 0;
	else if (outtm.tm_mday == intm.tm_mday &&
		outtm.tm_mon == intm.tm_mon &&
		outtm.tm_year == intm.tm_year)
			okay = 1;
	else if (!saw24)
			okay = 0;
	else if (outtm.tm_mday == intm.tm_mday + 1)
		okay = outtm.tm_mon == intm.tm_mon &&
			outtm.tm_year == intm.tm_year;
	else if (outtm.tm_mday != 1)
			okay = 0;
	else if (outtm.tm_mon == intm.tm_mon + 1)
			okay = outtm.tm_year == intm.tm_year;
	else if (outtm.tm_mon != 0)
			okay = 0;
	else		okay = outtm.tm_year == intm.tm_year + 1;
	return okay ? outt : -1;
}

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
#ifndef TSP_SETDATE
	return 0;
#else /* defined TSP_SETDATE */
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
		perror("gethostname");
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
			fprintf(stderr,
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
#endif /* defined TSP_SETDATE */
}
