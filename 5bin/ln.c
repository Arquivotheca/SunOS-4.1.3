#ifndef lint
static	char sccsid[] = "@(#)ln.c 1.1 92/07/30 SMI"; /* from UCB 4.6 11/15/85 */
#endif
/*
 * ln
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>

struct	stat stb;
int	Fflag;		/* force a hard link to a directory? */
int	fflag;		/* no questions asked */
int	sflag;		/* symbolic link */
char	*rindex();
extern	int errno;

main(argc, argv)
	int argc;
	register char **argv;
{
	register int i, r;
	extern int optind;
	int c;

	while ((c = getopt(argc, argv, "Ffs")) != EOF) {
		switch(c) {
			case 'F':
				Fflag++;
				break;
			case 'f':
				fflag++;
				break;
			case 's':
				sflag++;
				break;
			case '?':
			default:
				goto usage;
		}
	}

	argc -= optind;
	argv = &argv[optind];

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
	    "Usage: ln [-F] [-f] [-s] f1 or ln [-F] [-f] [-s] f1 f2 or ln [-s] f1 ... fn d1\n");
	exit(1);
	/* NOTREACHED */
}

int	link(), symlink();

linkit(from, to)
	char *from, *to;
{
	char destname[MAXPATHLEN + 1];
	char *tail;
	struct	stat stb2;
	int (*linkf)() = sflag ? symlink : link;

	if (stat(from, &stb) < 0 && !sflag){
		fprintf(stderr,"ln: cannot access %s\n", from);
		return(1);
	}

	/* is source a directory? */
	if (sflag == 0 && Fflag == 0 && (stb.st_mode&S_IFMT) == S_IFDIR) {
		fprintf(stderr,"ln: %s is a directory\n", from);
		return (1);
	}
	if ((stat(to, &stb2) >= 0) && (stb2.st_mode&S_IFMT) == S_IFDIR) {
		tail = rindex(from, '/');
		if (tail == 0)
			tail = from;
		else
			tail++;
		if (strlen(to) + strlen(tail) >= sizeof destname - 1) {
			(void) fprintf(stderr, 
				       "ln: %s/%s: Name too long\n", to, tail);
			return (1);
		}
		(void) sprintf(destname, "%s/%s", to, tail);
		to = destname;
	}

	if (stat(to, &stb2) >=0) {
		/* if filenames and inodes are the same, it's an error */
		if (stb.st_dev == stb2.st_dev && stb.st_ino == stb2.st_ino) {
			fprintf(stderr,"ln: %s and %s are identical\n",from, to);
			return (1);
		}
		/* if the user doesn't have access to the file, ask him */
		if (access(to, W_OK) < 0 && isatty(fileno(stdin)) && !fflag) {
			printf("ln: override protection %o for %s? ", 
					stb2.st_mode&0777, to);
			if (!yes())
				return(1);
		}
		if (unlink(to) < 0) {
			fprintf(stderr,"ln: cannot unlink %s\n", to);
			return(1);
		}
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

/*
 * Get a yes/no answer from the user.
 */
yes()
{
	int i, b;
	i = b = getchar();
	while (b != '\n' && b != EOF)
		b = getchar();
	return (i == 'y');
}
