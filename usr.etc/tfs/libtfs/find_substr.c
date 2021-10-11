#ifndef lint
static char sccsid[] = "@(#)find_substr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <nse/param.h>

char *
nse_find_substring(str, substr)
	char		*str;
	char		*substr;
{
	int		len = strlen(substr);

	while ((*str != '\0') &&
	       ((*str != substr[0]) || (strncmp(str, substr, len) != 0))) {
		str++;
	}
	if (*str != '\0') {
		return (str);
	} else {
		return (NULL);
	}
}
