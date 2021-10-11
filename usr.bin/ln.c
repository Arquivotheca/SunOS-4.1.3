#ifndef lint
static	char sccsid[] = "@(#)ln.c 1.1 92/07/30 SMI"; /* from UCB 4.6 11/15/85 */
#endif
/*
 * ln
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

struct	stat stb;
int	fflag;		/* force flag set? */
int	sflag;
char	*rindex();
extern	int errno;

main(argc, argv)
	int argc;
	register char **argv;
{
	register int i, r;

	argc--, argv++;
again:
	if (argc && strcmp(argv[0], "-f") == 0) {
		fflag++;
		argv++;
		argc--;
	}
	if (argc && strcmp(argv[0], "-s") == 0) {
		sflag++;
		argv++;
		argc--;
	}
	if (argc == 0) 
		goto usage;
	else if (argc == 1) {
		argv[argc] = ".";
		argc++;
	}
	if (argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	r = 0;
	for(i = 0; i < argc-1; i++)
		r |= linkit(argv[i], argv[argc-1]);
	exit(r);
usage:
	(void) fprintf(stderr,
	    "Usage: ln [-s] f1 or ln [-s] f1 f2 or ln [-s] f1 ... fn d1\n");
	exit(1);
	/* NOTREACHED */
}

int	link(), symlink();

linkit(from, to)
	char *from, *to;
{
	char destname[MAXPATHLEN + 1];
	char *tail;
	int (*linkf)() = sflag ? symlink : link;

	/* is target a directory? */
	if (sflag == 0 && fflag == 0 && stat(from, &stb) >= 0
	    && (stb.st_mode&S_IFMT) == S_IFDIR) {
		printf("%s is a directory\n", from);
		return (1);
	}
	if (stat(to, &stb) >= 0 && (stb.st_mode&S_IFMT) == S_IFDIR) {
		tail = rindex(from, '/');
		if (tail == 0)
			tail = from;
		else
			tail++;
		if (strlen(to) + strlen(tail) >= sizeof destname - 1) {
			(void) fprintf(stderr, "ln: %s/%s: Name too long\n",
			    to, tail);
			return (1);
		}
		(void) sprintf(destname, "%s/%s", to, tail);
		to = destname;
	}
	if ((*linkf)(from, to) < 0) {
		if (errno == EEXIST || sflag)
			Perror(to);
		else if (access(from, 0) < 0)
			Perror(from);
		else
			Perror(to);
		return (1);
	}
	return (0);
}

Perror(s)
	char *s;
{

	(void) fprintf(stderr, "ln: ");
	perror(s);
}
