#ifndef PRIVATE_H

#define PRIVATE_H

/*
** This header is for use ONLY with the time zone code.
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
** P((args))
*/

#ifndef P
#ifdef __STDC__
#define P(x)	x
#else /* !defined __STDC__ */
#define ASTERISK	*
#define P(x)	( /ASTERISK x ASTERISK/ )
#endif /* !defined __STDC__ */
#endif /* !defined P */

/*
** genericptr_t
*/

typedef void *		genericptr_t;

#include "stdio.h"
#include "ctype.h"
#include "errno.h"
#include "string.h"
#include "time.h"

#ifndef remove
extern int	unlink P((const char * filename));
#define remove	unlink
#endif /* !defined remove */

#ifndef FILENAME_MAX

#ifndef MAXPATHLEN
#ifdef unix
#include <sys/param.h>
#endif /* defined unix */
#endif /* !defined MAXPATHLEN */

#ifdef MAXPATHLEN
#define FILENAME_MAX	MAXPATHLEN
#else /* !defined MAXPATHLEN */
#define FILENAME_MAX	1024		/* Pure guesswork */
#endif /* !defined MAXPATHLEN */

#endif /* !defined FILENAME_MAX */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	0
#endif /* !defined EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	1
#endif /* !defined EXIT_FAILURE */

#ifdef __STDC__

#define alloc_size_t	size_t
#define qsort_size_t	size_t
#define fread_size_t	size_t
#define fwrite_size_t	size_t

#else /* !defined __STDC__ */

#ifndef alloc_size_t
typedef unsigned	alloc_size_t;
#endif /* !defined alloc_size_t */

#ifndef qsort_size_t
#ifdef unix
#include "sys/param.h"
#endif /* defined unix */
#ifdef BSD
typedef int		qsort_size_t;
#else /* !defined BSD */
typedef unsigned	qsort_size_t;
#endif /* !defined BSD */
#endif /* !defined qsort_size_t */

#ifndef fread_size_t
typedef int		fread_size_t;
#endif /* !defined fread_size_t */

#ifndef fwrite_size_t
typedef int		fwrite_size_t;
#endif /* !defined fwrite_size_t */

#endif /* !defined __STDC__ */

/*
** UNIX is a registered trademark of AT&T.
** VAX is a trademark of Digital Equipment Corporation.
*/

#endif /* !defined PRIVATE_H */
