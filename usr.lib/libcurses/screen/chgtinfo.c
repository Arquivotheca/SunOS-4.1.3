/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)chgtinfo.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1 */
#endif

#include <curses.h>
#include <signal.h>

/*

    NAME

	modifyinfo - create a temporary version of a terminfo entry

    SYNOPSIS

	TERMINFO=`modifyinfo modifications` program
	export TERMINFO

    DESCRIPTION

	One of the touted drawbacks of terminfo has been that one could
	not create a temporary modification of a terminfo entry.

	The terminfo-modifications are actual terminfo source statements
	and are simply passed on to tic for it to compile.

    EXAMPLES

	TERMINFO=`chgtinfo xhp, xon@, smso@, smul=\E[99m,`
	export TERMINFO

	This will set xhp, remove xon and smso, and add or change
	smul to the new value.

    AUTHOR
	Tony Hansen, May 29, 1986.
*/

extern char *getenv();
extern void exit(), _exit();

char *progname;
int rmstate = 0;
char TMPtermfile[512];
char *dirname;

/* routine to remove files if program dies for some reason */
#ifdef SIGPOLL
void doremoves()
#else
int doremoves()
#endif
{
    switch (rmstate)
	{
	default:
	case 3:
	    (void) unlink("tmp.ti");
	    /* no break */
	case 2:
	    (void) unlink(TMPtermfile);
	    /* no break */
	case 1:
	    (void) chdir("/");
	    (void) rmdir(dirname);
	case 0:
	    break;
	}
    exit(1);
}

/* error exit routine */
void errexit(msg)
char *msg;
{
    (void) fprintf (stderr, "%s: %s\n", progname, msg);
    doremoves();
}

void usage()
{
    (void) fprintf (stderr, "usage: %s terminfo-modifications\n", progname);
    exit(1);
}

int findtermfile(terminfo, term)
char *terminfo, *term;
{
    char termfile[512];
    if (terminfo && *terminfo)
	{
	(void) sprintf(termfile, "%s/%c/%s", terminfo, *term, term);
	if ((access(termfile, 0) == 0) &&
	    copy(termfile, TMPtermfile))
	    return 1;
	}
    return 0;
}

main(argc, argv)
int argc;
char **argv;
{
    register int i;
    register char *term = getenv("TERM");
    register FILE *tmpfp;
    char cmdbuf[512];

    progname = argv[0];

    /* check $TERM and usage */
    if (!term || !*term)
	errexit("TERM not set");
    if (argc == 1)
	usage();

    /* catch signals */
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	(void) signal(SIGINT, doremoves);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	(void) signal(SIGQUIT, doremoves);

    /* create temporary directories */
    dirname = tempnam((char*) 0, "tinfo");
    if (!dirname)
	errexit("tempnam() failed");
    if (mkdir(dirname, 0755) < 0)
	errexit("mkdir() failed");
    rmstate++;
    if (chdir(dirname) < 0)
	errexit("chdir() failed");
    if (mkdir(*term, 0755) < 0)
	errexit("mkdir() failed");
    rmstate++;

    /* copy the current $TERM file to terminal named ?TMP+TMP */
    /* where ? is the first letter of $TERM */
    (void) sprintf(TMPtermfile, "%s/%c/%cTMP+TMP", dirname, *term, *term);
    if (!findtermfile(getenv("TERMINFO"), term) &&
	!findtermfile("/usr/lib/terminfo", term))
	errexit("Cannot find terminfo file");
    rmstate++;

    /* create a temporary .ti file that references use=?TMP+TMP */
    if (!(tmpfp = fopen("tmp.ti", "w")))
	errexit("Cannot open temp file");
    rmstate++;

    if (fprintf (tmpfp, "%s|modified version of %s,\n", term, term) == EOF)
	errexit("Cannot write to temp file");
    for (i = 1; i < argc; i++)
	if (fprintf (tmpfp, "\t%s\n", argv[i]) == EOF)
	    errexit("Cannot write to temp file");
    if (fprintf (tmpfp, "\tuse=%cTMP+TMP,\n", *term) == EOF)
	errexit("Cannot write to temp file");
    if (fclose (tmpfp) == EOF)
	errexit("Cannot close temp file");

    /* start up tic on tmp.ti */
    (void) sprintf(cmdbuf, "TERMINFO=%s tic tmp.ti", dirname);
    if (system(cmdbuf) != 0)
	errexit("tic failed");

    /* clean up unnecessary files */
    (void) unlink("tmp.ti");
    (void) unlink(TMPtermfile);

    /* print out directory name */
    (void) printf("%s\n", dirname);
    exit(0);
    /*NOTREACHED*/
}

#if (defined(SYSV) || defined(USG)) && !defined(SIGPOLL)
/*
 *	mkdir(dirname, mode)
 *
 *	forks and execs the mkdir program to create the given directory
 *
 */

mkdir(directory, mode)
char	*directory;
int mode;
{
    int	fork_rtn;
    int	status;

    fork_rtn = fork();

    switch (fork_rtn)
	{
	case 0:		/* Child */
	    (void) execl("/bin/mkdir", "mkdir", directory, (char*)0);
	    _exit(1);

	case -1:	/* Error */
	    errexit("Fork() failed!");
	    return -1;

	default:
	    (void) wait(&status);
	    if (!status)
		return chmod(directory, mode);
	    return -1;
	}
}

/*
 *	rmdir(dirname)
 *
 *	forks and execs the rmdir program to remove the given directory
 *
 */

int rmdir(directory)
char *directory;
{
    int	fork_rtn;
    int	status;

    fork_rtn = fork();

    switch (fork_rtn)
	{
	case 0:		/* Child */
	    (void) execl("/bin/rmdir", "rmdir", directory, (char*)0);
	    _exit(1);

	case -1:	/* Error */
	    errexit("Fork() failed!");
	    return -1;

	default:
	    (void) wait(&status);
	    return !status;
	}
}
#endif

/* copy file f1 into file f2 */
#include <fcntl.h>

int copy(f1, f2)
char *f1, *f2;
{
    register int infd, outfd, n, ret;
    char buf[BUFSIZ];

    infd = open(f1, O_RDONLY);
    if (infd < 0)
	return 0;
    outfd = creat(f2, 0644);
    if (outfd < 0)
	{
	(void) close(infd);
	return 0;
	}

    ret = 1;
    while ((n = read(infd, buf, sizeof(buf))) > 0)
	if (write(outfd, buf, n) != n)
	    {
	    ret = 0;
	    break;
	    }

    (void) close(infd);
    (void) close(infd);
    return ret;
}
