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

#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from UCB 5.7 10/1/87 */
#endif not lint

/*
 * getty -- adapt to terminal speed on dialup, and call login
 *
 * Melbourne getty, June 83, kre.
 */

#include <stdio.h>
#include <string.h>
#include <sgtty.h>
#include <signal.h>
#include <ctype.h>
#include <setjmp.h>
#include <syslog.h>
#include <sys/file.h>
#include "gettytab.h"

extern	char **environ;

struct	sgttyb tmode = {
	0, 0, CERASE, CKILL, 0
};
struct	tchars tc = {
	CINTR, CQUIT, CSTART,
	CSTOP, CEOF, CBRK,
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT,
	CFLUSH, CWERASE, CLNEXT
};

int	crmod;
int	upper;
int	lower;
int	digit;

char	hostname[32];
char	name[16];
char	dev[] = "/dev/";
char	ctty[] = "/dev/console";
char	ttyn[32];
char	*portselector();
char	*ttyname();

#define	OBUFSIZ		128
#define	TABBUFSIZ	512

char	defent[TABBUFSIZ];
char	defstrs[TABBUFSIZ];
char	tabent[TABBUFSIZ];
char	tabstrs[TABBUFSIZ];

char	*env[128];

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201
};

#define	ERASE	tmode.sg_erase
#define	KILL	tmode.sg_kill
#define	EOT	tc.t_eofc

jmp_buf timeout;

dingdong()
{

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	longjmp(timeout, 1);
}

jmp_buf	intrupt;

interrupt()
{

	signal(SIGINT, interrupt);
	longjmp(intrupt, 1);
}

main(argc, argv)
	char *argv[];
{
	char *tname;
	long allflags;
	int repcnt = 0;

	signal(SIGINT, SIG_IGN);
/*
	signal(SIGQUIT, SIG_DFL);
*/
	openlog("getty", LOG_ODELAY|LOG_CONS, LOG_AUTH);
	gethostname(hostname, sizeof(hostname));
	if (hostname[0] == '\0')
		strcpy(hostname, "Amnesiac");
	/*
	 * The following is a work around for vhangup interactions
	 * which cause great problems getting window systems started.
	 * If the tty line is "-", we do the old style getty presuming
	 * that the file descriptors are already set up for us. 
	 * J. Gettys - MIT Project Athena.
	 */
	if (argc <= 2 || strcmp(argv[2], "-") == 0)
	    strcpy(ttyn, ttyname(0));
	else {
	    strcpy(ttyn, dev);
	    strncat(ttyn, argv[2], sizeof(ttyn)-sizeof(dev));
	    if (strcmp(argv[0], "+") != 0) {

		/*
		 * Reset owner/group/permissions of framebuffer devices
		 */
		(void) set_fb_attrs(ttyn, 0, 0);

		chown(ttyn, 0, 0);
		chmod(ttyn, 0622);
		/*
		 * Delay the open so DTR stays down long enough to be detected.
		 */
		sleep(2);
		while (open(ttyn, O_RDWR) != 0) {
			if (repcnt % 10 == 0) {
				syslog(LOG_ERR, "%s: %m", ttyn);
				closelog();
			}
			repcnt++;
			sleep(60);
		}
		signal(SIGHUP, SIG_IGN);
		vhangup();
		(void) open(ttyn, O_RDWR);
		close(0);
		dup(1);
		dup(0);
		/*
		 * grab the ctty even if someone else has it
		 */
		(void) ioctl(0, TIOCSCTTY, 1);
		signal(SIGHUP, SIG_DFL);
	    }
	}

	if (!gettable("default", defent, defstrs))
		syslog(LOG_ERR, "Can't find \"default\" entry in gettytab");
	gendefaults();
	tname = "default";
	if (argc > 1)
		tname = argv[1];
	for (;;) {
		int ldisp = OTTYDISC;
		int off = 0;

		(void) gettable(tname, tabent, tabstrs);
		if (OPset || EPset || APset)
			APset++, OPset++, EPset++;
		setdefaults();
		ioctl(0, TIOCFLUSH, 0);		/* clear out the crap */
		ioctl(0, FIONBIO, &off);	/* turn off non-blocking mode */
		ioctl(0, FIOASYNC, &off);	/* ditto for asynchronous mode */
		if (IS)
			tmode.sg_ispeed = speed(IS);
		else if (SP)
			tmode.sg_ispeed = speed(SP);
		if (OS)
			tmode.sg_ospeed = speed(OS);
		else if (SP)
			tmode.sg_ospeed = speed(SP);
		allflags = setflags(0);
		tmode.sg_flags = allflags & 0xffff;
		allflags >>= 16;
		ioctl(0, TIOCSETP, &tmode);
		ioctl(0, TIOCLSET, &allflags);
		setchars();
		ioctl(0, TIOCSETC, &tc);
		ioctl(0, TIOCSETD, &ldisp);
		if (HC)
			ioctl(0, TIOCHPCL, 0);
		ioctl(0, TIOCSETX, &off);
		if (MS)
			setmode(MS, 0);
		if (M0)
			setmode(M0, 0);
		if (AB) {
			extern char *autobaud();

			tname = autobaud();
			continue;
		}
		if (PS) {
			tname = portselector();
			continue;
		}
		if (DE > 0)
			sleep(DE);	/* wait for the line to settle */
		if (CL && *CL)
			putpad(CL);
		edithost(HE);
		if (IM && *IM)
			putf(IM);
		if (setjmp(timeout)) {
			tmode.sg_ispeed = tmode.sg_ospeed = 0;
			ioctl(0, TIOCSETP, &tmode);
			exit(1);
		}
		if (TO) {
			signal(SIGALRM, dingdong);
			alarm(TO);
		}
		if (getname()) {
			register int i;

			oflush();
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			if (name[0] == '-') {
				puts("login names may not start with '-'.");
				continue;
			}
			if (!(upper || lower || digit))
				continue;
			allflags = setflags(2);
			tmode.sg_flags = allflags & 0xffff;
			allflags >>= 16;
			if (crmod || NL)
				tmode.sg_flags |= CRMOD;
			if (upper || UC)
				tmode.sg_flags |= LCASE;
			if (lower || LC)
				tmode.sg_flags &= ~LCASE;
			ioctl(0, TIOCSETP, &tmode);
			ioctl(0, TIOCSLTC, &ltc);
			ioctl(0, TIOCLSET, &allflags);
			if (MS)
				setmode(MS, 0);
			if (M2)
				setmode(M2, 0);
			signal(SIGINT, SIG_DFL);
			for (i = 0; environ[i] != (char *)0; i++)
				env[i] = environ[i];
			makeenv(&env[i]);
			execle(LO, "login", "-p", name, (char *) 0, env);
			syslog(LOG_ERR, "%s: %m", LO);
			exit(1);
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		signal(SIGINT, SIG_IGN);
		if (NX && *NX)
			tname = NX;
	}
}

getname()
{
	register char *np;
	register c;
	char cs;
	long allflags;

	/*
	 * Interrupt may happen if we use CBREAK mode
	 */
	if (setjmp(intrupt)) {
		signal(SIGINT, SIG_IGN);
		return (0);
	}
	signal(SIGINT, interrupt);
	allflags = setflags(0);
	tmode.sg_flags = allflags & 0xffff;
	allflags >>= 16;
	ioctl(0, TIOCSETP, &tmode);
	ioctl(0, TIOCLSET, &allflags);
	if (MS)
		setmode(MS, 0);
	if (M0)
		setmode(M0, 0);
	allflags = setflags(1);
	tmode.sg_flags = allflags & 0xffff;
	allflags >>= 16;
	prompt();
	if (PF > 0) {
		oflush();
		sleep(PF);
		PF = 0;
	}
	ioctl(0, TIOCSETP, &tmode);
	ioctl(0, TIOCLSET, &allflags);
	if (MS)
		setmode(MS, 0);
	if (M1)
		setmode(M1, 0);
	crmod = 0;
	upper = 0;
	lower = 0;
	digit = 0;
	np = name;
	for (;;) {
		oflush();
		if (read(0, &cs, 1) <= 0)
			exit(0);
		if ((c = cs&0177) == 0)
			return (0);
		if (c == EOT)
			exit(1);
		if (c == '\r' || c == '\n' || np >= &name[sizeof name]) {
			putf("\r\n");
			break;
		}
		if (c >= 'a' && c <= 'z')
			lower++;
		else if (c >= 'A' && c <= 'Z')
			upper++;
		else if (c == ERASE || c == '#' || c == '\b') {
			if (np > name) {
				np--;
				if (tmode.sg_ospeed >= B1200)
					puts("\b \b");
				else
					putchr(cs);
			}
			continue;
		} else if (c == KILL || c == '@') {
			putchr(cs);
			putchr('\r');
			if (tmode.sg_ospeed < B1200)
				putchr('\n');
			/* this is the way they do it down under ... */
			else if (np > name)
				puts("                                     \r");
			prompt();
			np = name;
			continue;
		} else if (c >= '0' && c <= '9')
			digit++;
		if (IG && (c <= ' ' || c > 0176))
			continue;
		*np++ = c;
		putchr(cs);
	}
	signal(SIGINT, SIG_IGN);
	*np = 0;
	if (c == '\r')
		crmod++;
	if (upper && !lower && !LC || UC)
		for (np = name; *np; np++)
			if (isupper(*np))
				*np = tolower(*np);
	return (1);
}

static
short	tmspc10[] = {
	0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5, 15
};

putpad(s)
	register char *s;
{
	register pad = 0;
	register mspc10;

	if (isdigit(*s)) {
		while (isdigit(*s)) {
			pad *= 10;
			pad += *s++ - '0';
		}
		pad *= 10;
		if (*s == '.' && isdigit(s[1])) {
			pad += s[1] - '0';
			s += 2;
		}
	}

	puts(s);
	/*
	 * If no delay needed, or output speed is
	 * not comprehensible, then don't try to delay.
	 */
	if (pad == 0)
		return;
	if (tmode.sg_ospeed <= 0 ||
	    tmode.sg_ospeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return;

	/*
	 * Round up by a half a character frame,
	 * and then do the delay.
	 * Too bad there are no user program accessible programmed delays.
	 * Transmitting pad characters slows many
	 * terminals down and also loads the system.
	 */
	mspc10 = tmspc10[tmode.sg_ospeed];
	pad += mspc10 / 2;
	for (pad /= mspc10; pad > 0; pad--)
		putchr(*PC);
}

puts(s)
	register char *s;
{

	while (*s)
		putchr(*s++);
}

char	outbuf[OBUFSIZ];
int	obufcnt = 0;

putchr(cc)
{
	char c;

	c = cc;
	if (!P8) {
		c |= partab[c&0177] & 0200;
		if (OP)
			c ^= 0200;
	}
	if (!UB) {
		outbuf[obufcnt++] = c;
		if (obufcnt >= OBUFSIZ)
			oflush();
	} else
		write(1, &c, 1);
}

oflush()
{
	if (obufcnt)
		write(1, outbuf, obufcnt);
	obufcnt = 0;
}

prompt()
{

	putf(LM);
	if (CO)
		putchr('\n');
}

putf(cp)
	register char *cp;
{
	char *ttyn, *slash;
	char datebuffer[60];
	extern char editedhost[];
	extern char *ttyname(), *rindex();

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 't':
			ttyn = ttyname(0);
			slash = rindex(ttyn, '/');
			if (slash == (char *) 0)
				puts(ttyn);
			else
				puts(&slash[1]);
			break;

		case 'h':
			puts(editedhost);
			break;

		case 'd':
			get_date(datebuffer);
			puts(datebuffer);
			break;

		case '%':
			putchr('%');
			break;
		}
		cp++;
	}
}

/*
 * set_fb_attrs -- change owner/group/permissions of framebuffers
 *		   listed in /etc/fbtab.
 *
 * Note:
 * Exiting from set_fb_attrs upon error is not advisable
 * since it would disable logins on console devices.
 *
 * File format:
 * console mode device_name[:device_name ...]
 * # begins a comment and may appear anywhere on a line.
 *
 * Example:
 * /dev/console 0660 /dev/fb:/dev/cgtwo0:/dev/bwtwo0
 * /dev/console 0660 /dev/gpone0a:/dev/gpone0b
 *
 * Description:
 * The example above sets the owner/group/permissions of the listed
 * devices to uid/gid/0660 if ttyn is /dev/console
 */

#define	FIELD_DELIMS 	" \t\n"
#define	COLON 		":"
#define	MAX_LINELEN 	256
#define	FBTAB 		"/etc/fbtab"

set_fb_attrs(ttyn, uid, gid)
	char *ttyn;
	int uid;
	int gid;
{
	char line[MAX_LINELEN];
	char *console;
	char *mode_str;
	char *dev_list;
	char *device;
	char *ptr;
	int  mode;
	long strtol();
	FILE *fp;

	if ((fp = fopen(FBTAB, "r")) == NULL)
		return;
	while (fgets(line, MAX_LINELEN, fp)) {
		if (ptr = strchr(line, '#'))
			*ptr = '\0';	/* handle comments */
		if ((console = strtok(line, FIELD_DELIMS)) == NULL)
			continue;	/* ignore blank lines */
		if (strcmp(console, ttyn) != 0)
			continue;	/* ignore non-consoles */
		mode_str = strtok((char *)NULL, FIELD_DELIMS);
		if (mode_str == NULL) {
			(void) fprintf(stderr, "%s: invalid entry -- %s\n",
				FBTAB, line);
			continue;
		}
		/* convert string to octal value */
		mode = (int) strtol(mode_str, (char **)NULL, 8);
		if (mode < 0 || mode > 0777) {
			(void) fprintf(stderr, "%s: invalid mode -- %s\n",
				FBTAB, mode_str);
			continue;
		}
		dev_list = strtok((char *)NULL, FIELD_DELIMS);
		if (dev_list == NULL) {
			(void) fprintf(stderr, "%s: %s -- empty device list\n",
				FBTAB, console);
			continue;
		}
		device = strtok(dev_list, COLON);
		while (device) {
			(void) chown(device, uid, gid);
			(void) chmod(device, mode);
			device = strtok((char *)NULL, COLON);
		}
	}
	(void) fclose(fp);
}
