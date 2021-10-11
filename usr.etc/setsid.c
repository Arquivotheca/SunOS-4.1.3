#ifndef lint
static char sccsid[] = "@(#)setsid.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <sys/session.h>
#include <sys/syscall.h>

/*
 * usage: setsid [-b] cmd [args]
 */
main(ac, av)
	char  **av;
{
	char **nav;

	if (ac < 2)
	    usage(av[0]);
	if (av[1][0] == '-') {
	    if (av[1][1] != 'b')
		usage(av[0]);
	    syscall(SYS_setsid, SESS_SYS);
	    nav = &av[2];
	}
	else {
	    syscall(SYS_setsid, SESS_NEW);
	    nav = &av[1];
	}
	execvp(*nav, nav);
}

usage(name)
{
	fprintf(stderr, "usage: %s [-b] cmd [args]\n", name);
	exit(1);
}
