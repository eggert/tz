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
				"%s: wild result from settz(\"%s\")\n",
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
					"%s: wild long timezone name %s\n",
					argv[0], argv[i]);
				exit(1);
			}
			(void) sprintf(buf, "%s/%s", TZDIR, argv[i]);
			fp = fopen(buf, "r");
		}
		if (fp == NULL) {
			(void) fprintf(stderr,
				"%s: wild result opening %s file\n",
				argv[0], argv[i]);
			exit(1);
		}
		if (fread((char *) &h, sizeof h, 1, fp) != 1) {
			(void) fprintf(stderr,
				"%s: wild result reading %s file\n",
				argv[0], argv[i]);
			exit(1);
		}
		tp = (long *) calloc((alloc_t) h.tzh_timecnt, sizeof *tp);
		if (tp == NULL) {
			(void) fprintf(stderr,
				"%s: wild result from calloc\n", argv[0]);
			exit(1);
		}
		if (h.tzh_timecnt != 0)
			if (fread((char *) tp, sizeof *tp, (int) h.tzh_timecnt,
				fp) != h.tzh_timecnt) {
				(void) fprintf(stderr,
					"%s: wild result reading %s file\n",
					argv[0], argv[i]);
				exit(1);
			}
		if (fclose(fp)) {
			(void) fprintf(stderr,
				"%s: wild result closing %s file\n",
				argv[0], argv[i]);
			exit(1);
		}
		for (j = 0; j < h.tzh_timecnt; ++j) {
			show(argv[i], tp[j] - 1, TRUE);
			show(argv[i], tp[j], TRUE);
		}
		free((char *) tp);
	}
	if (fflush(stdout) || ferror(stdout)) {
		(void) fprintf(stderr,
			"%s: wild result writing to standard output\n",
			argv[0]);
		exit(1);
	}
	exit(0);
}

static
show(zone, t, v)
char *	zone;
long	t;
{
	struct tm *		tmp;
	extern struct tm *	newlocaltime();

	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	tmp = newlocaltime(&t);
	(void) printf("%.24s", asctime(tmp));
	if (*tz_abbr != '\0')
		(void) printf(" %s", tz_abbr);
	if (v)
		(void) printf(" isdst=%d\n", tmp->tm_isdst);
	(void) printf("\n");
}
