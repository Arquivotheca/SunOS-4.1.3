#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getsubopt.c 1.1 92/07/30 SMI"; /* created from scratch */
#endif

/*
 * getsubopt - parse suboptions from a flag argument.
 *
 * Copyright 1988, Sun Microsystems
 *
 * THIS IS PROPRIETARY UNPUBLISHED SOURCE CODE FROM SUN MICROSYSTEMS
 * IT IS COMPANY CONFIDENTIAL INFORMATION AND NOT TO BE DISCLOSED
 */
#include <string.h>
#include <stdio.h>

int
getsubopt(optionsp, tokens, valuep)
	char **optionsp;
	char *tokens[];
	char **valuep;
{
	register char *s = *optionsp, *p;
	register int i, optlen;

	*valuep = NULL;
	if (*s == '\0')
		return (-1);
	p = strchr(s, ',');		/* find next option */
	if (p == NULL) {
		p = s + strlen(s);
	} else {
		*p++ = '\0';		/* mark end and point to next */
	}
	*optionsp = p;			/* point to next option */
	p = strchr(s, '=');		/* find value */
	if (p == NULL) {
		optlen = strlen(s);
		*valuep = NULL;
	} else {
		optlen = p - s;
		*valuep = ++p;
	}
	for (i = 0; tokens[i] != NULL; i++) {
		if ((optlen == strlen(tokens[i])) &&
		    (strncmp(s, tokens[i], optlen) == 0))
			return (i);
	}
	/* no match, point value at option and return error */
	*valuep = s;
	return (-1);
}

