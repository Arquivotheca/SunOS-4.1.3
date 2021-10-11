#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getttynam.c 1.1 92/07/30 SMI"; /* from UCB 5.2 3/9/86 */
#endif not lint
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <ttyent.h>

struct ttyent *
getttynam(tty)
	char *tty;
{
	register struct ttyent *t;

	setttyent();
	while (t = getttyent()) {
		if (strcmp(tty, t->ty_name) == 0)
			break;
	}
	endttyent();
	return (t);
}
