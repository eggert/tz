#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#ifndef arg4alloc
#define arg4alloc	unsigned
#endif

#ifndef MAL
#define MAL		0
#endif

extern char *		malloc();
extern char *		calloc();
extern char *		realloc();
extern char *		strcat();

static	E_oops()
{
	wildrexit("allocating memory");
}

char *	emalloc(size)
{
	register char *	ret;

	if ((ret = malloc((arg4alloc) size)) == MAL || ret == NULL)
		E_oops();
	return ret;
}

char *	ecalloc(nelem, elsize)
{
	register char *	ret;

	if ((ret = calloc((arg4alloc) nelem, (arg4alloc) elsize)) == NULL)
		E_oops();
	return ret;
}

char *	erealloc(ptr, size)
char *	ptr;
{
	register char *	ret;

	if ((ret = realloc(ptr, (arg4alloc) size)) == NULL)
		E_oops();
	return ret;
}

char *	allocat(old, new)
char *	old;
char *	new;
{
	register char *	ret;
	register	toalloc;

	if (new == NULL)
		new = "";
	toalloc = strlen(new) + 1;
	return (old == NULL) ? ecalloc(toalloc, sizeof *ret) :
		erealloc(old, (toalloc + strlen(old)) * sizeof *ret);
}

char *	allocpy(string)
char *	string;
{
	return allocat((char *) NULL, string);
}
