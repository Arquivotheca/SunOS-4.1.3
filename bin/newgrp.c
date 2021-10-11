#ifndef lint
static	char sccsid[] = "@(#)newgrp.c 1.1 92/07/30 SMI";	/* from S5R3 1.6 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * newgrp [-] [group]
 *
 * rules
 *	if no arg, group id in password file is used
 *	else if group id == id in password file
 *	else if login name is in member list
 *	else if password is present and user knows it
 *	else too bad
 */
#include <stdio.h>
#include <pwd.h>
#include <grp.h>

#define	SHELL	"/bin/sh"
#define	TERMUNKNOWN	"su"

#define PATH	"PATH=:/usr/ucb:/bin:/usr/bin"
#define SUPATH	"PATH=:/usr/ucb:/bin:/etc:/usr/bin"
#define ELIM	128

char	PW[] = "newgrp: Password:";
char	NG[] = "newgrp: Sorry";
char	PD[] = "newgrp: Permission denied";
char	UG[] = "usage:\tnewgrp [-] [group]\nwhere:\n\t-\treinitialize the environment\n\tgroup\tnew group (default: gid in /etc/passwd)\n";
char	NS[] = "newgrp: You have no shell";

struct	group *getgrnam();
struct	passwd *getpwuid();
char	*getpass();

char homedir[]=	"HOME=";
char logname[]=	"LOGNAME=";
char user[]=	"USER=";
char termenv[]=	"TERM=";
char shellenv[]="SHELL=";

char	*crypt();
char	*malloc();
char	*strcpy();
char	*strcat();
char	*strrchr();

char *envinit[ELIM];
extern char **environ;
char *path=PATH;
char *supath=SUPATH;


main(argc,argv)
char *argv[];
{
	register char *s;
	register struct passwd *p;
	register int i;
	char *rname(), *getenv();
	int eflag = 0;
	int uid;
	char *shell, *term, *dir, *name;

#ifdef	DEBUG
	chroot(".");
#endif
	if ((p = getpwuid(getuid())) == NULL) 
		error(NG);
	endpwent();
	if(argc > 1 && (strcmp(argv[1], "-") == 0)){
		eflag++;
		argv++;
		--argc;
	}
	if (argc > 1)
		p->pw_gid = chkgrp(argv[1], p);

	uid = p->pw_uid;
	dir = strcpy(malloc(strlen(p->pw_dir)+1),p->pw_dir);
	name = strcpy(malloc(strlen(p->pw_name)+1),p->pw_name);
	if (setgid(p->pw_gid) < 0 || setuid(getuid()) < 0)
		error(NG);
	if (!*p->pw_shell) {
		if ((shell = getenv("SHELL")) != NULL) {
			p->pw_shell = shell;
		} else {
			p->pw_shell = SHELL;
		}
	}
	if ((term = getenv("TERM")) == NULL) {
		term = TERMUNKNOWN;
	}
	if(eflag){
		char *simple;

		i = 0;
		if (uid == 0)
			envinit[i++] = supath;
		else
			envinit[i++] = path;
		envinit[i] = malloc(strlen(homedir) + strlen(dir) + 1);
		strcpy(envinit[i], homedir);
		strcat(envinit[i++], dir);
		envinit[i] = malloc(strlen(logname) + strlen(name) + 1);
		strcpy(envinit[i], logname);
		strcat(envinit[i++], name);
		envinit[i] = malloc(strlen(user) + strlen(name) + 1);
		strcpy(envinit[i], user);
		strcat(envinit[i++], name);
		envinit[i] = malloc(strlen(termenv) + strlen(term) + 1);
		strcpy(envinit[i], termenv);
		strcat(envinit[i++], term);
		envinit[i] = malloc(strlen(shellenv) + strlen(p->pw_shell) + 1);
		strcpy(envinit[i], shellenv);
		strcat(envinit[i++], p->pw_shell);
		envinit[i] = NULL;
		environ = envinit;
		chdir(dir);
		shell = strcpy(malloc(strlen(p->pw_shell) + 2), "-");
		shell = strcat(shell,p->pw_shell);
		simple = strrchr(shell,'/');
		if(simple){
			*(shell+1) = '\0';
			shell = strcat(shell,++simple);
		}
	}
	else	shell = p->pw_shell;
	execl(p->pw_shell,shell, NULL);
	error(NS);
	/* NOTREACHED */
}

warn(s)
char *s;
{
	fprintf(stderr, "%s\n", s);
}

error(s)
char *s;
{
	warn(s);
	exit(1);
}

chkgrp(gname, p)
char	*gname;
struct	passwd *p;
{
	register char **t;
	register struct group *g;

	g = getgrnam(gname);
	endgrent();
	if (g == NULL) {
		warn(UG);
		return getgid();
	}
	if (p->pw_gid == g->gr_gid || getuid() == 0)
		return g->gr_gid;
	for (t = g->gr_mem; *t; ++t) {
		if (strcmp(p->pw_name, *t) == 0)
			return g->gr_gid;
	}
	if (*g->gr_passwd) {
		if (!isatty(fileno(stdin)))
			error(PD);
		if (strcmp(g->gr_passwd, crypt(getpass(PW), g->gr_passwd)) == 0)
			return g->gr_gid;
	}
	warn(NG);
	return getgid();
}
/*
 * return pointer to rightmost component of pathname
 */
char *rname(pn)
char *pn;
{
	register char *q;

	q = pn;
	while (*pn)
		if (*pn++ == '/')
			q = pn;
	return q;
}
