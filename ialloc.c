#

/*LINTLIBRARY*/

#include "stdio.h"

#ifndef lint
#ifndef NOID
static char	sccsid[] = "%W%";
#endif /* !NOID */
#endif /* !lint */

#ifndef alloc_t
#define alloc_t	unsigned
#endif

#ifdef MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#endif
#ifndef MAL
#define NULLMAL(x)	((x) == NULL)
#endif

extern char *	calloc();
extern char *	malloc();
extern char *	realloc();
extern char *	strcpy();

char *
imalloc(n)
{
#ifdef MAL
	register char *	result;

	if (n == 0)
		n = 1;
	result = malloc((alloc_t) n);
	return (result == MAL) ? NULL : result;
#endif
#ifndef MAL
	if (n == 0)
		n = 1;
	return malloc((alloc_t) n);
#endif
}

char *
icalloc(nelem, elsize)
{
	if (nelem == 0)
		nelem = 1;
	if (elsize == 0)
		elsize = 1;
	return calloc((alloc_t) nelem, (alloc_t) elsize);
}

char *
irealloc(pointer, size)
char *	pointer;
{
	if (size == 0)
		size = 1;
	if (NULLMAL(pointer))
		return imalloc(size);
	else	return realloc(pointer, (alloc_t) size);
}

char *
icatalloc(old, new)
char *	old;
char *	new;
{
	register char *	result;
	register	oldsize, newsize;

	oldsize = NULLMAL(old) ? 0 : strlen(old);
	newsize = NULLMAL(new) ? 0 : strlen(new);
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

ifree(p)
char *	p;
{
	if (!NULLMAL(p))
		free(p);
}
