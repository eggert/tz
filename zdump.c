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
static void     readerr P((FILE * fp,
			const char * progname, const char * filename));
static void	show P((const char * zone, time_t t, int v));
static long	tzdecode P((const char * buffer));

static long
tzdecode(codep)
const char *	codep;
{
	register int	i;
	register long	result;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

main(argc, argv)
int	argc;
char *	argv[];
{
	register FILE *		fp;
	register int		i, j, c;
	register int		vflag;
	register const char *	cutoff;
	register int		cutyear;
	register long		cuttime, k;
	time_t			now;
	time_t			t;
	long			leapcnt, timecnt;
	long			typecnt, charcnt;
	char			buf[FILENAME_MAX + 1];

	vflag = 0;
	cutoff = NULL;
	while ((c = getopt(argc, argv, "c:v")) == 'c' || c == 'v')
		if (c == 'v')
			vflag = 1;
		else	cutoff = optarg;
	if (c != EOF || optind == argc - 1 && strcmp(argv[optind], "=") == 0) {
		(void) fprintf(stderr, "%s: usage is %s [ -v ] zonename ...\n",
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
		environ = saveenv;
		show(argv[i], now, FALSE);
		if (!vflag)
			continue;
		if (argv[i][0] == '\0') {
			fp = NULL;
			timecnt = 0;
			leapcnt = 0;
		} else {
			struct tzhead	tzh;

			if (argv[i][0] == '/')
				fp = fopen(argv[i], "rb");
			else {
				j = strlen(TZDIR) + 1 + strlen(argv[i]) + 1;
				if (j > sizeof buf) {
					(void) fprintf(stderr,
"%s: timezone name %s/%s is too long\n",
						argv[0], TZDIR, argv[i]);
					(void) exit(EXIT_FAILURE);
				}
				(void) sprintf(buf, "%s/%s", TZDIR, argv[i]);
				fp = fopen(buf, "rb");
			}
			if (fp == NULL) {
				(void) fprintf(stderr, "%s: Can't open ",
					argv[0]);
				(void) perror(argv[i]);
				(void) exit(EXIT_FAILURE);
			}
			if (fread((genericptr_t) &tzh,
				(fread_size_t) sizeof tzh,
				(fread_size_t) 1, fp) != 1)
					readerr(fp, argv[0], argv[i]);
			leapcnt = tzdecode(tzh.tzh_leapcnt);
			timecnt = tzdecode(tzh.tzh_timecnt);
			typecnt = tzdecode(tzh.tzh_typecnt);
			charcnt = tzdecode(tzh.tzh_charcnt);
		}
		t = 0x80000000;
		if (t > 0)		/* time_t is unsigned */
			t = 0;
		show(argv[i], t, TRUE);
		t += SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
		for (k = timecnt; k > 0; --k) {
			char	code[4];

			if (fread((genericptr_t) code,
				(fread_size_t) sizeof code,
				(fread_size_t) 1, fp) != 1)
					readerr(fp, argv[0], argv[i]);
			t = tzdecode(code);
			if (cutoff != NULL && t > cuttime)
				break;
			show(argv[i], t - 1, TRUE);
			show(argv[i], t, TRUE);
		}
		if (fp != NULL)
			(void) fseek(fp, (long) sizeof (struct tzhead) +
				timecnt * 5 + typecnt * 6 + charcnt, 0);
		for (k = leapcnt; k > 0; --k) {
			char	code[4];

			if (fread((genericptr_t) code,
				(fread_size_t) sizeof code,
				(fread_size_t) 1, fp) != 1)
					readerr(fp, argv[0], argv[i]);
			(void) fseek(fp, (long) sizeof code, 1);
			t = tzdecode(code);
			if (cutoff != NULL && t > cuttime)
				break;
			show(argv[i], t - 1, TRUE);
			show(argv[i], t, TRUE);
			show(argv[i], t + 1, TRUE);
		}
		if (fp != NULL && fclose(fp)) {
			(void) fprintf(stderr, "%s: Error closing ", argv[0]);
			(void) perror(argv[i]);
			(void) exit(EXIT_FAILURE);
		}
		t = 0xffffffff;
		if (t < 0)		/* time_t is signed */
			t = 0x7fffffff ;
		t -= SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
		t += SECSPERHOUR * HOURSPERDAY;
		show(argv[i], t, TRUE);
		ifree(tzequals);
	}
	if (fflush(stdout) || ferror(stdout)) {
		(void) fprintf(stderr, "%s: Error writing standard output ",
			argv[0]);
		(void) perror("standard output");
		(void) exit(EXIT_FAILURE);
	}
	return 0;
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
#ifdef KRE_COMPAT
		(void) printf(" gmtoff=%ld", tmp->tm_gmtoff);
#endif /* KRE_COMPAT */
	}
	(void) printf("\n");
}

static void
readerr(fp, progname, filename)
FILE *	fp;
const char *	progname;
const char *	filename;
{
	(void) fprintf(stderr, "%s: Error reading ", progname);
	if (ferror(fp))
		(void) perror(filename);
	else	(void) fprintf(stderr, "%s: Premature EOF\n", filename);
	(void) exit(EXIT_FAILURE);
}
