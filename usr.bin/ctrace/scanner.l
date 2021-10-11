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
static	char sccsid[] = "@(#)scanner.l 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif
%}

%{
/*	ctrace - C program debugging tool
 *
 *	C statement scanner
 *
 */
#include <ctype.h>	/* isdigit macro */
#include "global.h"
#include "y.tab.h"

#define scanmore()	yymore()
#define token(X)	return(token_fcn(X))

#define LEX_EOF		0		/* EOF is 0 in lex */

char	indentation[INDENTMAX];		/* leading white space for statement */
enum	bool too_long = no;		/* statement too long to fit in buffer */
int	token_start = 0;		/* buffer index for current token */
int	last_yychar = LEX_EOF;		/* used for parser error handling */

static	int	last_token = LEX_EOF;	/* used for typedef name identification */
static	int	stmt_start = 0;		/* buffer index for the start of the statement */
static	char	macro[TOKENMAX];	/* preprocessor macro name */
static	char	dummy;			/* dummy target for sscanf match */
static	char	*pp;			/* preprocessor statement name */
static	char	*s;			/* string pointer */

char	*strchr(), *strrchr(), *strsave();
%}
assignchars	[+\-*/%&^|><][><]?
identifier	[a-zA-Z_][a-zA-Z_0-9]*
number		"."?[0-9][.0-9a-fA-FlLxX]*
indent		[ \t]
space		[ \t\f\b]
nlspace		[ \t\f\b\n]
%%
\#{space}*		{ /* read the preprocessor statement */
			pp = yytext + yyleng;
			preprocessor();
			
			/* if this is a #define statement */
			if (*pp == 'd') {
				
				/* look for a constant terminated by a 
				   semicolon.  This will cause compiler errors
				   when used as "a=CONSTANT" even if not traced
				   */
				if (yytext[yyleng - 1] == ';') {
					fatal("cannot handle preprocessor code, use -P option");
				}
				/* look for a macro that, if traced, will cause compiler errors */
				if (sscanf(pp, "define %[a-zA-Z0-9_](%*[^)]) %*[^,.=;{}]%1c", macro, &dummy) == 2
				||  sscanf(pp, "define %[a-zA-Z0-9_] %*[^,.=;{}]%1c", macro, &dummy) == 2) {
					add_symbol(macro, MACRO);
				}
			}
			/* save the new line number from a #line statement */
			if (sscanf(pp, "line %d", &yylineno) == 1) {
				--yylineno;	/* \n hasn't been read */
				
				/* save any file name */
				if ((s = strchr(pp, '"')) != NULL) {
					*(strrchr(s, '"')) = '\0';
					if ((filename = strsave(s + 1)) == NULL) {
						fatal("out of storage");
					}
					s[strlen(s)] = '"'; /* replace quote */
				}
			}
			/* treat some preprocessor statements as tokens when in a function */
			if (fcn_body) {
				if (strncmp(pp, "if", 2) == 0) {
					token(PP_IF);
				}
				else if (strncmp(pp, "el", 2) == 0) {
					token(PP_ELSE);
				}
				else if (strncmp(pp, "en", 2) == 0) {
					token(PP_ENDIF);
				}
			}
			/* echo only the preprocessor statement when in a 
			   function because it may be within a C statement */
			if (fcn_body) {
				--token_start;	/* back up over \n */
				printf("%s", yytext + token_start);
				yytext[token_start] = '\0';
				yyleng = token_start;
				scanmore();
			}
			else {	/* start fresh with a new token */
				token_start = 0;
			}
			/* add a newline so any following trace code is put on
			   a new line.  This of course changes the line count
			   so a (non-zero) #line statement must be added
			   -- unless we are looking at preprocessor output */
			putchar('\n');
			if (pound_line == no) {
				printf("#line %d \"%s\"\n",
				 (yylineno?yylineno:1), filename);
			}
			}
"/*"			{ /* output comments immediately to prevent line
			     buffer overflow and to preserve embedded newlines */
			yyleng -= 2;	/* delete the leading comment delimiter */
			comment();
			scanmore();	/* this comment may be embedded in a statement */
			}
\n{space}*/(\n|"/*")	|
{space}+/(\n|"/*")	{ /* output empty lines, trailing space, and comment leading space */
			if (token_start == 0)
				ECHO;
			else {
				token_start = yyleng; scanmore(); }
			}
{space}+		{ /* look for the start of a statement */
			if (token_start == 0)
				stmt_start = yyleng;
			token_start = yyleng; scanmore(); }
\n{indent}*		{ /* look for the start of a statement */
			if (token_start == 0) {
				stmt_start = yyleng;

				/* save the indentation of the last statement that begins on a new line */
				strncpy(indentation, yytext + 1, INDENTMAX);
			}
			token_start = yyleng; scanmore(); }
{number}		token(CONSTANT);
\'			token(scanfor('\'')); /* allow meta-character constants */
\"			token(scanfor('"'));
{identifier}/{nlspace}+{identifier}	| /* must have intermediate blanks to prevent a match on "aa" */
{identifier}/{nlspace}*"*"[^=]		{
			/* check for a new typedef name */
			if (token_start == stmt_start || last_token == CLASS)
				token(add_symbol(yytext + token_start, TYPE));
			else
				token(lookup(yytext + token_start));
			}
{identifier}/{nlspace}*"("	{
			/* check for a new function name */
			token(add_symbol(yytext + token_start, FUNCTION));
			}
{identifier}		token(lookup(yytext + token_start));
"||"			token(OROR);
"&&"			token(ANDAND);
"=="			|
"!="			token(EQUOP);
"<="			|
">="			|
"<"			|
">"			token(RELOP);
{assignchars}=		| /* must be after "<=" and ">=" rules */
={assignchars}		token(ASSIGNOP);
">>"			|
"<<"			token(SHIFTOP);
"/"			|
"%"			token(DIVOP);
"!"			|
"~"			token(NOTCOMPL);
"++"			|
"--"			token(INCOP);
"."			|
"->"			token(DOTARROW);
.			token(yytext[yyleng - 1]);
%%
static
preprocessor()
{
	register char	c, next_c;	/* EOF is 0 in lex so these can be characters */

	/* change "# <number>" to "#line <number>" */
	if (isdigit(c = input())) {
		strcat(yytext, "line ");
		yyleng += 5;
	}
	unput(c);
	
	/* the preprocessor statement will be output as it is read so
	   imbedded comments are not moved to the beginning of the line */
	/* it will also be saved in yytext for later scanning */
	if (!fcn_body) {
		ECHO;
	}
	/* scan for the terminating newline */
	while ((c = input()) != '\n') {
		switch (c) {
		case '\\':	/* escape character */
			if (!fcn_body)
				output(c);
			if (yyleng < STMTMAX)
				yytext[yyleng++] = c;
			c = input();
			break;
		case '/':	/* check for a comment */
			if ((next_c = input()) == '*') {
				comment();	/* output the comment */
				continue;	/* don't save the slash */
			}
			else
				unput(next_c);	/* c may be a newline */
			break;
		}
		if (!fcn_body)
			output(c);
		if (yyleng < STMTMAX)
			yytext[yyleng++] = c;
	}
	unput(c);		/* put back the newline */
	yytext[yyleng] = '\0';	/* terminate the string */
}
static
comment()
{
	register char	c, last_c;	/* EOF is 0 in lex so these can be characters */

	/* output comments immediately to prevent line buffer overflow
	   and to preserve embedded newlines */
	printf("/*");

	/* find the trailing comment delimiter */
	last_c = input();
	putchar(last_c);
	while ((c = input()) != LEX_EOF) {
		putchar(c);
		if (last_c == '*' && c == '/')
			break;
		last_c = c;
	}
}
static
scanfor(terminator)
register char	terminator;
{
	register char	c;	/* EOF is 0 in lex so c can be a char */

	/* scan until the terminator is found */
	while ((c = yytext[yyleng++] = input()) != terminator) {
		switch (c) {
		case '\\':	/* escape character */
			yytext[yyleng++] = input();
			break;
		case LEX_EOF:	/* end of file */
			return(LEX_EOF);
			break;
		}
		/* see if this token will overflow the statement buffer */
		if (yyleng >= STMTMAX) /* '\\' may cause yyleng to be > STMTMAX */ {
			dump_stmt();
		}
	}
	yytext[yyleng] = '\0';
	return(CONSTANT);
}
static
token_fcn(token_value)
register int	token_value;
{
	/* check for a long statement */
	if (yyleng > STMTMAX) {
		dump_stmt();
	}
	yylval.symbol.start = token_start;
	yylval.symbol.end = token_start = yyleng;
	scanmore();
	last_yychar = last_token;
	last_token = token_value;
	return(token_value);
}
dump_stmt()
{
	too_long = yes;		/* statement is too long */
	yytext[yyleng] = '\0';	/* empty the statement buffer */
	ECHO;
	yyleng = 0;
}
/* dummy routine to avoid need for lex library on non-UNIX systems */
static
yywrap()
{
	return(1);
}
