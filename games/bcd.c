#ifndef lint
static	char sccsid[] = "@(#)bcd.c 1.1 92/07/30 SMI"; /* from UCB 4.1 82/10/24 */
#endif

#include <stdio.h>
#include <ctype.h>


#define	CARDLEN		48	/* number of columns in the card */

int chtab[] = {
00000, /*   */
03004, /* ! */
02404, /* " */
02040, /* sharp */
02042, /* $ */
02104, /* % */
00001, /* & */
03002, /* ' */
02201, /* ( */
02202, /* ) */
02102, /* * */
00005, /* + */
02044, /* , */
00002, /* - */
02041, /* . */
00014, /* / */
00004, /* 0 */
00010, /* 1 */
00020, /* 2 */
00040, /* 3 */
00100, /* 4 */
00200, /* 5 */
00400, /* 6 */
01000, /* 7 */
02000, /* 8 */
04000, /* 9 */
02200, /* : */
02402, /* ; */
02401, /* < */
02204, /* = */
02400, /* > */
03000, /* ? */
02100, /* at */
 011,
 021,
 041,
0101,
0201,
0401,
01001,
02001,
04001,
012,
022,
042,
0102,
0202,
0402,
01002,
02002,
02002,
024,
044,
0104,
0204,
0404,
01004,
02004,
04004,
02020, /* [ */
03001, /* \ */
02101, /* ] */
00006, /* ^ */
02024 /* _ */
};
	char s[128];
	char *sp = {&s[0]};
main(argc, argv)
char *argv[];
{
	char *spp;
	int i;
	int j;
	int c;
	int l;

	if (argc<2) {
		fputs("$ ", stdout);
		while ((c=getchar())!='\0'&&c!='\n')
			*sp++ = c;
		*sp = 0;
		sp = &s[0];
	} else
		sp = *++argv;
	fputs("\n\n\n\n", stdout);
	putchar(' ');
	for(i=0;i<CARDLEN;i++)
		putchar('_');
	putchar('\n');
	putchar('/');
	spp = sp;
	l = 0;
	while(c = *spp) {
		if (islower(c)) c = toupper(c);
		*spp++ = c;
		if(l<CARDLEN) putchar(c);
		l++;
	}
	i = CARDLEN + 1 - l;
	while(--i>0) putchar(' ');
	fputs("|\n", stdout);
	j = 0;
	spp = sp;
	while (j++<12) {
		putchar('|');
		i = 0;
		spp = sp;
		while (i<CARDLEN) {
			if(i>l) c = 0;
			else c = *spp++ - 040;
			i++;
			if (c<0 || c>137) c = 0;
			if ((chtab[c]>>(j-1))&1) 
				fputs("[\b]", stdout);
			else
				putchar(j>3?'0'+j-3:' ');
		}
		fputs("|\n", stdout);
	}
	putchar('|');
	for(i=0;i<CARDLEN;i++)
		putchar('_');
	fputs("|\n", stdout);
	fputs("\n\n\n\n", stdout);

	exit(0);
	/* NOTREACHED */
}
