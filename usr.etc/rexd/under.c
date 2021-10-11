# ifdef lint
static char sccsid[] = "@(#)under.c 1.1 92/07/30 Copyr 1985 Sun Micro";
# endif lint

/*
 *  under.c - program to execute a command under a given directory
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>

main(argc, argv)
	int argc;
	char **argv;
{
	static char usage[] = "Usage: under dir command...\n";
	char *dir, *p;
	char *index(), *mktemp();
	char hostname[255];
	char *tmpdir, *subdir, *parsefs();
	char dirbuf[1024];
	char error[1024];
	int status;
	int len;

	if (argc < 3) {
		fprintf(stderr, usage);
		exit(1);
	}
	gethostname(hostname, 255);
	strcat(hostname, ":/");
	len = strlen(hostname);
	dir = argv[1];
	if (strlen(dir) > len && strncmp(dir, hostname, len) == 0)
		dir = index(dir, ':') + 1;
	else if (p = index(dir, ':')) {
		if (p[1] != '/') {
			fprintf(stderr, "under: %s invalid name\n", dir);
			exit(1);
		}
		tmpdir = mktemp("/tmp/underXXXXXX");
		if (mkdir(tmpdir, 0777)) {
			perror(tmpdir);
			exit(1);
		}
		subdir = parsefs(dir,error);
		if (subdir == NULL) {
			exit(1);
		}
		if (mount_nfs(dir, tmpdir, error)) {
			exit(1);
		}
		strcpy(dirbuf, tmpdir);
		strcat(dirbuf, "/");
		strcat(dirbuf, subdir);
		status = runcmd(dirbuf, argv[2], &argv[2]);
		if (umount_nfs(dir, tmpdir))
			fprintf(stderr, "under: couldn't umount %s\n", dir);
		rmdir(tmpdir);
		exit(status);
	}
		
	setgid(getgid());
	setuid(getuid());
	if (chdir(dir)) {
		perror(dir);
		exit(1);
	}
	execvp(argv[2], &argv[2]);
	perror(argv[2]);
	exit(1);
	/* NOTREACHED */
}

typedef void (*sig_t)();

runcmd(dir, cmd, args)
	char *cmd;
	char **args;
{
	int pid, child, status;
	sig_t sigint, sigquit;

	sigint = signal(SIGINT, SIG_IGN);
	sigquit = signal(SIGQUIT, SIG_IGN);
	pid = fork();
	if (pid == -1)
		return (0177);
	if (pid == 0) {
		setgid(getgid());
		setuid(getuid());
		if (chdir(dir)) {
			perror(dir);
			exit(1);
		}
		(void) signal(SIGINT, sigint);
		(void) signal(SIGQUIT, sigquit);
		execvp(cmd, args);
		perror(cmd);
		exit(1);
	}
	while ((child = wait(&status)) != pid && child != -1)
		;
	(void) signal(SIGINT, sigint);
	(void) signal(SIGQUIT, sigquit);
	if (child == -1)
		return (0177);
	if (status & 0377)
		return (status & 0377);
	return ((status >> 8) & 0377);
}
