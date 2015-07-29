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
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "private.h"
#include "locale.h"

/*
** The two things date knows about time are. . .
*/

#ifndef TM_YEAR_BASE
#define TM_YEAR_BASE	1900
#endif /* !defined TM_YEAR_BASE */

#ifndef SECSPERMIN
#define SECSPERMIN	60
#endif /* !defined SECSPERMIN */

extern char **		environ;
extern char *		optarg;
extern int		optind;
extern char *		tzname[2];

static int		retval = EXIT_SUCCESS;

static void		display(const char *, time_t);
static void		dogmt(void);
static void		errensure(void);
static void		timeout(FILE *, const char *, const struct tm *);
static void		usage(void);

int
main(const int argc, char *argv[])
{
	register const char *	format;
	register const char *	cp;
	register int		ch;
	register bool		rflag = false;
	time_t			t;
	intmax_t		secs;
	char *			endarg;

#ifdef LC_ALL
	setlocale(LC_ALL, "");
#endif /* defined(LC_ALL) */
#if HAVE_GETTEXT
#ifdef TZ_DOMAINDIR
	bindtextdomain(TZ_DOMAIN, TZ_DOMAINDIR);
#endif /* defined(TEXTDOMAINDIR) */
	textdomain(TZ_DOMAIN);
#endif /* HAVE_GETTEXT */
	t = time(NULL);
	format = NULL;
	while ((ch = getopt(argc, argv, "ucr:")) != EOF && ch != -1) {
		switch (ch) {
		default:
			usage();
		case 'u':		/* do it in UT */
		case 'c':
			dogmt();
			break;
		case 'r':		/* seconds since 1970 */
			if (rflag) {
				fprintf(stderr,
					_("date: error: multiple -r's used"));
				usage();
			}
			rflag = true;
			errno = 0;
			secs = strtoimax (optarg, &endarg, 0);
			if (*endarg || optarg == endarg)
				errno = EINVAL;
			else if (! (time_t_min <= secs && secs <= time_t_max))
				errno = ERANGE;
			if (errno) {
				perror(optarg);
				errensure();
				exit(retval);
			}
			t = secs;
			break;
		}
	}
	while (optind < argc) {
		cp = argv[optind++];
		if (*cp == '+')
			if (format == NULL)
				format = cp + 1;
			else {
				fprintf(stderr,
_("date: error: multiple formats in command line\n"));
				usage();
			}
		else {
		  fprintf(stderr, _("date: unknown operand: %s\n"), cp);
		  usage();
		}
	}

	display(format, t);
	return retval;
}

static void
dogmt(void)
{
	static char **	fakeenv;

	if (fakeenv == NULL) {
		register int	from;
		register int	to;
		register int	n;
		static char	tzegmt0[] = "TZ=GMT0";

		for (n = 0;  environ[n] != NULL;  ++n)
			continue;
		fakeenv = malloc((n + 2) * sizeof *fakeenv);
		if (fakeenv == NULL) {
			perror(_("Memory exhausted"));
			errensure();
			exit(retval);
		}
		to = 0;
		fakeenv[to++] = tzegmt0;
		for (from = 1; environ[from] != NULL; ++from)
			if (strncmp(environ[from], "TZ=", 3) != 0)
				fakeenv[to++] = environ[from];
		fakeenv[to] = NULL;
		environ = fakeenv;
	}
}

static void
errensure(void)
{
	if (retval == EXIT_SUCCESS)
		retval = EXIT_FAILURE;
}

static void
usage(void)
{
	fprintf(stderr,
		       _("date: usage: date [-u] [-c] [-r seconds]"
			 " [+format]\n"));
	errensure();
	exit(retval);
}

static void
display(char const *format, time_t now)
{
	struct tm *tmp;

	tmp = localtime(&now);
	if (!tmp) {
		fprintf(stderr,
			_("date: error: time out of range\n"));
		errensure();
		return;
	}
	timeout(stdout, format ? format : "%+", tmp);
	putchar('\n');
	fflush(stdout);
	fflush(stderr);
	if (ferror(stdout) || ferror(stderr)) {
		fprintf(stderr,
			_("date: error: couldn't write results\n"));
		errensure();
	}
}

#define INCR	1024

static void
timeout(FILE *fp, char const *format, struct tm const *tmp)
{
	char *	cp;
	size_t	result;
	size_t	size;
	struct tm tm;

	if (*format == '\0')
		return;
	if (!tmp) {
		fprintf(stderr, _("date: error: time out of range\n"));
		errensure();
		return;
	}
	tm = *tmp;
	tmp = &tm;
	size = INCR;
	cp = malloc(size);
	for ( ; ; ) {
		if (cp == NULL) {
			fprintf(stderr,
				_("date: error: can't get memory\n"));
			errensure();
			exit(retval);
		}
		cp[0] = '\1';
		result = strftime(cp, size, format, tmp);
		if (result != 0 || cp[0] == '\0')
			break;
		size += INCR;
		cp = realloc(cp, size);
	}
	fwrite(cp, 1, result, fp);
	free(cp);
}
