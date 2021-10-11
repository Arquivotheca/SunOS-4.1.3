/*
 * Copyright (c) 1988 Sun Microsystems
 */

#ifndef lint
static	char sccsid[] = "@(#)intr.c 1.1 92/07/30 SMI";
#endif not lint

# include	<stdio.h>
# include	<sys/signal.h>
# include	<sys/termios.h>

# define	vprintf		(void)fprintf

/*
 * intr - execute a command and allow it to be killed but not stopped
 *
 * options:
 * -a	echo command & arguments
 * -n	don't echo a newline after anything
 * -v	echo command to be run
 * -t#	time out the command in # seconds
 */

main(ac, av)
    char          **av;
{
    int             pgrp, pid;
    int             a = 0;
    int             n = 0;
    int             v = 0;
    int             t = 0;
    int             c;
    extern char    *optarg;
    extern          optind;

    while ((c = getopt(ac, av, "aenvt:")) != -1) {
	switch (c) {
	case 'a': a++; break;
	case 'e': break;	/* compat, would you believe it? */
	case 'n': n++; break;
	case 'v': v++; break;
	case 't': t = atoi(optarg); break;
	default:
	    vprintf(stderr, "intr: bad opt '%c'\n", c);
	    break;
	}
    }
    av = &av[optind];
    if (a) {
	vprintf(stderr, "%s", av[0]);
	for (c = 1; av[c]; ++c)
	    vprintf(stderr, " %s", av[c]);
    }
    else if (v)
	vprintf(stderr, " %s", av[0]);
    if ((a || v) && !n)
	vprintf(stderr, "\n");

    /*
     * Herein lies a tale.  This has to do with having init give the
     * console to the shell running the rc files.  Since it is a bourne
     * shell and a session leader, the pid==pgrp==sid, and the pgrp
     * of all children will be the same as the shell's.  When the
     * shell is running an rc file we want only certain processes
     * to be interruptable.  So init forks to create the shell,
     * that process forks a garndkid of init, the grandchild of init
     * changes the console's pgrp (which got set when the child
     * of init opened it up as the ctty) to be its pid.  
     * A picture (tuples are pid,pgrp,sid):
     *
     * init (1,0,0)
     * 		forks (10,0,0), 10 setsid's, and opens the console
     *		becoming (10,10,10), and sets the console's pgrp to 10.
     *			10 forks 11, 11 setpgrp's(11, 11),
     *			sets the console's pgrp to 11,
     *			and exits, leaving the console with noone
     *			to send signals to.
     *		10 (remember 10?) now execs a shell, happily knowing 
     *		that tty signals will all be sent to pgrp 11 which
     *		doesn't exist.  Nifty.  We're exploiting a hole.
     *
     * So when we want a process to be interruptable we prefix it
     * with this wrapper which 
     * A) checks to see if my pgrp == tty pgrp.  If it is,
     * don't worry, be happy, just do it.
     * B) if the pgrp's don't match, then create a new one and
     * change the tty's pgrp to match, then exec.
     *
     * Another picture:  suppose that we are in the rc and the shell
     * is forking "intr mount -avt nfs" and the process is # 50.
     * intr comes in as 50,10,10, checks the tty pgrp, it's 11, creates 
     * pgrp 50, sets the tty to 50 and execs mount.  Mount is now 
     * killable but when it exits it leaves the sh and the forks from
     * the shell all alone.
     *
     * One final note (sigh): The test for pgrp == tty pgrp is important.
     * We found that always creating a new pgrp and setting the tty to
     * match it confuses interactive shells (in an interactive shell the
     * two will be equal and we do nothing).  In particular, consider 
     * the single user shell (/bin/sh) where the shell, the tty, and the 
     * shell's kids all have the same pgrp.  Change the pgrp in a child, 
     * change the tty's pgrp, and when the child exits and the shell comes
     * back and does a read, it will get a SIGTTIN because it is now a
     * "background" process, i.e., its pgrp != the tty's pgrp.
     *
     * tty signals are ignored so that the ioctl's can't hang.
     */
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    pgrp = 0;
    if (ioctl(0, TIOCGPGRP, &pgrp) == -1)
	perror("intr: TIOCGPGRP");
    if (getpgrp(0) != pgrp) {
	setpgrp(0, pid = getpid());
	if (ioctl(0, TIOCSPGRP, &pid) == -1)
	    perror("intr: TIOCSPGRP");
    }
    if (t)
	alarm((unsigned)t);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    execvp(av[0], av);
    perror(av[0]);
    exit(1);
    /*NOTREACHED*/
}
