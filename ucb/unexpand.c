#ifndef lint
static	char sccsid[] = "@(#)unexpand.c 1.1 92/07/30 SMI"; /* from UCB 4.2 02/09/83 */
#endif
/*
 * unexpand - put tabs into a file replacing blanks
 */
#include <stdio.h>

char	genbuf[BUFSIZ];
char	linebuf[BUFSIZ];
int	all;

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;

	argc--, argv++;
	if (argc > 0 && argv[0][0] == '-') {
		if (strcmp(argv[0], "-a") != 0) {
			fprintf(stderr, "usage: unexpand [ -a ] file ...\n");
			exit(1);
		}
		all++;
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			argc--, argv++;
		}
		while (fgets(genbuf, BUFSIZ, stdin) != NULL) {
			for (cp = linebuf; *cp; cp++)
				continue;
			if (cp > linebuf)
				cp[-1] = 0;
			tabify(all);
			printf("%s", linebuf);
		}
	} while (argc > 0);
	exit(0);
	/* NOTREACHED */
}

tabify(c)
	char c;
{
	register char *cp, *dp;
	register int dcol;
	int ocol;

	ocol = 0;
	dcol = 0;
	cp = genbuf, dp = linebuf;
	for (;;) {
		switch (*cp) {

		case ' ':
			dcol++;
			break;

		case '\t':
			dcol += 8;
			dcol &= ~07;
			break;

		default:
			while (((ocol + 8) &~ 07) <= dcol) {
				if (ocol + 1 == dcol)
					break;
				*dp++ = '\t';
				ocol += 8;
				ocol &= ~07;
			}
			while (ocol < dcol) {
				*dp++ = ' ';
				ocol++;
			}
			if (*cp == 0 || c == 0) {
				strcpy(dp, cp);
				return;
			}
			*dp++ = *cp;
			ocol++, dcol++;
		}
		cp++;
	}
}
