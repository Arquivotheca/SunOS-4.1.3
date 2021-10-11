#ifndef lint
static char sccsid[] = "@(#)lsw.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * lsw -- lists whiteout entries in a directory.
 */

#include <nse/param.h>
#include <nse/types.h>
#include <sys/dir.h>
#include <stdio.h>
#include <strings.h>
#include <sgtty.h>
#include <nse/util.h>
#include <nse/tfs_calls.h>

char		*dotp = ".";
bool_t		is_tty;
bool_t		usetabs;

extern char	*realloc();

void		printdir();
int		getdir();
void		formatdir();
int		strcompare();


main(argc, argv)
	int argc;
	char *argv[];
{
	struct sgttyb	sgbuf;

	nse_set_cmdname(argv[0]);
	argc--;
	argv++;
	if (isatty(1)) {
		is_tty = TRUE;
		(void) gtty(1, &sgbuf);
		if ((sgbuf.sg_flags & XTABS) == 0) {
			usetabs = TRUE;
		}
	} else {
		usetabs = TRUE;
	}
	if (argc == 0) {
		argc++;
		argv = &dotp;
	}
	printdir(*argv, (argc > 1));
	while (argc > 1) {
		argc--;
		argv++;
		printdir(*argv, TRUE);
	}
}


void
printdir(name, title)
	char		*name;
	bool_t		title;
{
	char		**wo_list;
	int		nentries;

	nentries = getdir(name, &wo_list);
	if ((title) && (nentries >= 0)) {
		printf("%s:\n", name);
	}
	if (nentries <= 0) {
		return;
	}
	qsort((char *) wo_list, nentries, sizeof (char *), strcompare);
	formatdir(wo_list, nentries);
}


int
getdir(dir, pfirst)
	char		*dir;
	char		***pfirst;
{
	DIR		*dirp;
	struct direct	*dp;
	int		maxentries = 2;
	int		nentries = 0;
	char		**p;

	dirp = opendir(dir);
	if (dirp == NULL) {
		printf("%s unreadable\n", dir);
		return (-1);
	}
	p = (char **) calloc((unsigned) maxentries, sizeof(char *));
	*pfirst = p;
	while (dp = nse_readdir_whiteout((char *) NULL, dir, dirp)) {
		*p = NSE_STRDUP(dp->d_name);
		p++;
		nentries++;
		if (p == *pfirst + maxentries) {
			*pfirst = (char **) realloc((char *) *pfirst,
				   (unsigned) 2 * maxentries * sizeof(char *));
			if (*pfirst == NULL) {
				fprintf(stderr, "lsw: out of memory\n");
				exit (1);
			}
			p = *pfirst + maxentries;
			maxentries *= 2;
		}
	}
	closedir(dirp);
	return (nentries);
}


void
formatdir(wo_list, nentries)
	char		**wo_list;
	int		nentries;
{
	char		**p;
	int		columns;
	int		width = 0;
	int		lines;
	int		len;
	int		i;
	int		j;
	int		w;

	if (is_tty) {
		for (p = wo_list; p < wo_list + nentries; p++) {
			len = strlen(*p);
			if (len > width) {
				width = len;
			}
		}
		if (usetabs) {
			width = (width + 8) &~ 7;
		} else {
			width += 2;
		}
		columns = 80 / width;
		if (columns == 0) {
			columns = 1;
		}
	} else {
		columns = 1;
	}
	lines = (nentries + columns - 1) / columns;
	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			p = wo_list + j * lines + i;
			printf("%s", *p);
			if (p + lines >= wo_list + nentries) {
				printf("\n");
				break;
			}
			w = strlen(*p);
			while (w < width)
				if (usetabs) {
					w = (w + 8) &~ 7;
					putchar('\t');
				} else {
					w++;
					putchar(' ');
				}
		}
	}
}


int
strcompare(str1, str2)
	char		**str1;
	char		**str2;
{
	return (strcmp(*str1, *str2));
}
