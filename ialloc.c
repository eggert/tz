#

/*LINTLIBRARY*/

#include "stdio.h"

#if !defined lint && !defined NOID
static char	elsieid[] = "%W%";
#endif /* !defined lint && !defined NOID */

#if defined __STDC__ || defined __TURBOC__

#include "stdlib.h"
#include "string.h"

#define alloc_t	size_t

#else /* !defined __STDC__ || defined __TURBOC__ */

extern char *	calloc();
extern char *	malloc();
extern char *	realloc();
extern char *	strcpy();

#if !defined alloc_t
#define alloc_t	unsigned
#endif /* !defined alloc_t */

#endif /* !defined __STDC__ || defined __TURBOC__ */

#if defined MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#else /* !defined MAL */
#define NULLMAL(x)	((x) == NULL)
#endif /* !defined MAL */

/*
** Beat a know TurboC 1.0 bug.
*/

#if defined __TURBOC__ && __TURBOC__ == 1
#define roundup(n)	(((n) + 1) & ~1)
#else /* !(defined __TURBOC__ && __TURBOC__ == 1) */
#if !defined roundup
#define roundup(n)	(n)
#endif /* !defined roundup */
#endif /* !(defined __TURBOC__ && __TURBOC__ == 1) */

char *
imalloc(n)
{
#if defined MAL
	register char *	result;

	if (n == 0)
		n = 1;
	result = malloc((alloc_t) n);
	return NULLMAL(result) ? NULL : result;
#else /* !defined MAL */
	if (n == 0)
		n = 1;
	return malloc((alloc_t) roundup(n));
#endif /* !defined MAL */
}

char *
icalloc(nelem, elsize)
{
	if (nelem == 0 || elsize == 0)
		nelem = elsize = 1;
	return calloc((alloc_t) nelem, (alloc_t) roundup(elsize));
}

char *
irealloc(pointer, size)
char *	pointer;
{
	if (NULLMAL(pointer))
		return imalloc(size);
	if (size == 0)
		size = 1;
	return realloc(pointer, (alloc_t) roundup(size));
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
	if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
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
