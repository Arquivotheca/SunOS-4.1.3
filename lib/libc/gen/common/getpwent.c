#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getpwent.c 1.1 92/07/30 SMI";
#endif

/* 
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifdef S5EMUL
#include <sys/param.h>
#endif
#include <stdio.h>
#include <pwd.h>
#include <rpcsvc/ypclnt.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>


#define MAXINT 0x7fffffff;

extern void rewind();
extern long strtol();
extern int strcmp();
#ifdef S5EMUL
extern int strncmp();
#endif
extern int strlen();
extern int fclose();
extern char *strcpy();
extern char *strncpy();
extern char *malloc();
extern  char *pwskip();

void setpwent(), endpwent();

static char *EMPTY = "";
static struct _pwjunk {
	struct passwd _NULLPW;
	FILE *_pwf;	/* pointer into /etc/passwd */
	char *_yp;		/* pointer into NIS */
	int _yplen;
	char *_oldyp;	
	int _oldyplen;
	struct list {
		char *name;
		struct list *nxt;
	} *_minuslist;
	struct passwd _interppasswd;
	char _interpline[BUFSIZ+1];
	char *_domain;
	char *_PASSWD;
	char _errstring[200];
} *__pwjunk;
#define	NULLPW (_pw->_NULLPW)
#define pwf (_pw->_pwf)
#define yp (_pw->_yp)
#define yplen (_pw->_yplen)
#define	oldyp (_pw->_oldyp)
#define	oldyplen (_pw->_oldyplen)
#define minuslist (_pw->_minuslist)
#define interppasswd (_pw->_interppasswd)
#define interpline (_pw->_interpline)
#define domain (_pw->_domain)
#define PASSWD (_pw->_PASSWD)
#define errstring (_pw->_errstring)
static struct passwd *interpret();
static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();
static struct passwd *getuidfromyellow();



#ifndef S5EMUL
/*
 * The following is for compatibility with the 4.3BSD version.
 */
int	_pw_stayopen;
#endif

static struct _pwjunk *
_pwjunk()
{
	register struct _pwjunk *_pw = __pwjunk;

	if (_pw == 0) {
		_pw = (struct _pwjunk *)calloc(1, sizeof (*__pwjunk));
		if (_pw == 0)
			return (0);
		PASSWD = "/etc/passwd";
		__pwjunk = _pw;
	}
	return (__pwjunk);
}

struct passwd *
getpwnam(name)
	register char *name;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	char line[BUFSIZ+1], namepw[20], *errstr = 0;

	if (_pw == 0)
		return (0);
	setpwent();
	if (!pwf)
		return NULL;
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		/* 
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			strncpy(namepw, line, sizeof (namepw));
			namepw[sizeof (namepw) - 1] = '\0';
			(void) pwskip(namepw);
			syslog(LOG_INFO|LOG_AUTH, 
			       "getpwnam: passwd entry exceeds %d characters:  \"%s\"", 
			       BUFSIZ, name);
			errstr = 0;
			while (strchr(line, '\n') == NULL) {
				if (fgets(line, BUFSIZ, pwf) == NULL) {
					break;
				}
			}
			continue;
		}

		if ((pw = interpret(line, strlen(line), &errstr)) == NULL) {
			syslog(LOG_INFO|LOG_AUTH, "getpwnam: %s", errstr);
			errstr = 0;
			continue;
		}

		if (matchname(line, &pw, name, &errstr)) {
#ifndef S5EMUL
			if (!_pw_stayopen)
#endif
				endpwent();
			return pw;
		} else if (errstr) {
			syslog(LOG_INFO|LOG_AUTH, "getpwnam: %s", errstr);
			errstr = 0;
		}
	}
#ifndef S5EMUL
	if (!_pw_stayopen)
#endif
		endpwent();
	return NULL;
}

struct passwd *
getpwuid(uid)
	register uid;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	char line[BUFSIZ+1], name[20], *errstr = 0;

	if (_pw == 0)
		return (0);
	setpwent();
	if (!pwf)
		return NULL;
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		/* 
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			strncpy(name, line, sizeof(name));
			name[sizeof(name) - 1] = '\0';
			(void) pwskip(name);
			syslog(LOG_INFO|LOG_AUTH, 
			       "getpwuid: passwd entry exceeds %d characters:  \"%s\"",
			       BUFSIZ, name);
			errstr = 0;
			while (strchr(line, '\n') == NULL) {
				if (fgets(line, BUFSIZ, pwf) == NULL) {
					break;
				}
			}
			continue;
		}
		if ((pw = interpret(line, strlen(line), &errstr)) == NULL) {
			syslog(LOG_INFO|LOG_AUTH, "getpwuid: %s", errstr);
			errstr = 0;
			continue;
		}
		if (matchuid(line, &pw, uid, &errstr)) {
#ifndef S5EMUL
			if (!_pw_stayopen)
#endif
				endpwent();
			return pw;
		} else if (errstr) {
			syslog(LOG_INFO|LOG_AUTH, "getpwuid: %s", errstr);
			errstr = 0;
		}
	}
#ifndef S5EMUL
	if (!_pw_stayopen)
#endif
		endpwent();
	return NULL;
}




void
setpwent()
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return;
	if (domain == NULL) {
		(void) yp_get_default_domain(&domain );
	}
	if (pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
}



void
endpwent()
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return;
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
	endnetgrent();
}


setpwfile(file)
	char *file;
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return (0);
	PASSWD = file;
	return (1);
}

/* fgetpwent removed to separate file fgetpwent.c */

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



struct passwd *
getpwent()
{
	register struct _pwjunk *_pw = _pwjunk();
	char line1[BUFSIZ+1], name[20], *errstr = 0;
	static struct passwd *savepw;
	struct passwd *pw;
	char *user; 
	char *mach;
	char *dom;

	if (_pw == 0)
		return (0);
	if (domain == NULL) {
		(void) yp_get_default_domain(&domain );
	}
	if (pwf == NULL && (pwf = fopen(PASSWD, "r")) == NULL) {
		return (NULL); 
	}

	for (;;) {
		if (yp) {
			pw = interpretwithsave(yp, yplen, savepw, &errstr); 
			free(yp);
			if (pw == NULL) {
				if (errstr) {
					syslog(LOG_INFO|LOG_AUTH, "getpwent: %s",  errstr);
					errstr = 0;
				}
				continue;
			}
			getnextfromyellow();
			if (!onminuslist(pw)) {
				return(pw);
			}
		} else if (getnetgrent(&mach,&user,&dom)) {
			if (user) {
				pw = getnamefromyellow(user, savepw, &errstr);
				if (pw != NULL && !onminuslist(pw)) {
					return(pw);
				} else if (errstr) {
					syslog(LOG_INFO|LOG_AUTH, "getpwent: %s", errstr);
					errstr = 0;
				}
			}
		} else {
			endnetgrent();
			if (fgets(line1, BUFSIZ, pwf) == NULL)  {
				return(NULL);
			}
			/* 
			 * If line is too long, consider invalid and skip over.
			 */
			if (strchr(line1, '\n') == NULL) {
				strncpy(name, line1, sizeof(name));
				name[sizeof(name) - 1] = '\0';
				(void) pwskip(name);
				syslog(LOG_INFO|LOG_AUTH,
				       "getpwent: passwd entry exceeds %d characters:  \"%s\"",
				       BUFSIZ, name);
				errstr = 0;
				while (strchr(line1, '\n') == NULL) {
					if (fgets(line1, BUFSIZ, pwf) == NULL) {
						break;
					}
				}
				continue;
			}

			if ((pw = interpret(line1, strlen(line1), &errstr)) ==
			     NULL) {
				syslog(LOG_INFO|LOG_AUTH, "getpwent: %s", errstr);
				errstr = 0;
				continue;
			}

			switch(line1[0]) {
			case '+':
				if (strcmp(pw->pw_name, "+") == 0) {
					getfirstfromyellow();
					savepw = save(pw);
				} else if (line1[1] == '@') {
					savepw = save(pw);
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* include the whole NIS database */
						getfirstfromyellow();
					} else {
						setnetgrent(pw->pw_name+2);
					}
				} else {
					/* 
					 * else look up this entry in NIS.
				 	 */
					savepw = save(pw);
					pw = getnamefromyellow(pw->pw_name+1, savepw, &errstr);
					if (pw != NULL && !onminuslist(pw)) {
						return(pw);
					} else if (pw == NULL && errstr) {
						syslog(LOG_INFO|LOG_AUTH,
						       "getpwent: %s", errstr);
						errstr = 0;
					}
				}
				break;
			case '-':
				if (line1[1] == '@') {
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* everybody was subtracted */
						return(NULL);
					}
					setnetgrent(pw->pw_name+2);
					while (getnetgrent(&mach,&user,&dom)) {
						if (user) {
							addtominuslist(user);
						}
					}
					endnetgrent();
				} else {
					addtominuslist(pw->pw_name+1);
				}
				break;
			default:
				if (!onminuslist(pw)) {
					return(pw);
				}
				break;
			}
		}
	}
}

static
matchname(line1, pwp, name, errstr)
	char line1[];
	struct passwd **pwp;
	char *name, **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *savepw;
	struct passwd *pw = *pwp;

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(name, savepw, errstr);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					savepw = save(pw);
					pw = getnamefromyellow(name, savepw, errstr);
					if (pw) {
						*pwp = pw;
						return 1;
					}
				}
				return 0;
			}
#ifdef S5EMUL
			if (strncmp(pw->pw_name+1, name, L_cuserid - 1) == 0) {
#else
			if (strcmp(pw->pw_name+1, name) == 0) {
#endif
				savepw = save(pw);
				pw = getnamefromyellow(pw->pw_name+1, savepw, errstr);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					*pwp = NULL;
					return 1;
				}
			}
#ifdef S5EMUL
			else if (strncmp(pw->pw_name+1, name, L_cuserid - 1) == 0) {
#else
			else if (strcmp(pw->pw_name+1, name) == 0) {
#endif
				*pwp = NULL;
				return 1;
			}
			break;
		default:
#ifdef S5EMUL
			if (strncmp(pw->pw_name, name, L_cuserid - 1) == 0)
#else
			if (strcmp(pw->pw_name, name) == 0)
#endif
				return 1;
	}
	return 0;
}

static
matchuid(line1, pwp, uid, errstr)
	char line1[];
	struct passwd **pwp;
	char **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *savepw;
	struct passwd *pw = *pwp;
	char group[256];

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getuidfromyellow(uid, savepw, errstr);
				if (pw) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				savepw = save(pw);
				pw = getuidfromyellow(uid, savepw, errstr);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			savepw = save(pw);
			pw = getnamefromyellow(pw->pw_name+1, savepw, errstr);
			if (pw && pw->pw_uid == uid) {
				*pwp = pw;
				return 1;
			} else
				return 0;
			break;
		case '-':
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				pw = getuidfromyellow(uid, &NULLPW, errstr);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = NULL;
					return 1;
				}
			} else if (uid == uidof(pw->pw_name+1, errstr)) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			if (pw->pw_uid == uid)
				return 1;
	}
	return 0;
}

static
uidof(name, errstr)
	char *name;
	char **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	
	if (_pw == 0)
		return (0);
	pw = getnamefromyellow(name, &NULLPW, errstr);
	if (pw)
		return pw->pw_uid;
	else
		return MAXINT;
}

static
getnextfromyellow()
{
	register struct _pwjunk *_pw = _pwjunk();
	int reason;
	char *key = NULL;
	int keylen;

	if (_pw == 0)
		return;
	reason = yp_next(domain, "passwd.byname",oldyp, oldyplen, &key
	    ,&keylen,&yp,&yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static
getfirstfromyellow()
{
	register struct _pwjunk *_pw = _pwjunk();
	int reason;
	char *key = NULL;
	int keylen;
	
	if (_pw == 0)
		return;
	reason =  yp_first(domain, "passwd.byname", &key, &keylen, &yp, &yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static struct passwd *
getnamefromyellow(name, savepw, errstr)
	char *name;
	struct passwd *savepw;
	char **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;
	
	if (_pw == 0)
		return (0);
	reason = yp_match(domain, "passwd.byname", name, strlen(name)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen, errstr);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_age && *savepw->pw_age)
			pw->pw_age =  savepw->pw_age;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
getuidfromyellow(uid, savepw, errstr)
	int uid;
	struct passwd *savepw;
	char **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;
	char uidstr[20];
	
	if (_pw == 0)
		return (0);
	(void) sprintf(uidstr, "%d", uid);
	reason = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen, errstr);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_age && *savepw->pw_age)
			pw->pw_age =  savepw->pw_age;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
interpretwithsave(val, len, savepw, errstr)
	char *val;
	struct passwd *savepw;
	char **errstr;
{
	struct passwd *pw;
	
	if ((pw = interpret(val, len, errstr)) == NULL)
		return NULL;
	if (savepw->pw_passwd && *savepw->pw_passwd)
		pw->pw_passwd =  savepw->pw_passwd;
	if (savepw->pw_age && *savepw->pw_age)
		pw->pw_age =  savepw->pw_age;
	if (savepw->pw_gecos && *savepw->pw_gecos)
		pw->pw_gecos = savepw->pw_gecos;
	if (savepw->pw_dir && *savepw->pw_dir)
		pw->pw_dir = savepw->pw_dir;
	if (savepw->pw_shell && *savepw->pw_shell)
		pw->pw_shell = savepw->pw_shell;
	return pw;
}

static struct passwd *
interpret(val, len, errstr)
	char *val;
	char **errstr;
{
	register struct _pwjunk *_pw = _pwjunk();
	register char *p, *np;
	char *end, name[20], *errmsg = 0;
	long x;
	register int ypentry;
	int nameok;

	if (_pw == 0)
		return (0);
	(void) strncpy(interpline, val, len);
	p = interpline;
	interpline[len] = '\n';
	interpline[len+1] = 0;

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
	interppasswd.pw_name = p;
	if (nameok == 0) {
		(void) pwskip(p);
		errmsg = "invalid name field";
		goto invalid;
	}

	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errmsg = "premature end of line after name";
		goto invalid;
	}

	/* p points to passwd */
	interppasswd.pw_passwd = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errmsg = "premature end of line after passwd";
		goto invalid;
	}

	/* p points to uid */
	if (*p == ':' && !ypentry) {
		/* check for non-null uid */
		errmsg = "null uid";
		goto invalid;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		errmsg = "invalid uid";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errmsg = "premature end of line after uid";
		goto invalid;
	}
	interppasswd.pw_uid = x;

	/* p points to gid */
	if (*p == ':' && !ypentry) {
		/* check for non-null gid */
		errmsg = "null gid";
		goto invalid;
	}
	x = strtol(p, &end, 10);	
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for numeric value - must have stopped on the colon */
		errmsg = "invalid gid";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errmsg = "premature end of line after gid";
		goto invalid;
	}
	interppasswd.pw_gid = x;
	interppasswd.pw_comment = EMPTY;

	interppasswd.pw_gecos = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errmsg = "premature end of line after gecos";
		goto invalid;
	}

	/* p points to dir */
	interppasswd.pw_dir = p;
	if ((strchr(p, ':') == NULL) && !ypentry) {
		errmsg = "premature end of line after home directory";
		goto invalid;
	}
	p = pwskip(p);
	interppasswd.pw_shell = p;

	/* Skip up to newline */
	while (*p && *p != '\n')
		p++;
	if (*p)
		*p = '\0';

	p = interppasswd.pw_passwd;
	while(*p && *p != ',')
		p++;
	if(*p)
		*p++ = '\0';
	interppasswd.pw_age = p;
	return(&interppasswd);

invalid:
	if (errstr != NULL) {
		strncpy(name, interpline, sizeof(name));
		name[sizeof(name) - 1] = '\0';
		(void) pwskip(name);
		strncpy(errstring, errmsg, sizeof(errstring) - sizeof(name) - 5);
		strcat(errstring, ": \"");
		strncat(errstring, name, sizeof(name));
		strcat(errstring, "\"");
		*errstr = errstring;
	}
	return(NULL);
}

static
freeminuslist() {
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;
	
	if (_pw == 0)
		return;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free((char *) ls);
	}
	minuslist = NULL;
}

static
addtominuslist(name)
	char *name;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;
	char *buf;
	
	if (_pw == 0)
		return;
	ls = (struct list *) malloc(sizeof(struct list));
	buf = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the NIS
 */
static struct passwd *
save(pw)
	struct passwd *pw;
{
	static struct passwd *sv;

	/* free up stuff from last call */
	if (sv) {
		free(sv->pw_passwd);
		free(sv->pw_gecos);
		free(sv->pw_dir);
		free(sv->pw_shell);
		free((char *) sv);
	}
	sv = (struct passwd *) malloc(sizeof(struct passwd));

	sv->pw_passwd = malloc((unsigned) strlen(pw->pw_passwd) + 1);
	(void) strcpy(sv->pw_passwd, pw->pw_passwd);

	sv->pw_age = malloc((unsigned) strlen(pw->pw_age) + 1);
	(void) strcpy(sv->pw_age, pw->pw_age);

	sv->pw_gecos = malloc((unsigned) strlen(pw->pw_gecos) + 1);
	(void) strcpy(sv->pw_gecos, pw->pw_gecos);

	sv->pw_dir = malloc((unsigned) strlen(pw->pw_dir) + 1);
	(void) strcpy(sv->pw_dir, pw->pw_dir);

	sv->pw_shell = malloc((unsigned) strlen(pw->pw_shell) + 1);
	(void) strcpy(sv->pw_shell, pw->pw_shell);

	return sv;
}

static
onminuslist(pw)
	struct passwd *pw;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;
	register char *nm;

	if (_pw == 0)
		return (0);
	nm = pw->pw_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		if (strcmp(ls->name,nm) == 0) {
			return(1);
		}
	}
	return(0);
}
