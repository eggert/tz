#include "stdio.h"

/*LINTLIBRARY*/

#ifndef lint
#ifndef NOID
static char	elsieid[] = "%W%";
#endif /* !defined NOID */
#endif /* !defined lint */

#ifdef __STDC__
#define HAVEHEADS
#endif /* defined __STDC__ */

#ifdef __TURBOC__
#define HAVEHEADS
#endif /* defined __TURBOC__ */

#ifdef HAVEHEADS

#include "stdlib.h"
#include "string.h"

#define alloc_t	size_t

#else /* !defined HAVEHEADS */

extern char *	calloc();
extern char *	malloc();
extern char *	realloc();
extern char *	strcpy();

#ifndef alloc_t
#define alloc_t	unsigned
#endif /* !defined alloc_t */

#endif /* !defined HAVEHEADS */

#ifdef MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#else /* !defined MAL */
#define NULLMAL(x)	((x) == NULL)
#endif /* !defined MAL */

#define nonzero(n)	(((n) == 0) ? 1 : (n))

char *
imalloc(n)
{
#ifdef MAL
	register char *	result;

	result = malloc((alloc_t) nonzero(n));
	return NULLMAL(result) ? NULL : result;
#else /* !defined MAL */
	return malloc((alloc_t) nonzero(n));
#endif /* !defined MAL */
}

char *
icalloc(nelem, elsize)
{
	if (nelem == 0 || elsize == 0)
		nelem = elsize = 1;
	return calloc((alloc_t) nelem, (alloc_t) elsize);
}

char *
irealloc(pointer, size)
char *	pointer;
{
	if (NULLMAL(pointer))
		return imalloc(size);
	return realloc(pointer, (alloc_t) nonzero(size));
}

char *
icatalloc(old, new)
char *	old;
char *	new;
{
	register char *	result;
	register	oldsize, newsize;

	newsize = NULLMAL(new) ? 0 : strlen(new);
	if (NULLMAL(old))
		oldsize = 0;
	else if (newsize == 0)
		return old;
	else	oldsize = strlen(old);
	if ((result = irealloc(old, (alloc_t) (oldsize + newsize + 1))) != NULL)
		if (!NULLMAL(new))
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
icpyalloc(string)
char *	string;
{
	return icatalloc((char *) NULL, string);
}

void
ifree(p)
char *	p;
{
	if (!NULLMAL(p))
		free(p);
}

void
icfree(p)
char *	p;
{
	if (!NULLMAL(p))
		free(p);
}
