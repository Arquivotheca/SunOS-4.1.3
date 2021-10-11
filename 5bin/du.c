#ifndef lint
static	char sccsid[] = "@(#)du.c 1.1 92/07/30 SMI"; /* from UCB 4.11 83/07/01 */
#endif
/*
**	du -- summarize disk usage
**		du [-ars] [name ...]
*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

char	path[MAXPATHLEN+1], name[MAXPATHLEN+1];
int	aflg;
int	rflg;
int	sflg;
char	*dot = ".";

#define ML	1000
struct {
	int	dev;
	ino_t	ino;
} ml[ML];
int	mlx;

long	descend();
extern char	*strchr(), *strrchr(), *strcpy();

#define	nlb(n)	(howmany(dbtob(n), 512))

main(argc, argv)
	int argc;
	char **argv;
{
	long blocks = 0;
	register c;
	extern char *optarg;
	extern int optind;
	register char *np;
	register int pid, wpid;
	int status, retcode = 0;

	while ((c = getopt(argc, argv, "ars")) != EOF)
		switch(c) {

		case 'a':
			aflg++;
			continue;

		case 'r':
			rflg++;
			continue;

		case 's':
			sflg++;
			continue;

		default:
			fprintf(stderr, "usage: du [-ars] [name ...]\n");
			exit(2);
		}
	if (optind == argc) {
		argv = &dot;
		argc = 1;
		optind = 0;
	}
	do {
		if (optind < argc - 1) {
			pid = fork();
			if (pid == -1) {
				perror("du: No more processes");
				exit(1);
			}
			if (pid != 0) {
				while ((wpid = wait(&status)) != pid
				    && wpid != -1)
					;
				if (pid != -1) {
					if (status != 0)
						retcode = 1;
				}
			}
		}
		if (optind == argc - 1 || pid == 0) {
			(void) strcpy(path, argv[optind]);
			(void) strcpy(name, argv[optind]);
			if (np = strrchr(name, '/')) {
				*np++ = '\0';
				if (chdir(*name ? name : "/") < 0) {
					if (rflg) {
						fprintf(stderr,
						    "du: ");
						perror(*name ? name : "/");
					}
					exit(1);
				}
			} else
				np = path;
			blocks = descend(path, *np ? np : ".");
			if (sflg)
				printf("%ld\t%s\n", nlb(blocks), path);
			if (optind < argc - 1)
				exit(0);
		}
		optind++;
	} while (optind < argc);
	exit(retcode);
	/* NOTREACHED */
}

DIR	*dirp = NULL;

long
descend(base, name)
	char *base, *name;
{
	char *ebase0, *ebase;
	struct stat stb;
	int i;
	long blocks = 0;
	long curoff = 0;
	register struct direct *dp;

	ebase0 = ebase = strchr(base, 0);
	if (ebase > base && ebase[-1] == '/')
		ebase--;
	if (lstat(name, &stb) < 0) {
		if (rflg) {
			fprintf(stderr, "du: ");
			perror(base);
		}
		*ebase0 = 0;
		return (0);
	}
	if (stb.st_nlink > 1 && (stb.st_mode&S_IFMT) != S_IFDIR) {
		for (i = 0; i <= mlx; i++)
			if (ml[i].ino == stb.st_ino && ml[i].dev == stb.st_dev)
				return (0);
		if (mlx < ML) {
			ml[mlx].dev = stb.st_dev;
			ml[mlx].ino = stb.st_ino;
			mlx++;
		}
	}
	blocks = stb.st_blocks;
	if ((stb.st_mode&S_IFMT) != S_IFDIR) {
		if (aflg)
			printf("%ld\t%s\n", nlb(blocks), base);
		return (blocks);
	}
	if (dirp != NULL)
		closedir(dirp);
	dirp = opendir(name);
	if (dirp == NULL) {
		if (rflg) {
			fprintf(stderr, "du: ");
			perror(base);
		}
		*ebase0 = 0;
		return (0);
	}
	if (chdir(name) < 0) {
		if (rflg) {
			fprintf(stderr, "du: ");
			perror(base);
		}
		*ebase0 = 0;
		closedir(dirp);
		dirp = NULL;
		return (0);
	}
	while (dp = readdir(dirp)) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		(void) sprintf(ebase, "/%s", dp->d_name);
		curoff = telldir(dirp);
		blocks += descend(base, ebase+1);
		*ebase = 0;
		if (dirp == NULL) {
			dirp = opendir(".");
			if (dirp == NULL) {
				if (rflg) {
					fprintf(stderr, "du: Can't reopen '.' in ");
					perror(base);
				}
				return (0);
			}
			seekdir(dirp, curoff);
		}
	}
	closedir(dirp);
	dirp = NULL;
	if (sflg == 0)
		printf("%ld\t%s\n", nlb(blocks), base);
	if (chdir("..") < 0) {
		if (rflg) {
			(void) sprintf(strchr(base, '\0'), "/..");
			fprintf("du: Can't change directories to '..' in ");
			perror(base);
		}
		exit(1);
	}
	*ebase0 = 0;
	return (blocks);
}
