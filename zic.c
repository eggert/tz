#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#include "tzfile.h"
#include "ctype.h"
#include "sys/types.h"
#include "sys/stat.h"

#ifndef alloc_t
#define alloc_t	unsigned
#endif

#ifndef MAL
#define MAL	NULL
#endif

#ifndef BUFSIZ
#define BUFSIZ	1024
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif


extern char *	calloc();
extern char *	malloc();
extern char *	optarg;
extern int	optind;
extern FILE *	popen();
extern char *	realloc();
extern char *	scheck();
#ifdef strchr
extern char *	sprintf();
#endif
extern char *	strcat();
extern char *	strchr();
extern char *	strcpy();

static long	charcnt;
static int	errors;
static char *	filename;
static long	gethms();
static char **	getfields();
static int	linenum;
static char *	progname;
static char *	rfilename;
static int	rlinenum;
static long	rpytime();
static long	tadd();
static long	timecnt;
static long	typecnt;

#define SECS_PER_MIN	60L
#define MINS_PER_HOUR	60L
#define HOURS_PER_DAY	24L
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	(SECS_PER_HOUR * HOURS_PER_DAY)

#define EPOCH_YEAR	1970L
#define EPOCH_WDAY	TM_THURSDAY
#define MIN_YEAR	1902L
#define MAX_YEAR	2037L

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
#define ZONE_MINFIELDS	5
#define ZONE_MAXFIELDS	9

/*
** Which fields are which on a Zone continuation line.
*/

#define ZFC_GMTOFF	0
#define ZFC_RULE	1
#define ZFC_FORMAT	2
#define ZFC_UNTILYEAR	3
#define ZFC_UNTILMONTH	4
#define ZFC_UNTILDAY	5
#define ZFC_UNTILTIME	6
#define ZONEC_MINFIELDS	3
#define ZONEC_MAXFIELDS	7

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

	int	r_todo;		/* a rule to do (used in outzone) */
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

	long		z_stdoff;

	struct rule *	z_rules;
	int		z_nrules;

	struct rule	z_untilrule;
	long		z_untiltime;
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

static struct lookup	begin_years[] = {
	"minimum",		MIN_YEAR,
	"maximum",		MAX_YEAR,
	NULL,			0
};

static struct lookup	end_years[] = {
	"minimum",		MIN_YEAR,
	"maximum",		MAX_YEAR,
#define ONLY	EPOCH_YEAR	/* surely neither MIN_YEAR nor MAX_YEAR */
	"only",			ONLY,
	NULL,			0
};

static long	mon_lengths[2][12] = {	/* ". . .knuckles are 31. . ." */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static long	year_lengths[2] = {
	365, 366
};

static long		ats[TZ_MAX_TIMES];
static unsigned char	types[TZ_MAX_TIMES];
static long		gmtoffs[TZ_MAX_TYPES];
static char		isdsts[TZ_MAX_TYPES];
static char		abbrinds[TZ_MAX_TYPES];
static char		chars[TZ_MAX_CHARS];

/*
** Memory allocation.
*/

static char *
emalloc(size)
{
	register char *	cp;

	if ((cp = malloc((alloc_t) size)) == MAL || cp == NULL) {
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

/*
** Error handling.
*/

static
eats(name, num, rname, rnum)
char *	name;
char *	rname;
{
	filename = name;
	linenum = num;
	rfilename = rname;
	rlinenum = rnum;
}

static
eat(name, num)
char *	name;
{
	eats(name, num, (char *) NULL, -1);
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

#ifdef unix
	umask(umask(022) | 022);
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
	if (optind == argc - 1 && strcmp(argv[optind], "=") == 0)
		usage();	/* usage message by request */
	if (directory == NULL)
		directory = TZDIR;
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
		for (j = i + 1; j < nzones && zones[j].z_name == NULL; ++j)
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
		nondunlink(links[i].l_to);
		if (link(links[i].l_from, links[i].l_to) != 0) {
			(void) fprintf(stderr, "%s: Can't link %s to ",
				progname, links[i].l_from);
			perror(links[i].l_to);
			exit(1);
		}
	}
	if (localtime != NULL) {
		nondunlink(TZDEFAULT);
		if (link(localtime, TZDEFAULT) != 0) {
			(void) fprintf(stderr, "%s: Can't link %s to ",
				progname, localtime);
			perror(TZDEFAULT);
			exit(1);
		}
	}
	exit((errors == 0) ? 0 : 1);
}

/*
** We get to be careful here since there's a fair chance of root running us.
*/

static
nondunlink(name)
char *	name;
{
	struct stat	s;

	if (stat(name, &s) != 0)
		return;
	if ((s.st_mode & S_IFMT) == S_IFDIR)
		return;
	(void) unlink(name);
}

/*
** Associate sets of rules with zones.
*/

/*
** Sort by rule name.
*/

static
rcomp(cp1, cp2)
char *	cp1;
char *	cp2;
{
	return strcmp(((struct rule *) cp1)->r_name,
		((struct rule *) cp2)->r_name);
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
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		zp->z_rules = NULL;
		zp->z_nrules = 0;
	}
	for (base = 0; base < nrules; base = out) {
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
	}
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		if (zp->z_nrules == 0) {
			/*
			** Maybe we have a local standard time offset.
			*/
			eat(zp->z_filename, zp->z_linenum);
			zp->z_stdoff = gethms(zp->z_rule, "unruly zone", TRUE);
			/*
			** Note, though, that if there's no rule,
			** a '%s' in the format is a bad thing.
			*/
			if (strchr(zp->z_format, '%') != 0)
				error("%s in ruleless zone");
		}
	}
	if (errors)
		exit(1);
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
	register int			wantcont;
	register int			num;
	char				buf[BUFSIZ];

	if (strcmp(name, "-") == 0) {
		name = "standard input";
		fp = stdin;
	} else if ((fp = fopen(name, "r")) == NULL) {
		(void) fprintf(stderr, "%s: Can't open ", progname);
		perror(name);
		exit(1);
	}
	wantcont = FALSE;
	for (num = 1; ; ++num) {
		eat(name, num);
		if (fgets(buf, sizeof buf, fp) != buf)
			break;
		cp = strchr(buf, '\n');
		if (cp == NULL) {
			error("line too long");
			exit(1);
		}
		*cp = '\0';
		fields = getfields(buf);
		nfields = 0;
		while (fields[nfields] != NULL) {
			if (ciequal(fields[nfields], "-"))
				fields[nfields] = "";
			++nfields;
		}
		if (nfields == 0) {
			/* nothing to do */
		} else if (wantcont) {
			wantcont = inzcont(fields, nfields);
		} else {
			lp = byword(fields[0], line_codes);
			if (lp == NULL)
				error("input line of unknown type");
			else switch ((int) (lp->l_value)) {
				case LC_RULE:
					inrule(fields, nfields);
					wantcont = FALSE;
					break;
				case LC_ZONE:
					wantcont = inzone(fields, nfields);
					break;
				case LC_LINK:
					inlink(fields, nfields);
					wantcont = FALSE;
					break;
				default:	/* "cannot happen" */
					(void) fprintf(stderr,
"%s: panic: Invalid l_value %ld\n",
						progname, lp->l_value);
					exit(1);
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
	if (wantcont)
		error("expected continuation line not found");
}

/*
** Convert a string of one of the forms
**	h	-h 	hh:mm	-hh:mm	hh:mm:ss	-hh:mm:ss
** into a number of seconds.
** A null string maps to zero.
** Call error with errstring and return zero on errors.
*/

static long
gethms(string, errstring, signable)
char *	string;
char *	errstring;
{
	long	hh, mm, ss, sign;

	if (string == NULL || *string == '\0')
		return 0;
	if (!signable)
		sign = 1;
	else if (*string == '-') {
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
register char **	fields;
{
	struct rule	r;

	if (nfields != RULE_FIELDS) {
		error("wrong number of fields on Rule line");
		return;
	}
	if (*fields[RF_NAME] == '\0') {
		error("nameless rule");
		return;
	}
	r.r_filename = filename;
	r.r_linenum = linenum;
	r.r_stdoff = gethms(fields[RF_STDOFF], "invalid saved time", TRUE);
	rulesub(&r, fields[RF_LOYEAR], fields[RF_HIYEAR], fields[RF_COMMAND],
		fields[RF_MONTH], fields[RF_DAY], fields[RF_TOD]);
	r.r_name = ecpyalloc(fields[RF_NAME]);
	r.r_abbrvar = ecpyalloc(fields[RF_ABBRVAR]);
	rules = (struct rule *) erealloc((char *) rules,
		(nrules + 1) * sizeof *rules);
	rules[nrules++] = r;
}

static
inzone(fields, nfields)
register char **	fields;
{
	register int	i;
	char		buf[132];

	if (nfields < ZONE_MINFIELDS || nfields > ZONE_MAXFIELDS) {
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
register char **	fields;
{
	if (nfields < ZONEC_MINFIELDS || nfields > ZONEC_MAXFIELDS) {
		error("wrong number of fields on Zone continuation line");
		return FALSE;
	}
	return inzsub(fields, nfields, TRUE);
}

static
inzsub(fields, nfields, iscont)
register char **	fields;
{
	register char *	cp;
	struct zone	z;
	register int	i_gmtoff, i_rule, i_format;
	register int	i_untilyear, i_untilmonth;
	register int	i_untilday, i_untiltime;
	register int	hasuntil;

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
	z.z_gmtoff = gethms(fields[i_gmtoff], "invalid GMT offset", TRUE);
	if ((cp = strchr(fields[i_format], '%')) != 0) {
		if (*++cp != 's' || strchr(cp, '%') != 0) {
			error("invalid abbreviation format");
			return FALSE;
		}
	}
	z.z_rule = ecpyalloc(fields[i_rule]);
	z.z_format = ecpyalloc(fields[i_format]);
	hasuntil = nfields > i_untilyear;
	if (hasuntil) {
		z.z_untilrule.r_filename = filename;
		z.z_untilrule.r_linenum = linenum;
		rulesub(&z.z_untilrule,
			fields[i_untilyear],
			"only",
			"",
			(nfields > i_untilmonth) ? fields[i_untilmonth] : "Jan",
			(nfields > i_untilday) ? fields[i_untilday] : "1",
			(nfields > i_untiltime) ? fields[i_untiltime] : "0");
		z.z_untiltime = rpytime(&z.z_untilrule, z.z_untilrule.r_loyear);
		if (iscont && nzones > 0 &&
			zones[nzones - 1].z_untiltime >= z.z_untiltime) {
error("Zone continuation line end time is not after end time of previous line");
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
	return hasuntil;
}

static
inlink(fields, nfields)
register char **	fields;
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

static
rulesub(rp, loyearp, hiyearp, typep, monthp, dayp, timep)
register struct rule *	rp;
char *			loyearp;
char *			hiyearp;
char *			typep;
char *			monthp;
char *			dayp;
char *			timep;
{
	register struct lookup *	lp;
	register char *			cp;

	if ((lp = byword(monthp, mon_names)) == NULL) {
		error("invalid month name");
		return;
	}
	rp->r_month = lp->l_value;
	rp->r_todisstd = FALSE;
	cp = timep;
	if (*cp != '\0') {
		cp += strlen(cp) - 1;
		switch (lowerit(*cp)) {
			case 's':
				rp->r_todisstd = TRUE;
				*cp = '\0';
				break;
			case 'w':
				rp->r_todisstd = FALSE;
				*cp = '\0';
				break;
		}
	}
	rp->r_tod = gethms(timep, "invalid time of day", FALSE);
	/*
	** Year work.
	*/
	cp = loyearp;
	if ((lp = byword(cp, begin_years)) != NULL)
		rp->r_loyear = lp->l_value;
	else if (sscanf(cp, scheck(cp, "%ld"), &rp->r_loyear) != 1 ||
		rp->r_loyear < MIN_YEAR || rp->r_loyear > MAX_YEAR) {
			error("invalid starting year");
			return;
	}
	cp = hiyearp;
	if ((lp = byword(cp, end_years)) != NULL) {
		if ((rp->r_hiyear = lp->l_value) == ONLY)
			rp->r_hiyear = rp->r_loyear;
	} else if (sscanf(cp, scheck(cp, "%ld"), &rp->r_hiyear) != 1 ||
		rp->r_hiyear < MIN_YEAR || rp->r_hiyear > MAX_YEAR) {
			error("invalid ending year");
			return;
	}
	if (rp->r_loyear > rp->r_hiyear) {
		error("starting year greater than ending year");
		return;
	}
	if (*typep == '\0')
		rp->r_yrtype = NULL;
	else {
		if (rp->r_loyear == rp->r_hiyear) {
			error("typed single year");
			return;
		}
		rp->r_yrtype = ecpyalloc(typep);
	}
	/*
	** Day work.
	** Accept things such as:
	**	1
	**	last-Sunday
	**	Sun<=20
	**	Sun>=7
	*/
	if ((lp = byword(dayp, lasts)) != NULL) {
		rp->r_dycode = DC_DOWLEQ;
		rp->r_wday = lp->l_value;
		rp->r_dayofmonth = mon_lengths[1][rp->r_month];
	} else {
		if ((cp = strchr(dayp, '<')) != 0)
			rp->r_dycode = DC_DOWLEQ;
		else if ((cp = strchr(dayp, '>')) != 0)
			rp->r_dycode = DC_DOWGEQ;
		else {
			cp = dayp;
			rp->r_dycode = DC_DOM;
		}
		if (rp->r_dycode != DC_DOM) {
			*cp++ = 0;
			if (*cp++ != '=') {
				error("invalid day of month");
				return;
			}
			if ((lp = byword(dayp, wday_names)) == NULL) {
				error("invalid weekday name");
				return;
			}
			rp->r_wday = lp->l_value;
		}
		if (sscanf(cp, scheck(cp, "%ld"), &rp->r_dayofmonth) != 1 ||
			rp->r_dayofmonth <= 0 ||
			(rp->r_dayofmonth > mon_lengths[1][rp->r_month])) {
				error("invalid day of month");
				return;
		}
	}
}

static
puttzcode(val, fp)
long	val;
FILE *	fp;
{
	register int	c;
	register int	shift;

	for (shift = 24; shift >= 0; shift -= 8) {
		c = val >> shift;
		putc(c, fp);
	}
}

static
writezone(name)
char *	name;
{
	register FILE *		fp;
	register int		i;
	char			fullname[BUFSIZ];
	struct tzhead *		tzhp;

	if (strlen(directory) + 1 + strlen(name) >= sizeof fullname) {
		(void) fprintf(stderr,
			"%s: File name %s/%s too long\n", progname,
			directory, name);
		exit(1);
	}
	(void) sprintf(fullname, "%s/%s", directory, name);
	if ((fp = fopen(fullname, "w")) == NULL) {
		if (mkdirs(fullname) != 0)
			exit(1);
		if ((fp = fopen(fullname, "w")) == NULL) {
			(void) fprintf(stderr, "%s: Can't create ", progname);
			perror(fullname);
			exit(1);
		}
	}
	(void) fseek(fp, (long) sizeof tzhp->tzh_reserved, 0);
	puttzcode((long) timecnt, fp);
	puttzcode((long) typecnt, fp);
	puttzcode((long) charcnt, fp);
	for (i = 0; i < timecnt; ++i)
		puttzcode((long) ats[i], fp);
	if (timecnt > 0)
		(void) fwrite((char *) types, sizeof types[0],
			(int) timecnt, fp);
	for (i = 0; i < typecnt; ++i) {
		puttzcode((long) gmtoffs[i], fp);
		putc(isdsts[i], fp);
		putc(abbrinds[i], fp);
	}
	if (charcnt != 0)
		(void) fwrite(chars, sizeof chars[0], (int) charcnt, fp);
	if (ferror(fp) || fclose(fp)) {
		(void) fprintf(stderr, "%s: Write error on ", progname);
		perror(fullname);
		exit(1);
	}
}

static
outzone(zpfirst, zonecount)
struct zone *	zpfirst;
{
	register struct zone *		zp;
	register struct rule *		rp;
	register int			i, j;
	register int			usestart, useuntil;
	register long			starttime, untiltime;
	register long			gmtoff;
	register long			stdoff;
	register long			year;
	register long			startoff;
	register int			startisdst;
	register int			type;
	char				startbuf[BUFSIZ];

	/*
	** Now. . .finally. . .generate some useful data!
	*/
	timecnt = 0;
	typecnt = 0;
	charcnt = 0;
	/*
	** Two guesses. . .the second may well be corrected later.
	*/
	gmtoff = zpfirst->z_gmtoff;
	stdoff = 0;
	for (i = 0; i < zonecount; ++i) {
		usestart = i > 0;
		useuntil = i < (zonecount - 1);
		zp = &zpfirst[i];
		eat(zp->z_filename, zp->z_linenum);
		startisdst = -1;
		if (zp->z_nrules == 0) {
			type = addtype(tadd(zp->z_gmtoff, zp->z_stdoff),
				zp->z_format, zp->z_stdoff != 0);
			if (usestart)
				addtt(starttime, type);
			gmtoff = zp->z_gmtoff;
			stdoff = zp->z_stdoff;
		} else for (year = MIN_YEAR; year <= MAX_YEAR; ++year) {
			if (useuntil && year > zp->z_untilrule.r_hiyear)
				break;
			/*
			** Mark which rules to do in the current year.
			*/
			for (j = 0; j < zp->z_nrules; ++j) {
				rp = &zp->z_rules[j];
				eats(zp->z_filename, zp->z_linenum,
					rp->r_filename, rp->r_linenum);
				rp->r_todo = year >= rp->r_loyear &&
						year <= rp->r_hiyear &&
						yearistype(year, rp->r_yrtype);
			}
			for ( ; ; ) {
				register int	k;
				register long	jtime, ktime, offset;
				char		buf[BUFSIZ];

				if (useuntil) {
					/*
					** Turn untiltime into GMT
					** assuming the current gmtoff and
					** stdoff values.
					*/
					offset = gmtoff;
					if (!zp->z_untilrule.r_todisstd)
						offset = tadd(offset, stdoff);
					untiltime = tadd(zp->z_untiltime,
						-offset);
				}
				/*
				** Find the rule (of those to do, if any)
				** that takes effect earliest in the year.
				*/
				k = -1;
				for (j = 0; j < zp->z_nrules; ++j) {
					rp = &zp->z_rules[j];
					if (!rp->r_todo)
						continue;
					eats(zp->z_filename, zp->z_linenum,
						rp->r_filename, rp->r_linenum);
					offset = gmtoff;
					if (!rp->r_todisstd)
						offset = tadd(offset, stdoff);
					jtime = tadd(rpytime(rp, year),
						-offset);
					if (k < 0 || jtime < ktime) {
						k = j;
						ktime = jtime;
					}
				}
				if (k < 0)
					break;	/* go on to next year */
				rp = &zp->z_rules[k];
				rp->r_todo = FALSE;
				if (useuntil && ktime >= untiltime)
					break;
				if (usestart) {
					if (ktime < starttime) {
						stdoff = rp->r_stdoff;
						startoff = tadd(zp->z_gmtoff,
							rp->r_stdoff);
						(void) sprintf(startbuf,
							zp->z_format,
							rp->r_abbrvar);
						startisdst =
							rp->r_stdoff != 0;
						continue;
					}
					if (ktime != starttime &&
						startisdst >= 0)
addtt(starttime, addtype(startoff, startbuf, startisdst));
					usestart = FALSE;
				}
				eats(zp->z_filename, zp->z_linenum,
					rp->r_filename, rp->r_linenum);
				(void) sprintf(buf, zp->z_format,
					rp->r_abbrvar);
				offset = tadd(zp->z_gmtoff, rp->r_stdoff);
				type = addtype(offset, buf, rp->r_stdoff != 0);
				if (timecnt != 0 || rp->r_stdoff != 0)
					addtt(ktime, type);
				gmtoff = zp->z_gmtoff;
				stdoff = rp->r_stdoff;
			}
		}
		/*
		** Now we may get to set starttime for the next zone line.
		*/
		if (useuntil)
			starttime = tadd(zp->z_untiltime,
				-gmtoffs[types[timecnt - 1]]);
	}
	writezone(zpfirst->z_name);
}

static
addtt(starttime, type)
long	starttime;
{
	if (timecnt >= TZ_MAX_TIMES) {
		error("too many transitions?!");
		exit(1);
	}
	ats[timecnt] = starttime;
	types[timecnt] = type;
	++timecnt;
}

static
addtype(gmtoff, abbr, isdst)
long	gmtoff;
char *	abbr;
{
	register int	i;

	/*
	** See if there's already an entry for this zone type.
	** If so, just return its index.
	*/
	for (i = 0; i < typecnt; ++i) {
		if (gmtoff == gmtoffs[i] && isdst == isdsts[i] &&
			strcmp(abbr, &chars[abbrinds[i]]) == 0)
				return i;
	}
	/*
	** There isn't one; add a new one, unless there are already too
	** many.
	*/
	if (typecnt >= TZ_MAX_TYPES) {
		error("too many local time types");
		exit(1);
	}
	gmtoffs[i] = gmtoff;
	isdsts[i] = isdst;
	abbrinds[i] = charcnt;
	newabbr(abbr);
	++typecnt;
	return i;
}

static
yearistype(year, type)
long	year;
char *	type;
{
	char	buf[BUFSIZ];
	int	result;

	if (type == NULL || *type == '\0')
		return TRUE;
	if (strcmp(type, "uspres") == 0)
		return (year % 4) == 0;
	if (strcmp(type, "nonpres") == 0)
		return (year % 4) != 0;
	(void) sprintf(buf, "yearistype %ld %s", year, type);
	result = system(buf);
	if (result == 0)
		return TRUE;
	if (result == 1 << 8)
		return FALSE;
	error("Wild result from command execution");
	(void) fprintf(stderr, "%s: command was '%s', result was %d\n",
		progname, buf, result);
	exit(1);
	/*NOTREACHED*/
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
register char *	abbr;
register char *	word;
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
	/*
	** Look for exact match.
	*/
	for (lp = table; lp->l_word != NULL; ++lp)
		if (ciequal(word, lp->l_word))
			return lp;
	/*
	** Look for inexact match.
	*/
	foundlp = NULL;
	for (lp = table; lp->l_word != NULL; ++lp)
		if (isabbr(word, lp->l_word))
			if (foundlp == NULL)
				foundlp = lp;
			else	return NULL;	/* multiple inexact matches */
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
			i = year_lengths[isleap(y)];
			++y;
		} else {
			--y;
			i = -year_lengths[isleap(y)];
		}
		dayoff = tadd(dayoff, i);
	}
	while (m != rp->r_month) {
		i = mon_lengths[isleap(y)][m];
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
	if (t / SECS_PER_DAY != dayoff) {
		error("time overflow");
		exit(1);
	}
	return tadd(t, rp->r_tod);
}

static
newabbr(string)
char *	string;
{
	register int	i;

	i = strlen(string) + 1;
	if (charcnt + i >= TZ_MAX_CHARS) {
		error("too many, or too long, time zone abbreviations");
		exit(1);
	}
	(void) strcpy(&chars[charcnt], string);
	charcnt += i;
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
		if (access(name, 0) != 0) {
			/*
			 * It doesn't seem to exist, so we try to create it.
			 */
			if (mkdir(name, 0755) != 0) {
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

/*
** UNIX is a registered trademark of AT&T.
*/
