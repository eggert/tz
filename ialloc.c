#include "objectid.h"
OBJECTID("%W%")

/*LINTLIBRARY*/

#include <stdio.h>
#include "alloc.h"

char *	allocat(old, new)
char *	old;
char *	new;
{
	register char *	ret;
	register	len;
	extern char *	calloc();
	extern char *	realloc();
	extern char *	strcat();

	if (new == NULL)
		new = "";
	len = strlen(new) + 1;
	ret = (old == NULL) ? calloc((arg4alloc) len, sizeof *ret) :
		realloc(old, (arg4alloc) ((len + strlen(old)) * sizeof *ret));
	if (ret != NULL)
		(void) strcat(ret, new);
	return ret;
}

char *	allocpy(string)
char *	string;
{
	return allocat((char *) NULL, string);
}
