#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#include "time.h"
#include "tzfile.h"

#ifndef alloc_t
#define alloc_t		unsigned
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

extern char *		asctime();
extern char *		calloc();
extern struct tm *	gmtime();
extern char *		newctime();
extern int		optind;
extern char *		sprintf();
extern long		time();
extern char *		tz_abbr;

static int		longest;

main(argc, argv)
int	argc;
char *	argv[];
{
	register FILE *	fp;
	register long *	tp;
	register int	i, j, c;
	register int	vflag;
	long		now;
	struct tzhead	h;
	char		buf[BUFSIZ];

	vflag = 0;
	while ((c = getopt(argc, argv, "v")) == 'v')
		vflag = 1;
	if (c != EOF || optind == argc - 1 && strcmp(argv[optind], "=") == 0) {
		(void) fprintf(stderr, "%s: usage is %s [ -v ] zonename ...\n",
			argv[0], argv[0]);
		exit(1);
	}
	(void) time(&now);
	longest = 0;
	for (i = optind; i < argc; ++i)
		if (strlen(argv[i]) > longest)
			longest = strlen(argv[i]);
	for (i = optind; i < argc; ++i) {
		if (settz(argv[i]) != 0) {
			(void) fprintf(stderr,
				"%s: settz(\"%s\") failed\n",
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
			(void) fprintf(stderr,
				"%s: Can't open ", argv[0]);
			perror(argv[i]);
			exit(1);
		}
		if (fread((char *) &h, sizeof h, 1, fp) != 1)
			readerr(fp, argv[0], argv[i]);
		tp = (long *) calloc((alloc_t) h.tzh_timecnt, sizeof *tp);
		if (tp == NULL) {
			perror(argv[0]);
			exit(1);
		}
		if (h.tzh_timecnt != 0)
			if (fread((char *) tp, sizeof *tp, (int) h.tzh_timecnt,
				fp) != h.tzh_timecnt)
				readerr(fp, argv[0], argv[i]);
		if (fclose(fp)) {
			(void) fprintf(stderr, "%s: Error closing ", argv[0]);
			perror(argv[i]);
			exit(1);
		}
		for (j = 0; j < h.tzh_timecnt; ++j) {
			show(argv[i], tp[j] - 1, TRUE);
			show(argv[i], tp[j], TRUE);
		}
		free((char *) tp);
	}
	if (fflush(stdout) || ferror(stdout)) {
		(void) fprintf(stderr, "%s: Error writing standard output ",
			progname);
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
	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	(void) printf("%.24s", newctime(&t));
	if (*tz_abbr != '\0')
		(void) printf(" %s", tz_abbr);
	if (v)
		(void) printf(" isdst=%d\n", tmp->tm_isdst);
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
	else
		(void) fprintf(stderr, "%s: Premature EOF\n", filename);
	exit(1);
}
