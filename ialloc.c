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

static
E_oops()
{
	wildrexit("allocating memory");
}

char *
emalloc(size)
{
	register char *	ret;

	if ((ret = malloc((alloc_t) size)) == NULL)
		E_oops();
#ifdef MAL
	if (ret == MAL)
		E_oops();
#endif
	return ret;
}

char *
ecalloc(nelem, elsize)
{
	register char *	ret;

	if ((ret = calloc((alloc_t) nelem, (alloc_t) elsize)) == NULL)
		E_oops();
	return ret;
}

char *
erealloc(ptr, size)
char *	ptr;
{
	register char *	ret;

	if ((ret = realloc(ptr, (alloc_t) size)) == NULL)
		E_oops();
	return ret;
}

char *
allocat(old, new)
char *	old;
char *	new;
{
	register char *	ret;
	register	oldsize, newsize;

	if (new == NULL)
		new = "";
	newsize = strlen(new);
	if (old == NULL) {
		oldsize = 0;
		ret = ecalloc(newsize + 1, sizeof *ret);
	} else {
		oldsize = strlen(old);
		ret = erealloc(old, (oldsize + newsize + 1) * sizeof *ret);
	}
	(void) strcpy(ret + oldsize, new);
	return ret;
}

char *
allocpy(string)
char *	string;
{
	return allocat((char *) NULL, string);
}
