#ifndef PRIVATE_H

#define PRIVATE_H

/*
** This header is for use ONLY with the time conversion code.
** There is no guarantee that it will remain unchanged,
** or that it will remain at all.
** Do NOT copy it to any system include directory.
** Thank you!
*/

/*
** ID
*/

#ifndef lint
#ifndef NOID
static char	privatehid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Here are defaults for various preprocessor symbols.
** You can override these in your C compiler options, e.g. `-DHAVE_ADJTIME=0'.
*/

#ifndef HAVE_ADJTIME
#define HAVE_ADJTIME	1
#endif /* !defined HAVE_ADJTIME */
#ifndef HAVE_MKDIR
#define HAVE_MKDIR	1
#endif /* !defined HAVE_MKDIR */
#ifndef HAVE_SETLOCALE
#define HAVE_SETLOCALE	1
#endif /* !defined HAVE_SETLOCALE */
#ifndef HAVE_SETTIMEOFDAY
#define HAVE_SETTIMEOFDAY	3
#endif /* !defined HAVE_SETTIMEOFDAY */
#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H	1
#endif /* !defined HAVE_STDLIB_H */
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H	1
#endif /* !defined HAVE_UNISTD_H */

#ifndef LOCALE_HOME
#define LOCALE_HOME	"/usr/lib/locale"
#endif /* !defined LOCALE_HOME */

#ifndef alloc_size_T
#define alloc_size_T	size_t
#endif /* !defined alloc_size_T */

#ifndef fwrite_size_T
#define fwrite_size_T	size_t
#endif /* !defined fwrite_size_T */

#ifndef qsort_size_T
#define qsort_size_T	size_t
#endif /* !defined qsort_size_T */

/*
** const
*/

#ifndef const
#ifndef __STDC__
#define const
#endif /* !defined __STDC__ */
#endif /* !defined const */

/*
** void
*/

#ifndef void
#ifndef __STDC__
#ifndef vax
#ifndef sun
#define void	char
#endif /* !defined sun */
#endif /* !defined vax */
#endif /* !defined __STDC__ */
#endif /* !defined void */

/*
** INITIALIZE
*/

#ifndef GNUC_or_lint
#ifdef lint
#define GNUC_or_lint
#endif /* defined lint */
#ifdef __GNUC__
#define GNUC_or_lint
#endif /* defined __GNUC__ */
#endif /* !defined GNUC_or_lint */

#ifndef INITIALIZE
#ifdef GNUC_or_lint
#define INITIALIZE(x)	((x) = 0)
#endif /* defined GNUC_or_lint */
#ifndef GNUC_or_lint
#define INITIALIZE(x)
#endif /* !defined GNUC_or_lint */
#endif /* !defined INITIALIZE */

/*
** P((args))
*/

#ifndef P
#ifdef __STDC__
#define P(x)	x
#endif /* defined __STDC__ */
#ifndef __STDC__
#define P(x)	()
#endif /* !defined __STDC__ */
#endif /* !defined P */

/*
** generic_T
*/

#ifdef __STDC__
#define generic_T	void
#endif /* defined __STDC__ */
#ifndef __STDC__
#define generic_T	char
#endif /* !defined __STDC__ */

#include "sys/types.h"	/* for time_t */
#include "stdio.h"
#include "ctype.h"
#include "errno.h"
#include "string.h"
#include "limits.h"	/* for CHAR_BIT */
#ifndef _TIME_
#include "time.h"
#endif /* !defined _TIME_ */

#if HAVE_STDLIB_H
#include "stdlib.h"
#endif /* HAVE_STDLIB_H */

#if HAVE_UNISTD_H
#include "unistd.h"
#endif /* HAVE_UNISTD_H */

#ifndef remove
extern int	unlink P((const char * filename));
#define remove	unlink
#endif /* !defined remove */

#ifndef FILENAME_MAX

#ifndef MAXPATHLEN
#ifdef unix
#include "sys/param.h"
#endif /* defined unix */
#endif /* !defined MAXPATHLEN */

#ifdef MAXPATHLEN
#define FILENAME_MAX	MAXPATHLEN
#endif /* defined MAXPATHLEN */
#ifndef MAXPATHLEN
#define FILENAME_MAX	1024		/* Pure guesswork */
#endif /* !defined MAXPATHLEN */

#endif /* !defined FILENAME_MAX */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	0
#endif /* !defined EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	1
#endif /* !defined EXIT_FAILURE */

/*
** Ensure that these are declared--redundantly declaring them shouldn't hurt.
*/

extern char *		getenv P((const char * name));
extern generic_T *	malloc P((alloc_size_T size));
extern generic_T *	calloc P((alloc_size_T nelem, alloc_size_T elsize));
extern generic_T *	realloc P((generic_T * oldptr, alloc_size_T newsize));

extern void		exit P((int s));
extern void		qsort P((generic_T * base, qsort_size_T nelem,
				qsort_size_T elsize,
				int (*comp) P((const generic_T *,
					const generic_T *))));
extern void		perror P((const char * string));
extern void		free P((generic_T * buf));

#ifndef TRUE
#define TRUE	1
#endif /* !defined TRUE */

#ifndef FALSE
#define FALSE	0
#endif /* !defined FALSE */

#ifndef INT_STRLEN_MAXIMUM
/*
** 302 / 1000 is log10(2.0) rounded up.
** Subtract one for the sign bit;
** add one for integer division truncation;
** add one more for a minus sign.
*/
#define INT_STRLEN_MAXIMUM(type) \
	((sizeof(type) * CHAR_BIT - 1) * 302 / 1000 + 2)
#endif /* !defined INT_STRLEN_MAXIMUM */

#if HAVE_MKDIR
#define emkdir(name, mode) mkdir(name, mode)
#endif /* HAVE_MKDIR */

#ifndef LOCALE_HOME
#define LOCALE_HOME	"/usr/lib/locale"
#endif /* !defined LOCALE_HOME */

/*
** UNIX was a registered trademark of UNIX System Laboratories in 1993.
** VAX is a trademark of Digital Equipment Corporation.
*/

#endif /* !defined PRIVATE_H */
