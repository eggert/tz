#include "objectid.h"
OBJECTID("%W%")

/*LINTLIBRARY*/

#include <stdio.h>
#include "alloc.h"

extern char *	malloc();
extern char *	realloc();
extern char *	strcpy();
extern char *	strcat();

char *		allocpy(string)
register char *	string;
{
	register char *		copy;
	register arg4alloc	n;

	n = (string == NULL) ? 0 : strlen(string);
	copy = malloc(n + 1);
	if (copy == MAL || copy == NULL)
		return NULL;
	if (string == NULL)
		*copy = '\0';
	else	(void) strcpy(copy, string);
	return copy;
}

char *		allocat(old, new)
register char *	old;
char *		new;
{
	register arg4alloc	n;

	n = (old == NULL) ? 0 : strlen(old);
	if (new != NULL)
		n += strlen(new);
	++n;
	if (old == MAL || old == NULL) {
		old = malloc(n);
		if (old == NULL)
			return NULL;
		*old = '\0';
	} else {
		old = realloc(old, n);
		if (old == NULL)
			return NULL;
	}
	if (new != NULL)
		(void) strcat(old, new);
	return old;
}
