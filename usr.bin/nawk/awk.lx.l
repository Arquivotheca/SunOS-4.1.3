%{
/*	Copyright (c) 1984 AT&T	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#ifndef lint
static	char sccsid[] = "@(#)awk.lx.l 1.1 92/07/30 SMI"; /* from S5R3.1 2.5 */
#endif
%}

%Start A str sc reg comment

%{
#include	"y.tab.h"
#include	"awk.h"

#undef	input	/* defeat lex */
#undef	unput

extern int	yylval;
extern int	infunc;

int	lineno	= 1;
int	bracecnt = 0;
int	brackcnt  = 0;
int	parencnt = 0;
#define DEBUG
#ifdef	DEBUG
#	define	RET(x)	{if(dbg)printf("lex %s [%s]\n", tokname(x), yytext); return(x); }
#else
#	define	RET(x)	return(x)
#endif
#define	CADD	cbuf[clen++]=yytext[0]; if (clen>=CBUFLEN) {yyerror("string/reg expr %.10s... too long", cbuf); BEGIN A;}
#define	CBUFLEN	401		/* one more for null chr at end */
uchar	cbuf[CBUFLEN], *s;
int	clen, cflag;
%}

A	[a-zA-Z_]
B	[a-zA-Z0-9_]
D	[0-9]
WS	[ \t]

%%
	switch (yybgin-yysvec-1) {	/* witchcraft */
	case 0:
		BEGIN A;
		break;
	case sc:
		BEGIN A;
		RET('}');
	}

<A>\n		{ lineno++; RET(NL); }
<A>#.*		{ lineno++; RET(NL); }	/* strip comment lines */
<A>{WS}+	{ ; }
<A>;		{ RET(';'); }

<A>"\\"\n	{ lineno++; }
<A>BEGIN	{ RET(XBEGIN); }
<A>END		{ RET(XEND); }
<A>func(tion)?	{ if (infunc) yyerror("illegal nested function"); RET(FUNC); }
<A>return	{ if (!infunc) yyerror("return not in function"); RET(RETURN); }
<A>"&&"		{ RET(AND); }
<A>"||"		{ RET(BOR); }
<A>"!"		{ RET(NOT); }
<A>"!="		{ yylval = NE; RET(NE); }
<A>"~"		{ yylval = MATCH; RET(MATCHOP); }
<A>"!~"		{ yylval = NOTMATCH; RET(MATCHOP); }
<A>"<"		{ yylval = LT; RET(LT); }
<A>"<="		{ yylval = LE; RET(LE); }
<A>"=="		{ yylval = EQ; RET(EQ); }
<A>">="		{ yylval = GE; RET(GE); }
<A>">"		{ yylval = GT; RET(GT); }
<A>">>"		{ yylval = APPEND; RET(APPEND); }
<A>"++"		{ yylval = INCR; RET(INCR); }
<A>"--"		{ yylval = DECR; RET(DECR); }
<A>"+="		{ yylval = ADDEQ; RET(ASGNOP); }
<A>"-="		{ yylval = SUBEQ; RET(ASGNOP); }
<A>"*="		{ yylval = MULTEQ; RET(ASGNOP); }
<A>"/="		{ yylval = DIVEQ; RET(ASGNOP); }
<A>"%="		{ yylval = MODEQ; RET(ASGNOP); }
<A>"^="		{ yylval = POWEQ; RET(ASGNOP); }
<A>"**="	{ yylval = POWEQ; RET(ASGNOP); }
<A>"="		{ yylval = ASSIGN; RET(ASGNOP); }
<A>"**"		{ RET(POWER); }
<A>"^"		{ RET(POWER); }

<A>"$"{D}+	{ yylval = (int) fieldadr(atoi(yytext+1)); RET(FIELD); }
<A>"$"{WS}*	{ RET(INDIRECT); }
<A>NF		{ yylval = (int)setsymtab(yytext, "", 0.0, NUM, symtab); RET(VARNF); }

<A>({D}+("."?){D}*|"."{D}+)((e|E)("+"|-)?{D}+)?	{
		  yylval = (int)setsymtab(yytext, "", atof(yytext), CON|NUM, symtab);
		  RET(NUMBER); }

<A>while	{ RET(WHILE); }
<A>for		{ RET(FOR); }
<A>do		{ RET(DO); }
<A>if		{ RET(IF); }
<A>else		{ RET(ELSE); }
<A>next		{ RET(NEXT); }
<A>exit		{ RET(EXIT); }
<A>break	{ RET(BREAK); }
<A>continue	{ RET(CONTINUE); }
<A>print	{ yylval = PRINT; RET(PRINT); }
<A>printf	{ yylval = PRINTF; RET(PRINTF); }
<A>sprintf	{ yylval = SPRINTF; RET(SPRINTF); }
<A>split	{ yylval = SPLIT; RET(SPLIT); }
<A>substr	{ RET(SUBSTR); }
<A>sub		{ yylval = SUB; RET(SUB); }
<A>gsub		{ yylval = GSUB; RET(GSUB); }
<A>index	{ RET(INDEX); }
<A>match	{ RET(MATCHFCN); }
<A>in		{ RET(IN); }
<A>getline	{ RET(GETLINE); }
<A>close	{ RET(CLOSE); }
<A>delete	{ RET(DELETE); }
<A>length	{ yylval = FLENGTH; RET(BLTIN); }
<A>log		{ yylval = FLOG; RET(BLTIN); }
<A>int		{ yylval = FINT; RET(BLTIN); }
<A>exp		{ yylval = FEXP; RET(BLTIN); }
<A>sqrt		{ yylval = FSQRT; RET(BLTIN); }
<A>sin		{ yylval = FSIN; RET(BLTIN); }
<A>cos		{ yylval = FCOS; RET(BLTIN); }
<A>atan2	{ yylval = FATAN; RET(BLTIN); }
<A>system	{ yylval = FSYSTEM; RET(BLTIN); }
<A>rand		{ yylval = FRAND; RET(BLTIN); }
<A>srand	{ yylval = FSRAND; RET(BLTIN); }

<A>{A}{B}*	{ int n, c;
		  c = input(); unput(c);	/* look for '(' */
		  if (c != '(' && infunc && (n=isarg(yytext)) >= 0) {
			yylval = n;
			RET(ARG);
		  } else {
			yylval = (int)setsymtab(yytext,"",0.0,STR|NUM,symtab);
			if (c == '(') {
				RET(CALL);
			} else {
				RET(VAR);
			}
		  }
		}
<A>\"		{ BEGIN str; clen = 0; }

<A>"}"		{ if (--bracecnt < 0) yyerror("extra }"); BEGIN sc; RET(';'); }
<A>"]"		{ if (--brackcnt < 0) yyerror("extra ]"); RET(']'); }
<A>")"		{ if (--parencnt < 0) yyerror("extra )"); RET(')'); }

<A>.		{ if (yytext[0] == '{') bracecnt++;
		  else if (yytext[0] == '[') brackcnt++;
		  else if (yytext[0] == '(') parencnt++;
		  RET(yylval = yytext[0]); /* everything else */ }

<reg>\\.	{ cbuf[clen++] = '\\'; cbuf[clen++] = yytext[1]; }
<reg>\n		{ yyerror("newline in regular expression %.10s...", cbuf); lineno++; BEGIN A; }
<reg>"/"	{ BEGIN A;
		  cbuf[clen] = 0;
		  yylval = (int) tostring(cbuf);
		  unput('/');
		  RET(REGEXPR); }
<reg>.		{ CADD; }

<str>\"		{ BEGIN A;
		  cbuf[clen] = 0; s = tostring(cbuf);
		  cbuf[clen] = ' '; cbuf[++clen] = 0;
		  yylval = (int)setsymtab(cbuf, s, 0.0, CON|STR, symtab);
		  RET(STRING); }
<str>\n		{ yyerror("newline in string %.10s...", cbuf); lineno++; BEGIN A; }
<str>"\\\""	{ cbuf[clen++] = '"'; }
<str>"\\"n	{ cbuf[clen++] = '\n'; }
<str>"\\"t	{ cbuf[clen++] = '\t'; }
<str>"\\"f	{ cbuf[clen++] = '\f'; }
<str>"\\"r	{ cbuf[clen++] = '\r'; }
<str>"\\"b	{ cbuf[clen++] = '\b'; }
<str>"\\\\"	{ cbuf[clen++] = '\\'; }
<str>"\\"({D}{D}{D}|{D}{D}|{D}) { int n;
		  sscanf(yytext+1, "%o", &n); cbuf[clen++] = n; }
<str>"\\".	{ cbuf[clen++] = yytext[1]; }
<str>.		{ CADD; }

%%

startreg()
{
	BEGIN reg;
	clen = 0;
}

/* input() and unput() are transcriptions of the standard lex
   macros for input and output with additions for error message
   printing.  God help us all if someone changes how lex works.
*/

uchar	ebuf[300];
uchar	*ep = ebuf;

input()
{
	register c;
	extern uchar *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr);
	else if (yyin == NULL)
		c = *lexprog++;
	else
		c = getc(yyin);
	if (c == '\n')
		yylineno++;
	else if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	return *ep++ = c;
}

unput(c)
{
	yytchar = c;
	if (yytchar == '\n')
		yylineno--;
	*yysptr++ = yytchar;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
}
