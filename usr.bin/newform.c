#ifndef lint
static	char sccsid[] = "@(#)newform.c 1.1 92/07/30 SMI"; /* from S5R2 1.10 */
#endif

/*	FUNCTION PAGE INDEX
Function	Page		Description
append		16	Append chars to end of line.
begtrunc	16	Truncate characters from beginning of line.
center		5	Center text in the work area.
cnvtspec	7	Convert tab spec to tab positions.
endtrunc	16	Truncate chars from end of line.
inputtabs	17	Expand according to input tab specs.
main		3	MAIN
inputn		5	Read a command line option number.
options		4	Process command line options.
outputtabs	19	Contract according to output tab specs.
prepend		16	Prepend chars to line.
process		15	Process one line of input.
readline	14	Read one line from the file.
readspec	12	Read a tabspec from a file.
sstrip		18	Strip SCCS SID char from beginning of line.
sadd		18	Add SCCS SID chars to end of line.
type		14	Determine type of a character.	*/

#include	<stdio.h>

#define	MAXOPTS	50
#define	NCOLS	512
#define	MAXLINE	512
#define	NUMBER	'0'
#define	LINELEN	80

int	tabtbl[500] = {		/* Table containing tab stops		*/
	1,9,17,25,33,41,49,57,65,73,0,
				/* Default tabs				*/
	1,10,16,36,72,0,	/* IBM 370 Assembler			*/
	1,10,16,40,72,0,	/* IBM 370 Assembler (alt.)		*/
	1,8,12,16,20,55,0,	/* COBOL				*/
	1,6,10,14,49,0,		/* COBOL (crunched)			*/
	1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67,0,
				/* COBOL (crunched, many cols.)		*/
	1,7,11,15,19,23,0,	/* FORTRAN				*/
	1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,0,
				/* PL/1					*/
	1,10,55,0,		/* SNOBOL				*/
	1,12,20,44,0 },		/* UNIVAC Assembler			*/

	*nexttab = &tabtbl[87],	/* Pointer to next empty slot		*/

	*spectbl[40] = {	/* Table of pointers into tabtbl	*/
	&tabtbl[0],		/* Default specification		*/
	&tabtbl[11],		/* -a  specification			*/
	&tabtbl[17],		/* -a2 specification			*/
	&tabtbl[23],		/* -c  specification			*/
	&tabtbl[30],		/* -c2 specification			*/
	&tabtbl[36],		/* -c3 specification			*/
	&tabtbl[54],		/* -f  specification			*/
	&tabtbl[61],		/* -p  specification			*/
	&tabtbl[78],		/* -s  specification			*/
	&tabtbl[82] },		/* -u  specification			*/

	savek;		/* Stores char count stripped from front of line. */
	nextspec = 10,		/* Index to next slot			*/
	sitabspec = -1,		/* Index to "standard input" spec.	*/
	effll	= 80,		/* Effective line length		*/
	optionf = 0,		/* 'f' option set			*/
	soption = 0;		/* 's' option used. */
	files	= 0,		/* Number of input files		*/
	kludge	= 0,		/* Kludge to allow reread of 1st line	*/
	okludge = 0,		/* Kludge to indicate reading "o" option*/
	lock	= 0;		/* Lock to prevent file indirection	*/

char	pachar = ' ',		/* Prepend/append character		*/
	work[3*NCOLS+1],	/* Work area				*/
	*pfirst,		/* Pointer to beginning of line 	*/
	*plast,			/* Pointer to end of line		*/
	*wfirst = &work[0],	/* Pointer to beginning of work area	*/
	*wlast  = &work[3*NCOLS], /* Pointer to end of work area	*/
	siline[NCOLS],		/* First standard input line		*/
	savchr[8],		/* Holds char stripped from line start */
	format[80] = "-8",	/* Array to hold format line		*/
	*strcpy();		/* Eliminates a warning by 'lint'. */

struct f {
	char	option;
	int	param;
	}	optl[MAXOPTS],	/* List of command line options 	*/
		*flp = optl;	/* Pointer to next open slot		*/
main(argc,argv)			/* Main procedure	 		*/
int	argc;			/* Count of command line arguments	*/
char	**argv;			/* Array of pointers to arguments	*/
{
	char	*scan;		/* String scan pointer			*/
	FILE	*fp;		/* Pointer to current file		*/

	options(argc,argv);
	if (optionf)		/* Write tab spec format line. */
		{fputs("<:t",stdout);
		fputs(format,stdout);
		fputs(" d:>\n",stdout);
		}
	if (files) {
		while (--argc) {
			scan = *++argv;
			if (*scan != '-') {
				if ((fp = fopen(scan,"r")) == NULL) {
					fprintf(stderr,
					"newform: can't open %s\n",scan);
					exit(1);
					}
				process(fp);
				fclose(fp);
				}
			}
		}
	else {
		process(stdin);
		}
	exit(0);
}


options(argc, argv)		/* Process command line options		*/
register int	argc;		/* Count of arguments			*/
register char	**argv;		/* Array of pointers to arguments	*/
{
	int	n;		/* Temporary number holder		*/
	register char	*scan,	/* Pointer to individual option strings	*/
			c;	/* Option character			*/

	while (--argc > 0) {
		scan = *++argv;
		if (*scan++ == '-') {
			switch (c = *scan++) {
			case 'a':
			case 'b':
			case 'e':
			case 'p':
				flp->option = c;
				flp->param = inputn(scan);
				flp++;
				break;
			case 'c':
				flp->option = 'c';
				flp->param = *scan ? *scan : ' ';
				flp++;
				break;
			case 'f':
				flp->option = 'f';
				optionf++;
				flp++;
				break;
			case 'i':
				flp->option = 'i';
				flp->param = cnvtspec(scan);
				flp++;
				break;
			case 'o':
 				if(*scan=='-' && *(scan+1)=='0' && *(scan+2)=='\0')break;
 		/* Above allows the -o-0 option to be ignored. */
				flp->option = 'o';
				strcpy(format,scan);
				okludge++;
				flp->param = cnvtspec(scan);
				okludge--;
				if(flp->param == 0) strcpy(format,"-8");
				flp++;
				break;
			case 'l':
				flp->option = 'l';
				flp->param = (n = inputn(scan)) ? n : 72;
				flp++;
				break;
			case 's':
				flp->option = 's';
				flp++;
				soption++;
				break;
			default:
				goto usageerr;
				}
			}
		else
			files++;
		}
	return;
usageerr:
	fprintf(stderr,"usage: newform  [-s] [-itabspec] [-otabspec] ");
	fprintf(stderr,"[-pn] [-en] [-an] [-f] [-cchar]\n\t\t");
	fprintf(stderr,"[-ln] [-s] [file ...]\n");
	exit(1);
}
/* _________________________________________________________________ */

int inputn(scan)		/* Read a command option number		*/
register char	*scan;		/* Pointer to string of digits		*/
{
	int	n;		/* Number				*/
	char	c;		/* Character being scanned		*/

	n = 0;
	while ((c = *scan++) >= '0' && c <= '9')
		n = n * 10 + c - '0';
	return(n);
}
/* _________________________________________________________________ */

center()			/* Center the text in the work area.	*/
{
	char	*tfirst,	/* Pointer for moving buffer down	*/
		*tlast,		/* Pointer for moving buffer up		*/
		*tptr;		/* Temporary				*/

	if (plast - pfirst > MAXLINE) {
		fprintf(stderr,"newform: internal line too long\n");
		exit(1);
		}
	if (pfirst < &work[NCOLS]) {
		tlast = plast + (&work[NCOLS] - pfirst);
		tptr = tlast;
		while (plast >= pfirst) *tlast-- = *plast--;
		pfirst = ++tlast;
		plast = tptr;
		}
	else {
		tfirst = &work[NCOLS];
		tptr = tfirst;
		while (pfirst <= plast) *tfirst++ = *pfirst++;
		plast = --tfirst;
		pfirst = tptr;
		}
}
int cnvtspec(p)		/* Convert tab specification to tab positions.	*/
register char	*p;		/* Pointer to spec string.		*/
{
	int	state,		/* DFA state				*/
		spectype,	/* Specification type			*/
		tspec,		/* Tab spec pointer			*/
		number[40],	/* Array of read-in numbers		*/
		tp,		/* Pointer to last number		*/
		ix;		/* Temporary				*/
	char	c,		/* Temporary				*/
		*filep,		/* Pointer to file name			*/
		type(),		/* Function				*/
		*readline();	/* Function				*/
	FILE	*fopen(),	/* File open routine			*/
		*fp;		/* File pointer				*/

	state = 0;
	while (state >= 0) {
		c = *p++;
		switch (state) {
		case 0:
			switch (type(c)) {
			case '\0':
				spectype = 0;
				state = -1;
				break;
			case NUMBER:
				state = 1;
				tp = 0;
				number[tp] = c - '0';
				break;
			case '-':
				state = 3;
				break;
			default:
				goto tabspecerr;
				}
			break;
		case 1:
			switch (type(c)) {
			case '\0':
				spectype = 11;
				state = -1;
				break;
			case NUMBER:
				state = 1;
				number[tp] = number[tp] * 10 + c - '0';
				break;
			case ',':
				state = 2;
				break;
			default:
				goto tabspecerr;
				}
			break;
		case 2:
			if (type(c) == NUMBER) {
				state = 1;
				number[++tp] = c - '0';
				}
			else
				goto tabspecerr;

			break;
		case 3:
			switch (type(c)) {
			case '-':
				state = 4;
				break;
			case 'a':
				state = 5;
				break;
			case 'c':
				state = 7;
				break;
			case 'f':
				state = 10;
				break;
			case 'p':
				state = 11;
				break;
			case 's':
				state = 12;
				break;
			case 'u':
				state = 13;
				break;
			case NUMBER:
				state = 14;
				number[0] = c - '0';
				break;
			default:
				goto tabspecerr;
				}
			break;
		case 4:
			if (c == '\0') {
				spectype = 12;
				state = -1;
				}
			else {
				filep = --p;
				spectype = 13;
				state = -1;
				}
			break;
		case 5:
			if (c == '\0') {
				spectype = 1;
				state = -1;
				}
			else if (c == '2')
				state = 6;
			else
				goto tabspecerr;
			break;
		case 6:
			if (c == '\0') {
				spectype = 2;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 7:
			switch (c) {
			case '\0':
				spectype = 3;
				state = -1;
				break;
			case '2':
				state = 8;
				break;
			case '3':
				state = 9;
				break;
			default:
				goto tabspecerr;
				}
			break;
		case 8:
			if (c == '\0') {
				spectype = 4;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 9:
			if (c == '\0') {
				spectype = 5;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 10:
			if (c == '\0') {
				spectype = 6;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 11:
			if (c == '\0') {
				spectype = 7;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 12:
			if (c == '\0') {
				spectype = 8;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 13:
			if (c == '\0') {
				spectype = 9;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
		case 14:
			if (type(c) == NUMBER) {
				state = 14;
				number[0] = number[0] * 10 + c - '0';
				}
			else if (c == '\0') {
				spectype = 10;
				state = -1;
				}
			else
				goto tabspecerr;
			break;
			}
		}
	if (spectype <= 9) return(spectype);
	if (spectype == 10) {
		spectype = nextspec++;
		spectbl[spectype] = nexttab;
		*nexttab = 1;
	 	if(number[0] == 0)number[0] = 1; /* Prevent infinite loop. */
		while (*nexttab < LINELEN) {
			*(nexttab + 1) = *nexttab;
			*++nexttab += number[0];
			}
		*nexttab++ = '\0';
		return(spectype);
		}
	if (spectype == 11) {
		spectype = nextspec++;
		spectbl[spectype] = nexttab;
		*nexttab++ = 1;
		for (ix = 0; ix <= tp; ix++) {
			*nexttab++ = number[ix];
			if ((number[ix] >= number[ix+1]) && (ix != tp))
				goto tabspecerr;
			}
		*nexttab++ = '\0';
		return(spectype);
		}
	if (lock == 1) {
		fprintf(stderr,"newform: tabspec indirection illegal\n");
		exit(1);
		}
	lock = 1;
	if (spectype == 12) {
		if (sitabspec >= 0) {
			tspec = sitabspec;
			}
		else {
			readline(stdin,siline);
			kludge = 1;
			tspec = readspec(siline);
			sitabspec = tspec;
			}
		}
	if (spectype == 13) {
		if ((fp = fopen(filep,"r")) == NULL) {
			fprintf(stderr,"newform: can't open %s\n", filep);
			exit(1);
			}
		readline(fp,work);
		fclose(fp);
		tspec = readspec(work);
		}
	lock = 0;
	return(tspec);
tabspecerr:
	fprintf(stderr,"newform: tabspec in error\n");
	fprintf(stderr,"tabspec is \t-a\t-a2\t-c\t-c2\t-c3\t-f\t-p\t-s\n");
	fprintf(stderr,"\t\t-u\t--\t--file\t-number\tnumber,..,number\n");
	exit(1);
return(-1);	/* Stops 'lint's complaint about return(e) VS return. */
}
int readspec(p)			/* Read a tabspec from a file		*/

register char	*p;		/* Pointer to buffer to process		*/
{
	int	state,		/* Current state			*/
		firsttime,	/* Flag to indicate spec found		*/
		value;		/* Function value			*/
	char	c,		/* Char being looked at			*/
		*tabspecp,	/* Pointer to spec string		*/
		*restore = " ",	/* Character to be restored		*/
		repch;		/* Character to replace with		*/

	state = 0;
	firsttime = 1;
	while (state >= 0) {
		c = *p++;
		switch (state) {
		case 0:
			state = (c == '<') ? 1 : 0;
			break;
		case 1:
			state = (c == ':') ? 2 : 0;
			break;
		case 2:
			state = (c == 't') ? 4
				: ((c == ' ') || (c == '\t')) ? 2 : 3;
			break;
		case 3:
			state = ((c == ' ') || (c == '\t')) ? 2 : 3;
			break;
		case 4:
			if (firsttime) {
				tabspecp = --p;
				p++;
				firsttime = 0;
				}
			if ((c == ' ') || (c == '\t') || (c == ':')) {
				repch = *(restore = p - 1);
				*restore = '\0';
				}
			state = (c == ':') ? 6
				: ((c == ' ') || (c == '\t')) ? 5 : 4;
			break;
		case 5:
			state = (c == ':') ? 6 : 5;
			break;
		case 6:
			state = (c == '>') ? -2 : 5;
			break;
			}
		if (c == '\n') state = -1;
		}
	if (okludge) strcpy(format,tabspecp);
	value = (state == -1) ? 0 : cnvtspec(tabspecp);
	*restore = repch;
	return(value);
}
char *readline(fp,area)		/* Read one line from the file.	*/
FILE	*fp;			/* File to read from			*/
char	*area;			/* Array of characters to read into	*/
{
	int	c;		/* Current character			*/
	char	*xarea,		/* Temporary pointer to character array	*/
		UNDEF = '\377',		/* Undefined char for return */
		*temp;		/* Array pointer			*/

	xarea = area;
	if (kludge && (fp == stdin)) {
		temp = siline;
		while ((*area++ = *temp++) != '\n') ;
		kludge = 0;
		return(xarea);
		}
	else {
		while ((*area++ = c = getc(fp)) != '\n' && c != EOF) ;
		if (c == EOF) {
			if (--area == xarea)return(NULL);
			else {
				*area = '\n';
				return(xarea);
				}
			}
		}
return(&UNDEF);	/* Stops 'lint's complaint about return(e) VS return. */
}
/* _________________________________________________________________ */

char type(c)			/* Determine type of a character	*/
char c;				/* Character to check			*/
{
	return((c >= '0') && (c <= '9') ? NUMBER : c);
}
process(fp)			/* Process one line of input		*/
FILE	*fp;			/* File pointer for current input	*/
{
	struct	f	*lp;	/* Pointer to structs			*/
	char	*readline();	/* Function				*/
	char	chrnow;		/* For int to char conversion. */

	while (readline(fp,&work[NCOLS]) != NULL) {
		effll = 80;
		pachar = ' ';
		pfirst = plast = &work[NCOLS];
		while (*plast != '\n') plast++;
		for (lp = optl ; lp < flp ; lp++) {
			switch (lp->option) {
			case 'a':
				append(lp->param);
				break;
			case 'b':
				begtrunc(lp->param);
				break;
			case 'c':
				chrnow = lp->param;
				pachar = chrnow ? chrnow : ' ';
				break;
			case 'e':
				endtrunc(lp->param);
				break;
			case 'f':
				/* Ignored */
				break;
			case 'i':
				inputtabs(lp->param);
				break;
			case 'l':	/* New eff line length */
				effll = lp->param ? lp->param : 72;
				break;
			case 's':
				sstrip();
				break;
			case 'o':
				outputtabs(lp->param);
				break;
			case 'p':
				prepend(lp->param);
				break;
				}
			}
		if(soption)sadd();
		*++plast = '\0';
		fputs(pfirst,stdout);
		}
}
append(n)			/* Append characters to end of line.	*/
int	n;			/* Number of characters to append.	*/
{
	if (plast - pfirst < effll) {
		n = n ? n : effll - (plast - pfirst);
		if (plast + n > wlast) center();
		while (n--) *plast++ = pachar;
		*plast = '\n';
		}
}
/* _________________________________________________________________ */

prepend(n)			/* Prepend characters to line.		*/
int	n;			/* Number of characters to prepend.	*/
{
	if (plast - pfirst < effll) {
		n = n ? n : effll - (plast - pfirst);
		if (pfirst - n < wfirst) center();
		while (n--) *--pfirst = pachar;
		}
}
/* _________________________________________________________________ */

begtrunc(n)		/* Truncate characters from beginning of line.	*/
int	n;			/* Number of characters to truncate.	*/
{
	if (plast - pfirst > effll) {
		n = n ? n : plast - pfirst - effll;
		pfirst += n;
		if (pfirst >= plast)
			*(pfirst = plast = &work[NCOLS]) = '\n';
		}
}
/* _________________________________________________________________ */

endtrunc(n)			/* Truncate characters from end of line.*/
int	n;			/* Number of characters to truncate.	*/
{
	if (plast - pfirst > effll) {
		n = n ? n : plast - pfirst - effll;
		plast -= n;
		if (pfirst >= plast)
			*(pfirst = plast = &work[NCOLS]) = '\n';
		else
			*plast = '\n';
		}
}
inputtabs(p)		/* Expand according to input tab specifications.*/
int	p;			/* Pointer to tab specification.	*/
{
	int	*tabs;		/* Pointer to tabs			*/
	char	*tfirst,	/* Pointer to new buffer start		*/
		*tlast;		/* Pointer to new buffer end		*/
	register char c;	/* Character being scanned		*/
	int	logcol;		/* Logical column			*/

	tabs = spectbl[p];
	tfirst = tlast = work;
	logcol = 1;
	center();
	while (pfirst <= plast) {
		if (logcol >= *tabs) tabs++;
		switch (c = *pfirst++) {
		case '\b':
			if (logcol > 1) logcol--;
			*tlast++ = c;
			if (logcol < *tabs) tabs--;
			break;
		case '\t':
			while (logcol < *tabs) {
				*tlast++ = ' ';
				logcol++;
				}
			tabs++;
			break;
		default:
			*tlast++ = c;
			logcol++;
			break;
			}
		}
	pfirst = tfirst;
	plast = --tlast;
}
/* Add SCCS SID (generated by a "get -m" command) to the end of each line.
Sequence is as follows for EACH line:
	Check for at least 1 tab.  Err if none.
	Strip off all char up to & including first tab.
	If more than 8 char were stripped, the 8 th is replaced by
		a '*' & the remainder are discarded.
	Unless user specified an "a", append blanks to fill
		out line to eff. line length (default= 72 char).
	Truncate lines > eff. line length (default=72).
	Add stripped char to end of line.	*/
sstrip()
{
	register int i, k;
	char *c;

	k = -1;
	c = pfirst;
	while(*c != '\t' && *c != '\n'){k++; c++;}
	if(*c != '\t'){fprintf(stderr,"not -s format\r\n"); exit(1);}

	c = pfirst;
	savek = (k > 7) ? 6 : k;
	for(i=0; i <= savek; i++)savchr[i] = *c++;	/* Tab not saved */
	if(k > 7)savchr[7] = '*';

	pfirst = ++c;		/* Point pfirst to char after tab */
}
/* ================================================================= */

sadd()
{
	register int i;

	for(i=0; i <= savek; i++)*plast++ = savchr[i];
	*plast = '\n';
}
outputtabs(p)	/* Contract according to output tab specifications.	*/
int	p;			/* Pointer to tab specification.	*/
{
	int	*tabs;		/* Pointer to tabs			*/
	char	*tfirst,	/* Pointer to new buffer start		*/
		*tlast,		/* Pointer to new buffer end		*/
		*mark;		/* Marker pointer			*/
	register char c;	/* Character being scanned		*/
	int	logcol;		/* Logical column			*/

	tabs = spectbl[p];
	tfirst = tlast = pfirst;
	logcol = 1;
	while (pfirst <= plast) {
		if (logcol == *tabs) tabs++;
		switch (c = *pfirst++) {
		case '\b':
			if (logcol > 1) logcol--;
			*tlast++ = c;
			if (logcol < *tabs) tabs--;
			break;
		case ' ':
			mark = tlast;
			do {
				*tlast++ = ' ';
				logcol++;
				if (logcol == *tabs) {
					*mark++ = '\t';
					tlast = mark;
					tabs++;
					}
				} while (*pfirst++ == ' ');
			pfirst--;
			break;
		default:
			logcol++;
			*tlast++ = c;
			break;
			}
		}
	pfirst = tfirst;
	plast = --tlast;
}
