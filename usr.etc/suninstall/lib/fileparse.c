/*      @(#)fileparse.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpcsvc/ypclnt.h>
#include <sys/types.h>
#include <sys/stat.h>

#define iseol(c)	(c == '\0' || c == '\n' || c == '#')
#define issep(c)	(index(sep,c) != NULL)
#define isignore(c) (index(ignore,c) != NULL)

#define	LINESIZE	512
#define NAMESIZE	256
#define DOMAINSIZE	256

extern char *index(), *strcpy(), *strcat();

/*
 * getline()
 * Read a line from a file.
 * What's returned is a cookie to be passed to getname
 */
char **
getline(line,maxlinelen,f)
	char *line;
	int maxlinelen;
	FILE *f;
{
	char *p;
	static char *lp;
	do {
		if (! fgets(line, maxlinelen, f)) {
			return(NULL);
		}
	} while (iseol(line[0]));
	p = line;
	for (;;) {	
		while (*p) {	
			p++;
		}
		if (*--p == '\n' && *--p == '\\') {
			if (! fgets(p, maxlinelen, f)) {
				break;	
			}
		} else {
			break;	
		}
	}
	lp = line;
	return(&lp);
}

/*
 * getname()
 * Get the next entry from the line.
 * You tell getname() which characters to ignore before storing them 
 * into name, and which characters separate entries in a line.
 * The cookie is updated appropriately.
 * return:
 *	   1: one entry parsed
 *	   0: partial entry parsed, ran out of space in name
 *    -1: no more entries in line
 */
int
getname(name, namelen, ignore, sep, linep)
	char *name;
	int namelen;
	char *ignore;	
	char *sep;
	char **linep;
{
	char c;
	char *lp;
	char *np;

	lp = *linep;
	do {
		c = *lp++;
	} while (isignore(c) && !iseol(c));
	if (iseol(c)) {
		*linep = lp - 1;
		return(-1);
	}
	np = name;
	while (! issep(c) && ! iseol(c) && np - name < namelen) {	
		*np++ = c;	
		c = *lp++;
	} 
	lp--;
	if (issep(c) || iseol(c)) {
		if (np - name != namelen) {
			*np = 0;
		}
		if (iseol(c)) {
			*lp = 0;
		} else {
			lp++; 	/* advance over separator */
		}
	} else {
		*linep = lp;
		return(0);
	}
	*linep = lp;
	return(1);
}

/*
 * getclntent reads the line buffer and returns a string of client entry
 * in NIS or the named file. Called by yp_getclntent.
 */
int
getclntent(lp, clnt_entry)
	register char **lp;			/* function input */
	register char *clnt_entry;		/* function output */
{
	char name[NAMESIZE];
	int append = 0;

	while (getname(name, sizeof(name), " \t", " \t", lp) >= 0) {
		if (!append) {
			(void) strcpy(clnt_entry, name);
			append++;
		} else {
			(void) strcat(clnt_entry, " ");
			(void) strcat(clnt_entry, name);
		}
	}
	return (0);
}	

/*
 * getfileent returns the client entry 
 * named file given the client name.
 */
int
getfileent(clnt_name, clnt_entry, filename)
	register char *clnt_name;		/* function input */
	register char *clnt_entry;		/* function output */
	register char *filename;		/* file to operate on */
{
	FILE *fp; 
	char line[LINESIZE];
	char name[NAMESIZE];
	char **lp;
	int reason;
 
	reason = -2;
	if ((fp = fopen(filename, "r")) == NULL) {
		return (-1);
	}
	while (lp = getline(line, sizeof(line), fp)) {
		if ((getname(name, sizeof(name), " \t", " \t",
		    lp) >= 0) && (strcmp(name,clnt_name) == 0)) {
			if (getclntent(lp, clnt_entry) == 0)
				reason = 0;
			break;
		}
	}
	(void) fclose(fp);
	return (reason);
}

/*
 * yp_getclntent returns the client entry in either the NIS  map or
 * the named file given the client name.
 */
int
yp_getclntent(clnt_name, clnt_entry, ypmapname, filename)
	register char *clnt_name;		/* function input */
	register char *clnt_entry;		/* function output */
	register char *ypmapname;		/* yp map to match on */
	register char *filename;		/* file to operate on */
{
	char *val, *buf;
	char domain[DOMAINSIZE];
	int vallen;
	int reason;
 
	if (useyp(domain, sizeof(domain))) {
		if (reason = yp_match(domain, ypmapname, clnt_name,
		     strlen(clnt_name), &val, &vallen)) {
			if (reason == YPERR_MAP)
				return(getfileent(clnt_name, clnt_entry, filename));	
			else
				return (reason);
		} else {
			buf = val;
			reason = getclntent(&buf, clnt_entry);
			free(val);
			return(reason);
		}
	} else {
		return(getfileent(clnt_name, clnt_entry, filename));
	}
}

/*
 * Determine whether or not to use the NIS  service to do lookups.
 */
int initted;
int usingyp;
int
useyp(domain, domainlen)
char *domain;
int domainlen;
{
	if (!initted) {
		(void) getdomainname(domain, domainlen);
		usingyp = !yp_bind(domain);
		initted = 1;
	}
	return (usingyp);
}
