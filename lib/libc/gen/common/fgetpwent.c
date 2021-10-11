#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fgetpwent.c 1.1 92/07/30 SMI";
#endif

/* 
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>

#define ERRLEN	200

extern long strtol();
extern int strlen();
extern char *strncpy();

static char *EMPTY = "";
static struct passwd *interpret();
static char *pwskip();


struct passwd *
fgetpwent(f)
	FILE *f;
{
	char line1[BUFSIZ+1], name[20], *errstr = 0;
	struct passwd *pw;

	for (;;) {
		if (fgets(line1, BUFSIZ, f) == NULL)
			return(NULL);
		/*
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line1, '\n') == NULL) {
			strncpy(name, line1, sizeof(name));
			name[sizeof(name) - 1] = '\0';
			(void) pwskip(name);
			syslog(LOG_INFO|LOG_AUTH,
			       "fgetpwent: passwd entry exceeds %d characters:  \"%s\"",
			       BUFSIZ, name);
			while (strchr(line1, '\n') == NULL) {
				if (fgets(line1, BUFSIZ, f) == NULL) {
					break;
				}
			}
			continue;
		}
		if ((pw = interpret(line1, strlen(line1), &errstr)) == NULL) {
			syslog(LOG_INFO|LOG_AUTH, "fgetpwent: %s", errstr);
			errstr = 0;
			continue;
		} else {
			return(pw);
		}
	}
}

static char *
pwskip(p)
	register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == ':')
		*p++ = '\0';
	return(p);
}

static struct passwd *
interpret(val, len, errstr)
	char *val;
	char **errstr;
{
	register char *p, *np;
	char *end, *error, name[20];
	long x;
	static struct passwd *pwp;
	static char *line, *errbuf;
	register int ypentry;
	int nameok;

	if (line == NULL) {
		line = (char *)calloc(1,BUFSIZ+1);
		if (line == NULL)
			return (0);
	}
	if (pwp == NULL) {
		pwp = (struct passwd *)calloc(1, sizeof (*pwp));
		if (pwp == NULL)
			return (0);
	}
	if (errbuf == NULL) {
		errbuf = (char *)calloc(1, ERRLEN+1);
		if (errbuf == NULL)
			return (0);
	}
	(void) strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	line[len+1] = 0;

	/*
 	 * Set "ypentry" if this entry references the NIS;
	 * if so, null UIDs and GIDs are allowed (because they will be
	 * filled in from the matching NIS entry).
	 */
	ypentry = (*p == '+');

	/* Skip entries if name is white space (includes blank lines) */
	for (nameok = 0, np = p; *np && *np != ':'; np++) {
		if (!isspace(*np)) {
			nameok = 1;
			break;
		}
	}
	pwp->pw_name = p;
	if (nameok == 0) {
		(void) pwskip(p);
		error = "invalid name field";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		error = "premature end of line after name";
		goto invalid;
	}

	/* p points to passwd */
	pwp->pw_passwd = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		error = "premature end of line after passwd";
		goto invalid;
	}

	/* p points to uid */
	if (*p == ':' && !ypentry) {
		/* check for non-null uid */
		error = "null uid";
		goto invalid;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		error = "invalid uid";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		error = "premature end of line after uid";
		goto invalid;
	}
	pwp->pw_uid = x;

	/* p points to gid */
	if (*p == ':' && !ypentry) {
		/* check for non-null gid */
		error = "null gid";
		goto invalid;
	}
	x = strtol(p, &end, 10);	
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		error = "invalid gid";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		error = "premature end of line after gid";
		goto invalid;
	}
	pwp->pw_gid = x;
	pwp->pw_comment = EMPTY;
	pwp->pw_gecos = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		error = "premature end of line after gecos";
		goto invalid;
	}

	/* p points to dir */
	pwp->pw_dir = p;
	if ((strchr(p, ':') == NULL) && !ypentry) {
		error = "premature end of line after home directory";
		goto invalid;
	}
	p = pwskip(p);

	/* p points to shell */
	pwp->pw_shell = p;

	while (*p && *p != '\n')
		p++;
	if (*p) 
		*p = '\0';

	p = pwp->pw_passwd;
	while(*p && *p != ',')
		p++;
	if(*p)
		*p++ = '\0';
	pwp->pw_age = p;
	return (pwp);

invalid:
	if (errstr != NULL) {
		strncpy(name, line, sizeof(name));
		name[sizeof(name) - 1] = '\0';
		(void) pwskip(name);
		strncpy(errbuf, error, ERRLEN - sizeof(name) - 5);
		strcat(errbuf, ": \"");
		strncat(errbuf, name, sizeof(name));
		strcat(errbuf, "\"");
		*errstr = errbuf;
	}
	return(NULL);
}
