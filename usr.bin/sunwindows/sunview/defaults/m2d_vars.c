#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)m2d_vars.c 1.1 92/07/30";
#endif
#endif

#include "m2d_def.h"
#include <ctype.h>
#include <sunwindow/sun.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Variable handling stuff.
 */

/*
 * Assign a value to a variable.
 */

struct      grouphead       *m2d_groups[HSHSIZE];/* Pointer to active groups */
static  struct      var     *variables[HSHSIZE];    /* Pointer to active var list */

m2d_assign(name, value)
	char name[], value[];
{
	register struct var *vp;
	register int h;
	int	assign;
	char	*stem;

	stem = name;
	if (name[0]=='-'){
		stem = name + 1;
		assign = 0;
	}
	else if (name[0]=='n' && name[1]=='o'){
		stem = name + 2;
		assign = 0;
	}
	else {
		stem = name;
		assign = 1;
	}
	h = m2d_hash(stem);
	vp = m2d_lookup(stem);
	if (vp == NOVAR) {
		vp = (struct var *) (LINT_CAST(calloc(
			1, (unsigned)(sizeof *vp))));
		vp->v_name = m2d_vcopy(stem);
		vp->v_link = variables[h];
		variables[h] = vp;
	} else
		vfree(vp->v_value);
	vp->v_value = m2d_vcopy(value);
	vp->v_assign = assign;
}

m2d_deassign(s)
register char *s;
{
	register struct var *vp, *vp2;
	register int h;

	if ((vp2 = m2d_lookup(s)) == NOVAR) 
		return(0);
	h = m2d_hash(s);
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

static
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
m2d_vcopy(str)
	char str[];
{
	register char *top, *cp, *cp2;

	if (equal(str, ""))
		return("");
	top = calloc(1, (unsigned)(strlen(str)+1));
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
m2d_value(name)
	char name[];
{
	register struct var *vp;
	register char *cp;

	if ((vp = m2d_lookup(name)) == NOVAR)
		cp = getenv(name);
	else
		cp = vp->v_value;
	return(cp);
}

/*
 * Locate a variable and return its variable
 * node.
 */

struct var *
m2d_lookup(name)
	char name[];
{
	register struct var *vp;
	register int h;

	h = m2d_hash(name);
	for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
		if (equal(vp->v_name, name))
			return(vp);
	return(NOVAR);
}

/*
 * Locate a group name and return it.
 */

struct grouphead *
m2d_findgroup(name)
	char name[];
{
	register struct grouphead *gh;
	register int h;

	h = m2d_hash(name);
	for (gh = m2d_groups[h]; gh != NOGRP; gh = gh->g_link)
		if (equal(gh->g_name, name))
			return(gh);
	return(NOGRP);
}

/*
 * Print a group out on stdout
 */

m2d_printgroup(name)
	char name[];
{
	register struct grouphead *gh;
	register struct group *gp;

	if ((gh = m2d_findgroup(name)) == NOGRP) {
		(void)printf("\"%s\": not a group\n", name);
		return;
	}
	(void)printf("%s\t", gh->g_name);
	for (gp = gh->g_list; gp != NOGE; gp = gp->ge_link)
		(void)printf(" %s", gp->ge_name);
	(void)printf("\n");
}

/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */

m2d_hash(name)
	char name[];
{
	register unsigned h;
	register char *cp;

	for (cp = name, h = 0; *cp; h = (h << 2) + *cp++)
		;
	return(h % HSHSIZE);
}

m2d_get_sets()
{
	int	h;
	struct var *vp;

	for (h = 0; h < HSHSIZE; h++) 
		for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
			if (isdigit((*(vp->v_value))))
				m2d_set_integer(vp->v_name, atoi(vp->v_value));
			else if (vp->v_value[0] != '\0')
				m2d_set_string("Set", vp->v_name, vp->v_value);
			else
				m2d_set_boolean(vp->v_name, vp->v_assign);
}

m2d_get_aliases()
{
	int	h, len, inc;
	struct grouphead *gh;
	char	alias_list[2048];
	struct group *gp;

	for (h = 0; h < HSHSIZE; h++) 
		for (gh = m2d_groups[h]; gh != NOGRP; gh = gh->g_link) {
			alias_list[0] = '\0';
			len = 0;
			for (gp = gh->g_list; gp != NOGE; gp = gp->ge_link) {
				inc = strlen(gp->ge_name);
				if (len + inc + 1 >= sizeof(alias_list)) {
					(void)fprintf(stderr, 
						"mailrc_to_defaults: alias list for %s is too long, maximum length is %d\n", 
						gh->g_name, sizeof(alias_list) - 1);
					return(-1);
				}
				(void)strcpy(alias_list + len, gp->ge_name);
				len += inc;
				(void)strcpy(alias_list + len, " ");
				len++;
			}
			if (len > 0)
				alias_list[len - 1] = '\0';
				
			m2d_set_string("Alias", gh->g_name, alias_list);
		}
	return 0;
}
