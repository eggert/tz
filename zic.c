#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "@(#)tzcomp.c	2.8";
#endif

#include "tzfile.h"
#include "ctype.h"

#ifndef alloc_t
#define alloc_t	unsigned
#endif

#ifndef BUFSIZ
#define BUFSIZ	1024
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifdef lint
#define scheck(string, format)	(format)
#endif
#ifndef lint
extern char *	scheck();
#endif

extern char *	calloc();
extern char *	malloc();
extern char *	optarg;
extern int	optind;
extern FILE *	popen();
extern char *	realloc();
extern char *	sprintf();
extern char *	strcat();
extern char *	strchr();
extern char *	strcpy();

#ifdef lint
char *		optarg;
int		optind;
#endif

static int	errors;
static char *	filename;
static char *	rfilename;
static char **	getfields();
static int	linenum;
static int	rlinenum;
static char *	progname;
static long	rpytime();
static long	tadd();

#define SECS_PER_MIN	60L
#define MINS_PER_HOUR	60L
#define HOURS_PER_DAY	24L
#define DAYS_PER_YEAR	365L	/* Except in leap years */
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	(SECS_PER_HOUR * HOURS_PER_DAY)
#define SECS_PER_YEAR	(SECS_PER_DAY * DAYS_PER_YEAR)

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

/*
** Values a la localtime(3)
*/

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

/*
** Line codes.
*/

#define LC_RULE		0
#define LC_ZONE		1
#define LC_LINK		2

/*
** Which fields are which on a Zone line.
*/

#define ZF_NAME		1
#define ZF_GMTOFF	2
#define ZF_RULE		3
#define ZF_FORMAT	4
#define ZF_UNTILYEAR	5
#define ZF_UNTILMONTH	6
#define ZF_UNTILDAY	7
#define ZF_UNTILTIME	8
#define ZONE_FIELDS	5
#define ZONE_UNTILYEAR	6
#define ZONE_UNTILMONTH	7
#define ZONE_UNTILDAY	8
#define ZONE_UNTILTIME	9

/*
** Which fields are which on a Zone continuation line.
*/

#define ZFC_GMTOFF		0
#define ZFC_RULE		1
#define ZFC_FORMAT		2
#define ZFC_UNTILYEAR		3
#define ZFC_UNTILMONTH		4
#define ZFC_UNTILDAY		5
#define ZFC_UNTILTIME		6
#define ZONEC_FIELDS		3
#define ZONEC_UNTILYEAR		4
#define ZONEC_UNTILMONTH	5
#define ZONEC_UNTILDAY		6
#define ZONEC_UNTILTIME		7

/*
** Which files are which on a Rule line.
*/

#define RF_NAME		1
#define RF_LOYEAR	2
#define RF_HIYEAR	3
#define RF_COMMAND	4
#define RF_MONTH	5
#define RF_DAY		6
#define RF_TOD		7
#define RF_STDOFF	8
#define RF_ABBRVAR	9
#define RULE_FIELDS	10

/*
** Which fields are which on a Link line.
*/

#define LF_FROM		1
#define LF_TO		2
#define LINK_FIELDS	3

struct rule {
	char *	r_filename;
	int	r_linenum;
	char *	r_name;

	long	r_loyear;	/* for example, 1986 */
	long	r_hiyear;	/* for example, 1986 */
	char *	r_yrtype;

	long	r_month;	/* 0..11 */

	int	r_dycode;	/* see below */
	long	r_dayofmonth;
	long	r_wday;

	long	r_tod;		/* time from midnight */
	int	r_todisstd;	/* above is standard time if TRUE */
				/* above is wall clock time if FALSE */
	long	r_stdoff;	/* offset from standard time */
	char *	r_abbrvar;	/* variable part of time zone abbreviation */
};

/*
**	r_dycode		r_dayofmonth	r_wday
*/
#define DC_DOM		0	/* 1..31 */	/* unused */
#define DC_DOWGEQ	1	/* 1..31 */	/* 0..6 (Sun..Sat) */
#define DC_DOWLEQ	2	/* 1..31 */	/* 0..6 (Sun..Sat) */

static struct rule *	rules;
static int		nrules;	/* number of rules */

struct zone {
	char *		z_filename;
	int		z_linenum;

	char *		z_name;
	long		z_gmtoff;
	char *		z_rule;
	char *		z_format;
	long		z_until;

	struct rule *	z_rules;
	int		z_nrules;
};

static struct zone *	zones;
static int		nzones;	/* number of zones */

struct link {
	char *		l_filename;
	int		l_linenum;
	char *		l_from;
	char *		l_to;
};

static struct link *	links;
static int		nlinks;

struct lookup {
	char *		l_word;
	long		l_value;
};

static struct lookup *	byword();

static struct lookup	line_codes[] = {
	"Rule",		LC_RULE,
	"Zone",		LC_ZONE,
	"Link",		LC_LINK,
	NULL,		0
};

static struct lookup	mon_names[] = {
	"January",	TM_JANUARY,
	"February",	TM_FEBRUARY,
	"March",	TM_MARCH,
	"April",	TM_APRIL,
	"May",		TM_MAY,
	"June",		TM_JUNE,
	"July",		TM_JULY,
	"August",	TM_AUGUST,
	"September",	TM_SEPTEMBER,
	"October",	TM_OCTOBER,
	"November",	TM_NOVEMBER,
	"December",	TM_DECEMBER,
	NULL,		0
};

static struct lookup	wday_names[] = {
	"Sunday",	TM_SUNDAY,
	"Monday",	TM_MONDAY,
	"Tuesday",	TM_TUESDAY,
	"Wednesday",	TM_WEDNESDAY,
	"Thursday",	TM_THURSDAY,
	"Friday",	TM_FRIDAY,
	"Saturday",	TM_SATURDAY,
	NULL,		0
};

static struct lookup	lasts[] = {
	"last-Sunday",		TM_SUNDAY,
	"last-Monday",		TM_MONDAY,
	"last-Tuesday",		TM_TUESDAY,
	"last-Wednesday",	TM_WEDNESDAY,
	"last-Thursday",	TM_THURSDAY,
	"last-Friday",		TM_FRIDAY,
	"last-Saturday",	TM_SATURDAY,
	NULL,			0
};

static long	mon_lengths[] = {	/* ". . .knuckles are 31. . ." */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static struct tzhead	h;
static long		ats[TZ_MAX_TIMES];
static unsigned char	types[TZ_MAX_TIMES];
static struct ttinfo	ttis[TZ_MAX_TYPES];
static char		chars[TZ_MAX_CHARS];

/*
** Memory allocation.
*/

static char *
emalloc(size)
{
	register char *	cp;

	if ((cp = malloc((alloc_t) size)) == NULL) {
		perror(progname);
		exit(1);
	}
	return cp;
}

static char *
erealloc(ptr, size)
char *	ptr;
{
	register char *	cp;

	if ((cp = realloc(ptr, (alloc_t) size)) == NULL) {
		perror(progname);
		exit(1);
	}
	return cp;
}

static char *
ecpyalloc(old)
char *	old;
{
	register char *	new;

	if (old == NULL)
		old = "";
	new = emalloc(strlen(old) + 1);
	(void) strcpy(new, old);
	return new;
}

static
usage()
{
	(void) fprintf(stderr,
"%s: usage is %s [ -l localtime ] [ -d directory ] [ filename ... ]\n",
		progname, progname);
	exit(1);
}

static char *	localtime = NULL;
static char *	directory = NULL;

main(argc, argv)
int	argc;
char *	argv[];
{
	register int	i, j;
	register int	c;

#ifdef lint
	(void) ftell(stdin);
#endif
	progname = argv[0];
	while ((c = getopt(argc, argv, "d:l:")) != EOF)
		switch (c) {
			default:
				usage();
			case 'd':
				if (directory == NULL)
					directory = optarg;
				else {
					(void) fprintf(stderr,
"%s: More than one -d option specified\n",
						progname);
					exit(1);
				}
				break;
			case 'l':
				if (localtime == NULL)
					localtime = optarg;
				else {
					(void) fprintf(stderr,
"%s: More than one -l option specified\n",
						progname);
					exit(1);
				}
		}
	if (directory == NULL)
		directory = TZDIR;
	if (optind == argc - 1 && strcmp(argv[optind], "=") == 0)
		usage();	/* usage message by request */
	zones = (struct zone *) emalloc(0);
	rules = (struct rule *) emalloc(0);
	links = (struct link *) emalloc(0);
	for (i = optind; i < argc; ++i)
		infile(argv[i]);
	if (errors)
		exit(1);
	associate();
	for (i = 0; i < nzones; i = j) {
		/*
		 * Find the next non-continuation zone entry.
		 */
		for (j = i + 1; j < nzones && zones[j].z_name == NULL; j++)
			;
		outzone(&zones[i], j - i);
	}
	/*
	** We'll take the easy way out on this last part.
	*/
	if (chdir(directory) != 0) {
		(void) fprintf(stderr, "%s: Can't chdir to ", progname);
		perror(directory);
		exit(1);
	}
	for (i = 0; i < nlinks; ++i) {
		(void) unlink(links[i].l_to);
		if (link(links[i].l_from, links[i].l_to) != 0) {
			(void) fprintf(stderr, "%s: Can't link %s to ",
				progname, links[i].l_from);
			perror(links[i].l_to);
			exit(1);
		}
	}
	if (localtime != NULL) {
		(void) unlink(TZDEFAULT);
		if (link(localtime, TZDEFAULT) != 0) {
			(void) fprintf(stderr, "%s: Can't link %s to ",
				progname, localtime);
			perror(TZDEFAULT);
			exit(1);
		}
	}
	exit(0);
}

/*
** Associate sets of rules with zones.
*/

/*
** Sort by rule name, and by magnitude of standard time offset for rules of
** the same name.  The second sort gets standard time entries to the start
** of the dsinfo table (and we want a standard time entry at the start of
** the table, since the first entry gets used for times not covered by the
** rules).
*/

static
rcomp(cp1, cp2)
char *	cp1;
char *	cp2;
{
	register struct rule *	rp1;
	register struct rule *	rp2;
	register long		l1, l2;
	register int		diff;

	rp1 = (struct rule *) cp1;
	rp2 = (struct rule *) cp2;
	if ((diff = strcmp(rp1->r_name, rp2->r_name)) != 0)
		return diff;
	if ((l1 = rp1->r_stdoff) < 0)
		l1 = -l1;
	if ((l2 = rp2->r_stdoff) < 0)
		l2 = -l2;
	if (l1 > l2)
		return 1;
	else if (l1 < l2)
		return -1;
	else	return 0;
}

static
associate()
{
	register struct zone *	zp;
	register struct rule *	rp;
	register int		base, out;
	register int		i;

	if (nrules != 0)
		(void) qsort((char *) rules, nrules, sizeof *rules, rcomp);
	base = 0;
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		zp->z_rules = NULL;
		zp->z_nrules = 0;
	}
	while (base < nrules) {
		rp = &rules[base];
		for (out = base + 1; out < nrules; ++out)
			if (strcmp(rp->r_name, rules[out].r_name) != 0)
				break;
		for (i = 0; i < nzones; ++i) {
			zp = &zones[i];
			if (strcmp(zp->z_rule, rp->r_name) != 0)
				continue;
			zp->z_rules = rp;
			zp->z_nrules = out - base;
		}
		base = out;
	}
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		if (*zp->z_rule != '\0' && zp->z_nrules == 0) {
			filename = zp->z_filename;
			linenum = zp->z_linenum;
			rfilename = NULL;
			rlinenum = 0;
			error("unruly zone");
		}
	}
	if (errors)
		exit(1);
}

static
error(string)
char *	string;
{
	(void) fprintf(stderr, "%s: file \"%s\", line %d: %s\n",
		progname, filename, linenum, string);
	if (rfilename != NULL)
		(void) fprintf(stderr, "%s: rule from file \"%s\", line %d\n",
			progname, rfilename, rlinenum);
	++errors;
}

static
infile(name)
char *	name;
{
	register FILE *			fp;
	register char **		fields;
	register char *			cp;
	register struct lookup *	lp;
	register int			nfields;
	char				buf[BUFSIZ];
	int				continuation;

	if (strcmp(name, "-") == 0) {
		name = "standard input";
		fp = stdin;
	} else if ((fp = fopen(name, "r")) == NULL) {
		(void) fprintf(stderr, "%s: Can't open ", progname);
		perror(name);
		exit(1);
	}
	filename = ecpyalloc(name);
	rfilename = NULL;
	continuation = 0;
	for (linenum = 1; ; ++linenum) {
		if (fgets(buf, sizeof buf, fp) != buf)
			break;
		cp = strchr(buf, '\n');
		if (cp == NULL) {
			error("line too long");
			exit(1);
		}
		*cp = '\0';
		fields = getfields(buf);
			/* can't return NULL, since "buf" is not NULL */
		nfields = 0;
		while (fields[nfields] != NULL) {
			if (ciequal(fields[nfields], "-"))
				fields[nfields] = "";
			++nfields;
		}
		if (nfields > 0) {	/* non-blank line */
			if (continuation != 0) {
				switch (continuation) {
				case LC_ZONE:
					if (inzcont(fields, nfields))
						continuation = LC_ZONE;
					else	continuation = 0;
					break;
				default:	/* "cannot happen" */
					(void) fprintf(stderr,
"%s: panic: Invalid continuation type %d\n",
						progname, continuation);
					exit(1);
				}
			} else {
				lp = byword(fields[0], line_codes);
				if (lp == NULL)
					error("input line of unknown type");
				else {
					switch ((int) (lp->l_value)) {
					case LC_RULE:
						inrule(fields, nfields);
						continuation = 0;
						break;
					case LC_ZONE:
						if (inzone(fields, nfields))
							continuation = LC_ZONE;
						else	continuation = 0;
						break;
					case LC_LINK:
						inlink(fields, nfields);
						continuation = 0;
						break;
					default:	/* "cannot happen" */
						(void) fprintf(stderr,
"%s: panic: Invalid l_value %ld\n",
							progname, lp->l_value);
						exit(1);
					}
				}
			}
		}
		free((char *) fields);
	}
	if (ferror(fp)) {
		(void) fprintf(stderr, "%s: Error reading ", progname);
		perror(filename);
		exit(1);
	}
	if (fclose(fp)) {
		(void) fprintf(stderr, "%s: Error closing ", progname);
		perror(filename);
		exit(1);
	}
}

/*
** Convert a string of one of the forms
**	h	-h 	hh:mm	-hh:mm	hh:mm:ss	-hh:mm:ss
** into a number of seconds. 
** Call error with errstring and return zero on errors.
*/

static long
getoff(string, errstring)
char *	string;
char *	errstring;
{
	long	hh, mm, ss, sign;

	if (*string == '-') {
		sign = -1;
		++string;
	} else	sign = 1;
	if (sscanf(string, scheck(string, "%ld"), &hh) == 1)
		mm = ss = 0;
	else if (sscanf(string, scheck(string, "%ld:%ld"), &hh, &mm) == 2)
		ss = 0;
	else if (sscanf(string, scheck(string, "%ld:%ld:%ld"),
		&hh, &mm, &ss) != 3) {
			error(errstring);
			return 0;
	}
	if (hh < 0 || hh >= HOURS_PER_DAY ||
		mm < 0 || mm >= MINS_PER_HOUR ||
		ss < 0 || ss >= SECS_PER_MIN) {
			error(errstring);
			return 0;
	}
	return (long) sign * (((hh * MINS_PER_HOUR) + mm) * SECS_PER_MIN + ss);
}

static
inrule(fields, nfields)
char **	fields;
{
	register struct lookup *	lp;
	register char *			cp;
	struct rule			r;

	if (nfields != RULE_FIELDS) {
		error("wrong number of fields on Rule line");
		return;
	}
	r.r_filename = filename;
	r.r_linenum = linenum;
	if ((lp = byword(fields[RF_MONTH], mon_names)) == NULL) {
		error("invalid month name");
		return;
	}
	r.r_month = lp->l_value;
	r.r_todisstd = FALSE;
	cp = fields[RF_TOD];
	if (strlen(cp) > 0) {
		cp += strlen(cp) - 1;
		switch (lowerit(*cp)) {
			case 's':
				r.r_todisstd = TRUE;
				*cp = '\0';
				break;
			case 'w':
				r.r_todisstd = FALSE;
				*cp = '\0';
				break;
		}
	}
	r.r_tod = getoff(fields[RF_TOD], "invalid time of day");
	r.r_stdoff = getoff(fields[RF_STDOFF], "invalid Standard Time offset");
	/*
	** Year work.
	*/
	cp = fields[RF_LOYEAR];
	if (sscanf(cp, scheck(cp, "%ld"), &r.r_loyear) != 1 ||
		r.r_loyear <= 0) {
			error("invalid starting year");
			return;
	}
	cp = fields[RF_HIYEAR];
	if (*cp == '\0' || ciequal(cp, "only"))
		r.r_hiyear = r.r_loyear;
	else if (sscanf(cp, scheck(cp, "%ld"), &r.r_hiyear) != 1 ||
		r.r_hiyear <= 0) {
			error("invalid ending year");
			return;
	}
	if (r.r_loyear > r.r_hiyear) {
		error("starting year greater than ending year");
		return;
	}
	if (*fields[RF_COMMAND] == '\0')
		r.r_yrtype = NULL;
	else {
		if (r.r_loyear == r.r_hiyear) {
			error("typed single year");
			return;
		}
		r.r_yrtype = ecpyalloc(fields[RF_COMMAND]);
	}
	/*
	** Day work.
	** Accept things such as:
	**	1
	**	last-Sunday
	**	Sun<=20
	**	Sun>=7
	*/
	cp = fields[RF_DAY];
	if ((lp = byword(cp, lasts)) != NULL) {
		r.r_dycode = DC_DOWLEQ;
		r.r_wday = lp->l_value;
		r.r_dayofmonth = mon_lengths[r.r_month];
		if (r.r_month == TM_FEBRUARY)
			++r.r_dayofmonth;
	} else {
		if ((cp = strchr(fields[RF_DAY], '<')) != 0)
			r.r_dycode = DC_DOWLEQ;
		else if ((cp = strchr(fields[RF_DAY], '>')) != 0)
			r.r_dycode = DC_DOWGEQ;
		else {
			cp = fields[RF_DAY];
			r.r_dycode = DC_DOM;
		}
		if (r.r_dycode != DC_DOM) {
			*cp++ = 0;
			if (*cp++ != '=') {
				error("invalid day of month");
				return;
			}
			if ((lp = byword(fields[RF_DAY], wday_names)) == NULL) {
				error("invalid weekday name");
				return;
			}
			r.r_wday = lp->l_value;
		}
		if (sscanf(cp, scheck(cp, "%ld"), &r.r_dayofmonth) != 1 ||
			r.r_dayofmonth <= 0 ||
			(r.r_dayofmonth > mon_lengths[r.r_month] &&
			r.r_month != TM_FEBRUARY && r.r_dayofmonth != 29)) {
				error("invalid day of month");
				return;
		}
	}
	if (*fields[RF_NAME] == '\0') {
		error("nameless rule");
		return;
	}
	r.r_name = ecpyalloc(fields[RF_NAME]);
	r.r_abbrvar = ecpyalloc(fields[RF_ABBRVAR]);
	rules = (struct rule *) erealloc((char *) rules,
		(nrules + 1) * sizeof *rules);
	rules[nrules++] = r;
}

static
inzone(fields, nfields)
char **	fields;
{
	register int	i;
	char		buf[132];

	if (nfields < ZONE_FIELDS || nfields > ZONE_UNTILTIME) {
		error("wrong number of fields on Zone line");
		return FALSE;
	}
	for (i = 0; i < nzones; ++i)
		if (zones[i].z_name != NULL &&
			strcmp(zones[i].z_name, fields[ZF_NAME]) == 0) {
				(void) sprintf(buf,
"duplicate zone name %s (file \"%s\", line %d)",
					fields[ZF_NAME],
					zones[i].z_filename,
					zones[i].z_linenum);
				error(buf);
				return FALSE;
		}
	return inzsub(fields, nfields, FALSE);
}

static
inzcont(fields, nfields)
char **	fields;
{
	if (nfields < ZONEC_FIELDS || nfields > ZONEC_UNTILTIME) {
		error("wrong number of fields on Zone continuation line");
		return FALSE;
	}
	return inzsub(fields, nfields, TRUE);
}

static
inzsub(fields, nfields, iscont)
char **	fields;
{
	register struct lookup *	lp;
	register char *			cp;
	struct zone			z;
	struct rule			r;
	register int			i_gmtoff, i_rule, i_format;
	register int			i_untilyear, i_untilmonth;
	register int			i_untilday, i_untiltime;
	long				year;

	if (iscont) {
		i_gmtoff = ZFC_GMTOFF;
		i_rule = ZFC_RULE;
		i_format = ZFC_FORMAT;
		i_untilyear = ZFC_UNTILYEAR;
		i_untilmonth = ZFC_UNTILMONTH;
		i_untilday = ZFC_UNTILDAY;
		i_untiltime = ZFC_UNTILTIME;
		z.z_name = NULL;
	} else {
		i_gmtoff = ZF_GMTOFF;
		i_rule = ZF_RULE;
		i_format = ZF_FORMAT;
		i_untilyear = ZF_UNTILYEAR;
		i_untilmonth = ZF_UNTILMONTH;
		i_untilday = ZF_UNTILDAY;
		i_untiltime = ZF_UNTILTIME;
		z.z_name = ecpyalloc(fields[ZF_NAME]);
	}
	z.z_filename = filename;
	z.z_linenum = linenum;
	z.z_gmtoff = getoff(fields[i_gmtoff], "invalid GMT offset");
	if ((cp = strchr(fields[i_format], '%')) != 0) {
		if (*++cp != 's' || strchr(cp, '%') != 0) {
			error("invalid abbreviation format");
			return FALSE;
		}
	}
	z.z_rule = ecpyalloc(fields[i_rule]);
	z.z_format = ecpyalloc(fields[i_format]);
	if (nfields <= i_untilyear)
		z.z_until = 0;	/* does not expire */
	else {
		/*
		** We have a date/time at which this zone subspecification
		** expires.  Stuff it into a dummy rule structure so that
		** we can use "rpytime" to convert it to a time.
		*/
		if (sscanf(fields[i_untilyear],
			scheck(fields[i_untilyear], "%ld"), &year) != 1 ||
			year <= 0) {
				error("invalid year");
				return FALSE;
		}
		if (nfields <= i_untilmonth)
			r.r_month = TM_JANUARY;	/* default to January */
		else {
			if ((lp = byword(fields[i_untilmonth],
				mon_names)) == NULL) {
					error("invalid month name");
					return FALSE;
			}
			r.r_month = lp->l_value;
		}
		if (nfields <= i_untilday)
			r.r_dayofmonth = 1;	/* default to first */
		else if (sscanf(fields[i_untilday],
			scheck(fields[i_untilday], "%ld"),
			&r.r_dayofmonth) != 1 ||
			r.r_dayofmonth <= 0 ||
			(r.r_dayofmonth > mon_lengths[r.r_month] &&
			r.r_month != TM_FEBRUARY &&
			!isleap(year) &&
			r.r_dayofmonth != 29)) {
				error("invalid day of month");
				return FALSE;
		}
		r.r_dycode = DC_DOM;
		if (nfields <= i_untiltime)
			r.r_tod = 0;	/* default to midnight */
		else	r.r_tod = getoff(fields[i_untiltime],
				"invalid time of day");
		z.z_until = rpytime(&r, year);
	}
	if (iscont) {
		if (nzones == 0) {	/* "cannot happen" */
			(void) fprintf(stderr,
"%s: panic: continuation line not preceded by Zone line\n", progname);
			exit(1);
		}
		if (z.z_until != 0 && zones[nzones - 1].z_until >= z.z_until) {
			error("Zone continuation line ending time is not after ending time of previous line");
			return FALSE;
		}
	}
	zones = (struct zone *) erealloc((char *) zones,
		(nzones + 1) * sizeof *zones);
	zones[nzones++] = z;
	/*
	** If there was an UNTIL field on this line,
	** there's more information about the zone on the next line.
	*/
	return nfields > i_untilyear;
}

static
inlink(fields, nfields)
char **	fields;
{
	struct link	l;

	if (nfields != LINK_FIELDS) {
		error("wrong number of fields on Link line");
		return;
	}
	if (*fields[LF_FROM] == '\0') {
		error("blank FROM field on Link line");
		return;
	}
	if (*fields[LF_TO] == '\0') {
		error("blank TO field on Link line");
		return;
	}
	l.l_filename = filename;
	l.l_linenum = linenum;
	l.l_from = ecpyalloc(fields[LF_FROM]);
	l.l_to = ecpyalloc(fields[LF_TO]);
	links = (struct link *) erealloc((char *) links,
		(nlinks + 1) * sizeof *links);
	links[nlinks++] = l;
}

#define PUTSHORT(val, fp) { \
	register int shortval; \
	register unsigned char c; \
	shortval = val; \
	c = shortval >> 8; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	c = shortval; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	}

#define PUTLONG(val, fp) { \
	register long longval; \
	register unsigned char c; \
	longval = val; \
	c = longval >> 24; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	c = longval >> 16; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	c = longval >> 8; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	c = longval; \
	if (putc(c, fp) == EOF) \
		goto wreck; \
	}
	
static
writezone(name)
char *	name;
{
	register FILE *	fp;
	register int	i;
	char		fullname[BUFSIZ];

	if (strlen(directory) + 1 + strlen(name) >= sizeof fullname) {
		(void) fprintf(stderr,
			"%s: File name %s/%s too long\n", progname,
			directory, name);
		exit(1);
	}
	(void) sprintf(fullname, "%s/%s", directory, name);
	if ((fp = fopen(fullname, "w")) == NULL) {
		if (mkdirs(fullname) < 0)
			exit(1);
		if ((fp = fopen(fullname, "w")) == NULL) {
			(void) fprintf(stderr, "%s: Can't create ", progname);
			perror(fullname);
			exit(1);
		}
	}
	if (fwrite((char *) h.tzh_reserved, sizeof h.tzh_reserved, 1, fp) != 1)
		goto wreck;
	PUTSHORT(h.tzh_timecnt, fp);
	PUTSHORT(h.tzh_typecnt, fp);
	PUTSHORT(h.tzh_charcnt, fp);
	if ((i = h.tzh_timecnt) != 0) {
		register long *atsp;
		register int j;

		atsp = &ats[0];
		for (j = i; j > 0; --j)
			PUTLONG(*atsp++, fp);
		if (fwrite((char *) types, sizeof types[0], i, fp) != i)
			goto wreck;
	}
	if ((i = h.tzh_typecnt) != 0) {
		register struct ttinfo *ttisp;

		ttisp = &ttis[0];
		for ( ; i > 0; --i) {
			PUTLONG(ttisp->tt_gmtoff, fp);
			if (putc(ttisp->tt_isdst, fp) == EOF)
				goto wreck;
			if (putc(ttisp->tt_abbrind, fp) == EOF)
				goto wreck;
			ttisp++;
		}
	}
	if ((i = h.tzh_charcnt) != 0)
		if (fwrite(chars, sizeof chars[0], i, fp) != i)
			goto wreck;
	if (fclose(fp))
		goto wreck;
	return;
wreck:
	(void) fprintf(stderr, "%s: Write error on ", progname);
	perror(fullname);
	exit(1);
}

/*
** "struct temp" defines a point at which a new local time offset, etc.
** comes into effect.  "t_time" is the time (in seconds since the epoch)
** when it comes into effect; "t_rp" points to the rule that generated
** it; "t_type" indicates the "type", i.e., the GMT offset, an indication
** of whether DST is in effect or not, and the time zone abbreviation.
*/

struct temp {
	long		t_time;
	struct rule *	t_rp;
	int		t_type;
};

static struct temp	temps[TZ_MAX_TIMES];
static int		ntemps;

static
tcomp(cp1, cp2)
char *	cp1;
char *	cp2;
{
	register struct temp *	tp1;
	register struct temp *	tp2;
	register char *		cp;
	register long		diff;

	tp1 = (struct temp *) cp1;
	tp2 = (struct temp *) cp2;
	if (tp1->t_time > 0 && tp2->t_time <= 0)
		return 1;
	if (tp1->t_time <= 0 && tp2->t_time > 0)
		return -1;
	if ((diff = tp1->t_time - tp2->t_time) > 0)
		return 1;
	else if (diff < 0)
		return -1;
	/*
	** Two equal start times appeared; something's wrong.
	*/
	if (tp1->t_rp == NULL || tp2->t_rp == NULL) {
		error("tzcomp's little mind is blown");
		exit(1);
	}
	if (tp1->t_type == tp2->t_type)
		cp = "duplicate rule?!";
	else	cp = "inconsistent rules?!";
	rfilename = NULL;
	rlinenum = 0;
	filename = tp1->t_rp->r_filename;
	linenum = tp1->t_rp->r_linenum;
	error(cp);
	filename = tp2->t_rp->r_filename;
	linenum = tp2->t_rp->r_linenum;
	error(cp);
	exit(1);
	/*NOTREACHED*/
}

static
outzone(zpfirst, zonecount)
register struct zone *	zpfirst;
int			zonecount;
{
	register struct zone *		zp;
	register struct rule *		rp;
	register int			i, j;
	long				starttime;

	h.tzh_timecnt = 0;
	h.tzh_typecnt = 0;
	h.tzh_charcnt = 0;
	/*
	** Now. . .finally. . .generate some useable data!
	*/
	ntemps = 0;
	starttime = 0;
	for (i = 0; i < zonecount; ++i) {
		/*
		** See what the different local time types are.
		** Plug the indices into the rules.
		*/
		zp = &zpfirst[i];
		if (zp->z_nrules == 0)
			trivial(zp, starttime);
		else for (j = 0; j < zp->z_nrules; ++j) {
			rp = &zp->z_rules[j];
			filename = rp->r_filename;
			linenum = rp->r_linenum;
			rfilename = NULL;
			rlinenum = 0;
			if (rp->r_yrtype != NULL && *rp->r_yrtype != '\0')
				hard(rp, zp, starttime, zp->z_until);
			else	easy(rp, zp, starttime, zp->z_until);
		}
		starttime = zp->z_until;
	}
	h.tzh_timecnt = ntemps;
	(void) qsort((char *) temps, ntemps, sizeof *temps, tcomp);
	for (i = 0; i < ntemps; ++i) {
		ats[i] = temps[i].t_time;
		types[i] = temps[i].t_type;
		if ((rp = temps[i].t_rp) != NULL) {
			filename = rp->r_filename;
			linenum = rp->r_linenum;
			rfilename = NULL;
			rlinenum = 0;
			if (!rp->r_todisstd) {
				/*
				** Credit to munnari!kre for pointing out
				** the need for the following.  (This can
				** still mess up on the earliest rule; who's
				** got the solution?  It can also mess up
				** if a time switch results in a day switch;
				** this is left as an exercise for the reader.)
				*/
				if (i == 0) {
					/*
					** Kludge--not guaranteed to work.
					*/
					if (ntemps > 1)
						rp = temps[1].t_rp;
					else	rp = NULL;
				} else	rp = temps[i - 1].t_rp;
				if (rp != NULL)
					ats[i] = tadd(ats[i], -rp->r_stdoff);
			}
		}
	}
	writezone(zpfirst->z_name);
	return;
}

static
addtype(gmtoff, abbr, isdst, zp)
long			gmtoff;
char *			abbr;
int			isdst;
register struct zone *	zp;
{
	register int	i;

	/*
	** See if there's already an entry for this zone type.
	** If so, just return its index.
	*/
	for (i = 0; i < h.tzh_typecnt; ++i) {
		if (gmtoff == ttis[i].tt_gmtoff &&
			strcmp(abbr, &chars[ttis[i].tt_abbrind]) == 0)
				return i;
	}
	/*
	** There isn't one; add a new one, unless there are already too
	** many.
	*/
	if (h.tzh_typecnt >= TZ_MAX_TYPES) {
		filename = zp->z_filename;
		linenum = zp->z_linenum;
		rfilename = NULL;
		rlinenum = 0;
		error("too many local time types");
		exit(1);
	}
	ttis[i].tt_gmtoff = gmtoff;
	ttis[i].tt_isdst = isdst;
	ttis[i].tt_abbrind = h.tzh_charcnt;
	newabbr(abbr);
	++h.tzh_typecnt;
	return i;
}

static
trivial(zp, from)
register struct zone *	zp;
long			from;
{
	if (ntemps >= TZ_MAX_TIMES) {
		error("too many transitions?!");
		exit(1);
	}
	filename = zp->z_filename;
	linenum = zp->z_linenum;
	rfilename = NULL;
	rlinenum = 0;
	temps[ntemps].t_time = tadd(from, -zp->z_gmtoff);
	temps[ntemps].t_rp = NULL;
	temps[ntemps].t_type = addtype(zp->z_gmtoff, zp->z_format, 0, zp);
	++ntemps;
}

static
addrule(rp, y, zp, from, until)
register struct rule *	rp;
long			y;
register struct zone *	zp;
long			from;
long			until;
{
	long		newtime;
	char		buf[BUFSIZ];

	newtime = rpytime(rp, y);
	if (from != 0 && newtime < from)
		return 1;	/* this zone data doesn't take effect yet */
	if (until != 0 && newtime >= until)
		return 0;	/* this zone data has expired */
	if (ntemps >= TZ_MAX_TIMES) {
		error("too many transitions?!");
		exit(1);
	}
	filename = zp->z_filename;
	linenum = zp->z_linenum;
	rfilename = rp->r_filename;
	rlinenum = rp->r_linenum;
	temps[ntemps].t_time = tadd(newtime, -zp->z_gmtoff);
	temps[ntemps].t_rp = rp;
	(void) sprintf(buf, zp->z_format, rp->r_abbrvar);
	temps[ntemps].t_type = addtype(tadd(zp->z_gmtoff, rp->r_stdoff),
		buf, rp->r_stdoff != 0, zp);
	++ntemps;
	return 1;
}

static
easy(rp, zp, from, until)
register struct rule *	rp;
register struct zone *	zp;
long			from;
long			until;
{
	long	y;

	for (y = rp->r_loyear; y <= rp->r_hiyear; ++y)
		if (!addrule(rp, y, zp, from, until))
			break;
}

static
hard(rp, zp, from, until)
register struct rule *	rp;
register struct zone *	zp;
long			from;
long			until;
{
	register FILE *	fp;
	register int	n;
	long		y;
	char		buf[BUFSIZ];
	char		command[BUFSIZ];

	(void) sprintf(command, "years %ld %ld %s",
		rp->r_loyear, rp->r_hiyear, rp->r_yrtype);
	if ((fp = popen(command, "r")) == NULL) {
		(void) fprintf(stderr, "%s: Can't run command \"%s\"\n",
			progname, command);
		exit(1);
	}
	for (n = 0; fgets(buf, sizeof buf, fp) == buf; ++n) {
		if (strchr(buf, '\n') == 0) {
			(void) fprintf(stderr,
"%s: Line read from command \"%s\" is too long\n",
				progname, command);
			(void) fprintf(stderr, "Line began with \"%s\"\n", buf);
			exit(1);
		}
		*strchr(buf, '\n') = '\0';
		if (sscanf(buf, scheck(buf, "%ld"), &y) != 1) {
			(void) fprintf(stderr,
"%s: Line read from command \"%s\" is not a number\n",
				progname, command);
			(void) fprintf(stderr, "Line was \"%s\"\n", buf);
			exit(1);
		}
		if (y < rp->r_loyear || y > rp->r_hiyear) {
			(void) fprintf(stderr,
"%s: Year %ld read from command \"%s\" is not valid\n",
				progname, y, command);
			exit(1);
		}
		if (!addrule(rp, y, zp, from, until))
			break;
	}
	if (ferror(fp)) {
		(void) fprintf(stderr,
			"%s: Error reading from command \"%s\": ",
			progname, command);
		perror("");
		exit(1);
	}
	if (pclose(fp)) {
		(void) fprintf(stderr,
			"%s: Error closing pipe to command \"%s\": ",
			progname, command);
		perror("");
		exit(1);
	}
	if (n == 0) {
		error("no year in range matches type");
		exit(1);
	}
}

static
lowerit(a)
{
	return (isascii(a) && isupper(a)) ? tolower(a) : a;
}

static
ciequal(ap, bp)		/* case-insensitive equality */
register char *	ap;
register char *	bp;
{
	while (lowerit(*ap) == lowerit(*bp++))
		if (*ap++ == '\0')
			return TRUE;
	return FALSE;
}

static
isabbr(abbr, word)
register char *		abbr;
register char *		word;
{
	if (lowerit(*abbr) != lowerit(*word))
		return FALSE;
	++word;
	while (*++abbr != '\0')
		do if (*word == '\0')
			return FALSE;
				while (lowerit(*word++) != lowerit(*abbr));
	return TRUE;
}

static struct lookup *
byword(word, table)
register char *			word;
register struct lookup *	table;
{
	register struct lookup *	foundlp;
	register struct lookup *	lp;

	if (word == NULL || table == NULL)
		return NULL;
	foundlp = NULL;
	for (lp = table; lp->l_word != NULL; ++lp)
		if (ciequal(word, lp->l_word))		/* "exact" match */
			return lp;
		else if (!isabbr(word, lp->l_word))
			continue;
		else if (foundlp == NULL)
			foundlp = lp;
		else	return NULL;		/* two inexact matches */
	return foundlp;
}

static char **
getfields(cp)
register char *	cp;
{
	register char *		dp;
	register char **	array;
	register int		nsubs;

	if (cp == NULL)
		return NULL;
	array = (char **) emalloc((strlen(cp) + 1) * sizeof *array);
	nsubs = 0;
	for ( ; ; ) {
		while (isascii(*cp) && isspace(*cp))
			++cp;
		if (*cp == '\0' || *cp == '#')
			break;
		array[nsubs++] = dp = cp;
		do {
			if ((*dp = *cp++) != '"')
				++dp;
			else while ((*dp = *cp++) != '"')
				if (*dp != '\0')
					++dp;
				else	error("Odd number of quotation marks");
		} while (*cp != '\0' && *cp != '#' &&
			(!isascii(*cp) || !isspace(*cp)));
		if (isascii(*cp) && isspace(*cp))
			++cp;
		*dp++ = '\0';
	}
	array[nsubs] = NULL;
	return array;
}

static long
tadd(t1, t2)
long	t1;
long	t2;
{
	register long	t;

	t = t1 + t2;
	if (t1 > 0 && t2 > 0 && t <= 0 || t1 < 0 && t2 < 0 && t >= 0) {
		error("time overflow");
		exit(1);
	}
	return t;
}

static
isleap(y)
long	y;
{
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

/*
** Given a rule, and a year, compute the date - in seconds since January 1,
** 1970, 00:00 LOCAL time - in that year that the rule refers to.
*/

static long
rpytime(rp, wantedy)
register struct rule *	rp;
register long		wantedy;
{
	register long	i, y, wday, t, m;
	register long	dayoff;			/* with a nod to Margaret O. */

	dayoff = 0;
	m = TM_JANUARY;
	y = EPOCH_YEAR;
	while (wantedy != y) {
		if (wantedy > y) {
			i = DAYS_PER_YEAR;
			if (isleap(y))
				++i;
			++y;
		} else {
			--y;
			i = -DAYS_PER_YEAR;
			if (isleap(y))
				--i;
		}
		dayoff = tadd(dayoff, i);
	}
	while (m != rp->r_month) {
		i = mon_lengths[m];
		if (m == TM_FEBRUARY && isleap(y))
			++i;
		dayoff = tadd(dayoff, i);
		++m;
	}
	i = rp->r_dayofmonth;
	if (m == TM_FEBRUARY && i == 29 && !isleap(y)) {
		if (rp->r_dycode == DC_DOWLEQ)
			--i;
		else {
			error("use of 2/29 in non leap-year");
			exit(1);
		}
	}
	--i;
	dayoff = tadd(dayoff, i);
	if (rp->r_dycode == DC_DOWGEQ || rp->r_dycode == DC_DOWLEQ) {
		wday = EPOCH_WDAY;
		/*
		** Don't trust mod of negative numbers.
		*/
		if (dayoff >= 0)
			wday = (wday + dayoff) % 7;
		else {
			wday -= ((-dayoff) % 7);
			if (wday < 0)
				wday += 7;
		}
		while (wday != rp->r_wday) {
			if (rp->r_dycode == DC_DOWGEQ)
				i = 1;
			else	i = -1;
			dayoff = tadd(dayoff, i);
			wday = (wday + i + 7) % 7;
		}
	}
	t = dayoff * SECS_PER_DAY;
	/*
	** Cheap overflow check.
	*/
	if (t / SECS_PER_DAY != dayoff)
		error("time overflow");
	return tadd(t, rp->r_tod);
}

static
newabbr(string)
char *	string;
{
	register int	i;

	i = strlen(string);
	if (h.tzh_charcnt + i >= TZ_MAX_CHARS)
		error("too many, or too long, time zone abbreviations");
	(void) strcpy(&chars[h.tzh_charcnt], string);
	h.tzh_charcnt += i + 1;
}

static
mkdirs(name)
char *	name;
{
	register char *	cp;

	if ((cp = name) == NULL || *cp == '\0')
		return 0;
	while ((cp = strchr(cp + 1, '/')) != 0) {
		*cp = '\0';
		if (access(name, 0) < 0) {
			/*
			 * It doesn't seem to exist, so we try to create it.
			 */
			if (mkdir(name, 0777) < 0) {
				(void) fprintf(stderr,
					"%s: Can't create directory ",
					progname);
				perror(name);
				return -1;
			}
		}
		*cp = '/';
	}
	return 0;
}
