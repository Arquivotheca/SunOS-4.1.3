#ifndef lint
static	char sccsid[] = "@(#)tcov.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>

enum	boolean	{ False, True };
typedef enum 	boolean Boolean;

/*
 * Structure for each line in the .d file
 */
struct	dotd	{
	int	lineno;			/* Line where this basic block begins*/
	unsigned int	count;		/* Count for this basic block */
	int	oldcount;		/* Count for the previous basic block*/
} dotd;

/*
 * Data structure to keep track of the most frequently executed lines
 */
struct	top	{
	int	line;			/* Line num of basic block */
	int	cnt;			/* Count for this basic block */
} *top;


static	int	topn;			/* Keep track of the top "n" */
static	int	nbasic;			/* Total number of basic blocks */
static	int	zero_basic;		/* Num of basic blocks not executed */
static	int	executions;		/* Total num basic block executions */
static	int	dotc_lineno;		/* Current line num in .c file */
static	Boolean	all;			/* Flag to annotate all lines */
static	Boolean	need_arrow;		/* Print an arrow before the line */
char	*cmdname;

char	*rindex();
char	*index();
Boolean	read_dotc();
Boolean	read_dotd();

main(argc, argv)
int	argc;
char	*argv[];
{
	cmdname = argv[0];
	topn = 10;
	argc--;
	argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch(argv[0][1]) {
		case 'a':
			all = True;
			break;

		default:
			if (isdigit(argv[0][1])) {
				topn = atoi(&argv[0][1]);
			} else {
				usage();
			}
			break;
		}
		argc--;
		argv++;
	}
	if (argc < 1) {
		usage();
	}
	init_topn();
	while (argc--) {
		clear_topn();
		nbasic = 0;
		zero_basic = 0;
		executions = 0;
		dotc_lineno = 0;
		dotd.count = 0;
		annotate(*argv++);
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * annotate - annotate the source file listing with execution counts
 */
static
annotate(dotc_file)
char	*dotc_file;
{
	FILE	*dotc_fp;
	FILE	*dotd_fp;
	FILE	*dottcov_fp;
	FILE	*tmp_fp;
	char	*tmp = "/tmp/sortd.XXXXXX";
	char	buff[150];
	char	dotd_file[500];
	char	dottcov_file[500];
	char	dotc_line[512];
	char	*cp;
	Boolean more_dotc;
	Boolean more_dotd;

	strcpy(dotd_file, dotc_file);
	cp = rindex(dotd_file, '.');
	if(cp == NULL)
		cp = index(dotd_file, '\0');
	strcpy(cp, ".d");
	dotd_fp = fopen(dotd_file, "r");
	if (dotd_fp == NULL) {
		fprintf(stderr, "%s: unable to open %s\n", cmdname, dotd_file);
		exit(1);
	}
	dotc_fp = fopen(dotc_file, "r");
	if (dotc_fp == NULL) {
		fprintf(stderr, "%s: unable to open %s\n", cmdname, dotc_file);
		exit(1);
	}
	strcpy(dottcov_file, dotc_file);
	cp = rindex(dottcov_file, '.');
	strcpy(cp, ".tcov");
	dottcov_fp = fopen(dottcov_file, "w");
	if (dottcov_fp == NULL) {
		fprintf(stderr, "%s: unable to open %s\n", cmdname,
			dottcov_file);
		exit(1);
	}

	mktemp (tmp);
	tmp_fp = fopen (tmp, "w");
	sprintf(buff, "sort -n %s > %s", dotd_file, tmp);
	system(buff);
	fclose(tmp_fp);
	fclose(dotd_fp);	
	dotd_fp = fopen (tmp, "r");

	more_dotc = read_dotc(dotc_fp, dotc_line, sizeof(dotc_line));
	more_dotd = read_dotd(dotd_fp, &dotd);
	while (more_dotc && more_dotd) {
		while (more_dotc && dotc_lineno < dotd.lineno) {
			pr_count(dottcov_fp, dotc_lineno, &dotd);
			pr_line(dottcov_fp, dotc_line);
			more_dotc = read_dotc(dotc_fp, dotc_line, 
				sizeof(dotc_line));
		}
		if (more_dotc) {
			pr_count(dottcov_fp, dotc_lineno, &dotd);
			more_dotd = read_dotd(dotd_fp, &dotd);
			while (more_dotd && dotc_lineno == dotd.lineno) {
				another_count(dottcov_fp, &dotd);
				more_dotd = read_dotd(dotd_fp, &dotd);
			}
			pr_line(dottcov_fp, dotc_line);
			more_dotc = read_dotc(dotc_fp, dotc_line, 
				sizeof(dotc_line));
		}
	}
	if (more_dotc) {
		do {
			pr_count(dottcov_fp, dotc_lineno, &dotd);
			pr_line(dottcov_fp, dotc_line);
			more_dotc = read_dotc(dotc_fp, dotc_line, 
				sizeof(dotc_line));
		} while(more_dotc);
	} else if (dotc_lineno < dotd.lineno) {
		fprintf(stderr, "Premature end-of-file on %s", dotc_file);
		exit(1);
	}
	pr_top(dottcov_fp);
	pr_stats(dottcov_fp);
	fclose(dotc_fp);
	fclose(dotd_fp);
	fclose(dottcov_fp);
	unlink (tmp);
}

/*
 * Read a line from the .c file
 */
static Boolean
read_dotc(fp, buf, size)
FILE	*fp;
char	*buf;
int	size;
{
	if (fgets(buf, size, fp) == NULL) {
		return(False);
	}
	dotc_lineno++;
	return(True);
}

/*
 * Read a line from the .d file
 */
static Boolean
read_dotd(fp, dp)
FILE	*fp;
struct	dotd	*dp;
{
	int	r;
	int	i, j;

	dp->oldcount = dp->count;
	r = fscanf(fp, "%d %d", &dp->lineno, &dp->count);
	if (r != 2) {
		return(False);
	}
	nbasic++;
	if (dp->count == 0) {
		zero_basic++;
	}
	executions += dp->count;
	for (i = 0; i < topn; i++) {
		if (dp->count > top[i].cnt) {
			for (j = topn - 1; j > i; j--) {
				top[j].line = top[j - 1].line;
				top[j].cnt = top[j - 1].cnt;
			}
			top[i].line = dp->lineno;
			top[i].cnt = dp->count;
			break;
		}
	}
	return(True);
}

/*
 * pr_count - print the count field on this line
 */
static
pr_count(fp, dotc_lineno, dp)
FILE	*fp;
int	dotc_lineno;
struct	dotd	*dp;
{
	if (dotc_lineno == dp->lineno) {
		pcount(fp, dp->count);
		need_arrow = True;
	} else if (all) {
		pcount(fp, dp->oldcount);
		need_arrow = True;
	} else {
		fprintf(fp, "       ");
		need_arrow = False;
	}
}

/*
 * pcount - print the count
 * Special case a zero count
 */
static
pcount(fp, count)
FILE	*fp;
int	count;
{
	if (count > 0) {
		fprintf(fp, "%7u", count);
	} else {
		fprintf(fp, "  #####");
	}
}

/*
 * another_count - more than one basic block on a line
 */
static
another_count(fp, dp)
FILE	*fp;
struct	dotd	*dp;
{
	fprintf(fp, ", ");
	pcount(fp, dp->count);
}

/*
 * print the source line
 */
static
pr_line(fp, line)
FILE	*fp;
char	*line;
{
	if (need_arrow) {
		fprintf(fp, " -> ");
	} else {
		fprintf(fp, "    ");
	}
	fprintf(fp, "%s", line);
}

/*
 * init_topn - allocate space for an array to hold
 * the top "n" most frequently executed lines.
 */
static
init_topn()
{
	top = (struct top *) malloc(topn * sizeof(struct top));
}

/*
 * clear_topn - set all the values in the top "n" list to zero
 */
static
clear_topn()
{
	struct	top	*tp;
	int 	i;

	tp = top;
	for (i = 0; i < topn; i++) {
		tp->line = 0;
		tp->cnt = 0;
		tp++;
	}
}

/*
 * pr_top - print the list of most frequently executed lines
 */
static
pr_top(dottcov_fp)
FILE	*dottcov_fp;
{
	int	i;

	fprintf(dottcov_fp, "\n");
	fprintf(dottcov_fp, "\n");
	fprintf(dottcov_fp, "\t\t Top %d Blocks\n", topn);
	fprintf(dottcov_fp, "\n");
	fprintf(dottcov_fp, "\t\t Line\t   Count\n");
	fprintf(dottcov_fp, "\n");
	for (i = 0; i < topn; i++) {
		if (top[i].cnt > 0) {
			fprintf(dottcov_fp, "\t\t%5d\t%8d\n", 
				top[i].line, top[i].cnt);
		} else {
			break;
		}
	}
}

static
pr_stats(fp)
FILE	*fp;
{
	float	percent;
	int	executed;

	fprintf(fp, "\n");
	fprintf(fp, "\n");
	executed = nbasic - zero_basic;
	percent = (executed * 100)/(float) nbasic;
	fprintf(fp, "\t%5d\tBasic blocks in this file\n", nbasic);
	fprintf(fp, "\t%5d\tBasic blocks executed\n", executed);
	fprintf(fp, "\t%5.2f\tPercent of the file executed\n", percent);
	fprintf(fp, "\n");
	fprintf(fp, "     %8d\tTotal basic block executions\n", executions);
	percent = executions/(float) nbasic;
	fprintf(fp, "     %8.2f\tAverage executions per basic block\n", percent);
}

static
usage()
{
	fprintf(stderr, "Usage: %s [-number] [-a] files ...\n", cmdname);
	exit(1);
}
