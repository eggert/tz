#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#include "time.h"
#include "tzfile.h"

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

extern char *		asctime();
extern struct tm *	gmtime();
extern char *		optarg;
extern int		optind;
#ifdef strchr
extern char *		sprintf();
#endif
#ifndef strchr
extern int		sprintf();
#endif
extern long		time();
extern char *		tzname[2];

static int		longest;

static long
tzdecode(codep)
char *	codep;
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
	register FILE *	fp;
	register int	i, j, c;
	register int	vflag;
	register char *	cutoff;
	register int	cutyear;
	register long	cuttime;
	long		now;
	long		t;
	int		timecnt;
	char		buf[BUFSIZ];

	vflag = 0;
	cutoff = NULL;
	while ((c = getopt(argc, argv, "c:v")) == 'c' || c == 'v')
		if (c == 'v')
			vflag = 1;
		else	cutoff = optarg;
	if (c != EOF || optind == argc - 1 && strcmp(argv[optind], "=") == 0) {
		(void) fprintf(stderr, "%s: usage is %s [ -v ] zonename ...\n",
			argv[0], argv[0]);
		exit(1);
	}
	if (cutoff != NULL)
		cutyear = atoi(cutoff);
	/*
	** VERY approximate.
	*/
	cuttime = (long) (cutyear - 1970) * 60 * 60 * 24 * 365;
	(void) time(&now);
	longest = 0;
	for (i = optind; i < argc; ++i)
		if (strlen(argv[i]) > longest)
			longest = strlen(argv[i]);
	for (i = optind; i < argc; ++i) {
		if (settz(argv[i]) != 0) {
			(void) fprintf(stderr, "%s: settz(\"%s\") failed\n",
				argv[0], argv[i]);
			exit(1);
		}
		show(argv[i], now, FALSE);
		if (!vflag)
			continue;
		if (argv[i][0] == '/')
			fp = fopen(argv[i], "r");
		else {
			j = strlen(TZDIR) + 1 + strlen(argv[i]) + 1;
			if (j > sizeof buf) {
				(void) fprintf(stderr,
					"%s: timezone name %s/%s is too long\n",
					argv[0], TZDIR, argv[i]);
				exit(1);
			}
			(void) sprintf(buf, "%s/%s", TZDIR, argv[i]);
			fp = fopen(buf, "r");
		}
		if (fp == NULL) {
			(void) fprintf(stderr, "%s: Can't open ", argv[0]);
			perror(argv[i]);
			exit(1);
		}
		{
			struct tzhead *	tzhp;
			char		code[4];

			(void) fseek(fp, (long) sizeof tzhp->tzh_reserved, 0);
			if (fread((char *) code, sizeof code, 1, fp) != 1)
				readerr(fp, argv[0], argv[i]);
			timecnt = tzdecode(code);
			(void) fseek(fp, (long) (2 * sizeof code), 1);
		}
		t = 0x80000000;
		show(argv[i], t, TRUE);
		t = 0x80000000 + 24 * 60 * 60;
		show(argv[i], t, TRUE);
		for (j = 0; j < timecnt; ++j) {
			char	code[4];

			if (fread((char *) code, sizeof code, 1, fp) != 1)
				readerr(fp, argv[0], argv[i]);
			t = tzdecode(code);
			if (cutoff != NULL && t > cuttime)
				break;
			show(argv[i], t - 1, TRUE);
			show(argv[i], t, TRUE);
		}
		if (fclose(fp)) {
			(void) fprintf(stderr, "%s: Error closing ", argv[0]);
			perror(argv[i]);
			exit(1);
		}
		t = 0x7fffffff - 24 * 60 * 60;
		show(argv[i], t, TRUE);
		t = 0x7fffffff;
		show(argv[i], t, TRUE);
	}
	if (fflush(stdout) || ferror(stdout)) {
		(void) fprintf(stderr, "%s: Error writing standard output ",
			argv[0]);
		perror("standard output");
		exit(1);
	}
	return 0;
}

static
show(zone, t, v)
char *	zone;
long	t;
{
	struct tm *		tmp;
	extern struct tm *	localtime();

	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	tmp = localtime(&t);
	(void) printf("%.24s", asctime(tmp));
	if (*tzname[tmp->tm_isdst] != '\0')
		(void) printf(" %s", tzname[tmp->tm_isdst]);
	if (v)
		(void) printf(" isdst=%d", tmp->tm_isdst);
	(void) printf("\n");
}

static
readerr(fp, progname, filename)
FILE *	fp;
char *	progname;
char *	filename;
{
	(void) fprintf(stderr, "%s: Error reading ", progname);
	if (ferror(fp))
		perror(filename);
	else	(void) fprintf(stderr, "%s: Premature EOF\n", filename);
	exit(1);
}
