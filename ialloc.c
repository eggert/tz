#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#ifndef alloc_t
#define alloc_t	unsigned
#endif

extern char *	malloc();
extern char *	calloc();
extern char *	realloc();
extern char *	strcpy();

char *
imalloc(n)
{
#ifdef MAL
	register char *	result;

	result = malloc((alloc_t) n);
	return (result == MAL) ? NULL : result;
#endif
#ifndef MAL
	return malloc((alloc_t) n);
#endif
}

char *
icalloc(nelem, elsize)
{
	return calloc((alloc_t) nelem, (alloc_t) elsize);
}

char *
irealloc(pointer, size)
char *	pointer;
{
#ifdef MAL
	if (pointer == MAL)
		pointer = NULL;
#endif
	if (pointer == NULL)
		return imalloc(size);
	else	return realloc(pointer, (alloc_t) size);
}

ifree(p)
char *	p;
{
#ifdef MAL
	if (p == MAL)
		p = NULL;
#endif
	if (p != NULL)
		free(p);
}

char *
icatalloc(old, new)
char *	old;
char *	new;
{
	register char *	result;
	register	oldsize, newsize;

#ifdef MAL
	if (old == MAL)
		old = NULL;
	if (new == MAL)
		new = NULL;
#endif
	oldsize = (old == NULL) ? 0 : strlen(old);
	newsize = (new == NULL) ? 0 : strlen(new);
	if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
		if (new != NULL)
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
icpyalloc(string)
char *	string;
{
	return icatalloc((char *) NULL, string);
}

char *
emalloc(size)
{
	register char *	result;

	if ((result = imalloc(size)) == NULL)
		oops();
	return result;
}

char *
ecalloc(nelem, elsize)
{
	register char *	result;

	if ((result = icalloc(nelem, elsize)) == NULL)
		oops();
	return result;
}

char *
erealloc(ptr, size)
char *	ptr;
{
	register char *	result;

	if ((result = irealloc(ptr, size)) == NULL)
		oops();
	return result;
}

efree(p)
char *	p;
{
	ifree(p);
}

char *
ecatalloc(old, new)
char *	old;
char *	new;
{
	register char *	result;

	if ((result = icatalloc(old, new)) == NULL)
		oops();
	return result;
}

char *
ecpyalloc(string)
char *	string;
{
	register char *	result;

	if ((result = icatalloc((char *) NULL, string)) == NULL)
		oops();
	return result;
}

static
oops()
{
	wildrexit("allocating memory");
}
