#ifndef lint
static	char sccsid[] = "@(#)parse.c 1.1 92/07/30 SMI"; /* from UCB 1.2 85/04/24 */
#endif

/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */

#include "externs.h"

wordinit()
{
	register struct wlist *w;

	for (w = wlist; w->string; w++)
		install(w);
}

hash(s)
	register char *s;
{
	register hashval = 0;

	while (*s) {
		hashval += *s++;
		hashval *= HASHMUL;
		hashval &= HASHMASK;
	}
	return hashval;
}

struct wlist *
lookup(s)
	char *s;
{
	register struct wlist *wp;

	for (wp = hashtab[hash(s)]; wp != NULL; wp = wp->next)
		if (*s == *wp->string && strcmp(s, wp->string) == 0)
			return wp;
	return NULL;
}

install(wp)
	register struct wlist *wp;
{
	int hashval;

	if (lookup(wp->string) == NULL) {
		hashval = hash(wp->string);
		wp->next = hashtab[hashval];
		hashtab[hashval] = wp;
	} else
		printf("Multiply defined %s.\n", wp->string);
}

parse()
{
	register struct wlist *wp;
	register n;

	wordnumber = 0;           /* for cypher */
	for (n = 0; n <= wordcount; n++) {
		if ((wp = lookup(words[n])) == NULL) {
			wordvalue[n] = -1;
			wordtype[n] = -1;
		} else {
			wordvalue[n] = wp -> value;
			wordtype[n] = wp -> article;
		}
	}
}
