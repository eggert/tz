/* %W% */

#define TZDIR		"tzdir"		/* Time zone object file directory */
#define TZ_MAX_RULES	120		/* Maximum number of rules */
#define TZ_ABBR_TOT	40		/* Maximum total length of... */
					/* ...time zone abbreviations... */
					/* ...(including trailing ASCII nuls) */

struct rule {
	long	r_start;	/* Starting at this time(2) value... */
	short	r_stdoff;	/* ...add this many seconds to time... */
	short	r_abbrind;	/* ...and use the time zone abbrevation... */
				/* ...that starts at this index */
};

/*
** Format of time zone information files.
*/

struct tzinfo {
	long		tz_gmtoff;		/* Offset from GMT in seconds */
	int		tz_rulecnt;		/* Number of rules used */
	struct rule	tz_rules[TZ_MAX_RULES];	/* Rules */
	char		tz_abbrs[TZ_ABBR_TOT];	/* Time Zone abbreviatons */
};
