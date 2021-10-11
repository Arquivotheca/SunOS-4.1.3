#ifndef lint
static	char sccsid[] = "@(#)su.c 1.1 92/07/30 SMI"; /* from UCB 5.4 86/01/13 */
#endif
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

/*
 * su - super user, temporarily switch effective user ID
 */
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/time.h>
#include <sys/resource.h>

char	userbuf[16]	= "USER=";
char	lognamebuf[20]	= "LOGNAME=";
char	homebuf[128]	= "HOME=";
char	shellbuf[128]	= "SHELL=";
char	pathbuf[128]	= "PATH=:/usr/ucb:/bin:/usr/bin";
#ifdef S5EMUL
char	supath[128]	= "PATH=.:/bin:/usr/bin:/usr/ucb:/etc:/usr/etc";
char    suprmt[20]	= "PS1=# ";		/* primary prompt for root */
#endif
char	*user = "root";
char	*shell = "/bin/sh";
char	*cleanenv[] = {
	userbuf, lognamebuf, homebuf, shellbuf, pathbuf, 0, 0, 0
};
char	*audit_argv[] = {
	0, 0, "root", userbuf, lognamebuf, homebuf, shellbuf, pathbuf
};
int	fulllogin;
int	fastlogin;

extern char	**environ;
struct	passwd *pwd;
struct	passwd_adjunct *pwda,*getpwanam();
char	*crypt();
char	*getpass();
char	*getenv();
char	*getenvstr();
char	*getlogin();

main(argc,argv)
	int argc;
	char *argv[];
{
	char *password;
	char buf[1000];
	register char *p;
	register int i;
	char *ttyp;		/* work for ttyname() */
	char *ttyname();
	
	audit_argv[0] = argv[0];
	tzsetwall();

again:
	if (argc > 1 && strcmp(argv[1], "-f") == 0) {
		fastlogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && strcmp(argv[1], "-") == 0) {
		fulllogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && argv[1][0] != '-') {
		user = argv[1];
		audit_argv[2] = user;
		argc--, argv++;
	}
	if ((pwd = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit(1);
	}
	strcpy(buf, pwd->pw_name);
	if ((pwd = getpwnam(user)) == NULL) {
		audit_note("Unknown login: ", 1);
		fprintf(stderr, "Unknown login: %s\n", user);
		exit(1);
	}
	openlog("su", LOG_CONS, LOG_AUTH);

	/*
	 * If group zero has members, allow only those
	 * members to su to root.
	 */
	if (pwd->pw_uid == 0) {
		struct	group *gr;
		int i;

		if ((gr = getgrgid(0)) != NULL && *gr->gr_mem != NULL) {
			for (i = 0; gr->gr_mem[i] != NULL; i++)
				if (strcmp(buf, gr->gr_mem[i]) == 0)
					goto userok;
			fprintf(stderr, "You do not have permission to su %s\n",
				user);
			exit(1);
		}
	userok:
		setpriority(PRIO_PROCESS, 0, -2);
	}

#define Getlogin()  (((p = getlogin()) && *p) ? p : buf)
	ttyp = ttyname(2);	/* get ttyname */
	if (pwd->pw_passwd[0] == '\0' || getuid() == 0)
		goto ok;
	password = getpass("Password:");
	if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) {
		fprintf(stderr, "Sorry\n");
		if (ttyp != NULL)
			syslog(LOG_CRIT, "'su %s' failed for %s on %s",
			    pwd->pw_name, Getlogin(), ttyp);
		else
			syslog(LOG_CRIT, "'su %s' failed for %s",
			    pwd->pw_name, Getlogin());
		audit_note("bad password", 2);
		exit(2);
	}
ok:
	endpwent();
	if (ttyp != NULL)
		syslog(getuid() == 0 ? LOG_INFO : LOG_NOTICE,
		    "'su %s' succeeded for %s on %s", 
		    pwd->pw_name, Getlogin(), ttyp);
	else
		syslog(getuid() == 0 ? LOG_INFO : LOG_NOTICE,
		    "'su %s' succeeded for %s",
		    pwd->pw_name, Getlogin());
	closelog();
	if (setgid(pwd->pw_gid) < 0) {
		audit_note("setgid", 3);
		perror("su: setgid");
		exit(3);
	}
	if (initgroups(user, pwd->pw_gid)) {
		audit_note("initgroups failed", 4);
		fprintf(stderr, "su: initgroups failed\n");
		exit(4);
	}
	/*
	 * Since the uid is about to be changed this is the last place
	 * we can be assured that an audit record will get written.
	 * In the case where you end up as root this may be the
	 * wrong place, but not so for 'su btcat'.
	 */
	audit_note("successful su", 0);

	if (setuid(pwd->pw_uid) < 0) {
		perror("su: setuid");
		exit(5);
	}
	if (pwd->pw_shell && *pwd->pw_shell)
		shell = pwd->pw_shell;
	if (strcmp(user, "root"))
		strcat(userbuf, pwd->pw_name);
	else {
		if ((p = getenv("USER")) != NULL)
			strcat(userbuf, p);
	}
	strcat(lognamebuf, pwd->pw_name);
	strcat(homebuf, pwd->pw_dir);
	strcat(shellbuf, shell);
	if (fulllogin) {
		i = 5;
		cleanenv[i] = getenvstr("TERM");
		if (cleanenv[i] != NULL)
			i++;
		cleanenv[i] = getenvstr("TZ");
		environ = cleanenv;
	} else {
#ifdef S5EMUL
		if (pwd->pw_uid == 0) {
			/*
			 * Change path and prompt if super-user and not
			 * full-login.
			 */
			if (putenv(supath) != 0 || putenv(suprmt) != 0) {
				fprintf(stderr,
				    "su: unable to obtain memory to expand environment\n");
				exit(4);
			}
		}
#else
		putenv(userbuf);
		putenv(shellbuf);
		putenv(homebuf);
#endif
	}
	setpriority(PRIO_PROCESS, 0, 0);
	if (fastlogin) {
		*argv-- = "-f";
		*argv = "su";
	} else if (fulllogin) {
		if (chdir(pwd->pw_dir) < 0) {
			audit_note("no directory", 6);
			fprintf(stderr, "No directory\n");
			exit(6);
		}
		*argv = "-su";
	} else
		*argv = "su";

	execv(shell, argv);
	audit_note("no shell", 7);
	fprintf(stderr, "No shell\n");
	exit(7);
	/* NOTREACHED */
}

char *
getenvstr(ename)
	char *ename;
{
	register char *cp, *dp;
	register char **ep = environ;

	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0))
			return (*--ep);
	}
	return ((char *)0);
}

audit_note(s, v)
	char *s;
	int v;
{
	audit_argv[1] = s;
	audit_text(AU_ADMIN, v, v, 8, audit_argv);
}
