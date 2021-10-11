# include	<stdio.h>
# include	<sys/types.h>
# include	"../hdr/macros.h"

SCCSID(@(#)what.c 1.1 92/07/30 SMI); /* from System III 6.6 */

char pattern[]  =  "@(#)";
char opattern[]  =  "~|^`";
int sflag;

main(argc,argv)
int argc;
register char **argv;
{
	register int i;
	register FILE *iop;

	if (argc < 2)
		dowhat(stdin);
	else
		for (i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "-s")) {
				sflag++;
				continue;
			}
			if ((iop = fopen(argv[i],"r")) == NULL)
				fprintf(stderr,"can't open %s (26)\n",argv[i]);
			else {
				printf("%s:\n",argv[i]);
				dowhat(iop);
			}
		}
	exit(0);
	/* NOTREACHED */
}


dowhat(iop)
register FILE *iop;
{
	register int c;
	int 	 ret = 0;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0])
			ret = trypat(iop, &pattern[1]);
		else if (c == opattern[0])
			ret = trypat(iop, &opattern[1]);
		if (ret && sflag)
			break;
	}
	fclose(iop);
}


int trypat(iop,pat)
register FILE *iop;
register char *pat;
{
	register int c;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		putchar('\t');
		while ((c = getc(iop)) != EOF && c && !any(c,"\"\\>\n"))
			putchar(c);
		putchar('\n');
		if (sflag)
			return(1);
	}
	else if (c != EOF)
		ungetc(c, iop);
	return(0);
}
