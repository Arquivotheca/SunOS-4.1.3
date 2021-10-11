/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * @(#)lookup.c 1.1 92/07/30 SMI from UCB 5.4 6/29/88
 */

#include "defs.h"

	/* symbol types */
#define VAR	1
#define CONST	2

struct syment {
	int	s_type;
	char	*s_name;
	struct	namelist *s_value;
	struct	syment *s_next;
};

static struct syment *hashtab[HASHSIZE];

/*
 * Define a variable from a command line argument.
 */
define(name)
	char *name;
{
	register char *cp, *s;
	register struct namelist *nl;
	struct namelist *value;

	if (debug)
		printf("define(%s)\n", name);

	cp = index(name, '=');
	if (cp == NULL)
		value = NULL;
	else if (cp[1] == '\0') {
		*cp = '\0';
		value = NULL;
	} else if (cp[1] != '(') {
		*cp++ = '\0';
		value = makenl(cp);
	} else {
		nl = NULL;
		*cp++ = '\0';
		do
			cp++;
		while (*cp == ' ' || *cp == '\t');
		for (s = cp; ; s++) {
			switch (*s) {
			case ')':
				*s = '\0';
			case '\0':
				break;
			case ' ':
			case '\t':
				*s++ = '\0';
				while (*s == ' ' || *s == '\t')
					s++;
				if (*s == ')')
					*s = '\0';
				break;
			default:
				continue;
			}
			if (nl == NULL)
				value = nl = makenl(cp);
			else {
				nl->n_next = makenl(cp);
				nl = nl->n_next;
			}
			if (*s == '\0')
				break;
			cp = s;
		}
	}
	(void) lookup(name, REPLACE, value);
}

/*
 * Lookup name in the table and return a pointer to it.
 * LOOKUP - just do lookup, return NULL if not found.
 * INSERT - insert name with value, error if already defined.
 * REPLACE - insert or replace name with value.
 */

struct namelist *
lookup(name, action, value)
	char *name;
	int action;
	struct namelist *value;
{
	register unsigned n;
	register char *cp;
	register struct syment *s;
	char buf[256];

	if (debug)
		printf("lookup(%s, %d, %x)\n", name, action, value);

	n = 0;
	for (cp = name; *cp; )
		n += *cp++;
	n %= HASHSIZE;

	for (s = hashtab[n]; s != NULL; s = s->s_next) {
		if (strcmp(name, s->s_name))
			continue;
		if (action != LOOKUP) {
			if (action != INSERT || s->s_type != CONST) {
				(void)sprintf(buf, "%s redefined", name);
				yyerror(buf);
			}
		}
		return(s->s_value);
	}

	if (action == LOOKUP) {
		(void)sprintf(buf, "%s undefined", name);
		yyerror(buf);
		return(NULL);
	}

	s = ALLOC(syment);
	if (s == NULL)
		fatal("ran out of memory\n");
	s->s_next = hashtab[n];
	hashtab[n] = s;
	s->s_type = action == INSERT ? VAR : CONST;
	s->s_name = name;
	s->s_value = value;
	return(value);
}
