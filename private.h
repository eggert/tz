#ifndef PRIVATE_H

#define PRIVATE_H

/*
** This header is for use ONLY with the time zone code.
** There is no guarantee that it will remain unchanged,
** or even that it will remain at all.
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

#ifndef OK_CONST
#ifndef NO_CONST
#ifdef __STDC__
#define OK_CONST
#endif /* defined __STDC__ */
#endif /* !defined NO_CONST */
#endif /* !defined OK_CONST */

#ifndef OK_CONST
#define const
#endif /* !defined OK_CONST */

#endif /* !defined const */

/*
** void
*/

#ifndef void

#ifndef OK_VOID
#ifndef NO_VOID
#ifdef __STDC__
#define OK_VOID
#endif /* defined __STDC__ */
#ifdef sun
#define OK_VOID
#endif /* defined sun */
#ifdef vax
#define OK_VOID
#endif /* defined vax */
#endif /* !defined NO_VOID */
#endif /* !defined OK_VOID */

#ifndef OK_VOID
#define void char
#endif /* !defined OK_VOID */

#endif /* !defined void */

/*
** P((args))
*/

#ifndef P

#ifndef OK_PROTO
#ifndef NO_PROTO
#ifdef __STDC__
#define OK_PROTO
#endif /* defined __STDC__ */
#endif /* !defined NO_PROTO */
#endif /* !defined OK_PROTO */

#ifdef OK_PROTO
#define P(x)	x
#endif /* defined OK_PROTO */

#ifndef OK_PROTO
#define ASTERISK	*
#define P(x)	( /ASTERISK x ASTERISK/ )
#endif /* !defined OK_PROTO */

#endif /* !defined P */

/*
** Includes.  First the ones we expect to show up on all systems.
*/

/*
** <assert.h>
*/

#ifdef ASSERT_H
#include ASSERT_H
#endif /* defined ASSERT_H */

#ifndef ASSERT_H
#include "assert.h"
#endif /* !defined ASSERT_H */

/*
** <ctype.h>
*/

#ifdef CTYPE_H
#include CTYPE_H
#endif /* defined CTYPE_H */

#ifndef CTYPE_H
#include "ctype.h"
#endif /* !defined CTYPE_H */

/*
** <errno.h>
*/

#ifdef ERRNO_H
#include ERRNO_H
#endif /* defined ERRNO_H */

#ifndef ERRNO_H
#include "errno.h"
#endif /* !defined ERRNO_H */

/*
** <stdio.h>
*/

#ifdef STDIO_H
#include STDIO_H
#endif /* defined STDIO_H */

#ifndef STDIO_H
#include "stdio.h"
extern int	fclose P((FILE * stream));
extern int	fflush P((FILE * stream));
extern int	fprintf P((FILE * stream, const char * format, ...));
extern int	fputc P((int c, FILE * stream));
extern int	fputs P((const char * s, FILE * stream));
extern int	fread P((char * p, int s, int n, FILE * stream));
extern int	fscanf P((FILE * stream, const char * format, ...));
extern int	fseek P((FILE * stream, long int offset, int whence));
extern int	fwrite P((char * p, int s, int n, FILE * stream));
extern int	printf P((const char * format, ...));
extern int	scanf P((const char * format, ...));
extern int	sscanf P((const char * string, const char * format, ...));
extern int	ungetc P((int c, FILE * stream));
/*
** To prevent gcc squawks when in "prototypes required" mode. . .
*/
extern int	_filbuf P((FILE * stream));
extern int	_flsbuf P((unsigned char c, FILE * stream));

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

#ifdef USG
extern void	rewind P((FILE * stream));
extern int	sprintf P((char * string, const char * format, ...));
#endif /* defined USG */
#ifndef USG
extern int	rewind P((FILE * stream));
extern char *	sprintf P((char * string, const char * format, ...));
#endif /* !defined USG */

#endif /* !defined STDIO_H */

/*
** <string.h>
*/

#ifdef STRING_H
#include STRING_H
#endif /* defined STRING_H */

#ifndef STRING_H
#include "string.h"
#ifdef sun
#include "memory.h"
#endif /* defined sun */
#endif /* !defined STRING_H */

/*
** <time.h>
*/

#ifdef TIME_H
#include TIME_H
#endif /* defined TIME_H */

#ifndef TIME_H
#include "time.h"
#endif /* !defined TIME_H */

#ifdef STDLIB_H
#include STDLIB_H
#endif /* defined STDLIB_H */

#ifndef STDLIB_H
extern int	atoi P((const char * nptr));
extern char *	getenv P((char * name));
extern char *	malloc P((unsigned size));
extern char *	calloc P((unsigned nelem, unsigned elsize));
extern char *	realloc P((void * pointer, unsigned size));
extern int	system P((const char * command));
extern int	abs P((int j));

#ifdef USG
extern void	exit P((int status));
extern void	free P((void * pointer));
extern void	qsort P((void * base, unsigned nmemb, unsigned size,
			int (*compar)(const void * left, const void * right)));
#endif /* defined USG */
#ifndef USG
extern int	exit P((int status));
extern int	free P((void * pointer));
extern int	qsort P((void * base, unsigned nmemb, unsigned size,
			int (*compar)(const void * left, const void * right)));
#endif /* !defined USG */
#endif /* !defined STDLIB_H */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	0
#endif /* !defined EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	1
#endif /* !defined EXIT_FAILURE */

#ifdef __STDC__

typedef void *		genericptr_t;
#define alloc_size_t	size_t
#define qsort_size_t	size_t
#define fread_size_t	size_t
#define fwrite_size_t	size_t

#else /* !defined __STDC__ */

#ifndef genericptr_t
typedef char *		genericptr_t;
#endif /* !defined genericptr_t */

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
