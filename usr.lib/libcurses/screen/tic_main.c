/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tic_main.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7 */
#endif

/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	comp_main.c --- Main program for terminfo compiler
 *
 *  $Log:	RCS/comp_main.v $
 * Revision 2.1  82/10/25  14:45:37  pavel
 * Added Copyright Notice
 *
 * Revision 2.0  82/10/24  15:16:37  pavel
 * Beta-one Test Release
 *
 * Revision 1.3  82/08/23  22:29:36  pavel
 * The REAL Alpha-one Release Version
 *
 * Revision 1.2  82/08/19  19:09:49  pavel
 * Alpha Test Release One
 *
 * Revision 1.1  82/08/12  18:36:55  pavel
 * Initial revision
 *
 *
 */


#define EXTERN		/* EXTERN=extern in other .c files */
#include <sys/types.h>
#include <sys/stat.h>
#include "compiler.h"

char	*source_file = "./terminfo.src";
char	*destination = SRCDIR;
char	*usage_string = "[-v[n]] [-c] source-file\n";
char	check_only = 0;
char	*progname;


main (argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	int	argflag = FALSE;

	debug_level = 0;
	progname = argv[0];

	umask(022);

	for (i=1; i < argc; i++)
	{
	    if (argv[i][0] == '-')
	    {
		switch (argv[i][1])
		{
		    case 'c':
			check_only = 1;
			break;

		    case 'v':
			debug_level = argv[i][2]  ?  atoi(&argv[i][2])  :  1;
			break;

		    default:
			fprintf(stderr,
			    "%s: Unknown option. Usage is:\n\t%s: %s\n",
						       argv[0],
						       progname,
						       usage_string);
			exit(1);
		}
	    }
	    else if (argflag)
	    {
		fprintf(stderr, "%s: Too many file names.  Usage is:\n\t%s\n",
							argv[0], usage_string);
		exit(1);
	    }
	    else
	    {
		argflag = TRUE;
		source_file = argv[i];
	    }
	}

	init();
	make_hash_table();
	compile();

	exit(0);
}

/*
 *	init()
 *
 *	Miscellaneous initializations
 *
 *	Open source file as standard input
 *	Change directory to working terminfo directory.
 *
 */

init()
{
	extern char	*getenv();
	char		*env = getenv("TERMINFO");

	start_time = time(0);

	curr_line = 0;

	if (freopen(source_file, "r", stdin) == NULL)
	{
	    fprintf(stderr, "%s: Can't open ", progname);
	    perror(source_file);
	    exit(1);
	}

	if (env && *env)
	    destination = env;

	if (check_only) {
		DEBUG(1,"Would be working in %s\n",destination);
	} else {
		DEBUG(1,"Working in %s\n",destination);
	}

	if (access(destination, 7) < 0)
	{
	    fprintf(stderr, "%s: ", progname);
	    perror(destination);
	    exit(1);
	}

	if (chdir(destination) < 0)
	{
	    fprintf(stderr, "%s: Can't change directories to ", progname);
	    perror(destination);
	    exit(1);
	}

}

/*
 *
 *	check_dir(dirletter)
 *
 *	Check for access rights to the destination directory.
 *	Create any directories which don't exist.
 *
 */

check_dir(dirletter)
char dirletter;
{
	struct stat	statbuf;
	static char	dirnames[128];
	static char	dir[2] = " ";

	if (dirnames[dirletter] == 0)
	{
	    dir[0] = dirletter;
	    if (stat(dir, &statbuf) < 0)
	    {
		if (mkdir(dir, 0755) < 0)
		    syserr_abort("Can't make directory %s", dir);
		dirnames[dirletter] = 1;
	    }
	    else if (access(dir, 7) < 0)
	    {
		fprintf(stderr, "%s: %s/%s: Permission denied\n",
						    progname, destination, dir);
		perror(dir);
		exit(1);
	    }
	    else if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	    {
		fprintf(stderr, "%s: %s/%s: Not a directory\n",
						    progname, destination, dir);
		perror(dir);
		exit(1);
	    }
	}
}

#include <curses.h>
#include <signal.h>
#if (defined(SYSV) || defined(USG)) && !defined(SIG_POLL)
/*
 *	mkdir(dirname, mode)
 *
 *	forks and execs the mkdir program to create the given directory
 *
 */

mkdir(dirname, mode)
char	*dirname;
int mode;
{
    int	fork_rtn;
    int	status;

    fork_rtn = fork();

    switch (fork_rtn)
	{
	case 0:		/* Child */
	    (void) execl("/bin/mkdir", "mkdir", dirname, (char*)0);
	    _exit(1);

	case -1:	/* Error */
	    fprintf(stderr, "%s: SYSTEM ERROR!! Fork failed!!!\n",
				progname);
	    exit(1);

	default:
	    (void) wait(&status);
	    if ((status != 0) || (chmod(dirname, mode) == -1))
		return -1;
	    return 0;
	}
}
#endif
