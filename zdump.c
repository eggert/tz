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
extern char *		newctime();
extern int		optind;
extern char *		sprintf();
extern long		time();
extern char *		tz_abbr;

#ifdef lint
int			optind;
#endif

static int		longest;

#define	GETSHORT(val, p) { \
	register int shortval; \
	shortval = *p++; \
	shortval = (shortval << 8) | *p++; \
	val = shortval; \
	}

#define GETLONG(val, p) { \
	register long longval; \
	longval = *p++; \
	longval = (longval << 8) | *p++; \
	longval = (longval << 8) | *p++; \
	longval = (longval << 8) | *p++; \
	val = longval; \
	}
	
main(argc, argv)
int	argc;
char *	argv[];
{
	register FILE *	fp;
	register int	i, j, c;
	register int	vflag;
	long		now;
	int		timecnt;
	char		buf[BUFSIZ];

#ifdef lint
	(void) ftell(stdin);
#endif
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
			register unsigned char *	p;
			unsigned char			two[2];
			struct tzhead			h;

			(void) fseek(fp, (long) sizeof h.tzh_reserved, 0);
			if (fread((char *) two, sizeof two, 1, fp) != 1)
				readerr(fp, argv[0], argv[i]);
			p = two;
			GETSHORT(timecnt, p);
			(void) fseek(fp, (long) (2 * sizeof (short)), 1);
		}
		for (j = 0; j < timecnt; ++j) {
			register unsigned char *	p;
			unsigned char			four[4];
			long				t;

			if (fread((char *) four, sizeof four, 1, fp) != 1)
				readerr(fp, argv[0], argv[i]);
			p = four;
			GETLONG(t, p);
			show(argv[i], t - 1, TRUE);
			show(argv[i], t, TRUE);
		}
		if (fclose(fp)) {
			(void) fprintf(stderr, "%s: Error closing ", argv[0]);
			perror(argv[i]);
			exit(1);
		}
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
	extern struct tm *	newlocaltime();

	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	tmp = newlocaltime(&t);
	(void) printf("%.24s", asctime(tmp));
	if (*tz_abbr != '\0')
		(void) printf(" %s", tz_abbr);
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
