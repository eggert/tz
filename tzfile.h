/* %W% */

/*
** Information about time zone files.
*/

#ifndef TZDIR
#define TZDIR		"/etc/timezones" /* Time zone object file directory */
#endif

#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	unsigned int	tt_abbrind;	/* abbreviation list index */
};

/*
** Each file begins with. . .
*/

struct tzhead {
	char		tzh_reserved[14];	/* reserved for future use */
	unsigned short	tzh_timecnt;		/* number of transition times */
	unsigned short	tzh_typecnt;		/* number of local time types */
	unsigned short	tzh_charcnt;		/* number of abbr. chars */
};

/*
** . . .followed by. . .
**
**	tzh_timecnt (long)s		transition times as returned by time(2)
**	tzh_timecnt (unsigned char)s	types of local time starting at above
**	tzh_typecnt (struct ttinfo)s	information for each time type
**	tzh_charcnt (char)s		'\0'-terminated zone abbreviaton strings
*/

/*
** In the current implementation, "settz()" refuses to deal with files that
** exceed any of the limits below.
*/

#ifndef TZ_MAX_TIMES
#define TZ_MAX_TIMES	170	/* Maximum number of transition times */
#endif

#ifndef TZ_MAX_TYPES
#define TZ_MAX_TYPES	10	/* Maximum number of local time types */
#endif

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
#endif
