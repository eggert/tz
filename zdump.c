#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

#include "stdio.h"
#include "time.h"
#include "tzfile.h"
#include "string.h"
#include "stdlib.h"
#include "nonstd.h"

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !defined TRUE */

extern char **	environ;
extern void	ifree P((char * p));
extern char *	imalloc P((int n));
extern int	getopt P((int argc, char * argv[], char * options));
extern char *	optarg;
extern int	optind;
extern char *	tzname[2];
extern void	tzset P((void));

static int	longest;
static void	show P((const char * zone, time_t t, int v));
static void	hunt P((const char * name, time_t lot, time_t hit));
static long	delta P((const struct tm * newp, const struct tm * oldp));

int
main(argc, argv)
int	argc;
char *	argv[];
{
	register int		i, c;
	register int		vflag;
	register const char *	cutoff;
	register int		cutyear;
	register long		cuttime;
	time_t			now;
	time_t			t, newt;
	struct tm		tm, newtm;

	vflag = 0;
	cutoff = NULL;
	while ((c = getopt(argc, argv, "c:v")) == 'c' || c == 'v')
		if (c == 'v')
			vflag = 1;
		else	cutoff = optarg;
	if (c != EOF || optind == argc - 1 && strcmp(argv[optind], "=") == 0) {
		(void) fprintf(stderr,
			"%s: usage is %s [ -v ] [ -c cutoff ] zonename ...\n",
			argv[0], argv[0]);
		(void) exit(EXIT_FAILURE);
	}
	if (cutoff != NULL)
		cutyear = atoi(cutoff);
	/*
	** VERY approximate.
	*/
	cuttime = (long) (cutyear - EPOCH_YEAR) *
		SECSPERHOUR * HOURSPERDAY * DAYSPERNYEAR;
	(void) time(&now);
	longest = 0;
	for (i = optind; i < argc; ++i)
		if (strlen(argv[i]) > longest)
			longest = strlen(argv[i]);
	for (i = optind; i < argc; ++i) {
		register char **	saveenv;
		char *			tzequals;
		char *			fakeenv[2];

		tzequals = imalloc(strlen(argv[i]) + 4);
		if (tzequals == NULL) {
			(void) fprintf(stderr, "%s: can't allocate memory\n",
				argv[0]);
			(void) exit(EXIT_FAILURE);
		}
		(void) sprintf(tzequals, "TZ=%s", argv[i]);
		fakeenv[0] = tzequals;
		fakeenv[1] = NULL;
		saveenv = environ;
		environ = fakeenv;
		(void) tzset();
		ifree(tzequals);
		environ = saveenv;
		show(argv[i], now, FALSE);
		if (!vflag)
			continue;
		t = 0x80000000;
		if (t > 0)		/* time_t is unsigned */
			t = 0;
		show(argv[i], t, TRUE);
		t += SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
		tm = *localtime(&t);
		for ( ; ; ) {
			if (cutoff != NULL && t >= cuttime)
				break;
			newt = t + SECSPERHOUR * 12;
			if (cutoff != NULL && newt >= cuttime)
				break;
			if (newt <= t)
				break;
			newtm = *localtime(&newt);
			if (delta(&newtm, &tm) != (newt - t))
				hunt(argv[i], t, newt);
			t = newt;
			tm = newtm;
		}
		t = 0xffffffff;
		if (t < 0)		/* time_t is signed */
			t = 0x7fffffff ;
		t -= SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
		t += SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
	}
	if (fflush(stdout) || ferror(stdout)) {
		(void) fprintf(stderr, "%s: Error writing standard output ",
			argv[0]);
		(void) perror("standard output");
		(void) exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
	for ( ; ; )
		;
}

static void
hunt(name, lot, hit)
const char *	name;
time_t		lot;
time_t		hit;
{
	time_t		t;
	struct tm	lotm;
	struct tm	tm;

	lotm = *localtime(&lot);
	while ((hit - lot) >= 2) {
		t = lot / 2 + hit / 2;
		if (t <= lot)
			++t;
		else if (t >= hit)
			--t;
		tm = *localtime(&t);
		if (delta(&tm, &lotm) == (t - lot)) {
			lot = t;
			lotm = tm;
		} else	hit = t;
	}
	show(name, lot, TRUE);
	show(name, hit, TRUE);
}

static long
delta(newp, oldp)
const struct tm *	newp;
const struct tm *	oldp;
{
	long	result;

	result = newp->tm_hour - oldp->tm_hour;
	if (result < 0)
		result += HOURSPERDAY;
	result *= SECSPERHOUR;
	result += (newp->tm_min - oldp->tm_min) * SECSPERMIN;
	return result + newp->tm_sec - oldp->tm_sec;
}

static void
show(zone, t, v)
const char *	zone;
time_t		t;
{
	const struct tm *	tmp;
	extern struct tm *	localtime();

	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	tmp = localtime(&t);
	(void) printf("%.24s", asctime(tmp));
	if (*tzname[tmp->tm_isdst] != '\0')
		(void) printf(" %s", tzname[tmp->tm_isdst]);
	if (v) {
		(void) printf(" isdst=%d", tmp->tm_isdst);
#ifdef TM_GMTOFF
		(void) printf(" gmtoff=%ld", tmp->TM_GMTOFF);
#endif /* defined TM_GMTOFF */
	}
	(void) printf("\n");
}
