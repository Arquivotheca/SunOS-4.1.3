/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static  char sccsid[] = "@(#)passwd.c 1.1 92/07/30 SMI"; /* from UCB 4.24 5/28/86 */
#endif not lint

/*
 * Modify a field in the password file (either
 * password, login shell, or gecos field).
 * This program should be suid with an owner
 * with write permission on /etc/passwd.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <rpcsvc/ypclnt.h>

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
#include <rpcsvc/yppasswd.h>
#include <syslog.h>
#include <string.h>

/*
 * This should be the first thing returned from a getloginshell()
 * but too many programs know that it is /bin/sh.
 */
#define DEFSHELL "/bin/sh"

/* 
 * flags indicate password attributes to be modified
 */
#define XFLAG	0x001	/* set max field -- # days passwd is valid */
#define NFLAG	0x002	/* set min field -- # days betw/ passwd changes */
#define EFLAG   0x004   /* expire user's password */
#define DFLAG	0x010	/* display password attributes */
#define AFLAG	0x020	/* display password attributes for all users */
#define DAFLAG  (DFLAG|AFLAG)

char	*temp = "/etc/ptmp";
char	*tempa = "/etc/security/ptmpa";
char	*PASSWD = "/etc/passwd";
char	*PASSWDA = "/etc/security/passwd.adjunct";
char	*ROOTKEY = "/etc/.rootkey";
char	*PKMAP = "publickey.byname";
char	*progname;
time_t	now, minweeks, maxweeks;
int	maxdays, mindays, when;
long	a64l();
char	*l64a();
char	*getpass();
char	*getlogin();
char	*getfingerinfo();
char	*getloginshell();
char	*getnewpasswd();
char	*malloc();
char	*calloc();
char	*getusershell();
char	*ptmppath();
long	strtol();
struct passwd * yp_getpwnam();

char	hostname[MAXHOSTNAMELEN];
extern	int errno;
int	u, dochsh, dochfn;
int     flag = 0;
int	errflag = 0;
int	fflag = 0;
int 	forceyp = 0;
int 	forcelocal = 0;
int	ypform = 0;
int 	doyp = 0;

struct passwd *pwd;

struct  passwd_adjunct {
	char	*pwa_name;
	char	*pwa_passwd;
	char	*pwa_minimum;
	char	*pwa_maximum;
	char	*pwa_def;
	char	*pwa_au_always;
	char	*pwa_au_never;
	char	*pwa_version;
	char	*pwa_age;
};

struct passwd_adjunct *getpwaent();
struct passwd *sgetpwent();
int	endpwaent();
char	*pwskip();

/*
 * Special versions of the password file code which do not expand NIS
 * entries.
 */
static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

#define DAY_WEEK	7				/* days per week */
#define SEC_WEEK	(24 * DAY_WEEK * 60 * 60)	/* seconds per week */
#define DIG_CH		63	/* a char represents up to 64 digits in b64 */

/*	Convert base-64 ASCII string to long integer.
 *	Convert long integer to maxweek, minweeks.
 *	Convert current time from seconds to weeks.
 */
#define CNV_AGE(pw_age)	{			 \
	when = (long) a64l(pw_age);		\
	maxweeks = when & 077;			\
	minweeks = (when >> 6) & 077;		\
	now = time ((long *) 0) / SEC_WEEK;	\
}

#define ckuidflg(flagval)	\
	if (u != 0)	{	\
		fprintf(stderr,"%s: Permission denied\n",progname);	\
		aexit(1,"");	\
	}			\
	if (flag & flagval) 	\
		usage(progname)		

main(argc, argv)
	char *argv[];
{
	char *cp, *lp, *uname = NULL, *shname = NULL;
	int fd,  err;
	FILE *tf;
	char  *domain, *master;
	int port;
	extern int optind;
	char newpass[10], name[20];
	char oldpass[10];
	int rootpasswdchanged = 0;
	struct yppasswd yppasswd;

	/*
	 * Set options based on program name.
	 * The program can run as passwd, chsh, or chfn.
	 */
	if ((progname = rindex(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		progname++;
	dochfn = 0, dochsh = 0;
	flag = 0;

	u = getuid();

	if (strncmp(progname, "yp", 2) == 0){
		forceyp = 1;
		ypform = 1;
		progname = progname + 2;
	}

	if (strcmp(progname, "chfn") == 0)
		dochfn = 1;
	else if (strcmp(progname, "chsh") == 0)
		dochsh = 1;
	else if (strcmp(progname, "passwd") != 0) {
		(void) fprintf(stderr,
			"passwd: cannot run as \"%s\"\n", progname);
		aexit(1, "");
	}

	if (argc > 1)
		ckarg(argc, argv);

	if (forcelocal) doyp = 0;
	else if (yp_get_default_domain(&domain) == 0 &&
		   yp_master(domain, "passwd.byname", &master) == 0 &&
		   (port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
                              IPPROTO_UDP)))
				doyp = 1;


	if (forceyp && ! doyp ) {
		(void) fprintf(stderr,
			"Can't find rpc.yppasswd server\n");
			exit(0);
	}

	/* get user name */
	switch (argc - optind) {

	case 0:		/* get user name from pid and passwd file */
		if (flag) {
			if (flag == DAFLAG) {
				if (issecure())
					aexit(dis_adjunct((char *)NULL), "");
				else
					aexit(dis_passwd((char *)NULL), "");
			} else
				usage(progname);
		}
		break;

	case 1:		/* user name is provided */
		uname = argv[optind];
		break;

	case 2:		/* option argument supplied to chsh */
		if (flag == DAFLAG)
			usage(progname);
		uname = argv[optind];
		shname = argv[optind + 1];
		if (!dochsh)
			++errflag;
		break;

	default:		/* too many names or weird error */
		++errflag;
		break;
	}

	if (errflag)
		usage(progname);

	openlog(progname, LOG_CONS|LOG_PID, LOG_AUTH);

	if (uname == NULL) {
		if ((uname = getlogin()) == NULL) {
			(void) fprintf(stderr, "%s: you don't have a login name\n",
			    progname);
			aexit(1, "");
		}
	}


	if (!forceyp){
		 pwd = getpwnam(uname);
		if (pwd) doyp = 0;
		else if (doyp) pwd= yp_getpwnam(uname, domain);
	}
	else {
		if (doyp) pwd= yp_getpwnam(uname, domain);
		else pwd = getpwnam(uname);
	}
	if (pwd == NULL) {
		(void) fprintf(stderr, "%s: %s: unknown user.\n", progname,
			uname);
		aexit(1, uname);
	}

	if (u != 0 && u != pwd->pw_uid) {
		(void) printf("Permission denied.\n");
		aexit(1, uname);
	}

	if (doyp)
		strcpy(hostname,master);
	else
		(void) gethostname(hostname, sizeof(hostname));
		(void) printf("Changing %s%s for %s on %s.\n",
		    doyp ? "NIS ":"",
		    dochfn ? "finger information" :
			dochsh ? "login shell" : "password",
		    uname, hostname);

	/* Update passwd database */
	if (dochfn) {
		cp = getfingerinfo(pwd);
		if (doyp) (void) strcpy(oldpass, getpass("Password:"));
		}
	else if (dochsh) {
		cp = getloginshell(pwd, u, shname);
		if (doyp) (void) strcpy(oldpass, getpass("Password:"));
		}
	else if (flag & DFLAG) {
		if (issecure()) 
			aexit(dis_adjunct(uname), uname);
		else 
			aexit(dis_passwd(uname), uname);		
	}
	else if (!flag)
		cp = getnewpasswd(pwd, u, newpass, doyp ? oldpass : NULL);

	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);
	/*this code is for the yp case*/


 	if (doyp) {
		int ans, ok;
 		if (port >= IPPORT_RESERVED) {
 			(void)fprintf(stderr,
 			"yppasswd daemon is not running on privileged port\n");
 			exit(1);
 		}
 		yppasswd.newpw = *pwd;
 		yppasswd.oldpass = oldpass;
 		/* oldpass was set up by getnewpasswd */
 		if (dochfn) {
 			yppasswd.newpw.pw_gecos = cp;
 		}
 		else if (dochsh) {
 			yppasswd.newpw.pw_shell = cp;
 		}
 		else
 			yppasswd.newpw.pw_passwd = cp;
 		ans = callrpc(master, YPPASSWDPROG, YPPASSWDVERS,
 			      YPPASSWDPROC_UPDATE, xdr_yppasswd, &yppasswd, 
 			      xdr_int, &ok);
 		if (ans != 0) {
 			clnt_perrno(ans);
 			(void)fprintf(stderr, "\n");
 			(void)fprintf(stderr, "%s couldn't change entry (rpc call failed)\n",argv[0]);
 			exit(1);
 		}
 		if (ok == 1) {
 			(void)fprintf(stderr, "remote %s wouldn't change entry (password probably is incorrect) \n", master);
 			exit(1);
 		}
		else if (ok != 0) {
			decodeans(ok, master);
			exit(1);
		}

 		(void)printf("NIS entry changed on %s\n", master);
		if (!dochfn && !dochsh)
			yp_reencrypt_secret(domain, oldpass, newpass);
 		exit(0);
 	}
 
	/*this code is for the no NIS case*/
	(void) umask(0);
	if (!dochfn && !dochsh && issecure())
		aexit(upd_adjunct(uname, cp, progname), uname);
	if (fflag)
		temp = ptmppath(PASSWD, "ptmp");
	fd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		err = errno;

		(void) fprintf(stderr, "%s: ", progname);
		if (err == EEXIST)
			(void) fprintf(stderr,
				"password file busy - try again.\n");
		else {
			errno = err;
			perror(temp);
		}
		aexit(1, uname);
	}
	if ((tf = fdopen(fd, "w")) == NULL) {
		(void) fprintf(stderr, "%s: fdopen failed?\n",progname);
		aexit(1, uname);
	}
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new password.  sgetpwent parses the passwd entry;  if
	 * the entry is invalid, sgetpwent returns null, and the buffer
	 * is unchanged.  Invalid entries are written back unchanged.
	 */
	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		/* 
		 * Check to see if line is within length limits.
		 * If too long, consider it invalid; write out to file.
		 */
		if (strchr(line, '\n') == NULL) {
			strncpy(name, line, sizeof(name));
			name[sizeof(name)] = '\0';
			(void) pwskip(name);
			syslog(LOG_ERR, "passwd entry exceeds %d characters: \"%s\"",
			       BUFSIZ, name);

			while (strchr(line, '\n') == NULL) {
				if (fputs(line, tf) == EOF) {
					(void) fprintf(stderr, "%s: write error\n", 
						       progname);
					goto out;
				}
				if ((lp = fgets(line, BUFSIZ, pwf)) == NULL)
					break;
			}
			if (lp != NULL) {
				if (fputs(line, tf) == EOF) {
					(void) fprintf(stderr, "%s: write error\n", 
						       progname);
					goto out;
				}
			}
			continue;
		}

		if ((pwd = sgetpwent(line)) == NULL) {
			if (fputs(line, tf) == EOF) {
				fprintf(stderr, "%s: write error\n", progname);
				goto out;
			}
			continue;
		}

		if (strcmp(pwd->pw_name, uname) == 0) {
			if (u && u != pwd->pw_uid) {
				(void) fprintf(stderr,
					 "%s: permission denied.\n",
					 progname);
				goto out;
			}
			if (dochfn)
				pwd->pw_gecos = cp;
			else if (dochsh)
				pwd->pw_shell = cp;
			else {
				if (!flag) {
					pwd->pw_passwd = cp;
					if (pwd->pw_uid == 0) 
						rootpasswdchanged++;
				}
				/* Update aging info */
				if (upd_pwage(&pwd->pw_age))
					goto out;
			}
			if (pwd->pw_gecos[0] == '*')	/* ??? */
				pwd->pw_gecos++;
		}
		(void) fprintf(tf, "%s:%s", pwd->pw_name, pwd->pw_passwd);
		if (*pwd->pw_age != NULL)
			(void)fprintf(tf,",%s", pwd->pw_age);
		(void) fprintf(tf, ":%d:%d:%s:%s:%s\n",
			pwd->pw_uid,
			pwd->pw_gid,
			pwd->pw_gecos,
			pwd->pw_dir,
			pwd->pw_shell);
	}
	fclose(pwf);
	pwf = NULL;
	(void) fflush(tf);
	if (ferror(tf)) {	/* fsync?? */
		fprintf(stderr, "Warning: %s write error, %s not updated\n",
		    temp, PASSWD);
		goto out;
	}
	(void) fclose(tf);
	if (rename(temp, PASSWD) < 0) {
		err = errno;
		(void) fprintf(stderr, "%s", progname);
		errno = err;
		perror(": rename");
	out:
		(void) unlink(temp);
		aexit(1, uname);
	}
	if (rootpasswdchanged) {
		reencrypt_secret(newpass);
	}
	closelog();
	aexit(0, uname);
	/* NOTREACHED */
}

upd_adjunct(uname, pass, progname)
	char	*uname;
	char	*pass;
	char	*progname;
{
	FILE	*tf;
	short	fd;
	struct passwd_adjunct	*pwa;
	int	err;

	if (fflag)
		tempa = ptmppath(PASSWDA, "ptmpa");
	if ((fd = open(tempa, O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0) {
		err = errno;
		(void) fprintf(stderr, "%s: ", progname);
		if (err == EEXIST)
			(void) fprintf(stderr,
				"passwd.adjunct file busy - try again.\n");
		else {
			errno = err;
			perror(tempa);
		}
		return (1);
	}
	if ((tf = fdopen(fd, "w")) == NULL) {
		(void) fprintf(stderr,
			"passwd.adjunct: fdopen failed?\n");
		return (1);
	}
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	while ((pwa = getpwaent()) != NULL) {
		if (strcmp(pwa->pwa_name, uname) == 0) {
			if (!flag)
				pwa->pwa_passwd = pass;
			if (upd_pwage(&pwa->pwa_age))
				goto out;
		}
		(void) fprintf(tf, "%s:%s", pwa->pwa_name, pwa->pwa_passwd);
		if (*pwa->pwa_age != NULL)
			(void) fprintf(tf, ",%s", pwa->pwa_age);
		(void) fprintf(tf, ":%s:%s:%s:%s:%s\n",
			pwa->pwa_minimum,
			pwa->pwa_maximum,
			pwa->pwa_def,
			pwa->pwa_au_always,
			pwa->pwa_au_never);
	}
	endpwaent();
	(void) fflush(tf);
	if (ferror(tf)) {	/* fsync?? */
		fprintf(stderr, "Warning: %s write error, %s not updated\n",
		    tempa, PASSWDA);
		goto out;
	}
	(void) fclose(tf);
	if (rename(tempa, PASSWDA) < 0) {
		err = errno;
		(void) fprintf(stderr, "%s: ", progname);
		errno = err;
		perror("rename");
	out:
		(void) unlink(tempa);
		return (1);
	}
	return (0);
}

unlimit(lim)
{
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void) setrlimit(lim, &rlim);
}

ckarg(argc, argv)
int	argc;
char	**argv;
{
	extern char *optarg;
	char   *char_p;
	char *name;
	char *flg;
	register int c;

	if ((name = rindex(argv[0], '/')) == NULL)
		name = argv[0];
	else
		name++;

	while ((c = getopt(argc, argv, "flsyx:n:daeF:")) != EOF) {
		switch (c) {

		case 'f':	/* update finger information */
			++dochfn;
			break;

		case 's':	/* change shell */
			++dochsh;
			break;

		case 'y':	/* force NIS */
			if (forcelocal) {
				(void) fprintf(stderr,
				    "%s: -l and -y may not both be specified.\n", name);
				aexit(1, "");
				usage(progname);
			} else {
				++forceyp;
			}
			break;

		case 'l':	/* force local */
			if (ypform) {
				(void) fprintf(stderr,
				    "%s: -l may not be specified.\n", name);
				usage(progname);
				aexit(1, "");
			} else if (forceyp) {
				(void) fprintf(stderr,
				    "%s: -l and -y may not both be specified.\n", name);
				usage(progname);
				aexit(1, "");
			} else {
				++forcelocal;
			}
			break;

		case 'F':	/* alternate filename */
			if (ypform) {
				(void) fprintf(stderr,
				    "%s: -F may not be specified.\n", 
				    name);
				usage(progname);
				aexit(1, "");
			}
			PASSWD = optarg;
			++forcelocal;
			++fflag;
			break;

		case 'x':	/* set the max date */
			ckuidflg(DAFLAG);
			flag |= XFLAG;
			if ((maxdays = (int)strtol(optarg, &char_p, 10)) < -1
			  || *char_p != '\0' || strlen(optarg) <= 0) {
				fprintf(stderr,
				"%s: Invalid argument to option -x\n",progname);
				aexit(1);
			}
			break;
		case 'n':	/* set the min date */
			ckuidflg(DAFLAG);
			flag |= NFLAG;
			if (((mindays = (int)strtol(optarg, &char_p, 10)) < 0
			  || *char_p != '\0') || strlen(optarg) <= 0) {
				fprintf(stderr,
				"%s: Invalid argument to option -n\n",progname);
				aexit(1);
			}
			break;
		case 'd':	/* display password attributes */
			if (flag && (flag != AFLAG)) {
				fprintf(stderr,
				"Invalid combination of options\n");
				aexit(1);
			}
			flag |= DFLAG;
			break;
		case 'a':	/*display password attributes of all entries*/
			if (u != 0)	{	
				fprintf(stderr,"%s: Permission denied\n",
				    progname);
				aexit(1,"");	
			}
			if (flag && flag != DFLAG)
				usage(progname);
			flag |= AFLAG;
			break;
		case 'e':	/* expire password attributes */
			ckuidflg(DAFLAG);
			flag |= EFLAG;
			break;
		case '?':
			/* getopt() printed an error message */
			usage(progname);
		}
		if (flag) ++forcelocal; /*not supported with NIS*/
	}

	/* check for syntax errors in options */
	if (dochsh && dochfn) {
		(void) fprintf(stderr,
			"%s: cannot change shell and finger information at the same time\n",
			progname);
		usage(progname);
	}

	if (flag == AFLAG) {
		usage(progname);
	}

	if (ypform && flag != 0) {
		if (flag & AFLAG) flg = "a";
		else if (flag & DFLAG) flg = "d";
		else if (flag & EFLAG) flg = "e";
		else if (flag & NFLAG) flg = "n";
		else if (flag & XFLAG) flg = "x";
		(void) fprintf(stderr, "%s: -%s may not be specified.\n", 
		    name, flg);
		usage(progname);
		aexit(1, "");
	}
	if ((maxdays == -1) && (flag & NFLAG)) {
		fprintf(stderr,"%s: Invalid argument -x\n",progname);
		aexit(1);
	}
}

char *
getnewpasswd(pwd, u, newpass, oldpass)
	register struct passwd *pwd;
	int u;
	char *newpass;
        char *oldpass;
{
	register struct passwd_adjunct *pwa;
	char saltc[2];
	long salt;
	int i, insist = 0, ok, flags;
	int c, pwlen;
	static char pwbuf[10];
	long time();
	char *_crypt(), *pw, *p;

	p = pwd->pw_passwd;
	/* If secure, use passwd.adjunct entry instead of passwd */
	if (issecure() && oldpass == NULL) {
		while (((pwa = getpwaent()) != NULL) &&
				strcmp(pwa->pwa_name, pwd->pw_name))
			;
		endpwaent();
		if (pwa == NULL) {
			(void) fprintf(stderr,
				"Not in passwd.adjunct file.\n");
			aexit(1, pwd->pw_name);
		}
		p = pwa->pwa_passwd;
	}

	if (p[0] && (u != 0 || oldpass)) {
		(void) strcpy(pwbuf, getpass("Old password:"));
		if (oldpass)
			strcpy(oldpass, pwbuf);
		else {
			pw = _crypt(pwbuf, p);
			if (strcmp(pw, p) != 0) {
				(void) printf("Sorry.\n");
				aexit(1, pwd->pw_name);
			}
		}
	}

	/* password age checking :
	 * Use the password entry in /etc/passwd if the adjunct file
	 * has no age information.
	 */
	if ((pwd->pw_age)&&	
	    (*pwd->pw_age != NULL)) {
		CNV_AGE(pwd->pw_age);
		when >>= 12;
		if (when <= now) {
			if (u != 0 && (now < when + minweeks)) {
			       fprintf(stderr,
			       "%s:, Sorry: < %ld weeks since the last change\n",
			       progname, minweeks);
			       aexit(1, pwd->pw_name);
			}
			if (minweeks > maxweeks && u != 0) {
				fprintf(stderr,
				"%s: You may not change this passwdord\n",
				progname);
				aexit(1, pwd->pw_name);
			}
		}
	}

tryagain:
	(void) strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		(void) printf("Password unchanged.\n");
		aexit(1, pwd->pw_name);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		(void) printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		(void) printf("Mismatch - password unchanged.\n");
		aexit(1, pwd->pw_name);
	}
	(void) time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	strcpy(newpass, pwbuf);
	return (_crypt(pwbuf, saltc));
}

#define	STRSIZE	100

#define	STRSIZE	100

char *
getloginshell(pwd, u, arg)
	struct passwd *pwd;
	int u;
	char *arg;
{
	static char newshell[STRSIZE];
	char *cp, *valid, *getusershell();

	if (pwd->pw_shell == 0 || *pwd->pw_shell == '\0')
		pwd->pw_shell = DEFSHELL;
	if (u != 0) {
		for (valid = getusershell(); valid; valid = getusershell())
			if (strcmp(pwd->pw_shell, valid) == 0)
				break;
		if (valid == NULL) {
			(void) printf("Cannot change from restricted shell %s\n",
				pwd->pw_shell);
			aexit(1, pwd->pw_name);
		}
	}
	if (arg != 0) {
		(void) strncpy(newshell, arg, sizeof newshell - 1);
		newshell[sizeof newshell - 1] = 0;
	} else {
		(void)printf("Old shell: %s\nNew shell: ", pwd->pw_shell);
		(void)fgets(newshell, sizeof (newshell) - 1, stdin);
		cp = index(newshell, '\n');
		if (cp)
			*cp = '\0';
	}
	if (newshell[0] == '\0' || strcmp(newshell, pwd->pw_shell) == 0) {
		(void) printf("Login shell unchanged.\n");
		aexit(1, pwd->pw_name);
	}
	/*
	 * Allow user to give shell name w/o preceding pathname.
	 */
	if (u == 0) {
		valid = newshell;
	} else {
		setusershell();	/* XXX */
		for (valid = getusershell(); valid; valid = getusershell()) {
			if (newshell[0] == '/') {
				cp = valid;
			} else {
				cp = rindex(valid, '/');
				if (cp == 0)
					cp = valid;
				else
					cp++;
			}
			if (strcmp(newshell, cp) == 0)
				break;
		}
	}
	if (valid == 0) {
		(void) printf("%s is unacceptable as a new shell.\n",
		    newshell);
		aexit(1, pwd->pw_name);
	}
	if (access(valid, X_OK) < 0) {
		(void) printf("%s is unavailable.\n", valid);
		aexit(1, pwd->pw_name);
	}
	return (valid);
}

/*
 * Get name.
 */
char *
getfingerinfo(pwd)
	struct passwd *pwd;
{
	char in_str[STRSIZE];
	static char answer[4*STRSIZE];

	answer[0] = '\0';
	(void) printf("Default values are printed inside of '[]'.\n");
	(void) printf("To accept the default, type <return>.\n");
	(void) printf("To have a blank entry, type the word 'none'.\n");
	/*
	 * Get name.
	 */
	do {
		(void) printf("\nName [%s]: ", pwd->pw_gecos);
		(void) fgets(in_str, STRSIZE, stdin);
		if (special_case(in_str, pwd->pw_gecos)) 
			break;
	} while (illegal_input(in_str));
	(void) strcpy(answer, in_str);
	if (strcmp(answer, pwd->pw_gecos) == 0) {
		(void) printf("Finger information unchanged.\n");
		aexit(1, pwd->pw_name);
	}
	return (answer);
}

/*
 * Display password attributes of password file.
 * If uname is NULL, then display password attributes for
 * all entries of the file.
 */
dis_passwd(uname)
char *uname;
{
	int found = 0;

	if (uname != NULL) {
		dis_attr(pwd->pw_name, pwd->pw_passwd, pwd->pw_age);
		return(0);
	}

	while ((pwd = getpwent()) != NULL) {
		found++;
		dis_attr(pwd->pw_name, pwd->pw_passwd, pwd->pw_age);
	}

	/* if password files do not have any entries or files are missing,
	 * return fatal error.
	 */
	if (!found) {
		fprintf(stderr,"%s: Unexpected failure, password file missing\n",progname);
		return(1);
	}
	return(0);
}

/*
 * Display password attributes of adjunct file.
 * If uname is NULL, then display password attributes for
 * all entries of the file.
 */
dis_adjunct(uname)
char *uname;
{
	int found = 0;
	struct passwd_adjunct *pwa;

	if (uname != NULL) {
		while (((pwa = getpwaent()) != NULL) &&
					strcmp(pwa->pwa_name, pwd->pw_name)) 
			;
		endpwaent();
		if (pwa == NULL) {
			fprintf(stderr,"Not in passwd.adjunct file.\n");
			aexit(1, pwd->pw_name);
		}
		dis_attr(pwa->pwa_name, pwa->pwa_passwd, pwa->pwa_age);
		return(0);
	}

	while ((pwa = getpwaent()) != NULL) {
		found++;
		dis_attr(pwa->pwa_name, pwa->pwa_passwd, pwa->pwa_age);
	}
	endpwaent();

	/* if password files do not have any entries or files are missing,
	 * return fatal error.
	 */
	if (!found) {
		fprintf(stderr,"%s: Unexpected failure, password file missing\n",progname);
		return(1);
	}
	return(0);
}

/* Display actual attributes */
dis_attr(name, passwd, age)
char *name, *passwd, *age;
{
	extern struct tm *gmtime();
	struct tm *tmp;

	fprintf(stdout,"%s   ", name);
	if (age != NULL && *age != NULL) {
		CNV_AGE(age);
		when >>= 12;
		if (when) {
			when = when * SEC_WEEK;
			tmp = gmtime(&when);
			fprintf(stdout,"%.2d/%.2d/%.2d   ", tmp->tm_mon+1,
					tmp->tm_mday, tmp->tm_year);
		}
		else
			fprintf(stdout,"00/00/00   ");
		fprintf(stdout,"%d  %d", minweeks * DAY_WEEK, 
							maxweeks * DAY_WEEK);
	}
	fprintf(stdout,"\n");
}

/*
 * Prints an error message if a ':' or a newline is found in the string.
 * A message is also printed if the input string is too long.
 * The password file uses :'s as seperators, and are not allowed in the "gcos"
 * field.  Newlines serve as delimiters between users in the password file,
 * and so, those too, are checked for.  (I don't think that it is possible to
 * type them in, but better safe than sorry)
 *
 * Returns '1' if a colon or newline is found or the input line is too long.
 */
illegal_input(input_str)
	char *input_str;
{
	char *ptr;
	int error_flag = 0;
	int length = strlen(input_str);

	if (index(input_str, ':')) {
		(void) printf("':' is not allowed.\n");
		error_flag = 1;
	}
	if (input_str[length-1] != '\n') {
		/* the newline and the '\0' eat up two characters */
		(void) printf("Maximum number of characters allowed is %d\n",
			STRSIZE-2);
		/* flush the rest of the input line */
		while (getchar() != '\n')
			/* void */;
		error_flag = 1;
	}
	/*
	 * Delete newline by shortening string by 1.
	 */
	input_str[length-1] = '\0';
	/*
	 * Don't allow control characters, etc in input string.
	 */
	for (ptr=input_str; *ptr != '\0'; ptr++) {
		if ((int) *ptr < 040) {
			(void) printf("Control characters are not allowed.\n");
			error_flag = 1;
			break;
		}
	}
	return (error_flag);
}



/*
 *  special_case returns true when either the default is accepted
 *  (str = '\n'), or when 'none' is typed.  'none' is accepted in
 *  either upper or lower case (or any combination).  'str' is modified
 *  in these two cases.
 */
special_case(str,default_str)
	char *str, *default_str;
{
	static char word[] = "none\n";
	char *ptr, *wordptr;

	/*
	 *  If the default is accepted, then change the old string do the 
	 *  default string.
	 */
	if (*str == '\n') {
		(void) strcpy(str, default_str);
		return (1);
	}
	/*
	 *  Check to see if str is 'none'.  (It is questionable if case
	 *  insensitivity is worth the hair).
	 */
	wordptr = word-1;
	for (ptr = str; *ptr != '\0'; ++ptr) {
		++wordptr;
		if (*wordptr == '\0')	/* then words are different sizes */
			return (0);
		if (*ptr == *wordptr)
			continue;
		if (isupper(*ptr) && (tolower(*ptr) == *wordptr))
			continue;
		/*
		 * At this point we have a mismatch, so we return
		 */
		return (0);
	}
	/*
	 * Make sure that words are the same length.
	 */
	if (*(wordptr+1) != '\0')
		return (0);
	/*
	 * Change 'str' to be the null string
	 */
	*str = '\0';
	return (1);
}


#ifdef NOTUSED
setpwent()
{
	if( pwf == NULL )
		pwf = fopen( PASSWD, "r" );
	else
		rewind( pwf );
}
#endif

static FILE    *pwfadj = NULL;

#ifdef NOTUSED
setpwaent()
{
	if (pwfadj == NULL)
		pwfadj = fopen(PASSWDA, "r");
	else
		rewind(pwfadj);
}
#endif

endpwent()
{
	if( pwf != NULL ){
		(void) fclose( pwf );
		pwf = NULL;
	}
}

endpwaent()
{
	if (pwfadj != NULL) {
		(void) fclose(pwfadj);
		pwfadj = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if (*p == ':')
		*p++ = 0;
	return(p);
}

struct passwd *
getpwent()
{
	char *cp, name[20];
	struct passwd *p;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}

	while ((cp = fgets(line, BUFSIZ, pwf)) != NULL) {
		/*
		 * If line is too long, consider it invalid and skip over.
		 */
		if (strchr(line, '\n') == NULL) {
			strncpy(name, line, sizeof(name));
			name[sizeof(name) - 1] = '\0';
			(void) pwskip(name);
			syslog(LOG_ERR, 
			       "passwd entry exceeds %d characters: \"%s\"", 
			       BUFSIZ, name);
			while (strchr(line, '\n') == NULL) {
				if ((cp = fgets(line, BUFSIZ, pwf)) == NULL) {
					break;
				}
			}
			continue;
		}

		if ((p = sgetpwent(line )) != NULL)  {
			break;
		}
	}

	if (cp == NULL)
		p = NULL;
	return(p);
}

/*
 *  If the line is OK, return pointer to passwd struct;
 *  else if error, return NULL and return the original line buffer; 
 *  also call syslog() to log invalid entries.
 */
struct passwd *
sgetpwent(line)
char *line;
{
	register char *p, *np;
	register int ypentry;
	char *end, *bp, *errstr;
	int nameok;
	long x;

	p = bp = line;

	/*
	 * Set "ypentry" if this entry references the Network Information Svcs;
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
	passwd.pw_name = p;
	if (nameok == 0)  {
		(void) pwskip(p);
		errstr = "invalid name field";
		goto invalid;
	}

	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errstr = "premature end of line after name";
		goto invalid;
	}

	/* p points to passwd */
	passwd.pw_passwd = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errstr = "premature end of line after passwd";
		goto invalid;
	}

	/* p points to uid */
	if (*p == ':' && !ypentry)  { /* skip entries with null uid */
		/* check for non-null uid */
		errstr = "null uid";
		goto invalid;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p != ':' && !ypentry)  { /* skip entries with null gid */
		/* check for all numeric uid - must have stopped on the colon */
		errstr = "non-numeric uid";
		goto invalid;
	}
	passwd.pw_uid = x;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errstr = "premature end of line after uid";
		goto invalid;
	}

	/* p points to gid */
	if (*p == ':' && !ypentry) {
		/* check for non-null gid */
		errstr = "null gid";
		goto invalid;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p != ':' && !ypentry) {
		/* check for all numeric gid - must have stopped on the colon */
		errstr = "non-numeric gid";
		goto invalid;
	}
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errstr = "premature end of line after gid";
		goto invalid;
	}
	passwd.pw_gid = x;

	passwd.pw_comment = EMPTY;
	passwd.pw_gecos = p;
	if ((*(p = pwskip(p)) == '\n') && !ypentry) {
		errstr = "premature end of line after gecos";
		goto invalid;
	}

	/* p points to dir */
	passwd.pw_dir = p;
	if ((strchr(p, ':') == NULL) && !ypentry) {
		errstr = "premature end of line after home directory";
		goto invalid;
	}
	p = pwskip(p);
	passwd.pw_shell = p;

	/* skip to end of line */
	while(*p && *p != '\n')
		p++;    
	if (*p)
		*p = '\0';

	p = passwd.pw_passwd;
	while (*p && *p != ',')
		p++;
	if (*p)
		*p++ = '\0';
	passwd.pw_age = p;

	return(&passwd);

invalid:
	syslog(LOG_ERR, "%s:  \"%s\"", errstr, passwd.pw_name);
	/* 
	 * Make a backwards pass over the buffer to change the
	 * nulls back to colon
	 */
	for (; p > bp; p--) {
		if (*p == '\0')
			*p = ':';
	}
	return(NULL);
}

struct passwd *
getpwnam(name)
char *name;
{
	register struct passwd *p;

	while ((p = getpwent()) != NULL && strcmp(name, p->pw_name) != 0)
		;
	endpwent();
	return (p);
}

struct passwd *
yp_getpwnam(name, domain)
char *name, *domain;
{
	register char *p;
	char *buf;
	int err, buflen;
	
	buf = NULL;
	err = yp_match(domain, "passwd.byname", name, strlen(name), 
		       &buf, &buflen);
	if (err)
		return(NULL);
	p = buf;
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	/*passwd.pw_quota = 0;*/
	passwd.pw_age =0;
	passwd.pw_comment = EMPTY;
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return(&passwd);
}

struct passwd_adjunct *
getpwaent()
{
	register char			*p;
	static struct passwd_adjunct	passwda;
	static char			line[BUFSIZ + 1];

	if (pwfadj == NULL && (pwfadj = fopen(PASSWDA, "r")) == NULL) {
		return (NULL); 
	}

	p = fgets(line, BUFSIZ, pwfadj);
	if (p == NULL)
		return (NULL);
	/* Name */
	passwda.pwa_name = p;
	p = pwskip(p);
	/* Password */
	passwda.pwa_passwd = p;
	p = pwskip(p);
	/* Min Label */
	passwda.pwa_minimum = p;
	p = pwskip(p);
	/* Max Label */
	passwda.pwa_maximum = p;
	p = pwskip(p);
	/* Default Label */
	passwda.pwa_def = p;
	p = pwskip(p);
	/* Audit always */
	passwda.pwa_au_always = p;
	p = pwskip(p);
	/* Audit never */
	passwda.pwa_au_never = p;
	p = pwskip(p);
	/* Version */
	passwda.pwa_version = p;
	while(*p && *p != '\n')
		p++;
	if (*p)
		*p = '\0';

	p = passwda.pwa_passwd;
	while (*p && *p != ',')
		p++;
	if (*p)
		*p++ = '\0';
	passwda.pwa_age = p;

	return (&passwda);
}

/*
 * Update pwd or pwa entry 
 */
upd_pwage(pw_age)
char **pw_age;
{

	if (!flag) {		/* modifying the password */
		if (**pw_age != NULL) {
			CNV_AGE(*pw_age);
			if (maxweeks == 0)  /*turn off aging */
			       *pw_age = "";
			else {
			       when = maxweeks + (minweeks <<6)
						+ (now << 12);
			       *pw_age = l64a(when); 
			}
		}
	}
	if (flag & EFLAG) {	/* expire password */
		if (**pw_age != NULL) {
			CNV_AGE(*pw_age);
			when = maxweeks + (minweeks << 6);
			*pw_age = l64a(when);
		}
		else
			*pw_age = ".";
	}
	if (flag & XFLAG) { /* set age max field  */
		if (maxdays == -1)
			*pw_age = "";
		else {
			if (**pw_age == NULL) {
				minweeks = 0;
				when = 0;
			} else
				CNV_AGE(*pw_age);
			maxweeks = (maxdays + 6) / DAY_WEEK;
			maxweeks = maxweeks > DIG_CH ? DIG_CH : maxweeks;
			when = maxweeks + (minweeks<<6) + ((when>>12) << 12);
			if (!when)
				*pw_age = ".";
			else
				*pw_age = l64a(when);		
		}
	}
	if (flag & NFLAG) { /* set age min field */
		if (**pw_age == NULL) {
			fprintf(stderr,
			"%s: Invalid combination of options\n",progname);
			return(1);
		}
		CNV_AGE(*pw_age);
		minweeks = (mindays + 6) / DAY_WEEK;
		minweeks = minweeks > DIG_CH ? DIG_CH : minweeks;
		when = maxweeks + (minweeks << 6) + ((when >> 12) << 12);
		if (!when)
			*pw_age = ".";
		else
			*pw_age = l64a(when);
	}
	return(0);
}

/*
 * Generate an audit record before exiting
 */
aexit(rc, uname)
	int		rc;
	char		*uname;
{
	(void) audit_text(AU_ADMIN, rc, rc, 1, &uname);
	exit(rc);
}

usage(name)
	char *name;
{

	if (strncmp(name,"yp",2) == 0) name = name + 2;

	if (strcmp(name, "passwd") == 0)
		if (ypform)
			(void) fprintf(stderr,
			    "Usage: yppasswd [-sf] [username]\n");
		else
			(void) fprintf(stderr,
			    "Usage: passwd [-l|-y] [-F file] [-afs] [-d user] [-e user]\n\t[-n numdays user] [-x numdays user] [user]\n");
	else if (strcmp(name, "chsh") == 0)
		if (ypform)
			(void) fprintf(stderr, "Usage: ypchfn [username]\n");
		else
			(void) fprintf(stderr,
			    "Usage: chsh [-l|-y] [-F file] [user] [shell]\n");
	else if (strcmp(name, "chfn") == 0)
		if (ypform)
			(void) fprintf(stderr, "Usage: ypchfn [username]\n");
		else
			(void) fprintf(stderr,
			    "Usage: chfn [-l|-y] [-F file] [user]\n");
	else
		(void) fprintf(stderr,
		    "Error: program must be named \"passwd\", \"chsh\" or \"chfn\"\n");
	aexit(1, "");
}

/*
 * If the user has a secret key, reencrypt it.
 * Otherwise, be quiet.
 */
reencrypt_secret(newpass)
	char *newpass;
{
	char who[MAXNETNAMELEN+1];
	char secret[HEXKEYBYTES+1];
	char public[HEXKEYBYTES+1];
	char crypt[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char pkent[sizeof(crypt) + sizeof(public) + 1];
	char *master;
	char domain[MAXHOSTNAMELEN+1];

	getnetname(who);
	if (!getpublickey(who, public)) {
		/*
		 * Quiet: net is not running secure RPC
		 */
		return;
	}
	if (!readrootkey(secret)) {
		(void)fprintf(stderr,
			   "Warning: can't read secret key for %s from %s.\n", 
			      who, ROOTKEY);
		return;
	}
	bcopy(secret, crypt, HEXKEYBYTES);
	bcopy(secret, crypt + HEXKEYBYTES, KEYCHECKSUMSIZE);
	crypt[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0;
	xencrypt(crypt, newpass);
	(void)sprintf(pkent, "%s:%s", public, crypt);
	(void)getdomainname(domain, sizeof(domain));
	if (yp_update(domain, PKMAP, YPOP_STORE,
		who, strlen(who), pkent, strlen(pkent)) != 0) {

		(void)fprintf(stderr,
			"Warning: couldn't reencrypt secret key for %s\n", who);
			return;
	}
	if (yp_master(domain, PKMAP, &master) != 0) {
		master = "NIS master";   /* should never happen */
	}
	(void)printf("secret key reencrypted for %s on %s\n", who, master);
}

/*
 * If the user has a secret key, reencrypt it.
 * Otherwise, be quiet.
 */
yp_reencrypt_secret(domain, oldpass, newpass)
	char *domain;
	char *oldpass, *newpass;
{
	char who[MAXNETNAMELEN+1];
	char secret[HEXKEYBYTES+1];
	char public[HEXKEYBYTES+1];
	char crypt[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char pkent[sizeof(crypt) + sizeof(public)];
	char *master;

	getnetname(who);
	if (!getsecretkey(who, secret, oldpass)) {
		/*
		 * Quiet: net is not running secure RPC
		 */
		return;
	}
	if (secret[0] == 0) {
		/*
		 * Quiet: user has no secret key
		 */
		return;
	}
	if (!getpublickey(who, public)) {
		(void)fprintf(stderr, 
			      "Warning: can't find public key for %s.\n", who);
		return;
	}
	bcopy(secret, crypt, HEXKEYBYTES); 
	bcopy(secret, crypt + HEXKEYBYTES, KEYCHECKSUMSIZE); 
	crypt[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0; 
	xencrypt(crypt, newpass); 
	(void)sprintf(pkent, "%s:%s", public, crypt);
	if (yp_update(domain, PKMAP, YPOP_STORE,
	              who, strlen(who), pkent, strlen(pkent)) != 0) {

		(void)fprintf(stderr, 
			      "Warning: couldn't reencrypt secret key for %s\n",
			      who);
		return;
	}
	if (yp_master(domain, PKMAP, &master) != 0) {
		master = "NIS master";	/* should never happen */
	}
	(void)printf("secret key reencrypted for %s on %s\n", who, master);
}

readrootkey(secret)
	char *secret;
{
	int fd;

	fd = open(ROOTKEY, 0, 0);
	if (fd < 0) {
		return (0);
	}
	if (read(fd, secret, HEXKEYBYTES) != HEXKEYBYTES) {
		close(fd);
		return (0);
	}
	secret[HEXKEYBYTES] = 0;
	close(fd);
	return (1);
}

#define MAXMSG 17
char *msgs[]=
{
"No error",				/*0*/
"Error from pre 4.1 version",		/*1*/
"Password incorrect", 	/*2*/ /*really login incorrect but why say so*/
"No changeable fields were changed",   /*3*/
"No password in adjunct",   /*4*/
"Bad password in adjunct",   /*5*/
"Inconsistency in adjunct",   /*6*/
"Password incorrect",   /*7*/
"Password file busy -- try again later",   /*8*/
"Password temp file open error -- contact system administrator",   /*9*/
"Password temp file fdopen error -- contact system administrator",   /*10*/
"Password adjunct file fopen error -- contact system administrator",   /*11*/
"Password file fopen error -- contact system administrator",   /*12*/
"Password temp file fputs failed; disk partition may be full on NIS master! -- contact system administrator",   /*13*/
"Password temp file ferror is set; disk partition may be full on NIS master! -- contact system administrator",   /*14*/
"Password temp file fflush failed; disk partition may be full on NIS master! -- contact system administrator",   /*15*/
"Password adjunct file rename failed; disk partition may be full on NIS master! -- contact system administrator",   /*16*/
"Password file rename failed; disk partition may be full on NIS master! -- contact system administrator",   /*17*/
};




decodeans(ok, master)
char *master;
int ok;
{
if (ok <0 || ok > MAXMSG )
	fprintf(stderr, "Remote %s error %d\n",master, ok);
else 
	fprintf(stderr,"Error from %s: %s \n",master, msgs[ok]);
}


/*
 * return a lock file of same path as passwd file but
 * named "ptmp"
 */
char *
ptmppath(pwdpath, ptmpfile)
char *pwdpath;
char *ptmpfile;
{
	char *end, *pptr;
	int len;

	end = rindex(pwdpath, '/');
	len = (end) ? (end - pwdpath + 1) : 0;
	pptr = calloc((unsigned) (len + strlen(ptmpfile) + 1), 1);
	if (pptr == NULL) {
		(void)fprintf(stderr, "Ran out of memory.\n");
		aexit(1, "");
	}
	(void)strncpy(pptr, pwdpath, len);
	(void)strcat(pptr, ptmpfile);
	return (pptr);
}
