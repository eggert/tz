#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

/*LINTLIBRARY*/

#include "stdio.h"

#ifndef arg4alloc
#define arg4alloc	unsigned
#endif

char *	allocat(old, new)
char *	old;
char *	new;
{
	register char *	ret;
	arg4alloc	toalloc;
	extern char *	calloc();
	extern char *	realloc();
	extern char *	strcat();

	if (new == NULL)
		new = "";
	toalloc = strlen(new) + 1;
	ret = (old == NULL) ? calloc(toalloc, sizeof *ret) :
		realloc(old, (toalloc + strlen(old)) * sizeof *ret);
	if (ret != NULL)
		(void) strcat(ret, new);
	return ret;
}

char *	allocpy(string)
char *	string;
{
	return allocat((char *) NULL, string);
}
