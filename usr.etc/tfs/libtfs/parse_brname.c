#ifndef lint
static char sccsid[] = "@(#)parse_brname.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include <strings.h>

/*
 * Parse a branch name of the form 'branch[/variant][@user[@machine]]' into
 * the branch & variant parts.  (Puts the [@user[@machine]] at the end of
 * the returned branch name.)
 */
void
nse_parse_branch_name(fullbrname, branch, variant)
	char		*fullbrname;
	char		*branch;
	char		*variant;
{
	char		tmpname[MAXPATHLEN];
	char		*cp;
	char		*cp2;

	strcpy(tmpname, fullbrname);
	cp = index(tmpname, '/');
	if (cp != NULL) {
		*cp = '\0';
		cp++;
	}
	if (branch != NULL) {
		strcpy(branch, tmpname);
		if (cp != NULL) {
			cp2 = index(cp, '@');
			if (cp2 != NULL) {
				strcat(branch, cp2);
			}
		}
	}
	if (variant != NULL) {
		if (cp == NULL) {
			*variant = '\0';
		} else {
			strcpy(variant, cp);
			cp = index(variant, '@');
			if (cp != NULL) {
				*cp = '\0';
			}
		}
	}
}

/* reassemble a branch name from branch[@user[@machine]] and variant
 * arguments
 */
void
nse_unparse_branch_name(branch, variant, fullname)
	char		*branch;
	char		*variant;
	char		*fullname;
{
	char		*p;

	p = index(branch, '@');
	if (p) {
		*p = '\0';
	}
	strcpy(fullname, branch);
	if (variant && *variant) {
		nse_pathcat(fullname, variant);
	}
	if (p) {
		*p = '@';
		strcat(fullname, p);
	}
}
