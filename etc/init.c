/*
 * Copyright (c) 1980,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)init.c 1.1 92/07/30 SMI"; /* from UCB 5.6 5/26/86 */
#endif not lint

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/reboot.h>
#include <errno.h>
#include <sys/file.h>
#include <ttyent.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>

#define	LINSIZ	sizeof(wtmp.ut_line)
#define	CMDSIZ	200	/* max string length for getty or window command*/
#define	ALL	p = itab; p ; p = p->next
#define	EVER	;;
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

char	shell[] = "/sbin/sh";
char	singleshell[]	= "/single/sh";
char	oldshell[]	= "/bin/sh";
char	minus[]	= "-";
char	bootc[]	= "/etc/rc.boot";
char	runc[]	= "/etc/rc";
char	utmpf[]	= "/etc/utmp";
char	wtmpf[]	= "/var/adm/wtmp";
char	ctty[]	= "/dev/console";
char	ttys[] = "/etc/ttys";
char	ttytab[] = "/etc/ttytab";
char	pwdf[]	= "/etc/passwd";
char	pwaf[]	= "/etc/security/passwd.adjunct";

struct utmp wtmp;
struct	tab
{
	char	line[LINSIZ];
	char	comn[CMDSIZ];
	char	xflag;
	int	pid;
	int	wpid;		/* window system pid for SIGHUP	*/
	char	wcmd[CMDSIZ];	/* command to start window system process */
	time_t	gettytime;
	int	gettycnt;
	time_t	windtime;
	int	windcnt;
	struct	tab *next;
} *itab;

jmp_buf	sjbuf, shutpass;
time_t	time0;

void	runrc();
void	merge();
void	reset();
void	idlehup();
void	idle();
char	*strcpy(), *strcat();
long	lseek();

struct	sigvec rvec = { reset, sigmask(SIGHUP), 0 };


#ifdef vax
main()
{
	register int r11;		/* passed thru from boot */
#else
main(argc, argv)
	char **argv;
{
#endif
	int howto, oldhowto;

	if (getpid() != 1) {
		fprintf(stderr, "init already running\n");
		exit(-1);
	}
	time0 = time(0);
#ifdef vax
	howto = r11;
#else
	if (argc > 1 && argv[1][0] == '-') {
		char *cp;

		howto = 0;
		cp = &argv[1][1];
		while (*cp) switch (*cp++) {
		case 'a':
			howto |= RB_ASKNAME;
			break;
		case 's':
			howto |= RB_SINGLE;
			break;
		case 'b':
			howto |= RB_NOBOOTRC;
			break;
		}
	} else {
		howto = RB_SINGLE;
	}
#endif
	openlog("init", LOG_CONS|LOG_ODELAY, LOG_AUTH);
	sigvec(SIGTERM, &rvec, (struct sigvec *)0);
	signal(SIGTSTP, idle);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	(void) setjmp(sjbuf);
	if ((howto&RB_NOBOOTRC) == 0 && access(bootc, 4) == 0) {
		howto |= RB_NOBOOTRC;
		if (bootrc(howto) == 0)
			/* if rc.boot exits abnormally, stay single-user */
			howto = RB_SINGLE|RB_NOBOOTRC;
	}
	for (EVER) {
		oldhowto = howto;
		howto = RB_SINGLE|RB_NOBOOTRC;
		if (setjmp(shutpass) == 0)
			shutdown();
		if (oldhowto & RB_SINGLE)
			single();
		if (runcom(oldhowto) == 0) 
			continue;
		merge();
		multiple();
	}
}

void	shutreset();

shutdown()
{
	register i;
	register struct tab *p, *p1;

	close(creat(utmpf, 0644));
	signal(SIGHUP, SIG_IGN);
	for (p = itab; p ; ) {
		term(p);
		p1 = p->next;
		free(p);
		p = p1;
	}
	itab = (struct tab *)0;
	signal(SIGALRM, shutreset);
	(void) kill(-1, SIGTERM);	/* one chance to catch it */
	sleep(5);
	alarm(30);
	for (i = 0; i < 5; i++)
		kill(-1, SIGKILL);
	while (wait((int *)0) != -1)
		;
	alarm(0);
	shutend();
}

char shutfailm[] = "WARNING: Something is hung (won't die); ps axl advised\n";

void
shutreset()
{
	if (fork() == 0) {
		int ct = open(ctty, 1);
		write(ct, shutfailm, sizeof (shutfailm));
		sleep(5);
		exit(1);
	}
	sleep(5);
	shutend();
	longjmp(shutpass, 1);
}

shutend()
{
	register i, f;

	acct(0);
	signal(SIGALRM, SIG_DFL);
	for (i = 0; i < 10; i++)
		close(i);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

bootrc(howto)
	int howto;
{
	register int pid;
	int status;

	pid = fork();
	if (pid == 0) {
		getcons(1);
		if (howto & RB_SINGLE)
			runrc(bootc, "singleuser");
		else
			runrc(bootc, (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	else
		return (1);
}

single()
{
	register pid;
	register xpid;
	extern	errno;

	do {
		pid = fork();
		if (pid == 0) {
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			signal(SIGTSTP, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);
			getcons(0);
			if (checkpass() == 0) {
				execl(shell, minus, (char *)0);
				execl(singleshell, minus, (char *)0);
				execl(oldshell, minus, (char *)0);
				perror(oldshell);
			}
			exit(0);
		}
		while ((xpid = wait((int *)0)) != pid)
			if (xpid == -1 && errno == ECHILD)
				break;
	} while (xpid == -1);
}

/*
 * Verify user knows root password
 */
checkpass()
{
	char *pass;
	char *epass;
	extern char *rootpass();
	extern char *getpass();
	int not_ok = 1;

	if (!isc2secure() && cons_secure())
		return (0);

	/* Get the root password */
	epass = rootpass();
	if (epass == NULL)
		/* Go ahead if no password */
		return (0);
	while (not_ok) {
		pass = getpass("Password:");
		if (*pass == '\0')
			return (1);
		not_ok = strcmp(_crypt(pass, epass), epass);
	}
	return (0);
}

runcom(oldhowto)
	int oldhowto;
{
	register pid, f;
	int status;

	pid = fork();
	if (pid == 0) {
		getcons(1);
		if (oldhowto & RB_SINGLE)
			runrc(runc, (char *)0);
		else
			runrc(runc, "autoboot");
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "reboot");
		SCPYN(wtmp.ut_host, "");
		if (time0) {
			wtmp.ut_time = time0;
			time0 = 0;
		} else
			time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

void
runrc(rcname, arg)
	char *rcname;
	char *arg;
{
	kbd_disable();
	execl(shell, shell, rcname, arg, (char *)0);
	execl(singleshell, singleshell, rcname, arg, (char *)0);
	execl(oldshell, oldshell, rcname, arg, (char *)0);
}

struct	sigvec	mvec = { merge, sigmask(SIGTERM), 0 };
/*
 * Multi-user.  Listen for users leaving, SIGHUP's
 * which indicate ttytab has changed, and SIGTERM's which
 * are used to shutdown the system.
 */
multiple()
{
	register struct tab *p;
	register pid;
	int omask;

	sigvec(SIGHUP, &mvec, (struct sigvec *)0);
	for (EVER) {
		pid = wait((int *)0);
		if (pid == -1)
			return;
		omask = sigblock(SIGHUP);
		for (ALL) {
			/* must restart window system BEFORE emulator */
			if (p->wpid == pid || p->wpid == -1)
				wstart(p);
			if (p->pid == pid || p->pid == -1) {
				/* disown the window system */
				if (p->wpid)
					kill(p->wpid, SIGHUP);
				rmut(p);
				dfork(p);
			}
		}
		sigsetmask(omask);
	}
}

/*
 * Merge current contents of ttytab file
 * into in-core table of configured tty lines.
 * Entered as signal handler for SIGHUP.
 */
#define	FOUND	1
#define	CHANGE	2
#define WCHANGE 4

void
merge()
{
	register struct tab *p;
	register struct ttyent *t;
	register struct tab *p1;
	int ttysfd;
	register FILE *ttysf = NULL;
	struct stat ttys_stat, ttytab_stat;

	for (ALL)
		p->xflag = 0;
	setttyent();
	if (stat(ttytab, &ttytab_stat) < 0)
		syslog(LOG_ERR, "%s: %m", ttytab);
	else {
		/*
		 * If "/etc/ttys" doesn't exist, or if it is not the same
		 * file as "/etc/ttytab", create it if necessary and put
		 * an old-style "ttys" file into it.
		 */
		if (stat(ttys, &ttys_stat) < 0
		    || ttys_stat.st_dev != ttytab_stat.st_dev
		    || ttys_stat.st_ino != ttytab_stat.st_ino) {
			if ((ttysfd = open(ttys, O_WRONLY|O_CREAT|O_TRUNC, 0444)) < 0)
				syslog(LOG_WARNING, "Can't create %s: %m",
				    ttys);
			else
				ttysf = fdopen(ttysfd, "w");
		}
	}
	while (t = getttyent()) {
		if (ttysf != NULL)
			(void) fprintf(ttysf, "%c2%s\n",
			    (t->ty_status & TTY_ON) ? '1' : '0', t->ty_name);
		if ((t->ty_status & TTY_ON) == 0)
			continue;
		for (ALL) {
			if (SCMPN(p->line, t->ty_name))
				continue;
			p->xflag |= FOUND;
			if (SCMPN(p->comn, t->ty_getty)) {
				p->xflag |= CHANGE;
				SCPYN(p->comn, t->ty_getty);
			}
			if (SCMPN(p->wcmd, t->ty_window ? t->ty_window : "")) {
				p->xflag |= WCHANGE|CHANGE;
				SCPYN(p->wcmd, t->ty_window);
			}
			goto contin1;
		}

		/*
		 * Make space for a new one
		 */
		p1 = (struct tab *)calloc(1, sizeof(*p1));
		if (!p1) {
			syslog(LOG_ERR, "no space for '%s' !?!", t->ty_name);
			goto contin1;
		}
		/*
		 * Put new terminal at the end of the linked list.
		 */
		if (itab) {
			for (p = itab; p->next ; p = p->next)
				;
			p->next = p1;
		} else
			itab = p1;

		p = p1;
		SCPYN(p->line, t->ty_name);
		p->xflag |= FOUND|CHANGE;
		SCPYN(p->comn, t->ty_getty);
		if (t->ty_window && strcmp(t->ty_window, "") != 0) {
			p->xflag |= WCHANGE;
			SCPYN(p->wcmd, t->ty_window);
		}
	contin1:
		;
	}
	endttyent();
	if (ttysf != NULL)
		(void) fclose(ttysf);
	p1 = (struct tab *)0;
	for (ALL) {
		if ((p->xflag&FOUND) == 0) {
			term(p);
			wterm(p);
			if (p1)
				p1->next = p->next;
			else
				itab = p->next;
			free(p);
			p = p1 ? p1 : itab;
		} else {
			/* window system should be started first */
			if (p->xflag&WCHANGE) {
				wterm(p);
				wstart(p);
			}
			if (p->xflag&CHANGE) {
				term(p);
				dfork(p);
			}
		}
		p1 = p;
	}
}

term(p)
	register struct tab *p;
{

	if (p->pid != 0) {
		rmut(p);
		kill(p->pid, SIGKILL);
	}
	p->pid = 0;
	/* send SIGHUP to get rid of connections */
	if (p->wpid > 0)
		kill(p->wpid, SIGHUP);
}

dfork(p)
	struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;

	time(&t);
	p->gettycnt++;
	if ((t - p->gettytime) >= 60) {
		p->gettytime = t;
		p->gettycnt = 1;
	} else if (p->gettycnt >= 5) {
		dowait = 1;
		p->gettytime = t;
		p->gettycnt = 1;
	}
	pid = fork();
	if (pid == 0) {
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_IGN);
		sigsetmask(0);	/* since can be called from masked code */
		if (dowait) {
			syslog(LOG_ERR, "'%s %s' failing, sleeping", p->comn, p->line);
			closelog();
			sleep(30);
		}
		execit(p->comn, p->line);
		exit(0);
	}
	p->pid = pid;
}

/*
 * Remove utmp entry.
 */
rmut(p)
	register struct tab *p;
{
	register f;
	int found = 0;
	static unsigned utmpsize;
	static struct utmp *utmp;
	register struct utmp *u;
	int nutmp;
	struct stat statbf;

	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		fstat(f, &statbf);
		if (utmpsize < statbf.st_size) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (!utmp)
				syslog(LOG_ERR, "utmp malloc failed");
		}
		if (statbf.st_size && utmp) {
			nutmp = read(f, utmp, statbf.st_size);
			nutmp /= sizeof(struct utmp);
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, p->line) ||
				    u->ut_name[0]==0)
					continue;
				lseek(f, ((long)u)-((long)utmp), L_SET);
				SCPYN(u->ut_name, "");
				SCPYN(u->ut_host, "");
				time(&u->ut_time);
				write(f, (char *)u, sizeof(*u));
				found++;
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, p->line);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
		/*
		 * After a proper login force reset
		 * of error detection code in dfork.
		 */
		p->gettytime = 0;
		p->windtime = 0;
	}
}

void
reset()
{

	longjmp(sjbuf, 1);
}

jmp_buf	idlebuf;

void
idlehup()
{

	longjmp(idlebuf, 1);
}

void
idle()
{
	register struct tab *p;
	register pid;

	signal(SIGHUP, idlehup);
	for (EVER) {
		if (setjmp(idlebuf))
			return;
		pid = wait((int *) 0);
		if (pid == -1) {
			sigpause(0);
			continue;
		}
		for (ALL) {
			/* if window system dies, mark it for restart */
			if (p->wpid == pid)
				p->wpid = -1;
			if (p->pid == pid) {
				rmut(p);
				p->pid = -1;
			}
		}
	}
}

wterm(p)
	register struct tab *p;
{
	if (p->wpid != 0) {
		kill(p->wpid, SIGKILL);
	}
	p->wpid = 0;
}

wstart(p)
	register struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;

	time(&t);
	p->windcnt++;
	if ((t - p->windtime) >= 60) {
		p->windtime = t;
		p->windcnt = 1;
	} else if (p->windcnt >= 5) {
		dowait = 1;
		p->windtime = t;
		p->windcnt = 1;
	}

	pid = fork();

	if (pid == 0) {
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP,  SIG_IGN);
		sigsetmask(0);	/* since can be called from masked code */
		if (dowait) {
			syslog(LOG_ERR, "'%s %s' failing, sleeping", p->wcmd, p->line);
			closelog();
			sleep(30);
		}
		execit(p->wcmd, p->line);
		exit(0);
	}
	p->wpid = pid;
}

#define NARGS	20	/* must be at least 4 */
#define ARGLEN	512	/* total size for all the argument strings */

execit(s, arg)
	char *s;
	char *arg;	/* last argument on line */
{
	char *argv[NARGS], args[ARGLEN], *envp[1];
	register char *sp = s;
	register char *ap = args;
	register char c;
	register int i;

	/*
	 * First we have to set up the argument vector.
	 * "prog arg1 arg2" maps to exec("prog", "-", "arg1", "arg2"). 
	 */
	for (i = 1; i < NARGS - 2; i++) {
		argv[i] = ap;
		for (EVER) {
			if ((c = *sp++) == '\0' || ap >= &args[ARGLEN-1]) {
				*ap = '\0';
				goto done;
			}
			if (c == ' ') {
				*ap++ = '\0';
				while (*sp == ' ')
					sp++;
				if (*sp == '\0')
					goto done;
				break;
			}
			*ap++ = c;
		}
	}
done:
	setpgrp(0,0);
	argv[0] = argv[1];
	argv[1] = "-";
	argv[i+1] = arg;
	argv[i+2] = 0;
	envp[0] = 0;
	execve(argv[0], &argv[1], envp);
	/* report failure of exec */
	syslog(LOG_ERR, "%s: %m", argv[0]);
	closelog();
	sleep(10);	/* prevent failures from eating machine */
}

/*
 * Disable keyboard interrupts
 */
kbd_disable()
{
	if (isc2secure() || !cons_secure()) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
	}
}

int	securestate	= -1;

/*
 * Is this a C2 secure system ?
 */
isc2secure()
{

	if (securestate == -1)
		securestate = (access(pwaf, F_OK) == 0);
	return (securestate);
}

/*
 * Get the password for user ID == 0
 */
char *
rootpass()
{
	register FILE *f;
	register struct passwd *pwd;
	register struct passwd_adjunct *pwa;
	extern struct passwd *fgetpwent();
	extern struct passwd_adjunct *fgetpwaent();

	if ((f = fopen(pwdf, "r")) == NULL)
		return (NULL);
	while ((pwd = fgetpwent(f)) != NULL) {
		if (pwd->pw_uid == 0) {
			break;
		}
	}
	fclose(f);
	if (pwd == NULL)
		return (NULL);
	if ((f = fopen(pwaf, "r")) == NULL)
		return (pwd->pw_passwd);
	while ((pwa = fgetpwaent(f)) != NULL) {
		if (strcmp(pwa->pwa_name, pwd->pw_name) == 0) {
			break;
		}
	}
	fclose(f);
	if (pwa == NULL)
		return (NULL);

	return (pwa->pwa_passwd);
}

/*
 * Is the console secure?
 */
cons_secure()
{
	register struct ttyent *t;
	register char *s;
	extern char *rindex();

	s = rindex(ctty, '/');
	if (s != NULL)
		s++;
	if ((t = getttynam(s)) != NULL)
		return (t->ty_status & TTY_SECURE);
	return (0);
}

#include <sys/termios.h>
/*
 * close std{in,out,err}
 * make this process a session leader
 * get the console as this process' ctty
 * if (protect) leave the tty in a non signaling state
 */
getcons(protect)
{

	(void) close(0); (void) close(1); (void) close(2);
	(void) setpgrp(0, 0);
	(void) open(ctty, O_RDWR);
	(void) dup2(0, 1); (void) dup2(0, 2);
	if (!protect)
		return;
	/*
	 * the child grabs the tty, protecting the parent and the
	 * parent's descendants.
	 */
	if (fork() == 0) {
		int pgrp = getpid();

		(void) setpgrp(pgrp, pgrp);
		(void) ioctl(0, TIOCSPGRP, &pgrp);
		exit(0);
		/*NOTREACHED*/
	}
}
