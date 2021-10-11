#ifndef lint
static	char *sccsid = "@(#)vars.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Variable handling stuff.
 */

/*
 * Assign a value to a variable.
 */

assign(name, value)
	char name[], value[];
{
	register struct var *vp;
	register int h;

	if (name[0]=='-')
		deassign(name+1);
	else if (name[0]=='n' && name[1]=='o')
		deassign(name+2);
	else {
		h = hash(name);
		vp = lookup(name);
		if (vp == NOVAR) {
			vp = (struct var *) calloc(sizeof *vp, 1);
			vp->v_name = vcopy(name);
			vp->v_link = variables[h];
			variables[h] = vp;
		} else
			vfree(vp->v_value);
		vp->v_value = vcopy(value);
		/*
		 * for efficiency, intercept certain assignments here
		 */
		if (strcmp(name, "prompt")==0)
			prompt = vp->v_value;
		else if (strcmp(name, "debug")==0)
			debug = 1;
		if (debug) fprintf(stderr, "assign(%s)=%s\n", vp->v_name, vp->v_value);
	}
}

deassign(s)
register char *s;
{
	register struct var *vp, *vp2;
	register int h;

	if ((vp2 = lookup(s)) == NOVAR) {
		if (sourcing)
			return(0);
		printf("\"%s\": undefined variable\n", s);
		return(1);
	}
	if (debug) fprintf(stderr, "deassign(%s)\n", s);
	if (strcmp(s, "prompt")==0)
		prompt = NOSTR;
	else if (strcmp(s, "debug")==0)
		debug = 0;
	h = hash(s);
	if (vp2 == variables[h]) {
		variables[h] = variables[h]->v_link;
		vfree(vp2->v_name);
		vfree(vp2->v_value);
		cfree((char *)vp2);
		return(0);
	}
	for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
		;
	vp->v_link = vp2->v_link;
	vfree(vp2->v_name);
	vfree(vp2->v_value);
	cfree((char *)vp2);
	return(0);
}

/*
 * Free up a variable string.  We do not bother to allocate
 * strings whose value is "" since they are expected to be frequent.
 * Thus, we cannot free same!
 */

vfree(cp)
	register char *cp;
{
	if (!equal(cp, ""))
		cfree(cp);
}

/*
 * Copy a variable value into permanent (ie, not collected after each
 * command) space.  Do not bother to alloc space for ""
 */

char *
vcopy(str)
	char str[];
{
	register char *top, *cp, *cp2;

	if (equal(str, ""))
		return("");
	if ((top = calloc((unsigned) strlen(str)+1, 1)) == NULL)
		panic ("Out of memory");
	cp = top;
	cp2 = str;
	while (*cp++ = *cp2++)
		;
	return(top);
}

/*
 * Get the value of a variable and return it.
 * Look in the environment if its not available locally.
 */

char *
value(name)
	char name[];
{
	register struct var *vp;
	register char *cp;

	if ((vp = lookup(name)) == NOVAR)
		cp = getenv(name);
	else
		cp = vp->v_value;
	if (debug) fprintf(stderr, "value(%s)=%s\n", name, (cp)?cp:"");
	return(cp);
}

/*
 * Locate a variable and return its variable
 * node.
 */

struct var *
lookup(name)
	char name[];
{
	register struct var *vp;
	register int h;

	h = hash(name);
	for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
		if (equal(vp->v_name, name))
			return(vp);
	return(NOVAR);
}

/*
 * Locate a group name and return it.
 */

struct grouphead *
findgroup(name)
	char name[];
{
	register struct grouphead *gh;
	register int h;

	h = hash(name);
	for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
		if (equal(gh->g_name, name))
			return(gh);
	return(NOGRP);
}

/*
 * Print a group out on stdout
 */

printgroup(name)
	char name[];
{
	register struct grouphead *gh;
	register struct group *gp;

	if ((gh = findgroup(name)) == NOGRP) {
		printf("\"%s\": not a group\n", name);
		return;
	}
	printf("%s\t", gh->g_name);
	for (gp = gh->g_list; gp != NOGE; gp = gp->ge_link)
		printf(" %s", gp->ge_name);
	printf("\n");
}

/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */

hash(name)
	char name[];
{
	register unsigned h;
	register char *cp;

	for (cp = name, h = 0; *cp; h = (h << 2) + *cp++)
		;
	return(h % HSHSIZE);
}
