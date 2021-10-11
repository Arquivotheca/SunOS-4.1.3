/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)vwidth.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/15/85 */
#endif not lint

/*
 *	Creates a width table for troff from a versatec font for a
 *		normal or special font.
 *	Usage: vwidth [-special] font [ point_size ]
 *		where -special indicates it is a special font,
 *		font is the file name of the versatec font, and
 *		point_size is its point size.
 *	If the point size is omitted it is taken from the suffix of
 *	the font name, as bocklin.14 => 14 point.
 *	It is better to use as large a point size font as possible 
 *	to avoid round off.
 */
#include <stdio.h>

struct swtable {
	char	charloc;
	char	*name;
} swtable[] = {
	'\200',	"null",
	'\200',	"null",
	'"',	"\"",
	'#',	"#",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'<',	"<",
	'\200',	"null",
	'>',	">",
	'\200',	"null",
	'@',	"@",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\\',	"\\",
	'\200',	"null",
	'^',	"^",
	'_',	"_",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'{',	"{",
	'\200',	"null",
	'}',	"}",
	'~',	"~",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'8',	"section",
	'\200',	"null",
	'`',	"acute accent",
	'\'',	"grave accent",
	'_',	"underrule",
	'\200',	"slash (missing)",
	'\200',	"null",
	'\200',	"null",
	'a',	"alpha",
	'b',	"beta",
	'c',	"gamma",
	'd',	"delta",
	'e',	"epsilon",
	'f',	"zeta",
	'g',	"eta",
	'h',	"theta",
	'i',	"iota",
	'j',	"kappa",
	'k',	"lambda",
	'l',	"mu",
	'm',	"nu",
	'n',	"xi",
	'o',	"omicron",
	'p',	"pi",
	'q',	"rho",
	'r',	"sigma",
	's',	"tau",
	't',	"upsilon",
	'u',	"phi",
	'v',	"chi",
	'w',	"psi",
	'x',	"omega",
	'C',	"Gamma",
	'D',	"Delta",
	'H',	"Theta",
	'K',	"Lambda",
	'N',	"Xi",
	'P',	"Pi",
	'R',	"Sigma",
	'\200',	"null",
	'T',	"Upsilon",
	'U',	"Phi",
	'W',	"Psi",
	'X',	"Omega",
	'\013',	"square root",
	'\014',	"terminal sigma",
	'\006',	"root en",
	'\012',	">=",
	'\011',	"<=",
	'0',	"identically equal",
	'-',	"minus",
	'1',	"approx =",
	'2',	"approximates",
	'3',	"not equal",
	'5',	"right arrow",
	'4',	"left arrow",
	'6',	"up arrow",
	'7',	"down arrow",
	'=',	"equal",
	'*',	"multiply",
	'/',	"divide",
	'\010',	"plus-minus",
	'\005',	"cup (union)",
	'\034',	"cap (intersection)",
	'\032',	"subset of",
	'\033',	"superset of",
	'[',	"improper subset",
	'\002',	"improper superset",
	'\001',	"infinity",
	'y',	"partial derivative",
	'(',	"gradient",
	'\035',	"not",
	'\015',	"integral sign",
	'\003',	"proportional to",
	'z',	"empty set",
	'\037',	"member of",
	'+',	"plus",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null", 	/*box vert rule (was 2.)*/
	'\200',	"null",
	'Y',	"dbl dagger",
	'\004',	"right hand",
	'\036',	"left hand",
	'9',	"math *",
	'\007',	"bell system sign",
	'Z',	"or",
	']',	"circle",
	'\024',	"left top (of big curly)",
	'\025',	"left bottom",
	'\026',	"right top",
	'\027',	"right bot",
	'\030',	"left center of big curly bracket",
	'\031',	"right center of big curly bracket",
	'\017',	"bold vertical",
	'\020',	"left floor (lb of bracket)",
	'\021',	"right floor (rb of bracket)",
	'\022',	"left ceiling (lt of bracket)",
	'\023',	"right ceiling (rt of bracket)",
	0,	0
};

struct wtable {
	char	charloc;
	char	*name;
} wtable[] = {
	'\214',	"space",
	'!',	"!",
	'"',	"\"",
	'#',	"#",
	'$',	"$",
	'%',	"%",
	'&',	"&",
	'\'',	"'",
	'(',	"(",
	')',	")",
	'*',	"*",
	'+',	"+",
	',',	",",
	'-',	"- hyphen",
	'.',	".",
	'/',	"/",
	'0',	"0",
	'1',	"1",
	'2',	"2",
	'3',	"3",
	'4',	"4",
	'5',	"5",
	'6',	"6",
	'7',	"7",
	'8',	"8",
	'9',	"9",
	':',	":",
	';',	";",
	'<',	"<",
	'=',	"=",
	'>',	">",
	'?',	"?",
	'@',	"@",
	'A',	"A",
	'B',	"B",
	'C',	"C",
	'D',	"D",
	'E',	"E",
	'F',	"F",
	'G',	"G",
	'H',	"H",
	'I',	"I",
	'J',	"J",
	'K',	"K",
	'L',	"L",
	'M',	"M",
	'N',	"N",
	'O',	"O",
	'P',	"P",
	'Q',	"Q",
	'R',	"R",
	'S',	"S",
	'T',	"T",
	'U',	"U",
	'V',	"V",
	'W',	"W",
	'X',	"X",
	'Y',	"Y",
	'Z',	"Z",
	'[',	"[",
	'\\',	"\\",
	']',	"]",
	'^',	"^",
	'_',	"_",
	'\`',	"\`",
	'a',	"a",
	'b',	"b",
	'c',	"c",
	'd',	"d",
	'e',	"e",
	'f',	"f",
	'g',	"g",
	'h',	"h",
	'i',	"i",
	'j',	"j",
	'k',	"k",
	'l',	"l",
	'm',	"m",
	'n',	"n",
	'o',	"o",
	'p',	"p",
	'q',	"q",
	'r',	"r",
	's',	"s",
	't',	"t",
	'u',	"u",
	'v',	"v",
	'w',	"w",
	'x',	"x",
	'y',	"y",
	'z',	"z",
	'{',	"{",
	'|',	"|",
	'}',	"}",
	'~',	"~",
	'\206',	"narrow space",
	'-',	"hyphen",
	'\07',	"bullet",
	'\010',	"square",
	'\06',	"3/4 em dash",
	'\05',	"rule",
	'\021',	"1/4",
	'\022',	"1/2",
	'\023',	"3/4",
	'\04',	"minus",
	'\01',	"fi",
	'\02',	"fl",
	'\03',	"ff",
	'\011',	"ffi",
	'\012',	"ffl",
	'\013',	"degree",
	'\014',	"dagger",
	'\200',	"section (unimplem)",
	'\015',	"foot mark",
	'\200',	"acute acc (unimplem)",
	'\200',	"grave acc (unimplem)",
	'\200',	"underrule (unimplem)",
	'\200',	"slash (unimplem)",
	'\203',	"half narrow space",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\200',	"null",
	'\017',	"registered",
	'\016',	"copyright",
	'\200',	"null",
	'\020',	"cent",
	0,	0
};

struct desc {
	short	addr;
	short	nbytes;
	char	up;
	char	down;
	char	left;
	char	right;
	short	width;
} desc[256];

int special;
char file[100] = "/usr/lib/vfont/";
char *rindex();

main(argc, argv)
	int argc;
	char *argv[];
{
	register int cl;
	register esc;
	register w;
	int i, psize;
	int fd, high;

	if (argc < 2) {
		printf("Usage: vwidth [-special] font [pointsize] > font.c\n");
		exit(1);
	}
	if (strcmp(argv[1], "-special") == 0) {
		special++;
		argc--;
		argv++;
	}
	if ((fd = open(argv[1], 0)) < 0) {
		strcat(file, argv[1]);
		if ((fd = open(file, 0)) < 0) {
			perror(argv[1]);
			exit(1);
		}
	}
	if (argc == 3)
		psize = atoi(argv[2]);
	else {
		char *p;
		if (((p = rindex(argv[1], '.')) == 0) ||
		    (psize = atoi(p+1)) <= 0) {
			psize = 10;
			fprintf(stderr, "Assuming %d point\n", psize);
		}
	}
	lseek(fd, 10L, 0);
	read(fd, desc, sizeof desc);
	high = desc['a'].up+1;
	printf("char XXw[256-32] = {\n");
	for (i = 0; (special?swtable[i].charloc:wtable[i].charloc) != 0; i++) {
		cl = (special?swtable[i].charloc:wtable[i].charloc) & 0377;
		if (cl & 0200)
			w = cl & 0177;
		else
			w = desc[cl].width*(54./25.)*(6./psize)+.5;
		esc = 0;
		if ((cl >= '0' && cl <= '9') || (cl >= 'A' && cl <= 'Z') ||
		    (cl >= 'a' && cl <= 'z')) {
			if (desc[cl].up > high)
				esc |= 0200;
			if (desc[cl].down > 0)
				esc |= 0100;
		}
		if (esc)
			printf("%d+0%o,\t/* %s */\n", w, esc,
				special ? swtable[i].name : wtable[i].name);
		else
			printf("%d,\t\t/* %s */\n", w,
				special ? swtable[i].name : wtable[i].name);
	}
	printf("};\n");
	exit(0);
	/* NOTREACHED */
}
