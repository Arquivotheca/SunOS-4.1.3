#ifndef lint
static char sccsid[] = "@(#)gcore.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 *  Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/errno.h>

char *id;	/* argv[0] */
void dump();

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	int c, i, errflg = 0;
	char *prefix = "core";

	id = argv[0];

	while ((c = getopt(argc, argv, "o:")) != EOF)
		switch(c) {
		case 'o':  /* prefix to core file */
			prefix = optarg;
			break;
		default:
			errflg++;
		}
	if (argc < 2 || errflg) {
		printf("Usage: %s  [-o file ]  pid ...\n", id);
		exit(1);
	}

	for (i = optind ; i < argc ; i++)
		dump(prefix, argv[i]);

	exit(0);
	/* NOTREACHED */
}


/*  Dump core image of a process to a file in
 *  the current directory with the ptrace
 *  dumpcore request.  The core file name is 
 *  formed from the prefix string and the pid.
 *  The process continues running when the
 *  dump is complete.
 */

void
dump(prefix, arg)
	char *prefix, *arg;
{
	int r;
	union wait status;
	int pid = atoi(arg);
	static char coref [128];

	sprintf(coref, "%s.%d", prefix, pid);

	if (call_ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
		fprintf(stderr, "%s: %s not dumped: ", id, coref);
		perror(arg);
		return;
	}


	while ((r = wait(&status)) != pid)	/* wait for pid to stop */
		continue;

	if (r < 0 || !(WIFSTOPPED(status))) {	/* going or gone ? */
		fprintf(stderr, "%s: %d exiting - not dumped\n", id, pid);
		return;
	}

	switch (-call_ptrace(PTRACE_DUMPCORE, pid, coref, 0)) {

		case 0:
		    fprintf(stderr, "%s: %s dumped\n", id, coref);
		    break;

		case EIO:
		    fprintf(stderr, "%s: %s: can't create\n", id, coref);
		    break;

		default:
		    fprintf(stderr, "%s: %s not dumped: ", id, coref);
		    perror(arg);
		    break;
	}

	if (call_ptrace(PTRACE_DETACH, pid, (int *) 1, 0) < 0) {
		fprintf(stderr, "%s: ptrace detach %d: ", id, pid);
		perror(arg);
		return;
	}
}


 /*
  * Call ptrace and check for errors. 
  */

int
call_ptrace(request, pid, addr, data)
    enum ptracereq request;
    int pid;
    char *addr;
    int data;
{
    extern int errno;

    errno = 0;
    ptrace(request, pid, addr, data);
    return errno != 0 ? -errno : 0;
}
