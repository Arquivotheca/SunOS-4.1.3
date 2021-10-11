#ifndef lint
static  char sccsid[] = "@(#)sdiff.c 1.1 92/08/05 Copyr 1986 Sun Micro";/* from S5R2 1.2 */
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define	LMAX	1024
#define	BMAX	1024
#define STDOUT 1
#define WGUTTER 6
#define WLEN	(WGUTTER * 2 + WGUTTER + 2)
#define PROMPT '%'
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
/* XXX */
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include "diff.h"

static char	GUTTER[WGUTTER] = "     ";
static char	diffcmd[BMAX];

/* XXX potential trouble spot here */
static int	llen	= 2000;	/* Default maximum line length written out */
static int	hlen;		/* Half line length with space for gutter */
static int	len1;		/* Calculated length of left side */
static int	nchars;		/* Number of characters in left side - used for tab expansion */
static char	change = ' ';
static int	midflg = 0;	/* set after middle was output */
static int	rcode = 0;	/* return code */

static char *pgmname;

static FILE	*fdes1;
static char	buf1[BMAX+1];

static FILE	*fdes2;
static char	buf2[BMAX+1];

static FILE	*diffdes;
static char	diffbuf[LMAX+1];

int	linecnt;

		/* decoded diff cmd- left side from to; right side from, to */
static int from1, to1, from2, to2;
static int num1, num2;		/*line count for left side file and right */

FILE *popen();
char *filename();
char *malloc();

/* returns true if OK, 0 otherwise */

sdiff(file1, file2, blanks)
	char *file1, *file2;
{
	int com;
	register int n1, n2, n;

	linecnt = 0;
	pgmname = "filemerge";
	hlen = (llen - WGUTTER +1)/2;

	if((fdes1 = fopen(file1,"r")) == NULL) {
		if (file1[0] == 0)
			error("Must specify name for File1", 0);
		else
			error("Cannot open: %s.",file1);
		return(0);
	}

	if((fdes2 = fopen(file2,"r")) == NULL) {
		if (file2[0] == 0)
			error("Must specify name for File2", 0);
		else
			error("Cannot open: %s.", file2);
		fclose(fdes1);
		return(0);
	}

	/* Call DIFF command */
	strcpy(diffcmd,"diff ");
	if (blanks)
		strcat(diffcmd, "-b ");
	strcat(diffcmd,file1);
	strcat(diffcmd," ");
	strcat(diffcmd,file2);
	diffdes = popen(diffcmd,"r");
	if (diffdes == NULL) {
		fprintf(stderr, "filemerge: can't popen\n");
		exit(1);
	}

	num1 = num2 = 0;

	/* Read in diff output and decode commands
	*  "change" is used to determine character to put in gutter
	*  num1 and num2 counts the number of lines in file1 and 2
	*/

	n = 0;
	while(fgets(diffbuf,LMAX,diffdes) != NULL){
		change = ' ';
		com = cmd(diffbuf);

	/* handles all diff output that is not cmd
	   lines starting with <, >, ., --- */
		if(com == 0)
			continue;

	/* Catch up to from1 and from2 */
		rcode = 1;
		n1=from1-num1;
		n2=from2-num2;
		n= n1>n2?n2:n1;
		if(com =='c' && n!=0)
			n--;
		while(n--){
			put1();
			put2();
			putnewline();
			midflg = 0;
		}

	/* Process diff cmd */
		switch(com){

		case 'a':
			change = '>';
			while(num2<to2){
				put2();
				putnewlineleftpad();
				midflg = 0;
			}
			break;

		case 'd':
			change = '<';
			while(num1<to1){
				put1();
				putnewlinerightpad();
				midflg = 0;
			}
			break;

		case 'c':
			n1 = to1-from1;
			n2 = to2-from2;
			n = n1>n2?n2:n1;
			change = '|';
			do {
				put1();
				put2();
				putnewline();
				midflg = 0;
			} while(n--);

			change = '<';
			while(num1<to1){
				put1();
				putnewlinerightpad();
				midflg = 0;
			}

			change = '>';
			while(num2<to2){
				put2();
				putnewlineleftpad();
				midflg = 0;
			}
			break;

		default:
			fprintf(stderr,
"\nfilemerge: trouble performing diff.  Perhaps you are diffing a binary file?\n");
			break;
		}

	}
	/* put out remainder of input files */
	while(put1()){
		put2();
		putnewline();
		midflg = 0;
	}
	pclose(diffdes);
	fclose(fdes1);
	fclose(fdes2);
	return(1);
}

put1()
{
	/* len1 = length of left side */
	/* nchars = num of chars including tabs */


	if(fgets(buf1,BMAX,fdes1) != NULL){
		len1 = getlen(0,buf1);
		if(len1 != 0)
			puttext1(buf1,nchars);
		
		if(change != ' ')
			putmid();
		num1++;
		return(1);
	} else 
		return(0);
}

put2()
{
	if(fgets(buf2,BMAX,fdes2) != NULL){
		getlen((hlen+WGUTTER)%8,buf2);

		/* if the left and right are different they are always
		   printed.
		   If the left and right are identical
		   right is only printed if leftonly is not specified
		   or silent mode is not specified 
		   or the right contains other than white space (len1 !=0)
		*/
		if(change != ' '){
		
		/* put right side to right temp file only
		   because left side was written to output for 
		   identical lines */
			
			if(midflg == 0)
				putmid();
			puttext2(buf2,nchars);
		} else if(len1!=0) {
			if(midflg == 0)
				putmid();
			puttext2(buf2,nchars);
		}
		num2++;
		len1 = 0;
		return(1);
	} else {
		len1 = 0;
		return(0);
	}
}

cmd(start)
char *start;
{

	char *cp, *cps;
	int com;

	if(*start == '>' || *start == '<' || *start == '-' || *start == '.')
		return(0);

	cp = cps = start;
	while(isdigit(*cp))
		cp++;
	from1 = atoi(cps);
	to1 = from1;
	if(*cp == ','){
		cp++;
		cps = cp;
		while(isdigit(*cp))
			cp++;
		to1 = atoi(cps);
	}

	com = *cp++;
	cps = cp;

	while(isdigit(*cp))
		cp++;
	from2 = atoi(cps);
	to2 = from2;
	if(*cp == ','){
		cp++;
		cps = cp;
		while(isdigit(*cp))
			cp++;
		to2 = atoi(cps);
	}
	return(com);
}

getlen(startpos,buffer)
char *buffer;
int startpos;
{
	/* get the length of the string in buffer
	*  expand tabs to next multiple of 8
	*/

	register char *cp;
	register int slen, tlen;
	int notspace;

	nchars = 0;
	notspace = 0;
	tlen = startpos;
	for(cp=buffer; *cp != '\n'; cp++){
		if(*cp == '\t'){
			slen = tlen;
			tlen += 8 - (tlen%8);
			if(tlen>=hlen) {
				tlen= slen;
				break;
			}
			nchars++;
		}else{
			if(tlen>=hlen)break;
			if(!isspace(*cp))
				notspace = 1;
			tlen++;
			nchars++;
		}
	}
	return(notspace ? tlen : 0);
}

putmid()
{
	/* len1 set by getlen to the possibly truncated
	*  length of left side
	*  hlen is length of half line
	*/

	midflg = 1;
	GUTTER[2] = change;
	/*putline(stdout,GUTTER,5); */
	if (change != ' ')
		addglyph(change, linecnt);
}

putnewline()
{
	linecnt++;
	puttext1("\n", 1);
	puttext2("\n", 1);
}

/*
 *    padding blanks on left are added during first pass
 *    padding blanks on right are recorded during second pass
 *    and actually added during the final merge.  Padding blanks
 *    on right come from 2 sources, additions made in the first pass
 *    and deletions made during the second pass.
 *
 */

/* extra represents padding in ancestor */
static	int rextra[MAXGAP];		/* line added on right */
static	int lextra[MAXGAP];		/* line added on left */
static	rind, lind;

resetextra()
{
	bzero(rextra, sizeof(rextra));
	bzero(lextra, sizeof(lextra));
	rind = lind = 0;
}

/*
 * If a blank was inserted into file1 solely to match up extra lines
 * in file2, then don't put those lines into file3
 */
putnewlineleftpad()
{
	static	shown;
	
	linecnt++;
	puttext1leftpad("\n", 1);
	puttext2("\n", 1);
	if (!firstpass) {
		if (rind >= MAXGAP) {
			if (!shown)
				fprintf(stderr, "filemerge: too many gaps\n");
			shown = 1;
			return;
		}
		shown = 0;
		rextra[rind++] = linecnt;
	}
}

/*
 * Keep track of lines added in left hand side
 */
putnewlinerightpad()
{
	static	shown;
	
	linecnt++;
	if (commonfile) {
		puttext1rightpad("\n", 1);
		if (firstpass) {
			if (lind >= MAXGAP) {
				if (!shown)
					fprintf(stderr,
					    "filemerge: too many gaps\n");
				shown = 1;
				return;
			}
			shown = 0;
			lextra[lind++] = linecnt;
		}
	}
	else
		puttext1("\n", 1);
	puttext2("\n", 1);
}

/* fp1 and fp2 are infiles, tmp3 and tmp4 are outfiles */
/* also, update glyph positions */
combine(fp1, fp2, tmp3, tmp4)
	FILE	*fp1, *fp2;
	char	*tmp3, *tmp4;
{
	FILE	*fp3, *fp4;
	register char	*p1, *p2;
	int	left, right, line, gl, l, r;
	char	buf1[1024], buf2[1024];
	int	done = 0;
	
	truncate(tmp3, 0);
	fp3 = fopen(tmp3, "w");
	if (fp3 == NULL) {
		fprintf(stderr, "filemerge: can't open %s\n", tmp3);
		exit(1);
	}
	truncate(tmp4, 0);
	fp4 = fopen(tmp4, "w");
	if (fp4 == NULL) {
		fprintf(stderr, "filemerge: can't open %s\n", tmp4);
		exit(1);
	}
	left = 0;
	right = 0;
	l = r = 0;
	line = -1;
	p1 = buf1;
	p2 = buf2;
	glyphind = 0;
	while (1) {
		p1 = fgets(p1, sizeof(buf1), fp1);
		p2 = fgets(p2, sizeof(buf2), fp2);
		if (p1 == NULL && p2 == NULL)
			break;
		while (rextra[r] == right+1) {
			shiftgap(line);
			r++;
			right++;
			line++;
			glyphlist[glyphind].g_lineno = line;
			glyphlist[glyphind].g_lchar = 0;
			glyphlist[glyphind++].g_rchar = '+';
			fputs("\n", fp3);
			fputs(p2, fp4);
			/* printf("%30.30s     + %30.30s\n", "", p2); */
			if (fgets(p2, sizeof(buf2), fp2) == NULL) {
				done = 1;
				break;
			}
		}
		while (lextra[l] == left+1) {
			l++;
			left++;
			line++;
			glyphlist[glyphind].g_lineno = line;
			glyphlist[glyphind].g_lchar = '+';
			glyphlist[glyphind++].g_rchar = 0;
			fputs(p1, fp3);
			fputs("\n", fp4);
			/* printf("%30.30s +     %30.30s\n", p1, ""); */
			if (fgets(p1, sizeof(buf1), fp1) == NULL) {
				done = 1;
				break;
			}
		}
		/* printf("%30.30s %c    %c %30.30s\n",
		    p1, lglph[left], rglph[right], p2); */
		if (done)
			goto out;
		fputs(p1, fp3);
		fputs(p2, fp4);
		line++;
		if ((gl = rglyph(right)) || lglyph(left)) {
			glyphlist[glyphind].g_lineno = line;
			glyphlist[glyphind].g_lchar = lglyph(left);
			glyphlist[glyphind++].g_rchar = gl;
		}
		left++;
		right++;
		if (glyphind >= MAXGLYPHS) {
			fprintf(stderr, "filemerge: too many differences\n");
			break;
		}
	}
  out:
	fclose(fp3);
	fclose(fp4);
	return(line+1);
}
