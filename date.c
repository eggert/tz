#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
/*
** UCB's date.c, plus BSD conditionalizing, pluse changes to use "mktime".
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

/*
** TO DO:  provide a way to disambiguate 1:30 a.m. on the day you "fall back"?
** 	   also:  provide a way to disambiguate between multiple noons
**	   in Riyadh, both of which are standard time.
*/

extern int	optind;

#ifndef USG

static struct timeval	tv;
static int	retval;

static void	display();

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
	char *username, *getlogin();
	time_t time();
	char *	format;
	char *	value;

	format = value = NULL;
	nflag = 0;
	tz.tz_dsttime = tz.tz_minuteswest = 0;
	format = NULL;
	while ((ch = getopt(argc, argv, "d:nut:")) != EOF)
		switch((char)ch) {
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
			exit(1);
		}
	argc -= optind;
	argv += optind;
	switch (argc) {
		default:
			usage();
		case 0:
			display(format);
		case 1:
			if (*argv[0] == '+')
				display(&argv[0][1]);
			value = argv[0];
			break;
		case 2:
			if ((*argv[0] == '+') == (*argv[1] == '+'))
				usage();
			if (*argv[0] == '+') {
				format = &argv[0][1];
				value = argv[1];
			} else {
				format = &argv[1][1];
				value = argv[0];
			}
	}
	if ((tz.tz_minuteswest || tz.tz_dsttime) &&
	    settimeofday((struct timeval *)NULL, &tz)) {
		perror("settimeofday");
		retval = 1;
		display(format);
	}

	if (gettimeofday(&tv, &tz)) {
		perror("gettimeofday");
		exit(1);
	}

	tv.tv_sec = gtime(value, (time_t) tv.tv_sec, -1);
	if (tv.tv_sec == -1) {
		usage();
		retval = 1;
		display(format);
	}

	if (nflag || !netsettime(tv)) {
		logwtmp("|", "date", "");
		if (settimeofday(&tv, (struct timezone *)NULL)) {
			perror("settimeofday");
			retval = 1;
			display(format);
		}
		logwtmp("{", "date", "");
	}

	username = getlogin();
	if (!username || *username == '\0')	/* single-user or no tty */
		username = "root";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", username);

	display();
	for ( ; ; )
		;
}

usage()
{
	fputs("usage: date [-nu] [-d dst] [-t minutes_west] [yymmddhhmm[.ss]]\n", stderr);
}

#else /* defined USG */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int	ch;
	time_t	time();
	time_t	t;
	char *	format;
	char *	value;

	format = value = NULL;
	while ((ch = getopt(argc, argv, "u")) != EOF)
		switch (ch) {
		case 'u':
			setgmt():
			break;
		default:
			usage();
			exit(1);
		}
	argc -= optind;
	argv += optind;
	switch (argc) {
		default:
			usage();
		case 0:
			display(format);
		case 1:
			if (*argv[0] == '+')
				display(&argv[0][1]);
			value = argv[0];
			break;
		case 2:
			if ((*argv[0] == '+') == (*argv[1] == '+'))
				usage();
			if (*argv[0] == '+') {
				format = &argv[0][1];
				value = argv[1];
			} else {
				formt = &argv[1][1];
				value = argv[0];
			}
	}
	(void) time(&t);
	t = gtime(value, t, -1);
	if (t == -1)
		usage();
	else {
		logwtmp("|", "date", "");
		if (stime(&t) == 0)
			logwtmp("{", "date", "");
		else {
			perror("stime");
			retval = 1;
		}
	}
	display(format);
}

usage()
{
	(void) fprintf(stderr, "date: usage is date [-u] yymmddhhmm[.ss]\n");
	exit(1);
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
	extern char *		tzname[2];

	(void) time(&t);
	if (format == NULL) {
		tp = localtime(&t);
		cp = asctime(tp);
		(void) printf("%.20s%s%s", cp, tzname[tp->tm_isdst], cp + 19);
	} else	timeout(format, t);
	/*
	** Should check stdout/stderr here.
	*/
	(void) exit(retval);
	for ( ; ; )
		;
}

static void
timeout(format, arg)
register char *	format;
time_t		arg;
{
	register int	c;
	struct tm	tm;
	char *		cp;

	tm = *localtime(&arg);
	cp = asctime(&tm);
	for ( ; ; ) {
		c = *format++;
		if (c == '\0') {
			(void) putchar('\n');
			return;
		}
		if (c != '%') {
			(void) putchar(c);
			continue;
		}
		switch (c = *format++) {
			default:
				(void) fprintf(stderr,
					"date: bad format character - %c\n", c);
				exit(1);
			case '%':
				(void) putchar(c);
				break;
			case 'n':
				(void) putchar('\n');
				break;
			case 't':
				(void) putchar('\t');
				break;
			case 'm':
				(void) printf("%02.2d", tm.tm_mon + 1);
				break;
			case 'd':
				(void) printf("%02.2d", tm.tm_mday);
				break;
			case 'y':
				(void) printf("%02.2d", tm.tm_year % 100);
				break;
			case 'D':
				(void) printf("%02.2d/%02.2d/%02.2d",
					tm.tm_mon + 1,
					tm.tm_mday,
					tm.tm_year % 100);
				break;
			case 'H':
				(void) printf("%02.2d", tm.tm_hour);
				break;
			case 'M':
				(void) printf("%02.2d", tm.tm_min);
				break;
			case 'S':
				(void) printf("%02.2d", tm.tm_sec);
				break;
			case 'T':
				(void) printf("%02.2d:%02.2d:%02.2d",
					tm.tm_hour, tm.tm_min, tm.tm_sec);
				break;
			case 'j':
				(void) printf("%03.3d", tm.tm_yday + 1);
				break;
			case 'w':
				(void) printf("%d", tm.tm_wday);
				break;
			case 'a':
				(void) printf("%03.3s", cp);
				break;
			case 'h':
				(void) printf("%03.3s", cp + 4);
				break;
			case 'r':
				{
					static int	ap, hour;

					hour = tm.tm_hour;
					if (hour >= 12) {
						ap = 'P';
						hour -= 12;
					} else	ap = 'A';
					(void) printf(
						"%02.2d:%02.2d:%02.2d %cM",
						hour, tm.tm_min, tm.tm_sec, ap);
				}
				break;
		}
	}
}

setgmt()
{
	register char **	saveenv;
	extern char **		environ;
	char *			fakeenv[2];

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
