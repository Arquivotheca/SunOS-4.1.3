#ifndef lint
static char     sccsid[] = "@(#)pipe.c 1.1 92/07/30 Copyr Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"
#include <sys/wait.h>

/* close the pipe */
close_pipe()
{
	close(cf_fd);
	close(to_fd);
}
start_canfield()
{
	int             i;
	union wait	status;
	while (wait3(&status, WNOHANG, 0) > 0)
		continue;
	if (pipe(to) < 0) {
		perror("canfieldtool: pipe to");
		exit(1);
	}
	if (pipe(from) < 0) {
		perror("canfieldtool: pipe from");
		exit(1);
	}
	ppid = getpid();
	if ((pid = fork()) < 0) {
		perror("canfieldtool: fork");
		exit(1);
	}
	if (pid == 0) {		/* child */
		if (dup2(to[0], 0) < 0) {
			perror("canfieldtool: dup2 to");
			kill(ppid, 9);
			exit(1);
		}
		if (dup2(from[1], 1) < 0) {
			perror("canfieldtool: dup2 from");
			kill(ppid, 9);
			exit(1);
		}
		for (i = 3; i < 31; i++)
			close(i);
#ifdef DEBUG
		if (execl("/usr/troika/jsc/cftool/src/Canfield/canfield", "canfield", 0)) {
			/*
			 * should never happen, because it shouldn't
			 * return from the execl 
			 */
			fprintf(stderr, "tool: execl of jsc canfield worked\n");
			exit(1);
#else
		if (execl(canfield, canfield, 0)) {
			/*
			 * should never happen, because it shouldn't
			 * return from the execl 
			 */
			fprintf(stderr, "tool: execl worked\n");
			exit(1);
#endif
		} else {
			perror("canfieldtool: execl failed");
			kill(ppid, 9);
			exit(1);
		}
	}
	if (close(to[0]) < 0) {
		perror("canfieldtool: close to");
		exit(1);
	}
	if (close(from[1]) < 0) {
		perror("canfieldtool : close from");
		exit(1);
	}
	to_fd = to[1];
	return from[0];
}
