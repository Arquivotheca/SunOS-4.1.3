/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)dag.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

#include "stdio.h"
#include "ctype.h"

#define	NLINK	8

typedef	struct	node {
	struct	node *n_next;
	struct	node *n_left;
	struct	node *n_right;
	char	*n_name;
	char	*n_data;
	int	n_rcnt;
	int	n_visit;
	int	n_lcnt;
	struct	link *n_link;
} node;

node	*first = NULL;
node	*last = NULL;
node	*root = NULL;

typedef	struct	link {
	struct	link *l_next;
	struct	node *l_node[NLINK];
} link;

#define	isnamec(c)	(isalpha(c)||isdigit(c)||c=='_')
#define	NUL	'\0'
#define BIGINT	32767

void	exit();
node	*getnode();
char	*copy();
int	lines, nodes, links, chars;
int	lineno = 0;
int	lvmax = BIGINT;
char	stdbuf[BUFSIZ];

main(argc, argv)
char *argv[];
{
	extern int optind;
	extern char *optarg;
	register node *np;
	node *getnode();
	int c;
	void reader(), dfs();

	setbuf(stdout, stdbuf);
	while ((c = getopt(argc, argv, "d:")) != EOF)
		if (c == 'd')
			{
			if ((lvmax = getnum(optarg, 10)) == 0)
				{
				lvmax = BIGINT;
				goto argerr;
				}
			}
		else
		argerr:
			(void)fprintf(stderr, "dag: bad option %c ignored\n", c);
	reader(stdin);
	argc -= optind + 1;
	if (argc <= 1)
		for (np = first; np != NULL; np = np->n_next)
			{
			if (np->n_rcnt == 0)
				dfs(np, 0);
			}
	else
		while (--argc > 0)
			dfs(getnode(*++argv), 0);
	
	exit(0);
	/* NOTREACHED */
}

getnum(p, base)
	register int base;
	register char *p;
	{
	register int n;

	n = 0;
	while (isdigit(*p))
		n = n * base + (*p++ - '0');
	return(n);
	}

void reader(fp)
register FILE *fp;
{
	register char *p1, *p2;
	int	c;
	node	*np, *getnode();
	char	line[BUFSIZ], *copy();

	while (getst(line, fp)) {
		++lines;
		p1 = line;
		while (isspace(*p1))
			++p1;
		if (*p1 == NUL)
			continue;
		p2 = p1;
		while (isnamec(*p2))
			++p2;
		do {
			c = *p2;
			*p2++ = NUL;
		} while (isspace(c));
		switch (c) {

		case '=':
			np = getnode(p1);
			while (isspace(*p2))
				++p2;
			if (np->n_data != NULL) {
				(void)fprintf(stderr, "redefinition of %s\n", np->n_name);
				(void)cfree(np->n_data);
			}
			np->n_data = copy(p2);
			continue;
		case ':':
			np = getnode(p1);
			while (*(p1 = p2) != NUL) {
				while (isspace(*p1))
					++p1;
				p2 = p1;
				while (isnamec(*p2))
					++p2;
				(void)addlink(np, getnode(p1));
				if (*p2 != NUL)
					*p2++ = NUL;
			}
			continue;
		}
	}
}

getst(s, fp)
char *s;
FILE *fp;
{
	register i, c;

	i = 0;
	while ((c = getc(fp)) != EOF) {
		if (c == '\n') {
			*s = NUL;
			return 1;
		}
		if (i++ < BUFSIZ)
			*s++ = c;
	}
	return 0;
}

char *
copy(s)
char	*s;
{
	char	*p, *strcpy(), *calloc();
	void	exit();

	p = calloc(sizeof(char), (unsigned)(strlen(s)+2));
	if (p == NULL) {
		(void)fprintf(stderr, "too many characters\n");
		exit(1);
	}
	(void)strcpy(p, s);
	chars += strlen(p);
	return p;
}

node *
getnode(name)
char	*name;
{
	register i;
	register node *np, **pp;
	char *copy(), *calloc();

	pp = &root;
	while ((np = *pp) != NULL) {
		i = strcmp(name, np->n_name);
		if (i > 0)
			pp = &np->n_right;
		else if (i < 0)
			pp = &np->n_left;
		else
			return np;
	}
	*pp = (node *)calloc(sizeof(node), 1);
	if ((np = *pp) == NULL) {
		(void)fprintf(stderr, "too many nodes");
		exit(1);
	}
	++nodes;
	np->n_name = copy(name);
	np->n_data = NULL;
	if (first == NULL)
		first = np;
	if (last != NULL)
		last->n_next = np;
	last = np;
	np->n_next = NULL;
	np->n_left = np->n_right = NULL;
	np->n_rcnt = np->n_lcnt = np->n_visit = 0;
	np->n_link = NULL;
	return np;
}

addlink(np, rp)
node	*np, *rp;
{
	register i, j;
	char *calloc();
	link	*lp, **pp;

	i = j = 0;
	pp = &np->n_link;
	while ((lp = *pp) != NULL && i < np->n_lcnt) {
		if (rp == lp->l_node[j])
			return 0;
		if (++j >= NLINK) {
			j = 0;
			pp = &lp->l_next;
		}
		++i;
	}
	if (lp == NULL) {
		*pp = (link *)calloc(sizeof(link), 1);
		if ((lp = *pp) == NULL) {
			(void)fprintf(stderr, "too many links\n");
			exit(1);
		}
		++links;
		lp->l_next = NULL;
	}
	lp->l_node[j] = rp;
	if (np != rp)
	++rp->n_rcnt;
	++np->n_lcnt;
	return 1;
}

void dfs(np, lv)
register
node *np;
{
	register i, j;
	link *lp;

	if (np == NULL || lv > lvmax)
		return;
	i = 0;
	(void)printf("%d\t",++lineno);
	while (i++ < lv)
		(void)putchar('\t');
	(void)printf("%s: ", np->n_name);
	if (np->n_visit > 0) {
		(void)printf("%d\n", np->n_visit);
		return; 
	}
	if (np->n_data != NULL)
		(void)printf("%s\n", np->n_data);
	else
		(void)printf("<>\n");
	np->n_visit = lineno;
	i = j = 0;
	lp = np->n_link;
	while (i < np->n_lcnt && lp != NULL) {
		dfs(lp->l_node[j], lv+1);
		if (++j >= NLINK) {
			j = 0;
			lp = lp->l_next;
		}
		++i;
	}
}
