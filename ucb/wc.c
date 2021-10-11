#ifndef lint
static	char sccsid[] = "@(#)wc.c 1.1 92/07/30 SMI"; /* from UCB 4.6 06/01/83 */
#endif
/* wc line and word count */

#include <stdio.h>

#include <ctype.h>
#include <locale.h>

long	linect, wordct, charct;
long	tlinect, twordct, tcharct;
char	*wd = "lwc";

main(argc, argv)
char **argv;
{
	int i, token;
	register FILE *fp;
	register int c;
	int status = 0;
	char *p;

	setlocale (LC_ALL, "");			/* get local environment */
	while (argc > 1 && *argv[1] == '-') {
		switch (argv[1][1]) {
		case 'l': case 'w': case 'c': 
			wd = argv[1]+1;
			break;
		default:
		usage:
			fprintf(stderr, "Usage: wc [-lwc] [files]\n");
			exit(1);
		}
		argc--;
		argv++;
	}

	i = 1;
	fp = stdin;
	do {
		if(argc>1 && (fp=fopen(argv[i], "r")) == NULL) {
			(void) fprintf(stderr, "wc: ");
			perror(argv[i]);
			status = 2;
			continue;
		}
		linect = 0;
		wordct = 0;
		charct = 0;
		token = 0;
		for(;;) {
			c = getc(fp);
			if (c == EOF)
				break;
			charct++;
			if (isgraph(c)) {
				if(!token) {
					wordct++;
					token++;
				}
				continue;
			}
			if(c=='\n') {
				linect++;
			}
			else if(c!=' '&&c!='\t')
				continue;
			token = 0;
		}
		/* print lines, words, chars */
		wcp(wd, charct, wordct, linect);
		if(argc>1) {
			printf(" %s\n", argv[i]);
		} else
			printf("\n");
		fclose(fp);
		tlinect += linect;
		twordct += wordct;
		tcharct += charct;
	} while(++i<argc);
	if(argc > 2) {
		wcp(wd, tcharct, twordct, tlinect);
		printf(" total\n");
	}
	exit(status);
	/* NOTREACHED */
}

wcp(wd, charct, wordct, linect)
register char *wd;
long charct; long wordct; long linect;
{
	while (*wd) switch (*wd++) {
	case 'l':
		ipr(linect);
		break;

	case 'w':
		ipr(wordct);
		break;

	case 'c':
		ipr(charct);
		break;

	}
}

ipr(num)
long num;
{
	printf(" %7ld", num);
}
