/* %W% */

/*
** Format of time zone information files.
*/

#ifndef TZDIR
#define TZDIR		"/etc/tzdir"	/* Time zone object file directory */
#endif

#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif

#ifndef TZ_MAX_TIMES
#define TZ_MAX_TIMES	170		/* Maximum number of transition times */
#endif

#ifndef TZ_MAX_TYPES
#define TZ_MAX_TYPES	10		/* Maximum number of Saving Times */
#endif

#ifndef TZ_ABBR_LEN
#define TZ_ABBR_LEN	10		/* Maximum Time Zone abbr. length */
#endif

struct tzinfo {
	int		tz_timecnt;	/* Number of entries used */
	long		tz_times[TZ_MAX_TIMES];
					/* Saving Time transition times */
	unsigned char	tz_types[TZ_MAX_TIMES];
					/* Saving Time types for the above */
	struct dsinfo {
		long	ds_gmtoff;		/* Offset from GMT in seconds */
		char	ds_abbr[TZ_ABBR_LEN+1];	/* Time Zone abbreviation */
		char	ds_isdst;		/* Used to fill tm_isdst */
	}	tz_dsinfo[TZ_MAX_TYPES];
};
