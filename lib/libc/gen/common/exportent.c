#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)exportent.c 1.1 92/07/30 SMI";
#endif

/*
 * Exported file system table manager. Reads/writes "/etc/xtab".
 * Copyright (C) 1986 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <exportent.h>
#include <sys/file.h>
#include <ctype.h>

extern char *strtok();
extern char *strcpy();

#define LINESIZE 1024

static char *TMPFILE = "/tmp/xtabXXXXXX";

static char *skipwhite();
static char *skipnonwhite();

FILE *
setexportent()
{
	FILE *f;
	int fd;

	/*
	 * Create the tab file if it does not exist already
	 */ 
	if (access(TABFILE, F_OK) < 0) {
		fd = open(TABFILE, O_CREAT, 0644);
		close(fd);
	}
	if (access(TABFILE, W_OK) == 0) {
		f = fopen(TABFILE, "r+");
	} else {
		f = fopen(TABFILE, "r");
	}
	if (f == NULL) {	
	   	return (NULL);
	}
	if (flock(fileno(f), LOCK_EX) < 0) {
		(void)fclose(f);
		return (NULL);
	}
	return (f);
}


void
endexportent(f)
	FILE *f;
{
	(void)fclose(f);
}


struct exportent *
getexportent(f)
	FILE *f;
{
	static char *line = NULL;
	static struct exportent xent;
	int len;
	char *p;

	if (line == NULL) {
		line = (char *)malloc(LINESIZE + 1);
	}
	if (fgets(line, LINESIZE, f) == NULL) {
		return (NULL);
	}
	len = strlen(line);
	if (line[len-1] == '\n') {
		line[len-1] = 0;
	}
	xent.xent_dirname = line;
	xent.xent_options = NULL;
	p = skipnonwhite(line);
	if (*p == 0) {
		return (&xent);
	}
	*p++ = 0;
	p = skipwhite(p);
	if (*p == 0) {
		return (&xent);
	}
	if (*p == '-') {
		p++;
	}
	xent.xent_options = p;
	return (&xent);
}

remexportent(f, dirname)
	FILE *f;
	char *dirname;
{
	char buf[LINESIZE];
	FILE *f2;
	int len;
	char *fname;
	int fd;
	long pos;
	long rempos;
	int remlen;
	int res;

	fname = (char *) malloc(strlen(TMPFILE) + 1);
	pos = ftell(f);
	rempos = 0;
	remlen = 0;
	(void)strcpy(fname, TMPFILE);
 	fd = mkstemp(fname);
	if (fd < 0) {
		return (-1);
	}
	if (unlink(fname) < 0) {
		(void)close(fd);
		return (-1);
	}
	f2 = fdopen(fd, "r+");
	if (f2 == NULL) {
		(void)close(fd);
		return (-1);
	}
	len = strlen(dirname);
	rewind(f);
	while (fgets(buf, sizeof(buf), f)) {
		if (strncmp(buf, dirname, 
		    len) != 0 || ! isspace((unsigned char)buf[len])) {
			if (fputs(buf, f2) <= 0) {
				(void)fclose(f2);	
				return (-1);
			}
		} else {
			remlen = strlen(buf);
			rempos = ftell(f) - remlen;
		}
	}
	rewind(f);
	if (ftruncate(fileno(f), 0L) < 0) {
		(void)fclose(f2);	
		return (-1);
	}
	rewind(f2);
	while (fgets(buf, sizeof(buf), f2)) {
		if (fputs(buf, f) <= 0) {
			(void)fclose(f2);
			return (-1);
		}
	}
	(void)fclose(f2);
	if (remlen == 0) {
		/* nothing removed */
		(void) fseek(f, pos, L_SET);
		res = -1;
	} else if (pos <= rempos) {
		res = fseek(f, pos, L_SET);
	} else if (pos > rempos + remlen) {
		res = fseek(f, pos - remlen, L_SET);
	} else {
		res = fseek(f, rempos, L_SET);
	}
	return (res < 0 ? -1 : 0); 
}


addexportent(f, dirname, options)
	FILE *f;
	char *dirname;
	char *options;
{
	long pos;	

	pos = ftell(f);
	if (fseek(f, 0L, L_XTND) >= 0 &&
	    fprintf(f, "%s", dirname) > 0 &&
	    (options == NULL || fprintf(f, " -%s", options) > 0) && 
	    fprintf(f, "\n") > 0 &&
	    fseek(f, pos, L_SET) >= 0) {
		return (0);
	}
	return (-1);
}
 

char *
getexportopt(xent, opt)
	struct exportent *xent;
	char *opt;
{
	static char *tokenbuf = NULL;
	char *lp;
	char *tok;
	int len;

	if (tokenbuf == NULL) {
		tokenbuf = (char *)malloc(LINESIZE);
	}
	if (xent->xent_options == NULL) {
		return (NULL);
	}
	(void)strcpy(tokenbuf, xent->xent_options);
	lp = tokenbuf;
	len = strlen(opt);
	while ((tok = strtok(lp, ",")) != NULL) {
		lp = NULL;
		if (strncmp(opt, tok, len) == 0) {
			if (tok[len] == '=') {
				return (&tok[len + 1]);
			} else if (tok[len] == 0) {
				return ("");
			}
		}
	}
	return (NULL);
}
	
 
#define iswhite(c) 	((c) == ' ' || c == '\t')

static char *
skipwhite(str)
	char *str;
{
	while (*str && iswhite(*str)) {
		str++;
	}
	return (str);
}

static char *
skipnonwhite(str)
	char *str;
{
	while (*str && ! iswhite(*str)) {
		str++;
	}
	return (str);
}
