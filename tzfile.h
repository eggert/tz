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
/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES	370
#endif /* !TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
#ifndef NOSOLAR
#define TZ_MAX_TYPES	256	/* Limited by what (unsigned char)'s can hold */
#else /* !NOSOLAR */
#define TZ_MAX_TYPES	10	/* Maximum number of local time types */
#endif /* !NOSOLAR */
#endif /* !TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
#endif /* !TZ_MAX_CHARS */

#define SECS_PER_MIN	60
#define MINS_PER_HOUR	60
#define HOURS_PER_DAY	24
#define DAYS_PER_WEEK	7
#define DAYS_PER_NYEAR	365
#define DAYS_PER_LYEAR	366
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define MONS_PER_YEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY
