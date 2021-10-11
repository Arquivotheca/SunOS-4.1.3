#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)setenv.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <strings.h>
#include <sunwindow/sv_malloc.h>

/*
 * Implement setenv and unsetenv
 */

extern	char **environ, *calloc();

static char **
blkend(up)
	register char **up;
{

	while (*up)
		up++;
	return (up);
}
 
static
blklen(av)
	register char **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

static char **
blkcpy(oav, bv)
	char **oav;
	register char **bv;
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

static char **
blkcat(up, vp)
	char **up, **vp;
{

	(void)blkcpy(blkend(up), vp);
	return (up);
}

/*
static
blkfree(av0)
	char **av0;
{
	register char **av = av0;

	while (*av)
		xfree(*av++);
	xfree((char *)av0);
}*/

static char *
strspl(cp, dp)
	register char *cp, *dp;
{
	register char *ep = sv_calloc(1, (unsigned)(strlen(cp) + strlen(dp) + 1));

	(void)strcpy(ep, cp);
	(void)strcat(ep, dp);
	return (ep);
}

static char **
blkspl(up, vp)
	register char **up, **vp;
{
	register char **wp = (char **) sv_calloc(
	    (unsigned)(blklen(up) + blklen(vp) + 1), sizeof (char **));

	(void)blkcpy(wp, up);
	return (blkcat(wp, vp));
}

static
xfree(cp)
	char *cp;
{
	extern int end;

	if (cp >= (char *)end && cp < (char *) &cp)
		free(cp);
}

setenv(name, value)
	char *name, *value;
{
	register char **ep = environ;
	register char *cp, *dp;
	char *blk[2], **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = strspl("=", value);
		xfree(*ep);
		*ep = strspl(name, cp);
		xfree(cp);
		return;
	}
	blk[0] = strspl(name, "="); blk[1] = 0;
	environ = blkspl(environ, blk);
	xfree((char *)oep);
	(void)setenv(name, value);
}

unsetenv(name)
	char *name;
{
	register char **ep = environ;
	register char *cp, *dp;
	char **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = *ep;
		*ep = 0;
		environ = blkspl(environ, ep+1);
		*ep = cp;
		xfree(cp);
		xfree((char *)oep);
		return;
	}
}

