#ifndef lint
static char sccsid[] = "@(#)abspath.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/dir.h>

#define	MAXSYMLINK	20

/*
 * Given any filename, determine its absolute path name guaranteeing
 * no symbolic links in the name
 */
char *
nse_abspath(argname, result)
	char *argname;
	char *result;
{
	char relname[MAXPATHLEN];
	char tmpname[MAXPATHLEN];
	char tmpseg[MAXNAMLEN];
	char *relp, *absp;
	char *p;
	int cc;
	char *index(), *rindex();
	struct stat stbuf;
	int nlink = 0;
	char *getwd();

	(void) strcpy(relname, argname);
	if (relname[0] != '/') {
		if (getwd(result) == NULL) /* NOTE: do not use _nse_getwd! */
			return ((char *)NULL);
	}
checkforroot:
	/* see if arg or symlink is relative to root */
	relp = relname;
	if (*relp == '/') {
		result[0] = '\0';
		relp++;
	}
nextseg:
	/* strip leading slashes */
	while (*relp == '/')
		relp++;
	absp = result + strlen(result) - 1;
	if (*relp == '\0') {
		/* all done - strip trailing slashes */
		while (absp > result && *absp == '/')
			*absp-- = '\0';
		goto out;
	}
	/* put slash in result if it needs one */
	if (absp < result) {
		result[0] = '/';
		result[1] = '\0';
		absp = result;
	} else if (*absp != '/') {
		*++absp = '/';
		*++absp = '\0';
	}

	/* get next component */
	p = index(relp, '/');
	if (p) {
		(void) strncpy(tmpseg, relp, p-relp);
		tmpseg[p-relp] = '\0';
		p++;
	} else {
		(void) strcpy(tmpseg, relp);
		p = "";
	}

	/* "." - nothing changes */
	if (strcmp(tmpseg, ".") == 0) {
		relp = p;
		goto nextseg;
	}

	/* ".." - strip previous name */
	if (strcmp(tmpseg, "..") == 0) {
		relp = p;
		p = rindex(result, '/');
		/* get past trailing slashes */
		while (p && p[1] == '\0') {
			*p = '\0';
			p = rindex(result, '/');
		}
		if (p == NULL)
			p = result;
		*p = '\0';
		goto nextseg;
	}
	(void) strcpy(tmpname, result);
	(void) strcat(tmpname, tmpseg);
	if (lstat(tmpname, &stbuf) != 0) {
		(void) strcat(result, relp);
		goto out;
	}
	switch (stbuf.st_mode & S_IFMT) {
	case S_IFLNK:
		if (++nlink > MAXSYMLINK) {
			(void) strcat(result, tmpseg);
			relp = p;
			goto nextseg;
		}
		cc = readlink(tmpname, tmpname, sizeof tmpname);
		tmpname[cc++] = '/';
		tmpname[cc++] = '\0';
		(void) strcat(tmpname, p);
		(void) strcpy(relname, tmpname);
		goto checkforroot;
	case S_IFDIR:
		(void) strcat(result, tmpseg);
		relp = p;
		goto nextseg;
	default:
		(void) strcat(result, relp);
		break;
	}
out:
	if (result[0] == '\0')
		(void) strcpy(result, "/");
	return (result);
}
