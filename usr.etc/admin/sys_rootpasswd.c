#ifndef lint
static	char sccsid[] = "@(#)sys_rootpasswd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * 	
 */

#include <stdio.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <sys/file.h>
#include <string.h>
#include <strings.h>

#define PASSWORD_PROMPT "   Password => "
#define END_PASSWD \
"\n\n\n_______________________________________________________________\n\
\n\"

#define ROOTPW_HELP \
"\n\n\n______________________________________________________________________\n\
\n\
                      SUPERUSER PASSWORD\n\n\
For security reasons it is important to give a password to\n\
the superuser (root) account.  A password should be six to\n\
eight characters long.  To give a password to the root account,\n\
enter the password below.  The password will not appear on the\n\
screen as you type it.\n\
\n\
Enter superuser password and press RETURN, or press RETURN to continue:\n\n\
[?] = Help \n\
\n"

#define ROOTPW_HELPSCREEN \
"\n\n\n\
_______________________________________________________________\n\
\n\
                SUPERUSER PASSWORD HELP\n\n\
Each user of your workstation must have a user account.\n\
The superuser account is an account on your system that has\n\
special privileges.  The user name of this account is\n\
\"root\".  You should use the root account only when you are\n\
doing administrative tasks on your system that require\n\
those special privileges.\n\
\n\
For security reasons it is important to give a password to\n\
the root account.  A password should include a mix of upper \n\
and lower case letters, numbers, and punctuation characters \n\
and should be six to eight characters long. \n\
\n\
The password will not appear on the screen as you type it.\n\
You will be asked to re-type the same password to ensure \n\
that you have not made any typographical errors.\n\
If you do not wish to assign a password to the superuser \n\
account at this time, press RETURN at the password prompt."


#define CONTINUE_KEY "\n\nPress any key to continue\n\n\
_______________________________________________________________\n"




/* Global definitions */
extern char *sprintf();


/*	EXTERN Functions	    */

/*	Local Definitions	    */
static struct termios ttyb;
static unsigned long flags;
static char	min_chars;	/* VMIN control character */
static char	time;		/* VTIME control character */

main(argc,argv)
	int argc;
	char *argv[];
{
#ifdef DEBUG
	printf("sys_rootpasswd: top\n");
#endif DEBUG

	/*
	 * make sure we're root here; exit if we're not
	 */
	if (getuid() != 0) {
		(void)fprintf(stderr,"Must be superuser\n");
		exit(1);
	}

	/*
	 * ask for root passwd
	 */
	get_pw( ROOTPW_HELP, 
		ROOTPW_HELPSCREEN, 
		"root");

	/*NOTREACHED*/
}


/*
 * create and print a form that asks for a password,
 * running the /bin/passwd program (via a pty) to actually to the work.
 *
 * ASSUMES: quiescent system - once we grabbed a pty we can continue to use it
 */
get_pw(help, extra_help, uname)
	char *help;   /* paragraph that gets put at top of screen */
	char *extra_help; /* paragraph that is displayed for '?' */
	char *uname;	/* the user name to get the passwd for */
{
	char	c;
	int	i;
	char	*line;
	int	p;		/* pty file des. */
	int	t;		/* tty file des. */
	int	pid;
	int	status;
	int	first;
	int	x;		/* cursor column number */
	int n_flag=0;		/* 1=passwd doesn't like user's input */
	int	pwlinestate;	/* state variable for passwd output handling */

#define PWS_NEWPROG	0	/* a new passwd program has been invoked */
#define PWS_EAT1ST	1	/* eat the first line, ie. "Changing ..." */
#define PWS_EAT2ND	2	/* eat second line, ie. "Enter..." */
#define PWS_PASSTHRU	3	/* pass thru everthing passwd outputs */

	/* grab a pty pair */
	for (c = 'p'; c <= 's'; c++) {
		struct stat stb;
		int pgrp;

		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = c;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			line[strlen("/dev/")] = 't';
			/*
			 * Lock the slave side so that no one else can
			 * open it after this.
			 */
			if (chmod (line, 0600) == -1)
				continue;
			line[strlen("/dev/")] = 'p';
			if ((p = open(line, O_RDWR)) == -1) {
				restore_pty(line,p);
				continue;
			}

			/*
			 * XXX - Using a side effect of TIOCGPGRP on ptys.
			 * May not work as we expect in anything other than
			 * SunOS 4.1.
			 */
			if ((ioctl(p, TIOCGPGRP, &pgrp) == -1) &&
			    (errno == EIO))
				goto gotpty;
			else {
				restore_pty(line,p);
			}
		}
	}
	fprintf(stderr, "out of ptys");
	return(1);

gotpty:

#ifdef DEBUG
	printf("sys_rootpasswd: got pty %d\n",p);
	printf("sys_rootpasswd: pty_name = %s\n",line);
#endif DEBUG

	pwlinestate = PWS_NEWPROG;

	/* fork, the child then runs /bin/passwd */
	pid = fork();
	if (pid < 0) {
		restore_pty(line,p);
		perror("pwspawn: error during fork");
		return(1);
	}


	/* This is the child  process that runs the passwd command. */
	/* This process redirects its stdin, stdout, and stderr to  */
	/* the pty, which is being read by the parent.		    */
	if (pid == 0) {
		int tt;
 
		/*
		 * The child process needs to be the session leader
		 * and have the pty as its controlling tty.
		 */
		(void) setpgrp(0, 0);		/* setsid */
		line[strlen("/dev/")] = 't';
		tt = open(line, O_RDWR);
		if (tt < 0) {
			perror("can't open pty");
			exit(1);
		}
		(void) close(p);
		if (tt != 0)
			(void) dup2(tt, 0);
		if (tt != 1)
			(void) dup2(tt, 1);
		if (tt != 2)
			(void) dup2(tt, 2);
		if (tt > 2)
			(void) close(tt);
		execl("/bin/passwd", "passwd", uname, (char *)0);
		perror("can't exec /bin/passwd");
		exit(1);
		/*NOTREACHED*/
	}

#ifdef DEBUG
	printf("sys_rootpasswd: in parent - entering loop\n");
	fflush(stdout);
#endif DEBUG
	/* Save current terminal modes */
        save_modes(0);

	/* We don't want echo on the chars the user types */
	/* And we want the tube in canonical mode so that */
	/* we get each char as its typed instead of waiting */
#ifdef DEBUG
	printf("sys_rootpasswd: in non-canonical mode\n");
	fflush(stdout);
#endif DEBUG

	canon_input(0);

	/* This is the parent process that reads from the pty to see */
	/* what passwd is up to and reads form stdin to see what    */
	/* the user types.					    */
re_draw:
	/* put up the initial banner */
	printf("\n\n\n");
	printf("%s",ROOTPW_HELP);

	/* skip a couple of lines, and print prompt */
	skip_lines(2);
	printf("%s",PASSWORD_PROMPT);
	fflush(stdout);

	/*
	 * now do a protocol that
	 * 1. ignores first line of output from passwd program
	 * 2. feeds characters to it (except a leading ?)
	 */
	first = 1;
	while (1) {
		int	ibits;


		ibits = (1 << 0);		/* stdin */
		ibits |= (1 << p);		/* pty */

		if (select(32, &ibits, (int *)0, (int *)0, 0) < 0) {
			sleep(3);
			perror("select");

			/* Turn echo on */
			restore_modes(0);
			restore_pty(line,p);
			return(2);
		}
		if (ibits & (1 << 0)) {		/* stdin */
			if (read(0, &c, 1) != 1) {
				/* Turn echo on */
				restore_modes(0);
				restore_pty(line,p);
				return(2);
			}
			/* if 1st, and "?", print help msg instead */
			/* if 1st, and <CR>, don't set password */
			if (first) {
				if (c == '?') {
					if (extra_help != NULL) {
						printf("?");
						skip_lines(5);
						printf("%s",ROOTPW_HELPSCREEN);
						skip_lines(2);
						printf(CONTINUE_KEY);
						fflush(stdout);
						if (read(0, &c, 1) != 1) {
							/* Turn echo on */
							restore_modes(0);
							restore_pty(line,p);
							return(2);
						}
					}
					goto re_draw;
				} else

				/* If user typed <CR>, don't set passwd */
				if ((c == '\n') || (c == '\r')) {
					skip_lines(1);
					/* Turn echo on */
					restore_modes(0);
					restore_pty(line,p);
					return(0);
				} else
					first = 0;
			}

			/* Write the char to the pty so the passwd */
			/* program can get it			   */
			if (write(p, &c, 1) != 1) {
				/* Turn echo on */
				restore_modes(0);
				restore_pty(line,p);
				return(2);
			}
		}
		if (ibits & (1 << p)) {		/* pty */
			i = read(p, &c, 1);
			if (i < 0) {
				if ((errno == EIO) || (errno == EINVAL))
					break;
				/* Turn echo on */
				restore_modes(0);
				restore_pty(line,p);
				return(2);
			}
			if (i == 0) {		/* gone away */
				break;
			}

			/* what to do? */
			switch (pwlinestate) {
			case PWS_NEWPROG:
			case PWS_EAT1ST:
				/* first line ends in newline */
				if (c != '\n')
					continue;	/* throw away */
				pwlinestate = PWS_EAT2ND;
				break;

			case PWS_EAT2ND:
				/* second line ends in colon */
				if (c != ':')
					continue;	/* throw away */
				pwlinestate = PWS_PASSTHRU;
				break;

			case PWS_PASSTHRU:
				if (write(fileno(stdout), &c, 1) != 1) {
					restore_modes(0);
					restore_pty(line,p);
					return(2);
				}
			}
		}
	}
	/* Turn echo on */
	restore_modes(0);

	if (wait(&status) < 0) {
		perror("pwspawn: wait on child");
		restore_pty(line,p);
		exit(2);
	}
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
#ifdef DEBUG
	printf("spawning another passwd program\n");
	fflush(stdout);
#endif DEBUG
			/* spawn off another passwd program */
			canon_input(0);

			printf(CONTINUE_KEY);
			fflush(stdout);
			if (read(0, &c, 1) != 1) {
				/* Turn echo on */
				restore_modes(0);
				restore_pty(line,p);
				return(2);
			}
			restore_modes(0);
			goto gotpty;
		}
		restore_pty(line,p);
		return(0);
	}
	restore_pty(line,p);
	return(3);
}

/*
 * Print the specified number of new lines
 */
skip_lines(number)
	int number;
{
	int i;

	for(i = 0; i < number; i++)
		printf("\n");
}

/*
 * This routine turns on canonical mode so that characters
 * can be received as typed
 */
canon_input(file_des)
	int file_des;
{

	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);
	ttyb.c_cc[VMIN] = 1;
	ttyb.c_cc[VTIME] = 0;
	(void) ioctl(file_des, TCSETSF, &ttyb);
}

/* 
 * Save the terminal modes in the static storage
 */
save_modes(file_des)
	int file_des;
{

	/* Save current terminal modes */
        (void) ioctl(file_des, TCGETS, &ttyb);
        flags = ttyb.c_lflag;
	min_chars = ttyb.c_cc[VMIN];
	time = ttyb.c_cc[VTIME];
}

/*
 * Restore terminal modes from values saved in static storage
 */
restore_modes(file_des)
	int file_des;
{
	ttyb.c_lflag = flags;
	ttyb.c_cc[VMIN] = min_chars;
	ttyb.c_cc[VTIME] = time;
       	(void) ioctl(file_des, TCSETSW, &ttyb);
}

/*
 * Restore the permissions on the pty and close it
 */
restore_pty(pty_name, pty_fd)
	char *pty_name;
	int   pty_fd;
{

	pty_name[strlen("/dev/")] = 't';
	(void) chmod(pty_name, 0666);
	pty_name[strlen("/dev/")] = 'p';

	if(pty_fd != -1)
		close(pty_fd);
}
