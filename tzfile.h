/* %W% */

/*
** Format of time zone information files.
*/

#define TZDIR		"/etc/tzdir"	/* Time zone object file directory */
#define TZDEFAULT	"localtime"
#define TZ_MAX_RULES	120		/* Maximum number of rules */
#define TZ_MAX_TYPES	10		/* Maximum number of Saving Times */
#define TZ_ABBR_LEN	7		/* Maximum Time Zone abbr. length */

struct dsinfo {
	long	ds_gmtoff;		/* Offset from GMT in seconds */
	char	ds_abbr[TZ_ABBR_LEN+1];	/* Time Zone abbreviation */
	char	ds_isdst;		/* Used to fill tm_isdst */
};

struct tzinfo {
	int	tz_rulecnt;		/* Number of rules used */
	long	tz_times[TZ_MAX_RULES];	/* Times when rules kick in */
	char	tz_types[TZ_MAX_RULES];	/* Saving Time types for the above */
	struct dsinfo	tz_dsinfo[TZ_MAX_TYPES];
					/* See above */
};
