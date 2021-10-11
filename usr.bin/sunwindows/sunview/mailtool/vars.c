#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)vars.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - variable handling, stolen from Mail
 */

#include <stdio.h>
#include <sys/types.h>
#include <sunwindow/sun.h>
#include "glob.h"

extern int mt_debugging;
char	*calloc();
static	void vfree(), mt_puthash();

/*
 * Structure of a variable node.  All variables are
 * kept on a singly-linked list of these, rooted by
 * "variables"
 */
struct var {
	struct	var *v_link;		/* Forward link to next variable */
	char	*v_name;		/* The variable's name */
	caddr_t	v_value;		/* And it's current value */
};

#define	HSHSIZE	19
static struct	var *variables[HSHSIZE];	/* Pointer to active var list */

char	*mt_value();
static char	*vcopy();
static struct	var *lookup();

/*
 * Assign a value to a mail variable.
 */
void
mt_assign(name, val)
	char name[], val[];
{
	if (name[0]=='-')
		(void) mt_deassign(name+1);
	else if (name[0]=='n' && name[1]=='o')
		(void) mt_deassign(name+2);
	else mt_puthash(name, vcopy(val), variables);
}

/*
 * associate val with name in hasharray
 */
static void
mt_puthash(name, val, hasharray)
	char name[];
	caddr_t	val;
	struct	var *hasharray[];
{
	register struct var *vp;
	register int h;

	vp = lookup(name, hasharray);
	if (vp == (struct var *)NULL) {
		h = hash(name);
		vp = (struct var *) (LINT_CAST(calloc(sizeof *vp, 1)));
		vp->v_name = vcopy(name);
		vp->v_link = hasharray[h];
		hasharray[h] = vp;
	} else
		vfree(vp->v_value);
	vp->v_value = val;
}

int
mt_deassign(s)
	register char *s;
{
	register struct var *vp, *vp2;
	register int h;

	if ((vp2 = lookup(s, variables)) == (struct var *)NULL) {
		if (mt_debugging)
		    (void)fprintf(stderr, "\"%s\": undefined variable\n", s);
		return (1);
	}
	h = hash(s);
	if (vp2 == variables[h]) {
		variables[h] = variables[h]->v_link;
		vfree(vp2->v_name);
		vfree(vp2->v_value);
		cfree((char *)vp2);
		return (0);
	}
	for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
		;
	vp->v_link = vp2->v_link;
	vfree(vp2->v_name);
	vfree(vp2->v_value);
	cfree((char *)vp2);
	return (0);
}

/*
 * Free up a variable string.  We do not bother to allocate
 * strings whose value is "" since they are expected to be frequent.
 * Thus, we cannot free same!
 */
static void
vfree(cp)
	register char *cp;
{

	if (strcmp(cp, "") != 0)
		cfree(cp);
}

/*
 * Copy a variable value into permanent space.
 * Do not bother to alloc space for "".
 */
static char *
vcopy(str)
	char str[];
{

	if (strcmp(str, "") == 0)
		return ("");
	return (mt_savestr(str));
}

/*
 * Get the value of a variable and return it.
 * Look in the environment if its not available locally.
 */
char *
mt_value(name)
	char name[];
{
	register struct var *vp;
	register char *cp;

	if ((vp = lookup(name, variables)) == (struct var *)NULL)
		cp = getenv(name);
	else
		cp = vp->v_value;
	return (cp);
}

/*
 * Locate a variable and return its variable
 * node.
 */
static struct var *
lookup(name, hasharray)
	char name[];
	struct	var *hasharray[];

{
	register struct var *vp;
	register int h;

	h = hash(name);
	for (vp = hasharray[h]; vp != (struct var *)NULL; vp = vp->v_link)
		if (strcmp(vp->v_name, name) == 0)
			return (vp);
	return ((struct var *)NULL);
}

/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */
static int
hash(name)
	char name[];
{
	register unsigned h;
	register char *cp;

	for (cp = name, h = 0; *cp; h = (h << 2) + *cp++)
		;
	return (h % HSHSIZE);
}
