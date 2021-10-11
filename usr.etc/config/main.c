#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from UCB 1.9 83/05/18 */
#endif

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include <sys/stat.h>
#include <errno.h>
#include "y.tab.h"

char *myname;

/*
 * Config builds a set of files for building a UNIX
 * system given a description of the desired system.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	char	*objname = "OBJ";
	char	*opts;
	char	buf[80];
	char	dir[80];
	int	no_makedepend = 0;
	int	fastmake = 0;
	extern int getfromsccs;
	struct	stat stat_buf;

	myname = argv[0];
	while (argc > 2) {
		if (argv[1][0] == '-') {
			opts = &argv[1][1];
			argc--, argv++;
			while (*opts) {
				switch (*opts) {
				case 'p':
					profiling++;
					break;
				case 'n':
					no_makedepend++;
					break;
				case 'f':
					fastmake++;
					break;
				case 'o':
					objname = argv[1];
					argc--, argv++;
					break;
				case 'g':
					getfromsccs++;
					break;
				default:
					fprintf(stderr, "unknown option '%c'\n",
						*opts);
					usage();
				}
				opts++;
			}
		} else {
			usage();
		}
	}

	if (argc != 2)
		usage();

	PREFIX = argv[1];
	strcpy(buf, argv[1]);
	if (freopen(buf, "r", stdin) == NULL) {
		perror(buf);
		exit(2);
	}
	(void) sprintf(dir, "../%s", argv[1]);
	if (stat(dir, &stat_buf)) {
		extern int errno;

		if (errno == ENOENT) {
			if (mkdir(dir, 0777)) {
				perror(dir);
				exit(2);
			}
		} else {
			perror(dir);
			exit(2);
		}
	} else if ((stat_buf.st_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr, "%s: %s is not a directory\n", myname, dir);
		exit(2);
	}
	dtab = NULL;
	confp = &conf_list;
	if (yyparse() || yyerror_invoked)
		exit(3);
	switch (machine) {

	case MACHINE_VAX:
		ubglue();		/* Create ubglue.s */
		vax_ioconf();		/* Print ioconf.c */
		break;

	case MACHINE_SUN2:
	case MACHINE_SUN3:
	case MACHINE_SUN3X:
		/* Must be in this order */
		mbglue();		/* Create mbglue.s */
		sun_ioconf();		/* Print ioconf.c */
		break;

	case MACHINE_SUN4:
	case MACHINE_SUN4C:
	case MACHINE_SUN4M:
		sun_ioconf();		/* Print ioconf.c */
		break;

	default:
		printf("Specify machine type, e.g. ``machine vax''\n");
		exit(1);
	}
	makefile(objname, fastmake);	/* build Makefile */
	headers();			/* make a lot of .h files */
	bootconf();			/* swap config files */
	if (no_makedepend) {
		printf("Don't forget to run \"make depend\"\n");
		exit(0);
	} else {
		register int exval;
		printf("Doing a \"make depend\"\n");
		fflush(stdout);
		(void) sprintf(buf, "cd %s; make depend > /dev/null\n", dir);
		exval = system(buf);
		if (exval & 0xff) /* interrupt occurred? */
			exit (exval & 0xff);
		else
			exit ((exval>>8) & 0xff);
	}
	/* NOTREACHED */
}

/*
 * get_word
 *	returns EOF on end of file
 *	NULL on end of line
 *	pointer to the word otherwise
 */
char *
get_word(fp)
	register FILE *fp;
{
	static char line[80];
	register int ch;
	register char *cp;

	while ((ch = getc(fp)) != EOF)
		if (ch != ' ' && ch != '\t')
			break;
	if (ch == EOF)
		return ((char *)EOF);
	if (ch == '\n')
		return (NULL);
	cp = line;
	*cp++ = ch;
	while ((ch = getc(fp)) != EOF) {
		if (isspace(ch))
			break;
		*cp++ = ch;
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	(void) ungetc(ch, fp);
	return (line);
}

/*
 * prepend the path to a filename
 */
char *
path(file)
	char *file;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(PREFIX)+strlen(file)+5));
	(void) strcpy(cp, "../");
	(void) strcat(cp, PREFIX);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

usage()
{
	fprintf(stderr, "usage: %s [-p] [-n] [-f] [-o OBJ] sysname\n", myname);
	exit(1);
}
