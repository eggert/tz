#ifndef lint
#ifndef NOID
static char	tzfilehid[] = "%W%";
#endif /* !NOID */
#endif /* !lint */

/*
** Information about time zone files.
*/

#ifndef TZDIR
#define TZDIR		"/etc/zoneinfo"	/* Time zone object file directory */
#endif /* !TZDIR */

#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif /* !TZDEFAULT */

/*
** Each file begins with. . .
*/

struct tzhead {
	char	tzh_reserved[32];	/* reserved for future use */
	char	tzh_timecnt[4];		/* coded number of transition times */
	char	tzh_typecnt[4];		/* coded number of local time types */
	char	tzh_charcnt[4];		/* coded number of abbr. chars */
};

/*
** . . .followed by. . .
**
**	tzh_timecnt (char [4])s		coded transition times a la time(2)
**	tzh_timecnt (unsigned char)s	types of local time starting at above
**	tzh_typecnt repetitions of
**		one (char [4])		coded GMT offset in seconds
**		one (unsigned char)	used to set tm_isdt
**		one (unsigned char)	that's an abbreviation list index
**	tzh_charcnt (char)s		'\0'-terminated zone abbreviaton strings
*/

/*
** In the current implementation, "settz()" refuses to deal with files that
** exceed any of the limits below.
*/

#ifndef TZ_MAX_TIMES
#ifdef SAUDI
#define TZ_MAX_TIMES	370	/* A bit more than a year's worth */
#else /* !SAUDI */
#define TZ_MAX_TIMES	300	/* Maximum number of transition times */
#endif /* !SAUDI */
#endif /* !TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
#ifdef SAUDI
#define TZ_MAX_TYPES	370	/* A bit more than a year's worth */
#else /* !SAUDI */
#define TZ_MAX_TYPES	10	/* Maximum number of local time types */
#endif /* !SAUDI */
#endif /* !TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
#endif /* !TZ_MAX_CHARS */
