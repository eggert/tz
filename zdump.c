#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#include "timezone.h"

extern char *	newctime();
extern int	optind;
extern char *	sprintf();
extern long	time();

main(argc, argv)
int	argc;
char *	argv[];
{
	register FILE *	fp;
	register int	i, j, c;
	int		vflag;
	long		now;
	struct tzinfo 	t;
	char		buf[BUFSIZ];

	vflag = 0;
	while ((c = getopt(argc, argv, "v")) == 'v')
		vflag = 1;
	if (c != EOF || optind == argc - 1 && strcmp(argv[optind], "=") == 0) {
		(void) fprintf(stderr, "%s: usage is %s zonename ...\n",
			argv[0], argv[0]);
		exit(1);
	}
	(void) time(&now);
	for (i = optind; i < argc; ++i) {
		if (settz(argv[i]) != 0) {
(void) fprintf(stderr, "%s: wild result from settz(\"%s\")\n",
			argv[0], argv[i]);
			exit(1);
		}
		(void) printf("%s: %s", argv[i], newctime(&now));
		if (!vflag)
			continue;
		(void) sprintf(buf, "%s/%s", TZDIR, argv[i]);
		if ((fp = fopen(buf, "r")) == NULL &&
			(fp = fopen(argv[i], "r")) == NULL) {
(void) fprintf(stderr, "%s: wild result opening %s file\n",
					argv[0], argv[i]);
				exit(1);
		}
		if (fread((char *) &t, sizeof t, 1, fp) != 1) {
(void) fprintf(stderr, "%s: wild result reading %s file\n",
					argv[0], argv[i]);
				exit(1);
		}
		if (fclose(fp)) {
(void) fprintf(stderr, "%s: wild result closing %s file\n",
					argv[0], argv[i]);
				exit(1);
		}
		for (j = 0; j < t.tz_rulecnt; ++j)
			(void) printf("%s: %s", argv[i],
				newctime(&t.tz_times[j]));
	}
	return 0;
}
