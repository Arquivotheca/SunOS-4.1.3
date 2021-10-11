#ifndef lint
static	char sccsid[] = "@(#)dispatcher.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <stdio.h>

#define MAXARGS 30
#define MAXHOSTNAMELEN  64

/******************************************************************************
 * The purpose of this program is to lower the priority of the forked test    *
 * for better SunView user interface response. Since we used vfork() in	      *
 * fork_test(), it is better not to lower the priority until now(this is also *
 * the reason for having this separate short progam).			      *
 ******************************************************************************/

extern	char	*getenv();

/*
 * arguments passed to dispatcher:
 *      priority remotehostname testname sd test_arguments
 * example:
 *      10 localhost nettest sd net_le0
 *
 * where argv[0] = 10, argv[1] = "localhost", etc.
 *
 * dispatcher will exec the either a local program "testname" if
 * remotehostname="localhost" or exec the program "remote" to execute 
 * "testname" on the remote machine.
 *
 * In the remote case, the array av[] is filled in:
 *	av[0] = "remote", av[1] = remotehostname, av[2]=testname,
 *	av[3] = "sd", av[4,etc] = [test_arguments, h, localhostname].
 * example:
 *	remote serf nettest sd net_le0 h oasis
 *	(where serf is the remote machine and oasis is the local machine)
 *
 */

/*ARGSUSED*/
main(argc, argv)
	int	argc;
	char	*argv[];
{
  int	priority;
  int	i=0;
  char	*av[MAXARGS];
  char  myhostname[MAXHOSTNAMELEN];

  priority = atoi(argv[0]);
  if (priority != 0)
    (void)nice(priority);	/* lower the priority now, if requested */

  if (strcmp(argv[1], "localhost") == 0) {  /* local execution */
    execvp(argv[2], &argv[2]);  /* run the test, skip the priority and
                                   remote hostname arguments. */
    perror("execvp(in dispatcher)");
  } else {			/* remote execution */
    av[0] = "remote";
    for (i=1; argv[i] != NULL; i++) {
      av[i] = argv[i];
    }
    av[i] = "h";
    /* get my own machine name */
    if ((gethostname(myhostname, MAXHOSTNAMELEN - 1)) == -1) {
        perror("dispatcher: can't get my hostname:");
	_exit(1);
    }
    av[i+1] = myhostname;
    av[i+2] = NULL;
    execvp(av[0], &av[0]);
    perror("execlp(in dispatcher)");
  }
  _exit(-1);
}
