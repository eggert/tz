#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "%W%";
#endif

#include "ctype.h"

#ifndef arg4alloc
#define arg4alloc	unsigned
#endif

#ifndef MAL
#define MAL	0
#endif

char *	scheck(string, format)
char *	string;
char *	format;
{
	register char *	fbuf;
	register char *	fp;
	register char *	tp;
	register int	c;
	register char *	result;

	if (string == NULL || format == NULL)
		return "";
	fbuf = malloc((arg4alloc) (2 * strlen(format) + 4));
	if (fbuf == MAL)
		return "";
	fp = format;
	tp = fbuf;
	while ((*tp++ = c = *fp++) != '\0') {
		if (c != '%')
			continue;
		if (*fp == '%') {
			*tp++ = *fp++;
			continue;
		}
		if (*fp == '*')
			*tp++ = *fp++;
		else	*tp++ = '*';
		while (*fp != '\0' && isascii(*fp) && isdigit(*fp))
			*tp++ = *fp++;
		if (*fp == 'l')
			*tp++ = *fp++;
		else if (*fp == '[')
			do *tp++ = *fp++;
				while (*fp != '\0' && *fp != ']');
		if ((*tp++ = *fp++) == '\0')
			break;
	}
	if (c != '\0')
		result = "";
	else if (sscanf(string, fbuf) != EOF)
		result = "";
	else	result = format;
	free(fbuf);
	return result;
}
