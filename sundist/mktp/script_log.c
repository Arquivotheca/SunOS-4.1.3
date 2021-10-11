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
static	char sccsid[] = "@(#)script_log.c 1.1 92/07/30 SMI"; /* from UCB 5.4 11/13/85 */
#endif not lint

/*
 * script
 */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/file.h>

char	*getenv();
char	*ctime();
char	*shell;
FILE	*fscript;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname = "typescript";
int	sigwinch();
int	finish();

struct	termios b;
struct	winsize size;
#ifdef notdef
int	lb;
int	l;
#endif
char	*line = "/dev/ptyXX";
int	aflg;

main(argc, argv)
	int argc;
	char *argv[];
{

#ifdef notdef
	shell = getenv("SHELL");
	if (shell == 0)
#endif
	shell = "/bin/sh";
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {

		case 'a':
			aflg++;
			break;

		default:
			fprintf(stderr,
			    "usage: script [ -a ]  [typescript]  command\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 1) {
		fname = argv[0];
		argc--, argv++;
	}
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
	}
	getmaster();
#ifdef notdef
	printf("Script started, file is %s\n", fname);
#endif
	fixtty();

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell(*argv);
	}
	doinput();
}

doinput()
{
	char ibuf[BUFSIZ];
	int cc;

	(void) fclose(fscript);
	signal(SIGWINCH, sigwinch);
	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
}

sigwinch()
{
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) == 0)
		(void) ioctl(master, TIOCSWINSZ, &ws);
}

#include <sys/wait.h>

finish()
{
	union wait status;
	register int pid;
	register int die = 0;

	while ((pid = wait3(&status, WNOHANG, 0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
		done();
}

dooutput()
{
	time_t tvec;
	char obuf[BUFSIZ];
	int cc;

	(void) close(0);
#ifdef notdef
	tvec = time((time_t *)0);
	fprintf(fscript, "Script started on %s", ctime(&tvec));
#endif
	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
		(void) fwrite(obuf, 1, cc, fscript);
	}
	done();
}

doshell(cmd)
	char *cmd;
{
	int t;

	t = open("/dev/tty", O_RDWR);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
	getslave();
	(void) close(master);
	(void) fclose(fscript);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	(void) close(slave);
	execl(shell, "sh", "-c", cmd, (char *)0);
	perror(shell);
	fail();
}

fixtty()
{
	struct termios sbuf;

	sbuf = b;
	sbuf.c_iflag &= ~(INLCR|IGNCR|ICRNL|IUCLC|IXON);
	sbuf.c_oflag &= ~OPOST;
	sbuf.c_lflag &= ~(ICANON|ISIG|ECHO);
	sbuf.c_cc[VMIN] = 1;
	sbuf.c_cc[VTIME] = 0;
	(void) ioctl(0, TCSETSW, (char *)&sbuf);
}

fail()
{

	(void) kill(0, SIGTERM);
	done();
}

done()
{
	time_t tvec;

	if (subchild) {
#ifdef notdef
		tvec = time((time_t *)0);
		fprintf(fscript,"\nscript done on %s", ctime(&tvec));
#endif
		(void) fclose(fscript);
		(void) close(master);
	} else {
		(void) ioctl(0, TCSETSW, (char *)&b);
#ifdef notdef
		printf("Script done, file is %s\n", fname);
#endif
	}
	exit(0);
}

getmaster()
{
	char *pty, *bank, *cp;
	struct stat stb;

	pty = &line[strlen("/dev/ptyp")];
	for (bank = "pqrs"; *bank; bank++) {
		line[strlen("/dev/pty")] = *bank;
		*pty = '0';
		if (stat(line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			master = open(line, O_RDWR);
			if (master >= 0) {
				char *tp = &line[strlen("/dev/")];
				int ok;

				/* verify slave side is usable */
				*tp = 't';
				ok = access(line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok) {
				    (void) ioctl(0, TCGETS, (char *)&b);
				    (void) ioctl(0, TIOCGWINSZ, (char *)&size);
					return;
				}
				(void) close(master);
			}
		}
	}
	(void) fprintf(stderr, "Out of pty's\n");
	fail();
}

getslave()
{

	line[strlen("/dev/")] = 't';
	slave = open(line, O_RDWR);
	if (slave < 0) {
		perror(line);
		fail();
	}
	(void) ioctl(slave, TCSETSF, (char *)&b);
	(void) ioctl(slave, TIOCSWINSZ, (char *)&size);
}
