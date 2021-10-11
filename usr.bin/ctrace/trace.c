/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)trace.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

/*	ctrace - C program debugging tool
 *
 *	trace code generation routines
 *
 */
#include "global.h"
#include <ctype.h>

#define max(X, Y)	(((int) X > (int) Y) ? X : Y)

/* boolean fields in the marked array */
#define	VAR_START	1
#define	VAR_END		2
#define	EXP_START	4
#define	EXP_END		8

static	enum	bool too_many_vars = no;	/* too many variables to trace */
static	int	marked[STMTMAX];
static	int	vtrace_count = 0;

struct	text {
	int	start, end;
};
static	struct {
	struct	text var;
	struct	text exp;
	enum	trace_type ttype; /* old C compiler does not allow structure member names to be the same */
} vtrace[TRACEMAX];
/* output the preprocessor statement */
putpp()
{
	puttext(yyleng);

	/* add a #line statement surrounded by newlines so
	   the next trace code is put on a new line */
	printf("\n#line %d \"%s\"\n", yylineno, filename);
}

/* output the statement text as is */
puttext(end)
register int	end;
{
	register int	i;
	char	*strcpy();

	if (end >= yyleng) {	/* a #if in dcl init list can make end > yyleng */
		fputs(yytext, stdout); /* cannot use puts because it adds a newline */
		yyleng = token_start = 0;
	}
	else {
		for (i = 0; i < end; ++i)
			putchar(yytext[i]);
		strcpy(yytext, yytext + end);
		yyleng = token_start = yyleng - end;
	}
	vtrace_count = 0; /* declarations can contain expressions */
}

/* cleanup after tracing a statement */
reset()
{
	vtrace_count = yyleng = token_start = 0;

	/* warn of too many variables to trace in this statement */
	if (too_many_vars) {
		warning("some variables are not traced in this statement");
		too_many_vars = no;
	}
	/* warn of a statement that was too long to fit in the buffer */
	if (too_long) {
		warning("statement too long to trace");
		puttext(yyleng);
		too_long = no;
	}
}
/* functions to maintain the variable trace list */

add_fcn(sym, exp_start, exp_end, type)
struct	symbol_struct sym;
int	exp_start, exp_end;
enum	trace_type type;
{
	register int	i;

	/* see if this function is being traced */
	if (!trace)
		return;	/* prevent unnecessary "too many vars" messages */
	
	/* see if the variable is already being traced */
	for (i = vtrace_count - 1; i >= 0; --i)
		if (vtrace[i].exp.start == sym.start && vtrace[i].exp.end == sym.end) {
			break;
		}
	/* if not, use the next trace element */
	if (i < 0)
		if (vtrace_count < tracemax)
			i = vtrace_count++;
		else {
			too_many_vars = yes;
			return;
		}
	/* save the trace information */
	vtrace[i].var.start = sym.start;
	vtrace[i].var.end   = sym.end;
	vtrace[i].exp.start = exp_start;
	vtrace[i].exp.end   = exp_end;
	vtrace[i].ttype	   = type;
}
expand_fcn(sym, new_start, new_end)
struct	symbol_struct sym;
int	new_start, new_end;
{
	register int	i;

	for (i = vtrace_count - 1; i >= 0; --i)
		if (vtrace[i].exp.start == sym.start && vtrace[i].exp.end == sym.end) {

			/* don't expand ++i to *++i */
			if (sym.type != side_effect) {
				vtrace[i].exp.start = vtrace[i].var.start = new_start;
				vtrace[i].exp.end = vtrace[i].var.end = new_end;
			}
			break;
		}
}
rm_trace(sym)
struct	symbol_struct sym;
{
	register int	i;

	for (i = vtrace_count - 1; i >= 0; --i)
		if (vtrace[i].exp.start == sym.start && vtrace[i].exp.end == sym.end) {
			--vtrace_count;
			
			/* eliminate the hole */
			for ( ; i < vtrace_count; ++i)
				vtrace[i] = vtrace[i + 1];
			break;
		}
}
rm_all_trace(sym)
struct	symbol_struct sym;
{
	register int	i, j;

	for (i = vtrace_count - 1; i >= 0; --i) {
		if (vtrace[i].exp.start >= sym.start && vtrace[i].exp.start <= sym.end) {
			--vtrace_count;
			
			/* eliminate the hole */
			for (j = i; j < vtrace_count; ++j)
				vtrace[j] = vtrace[j + 1];
		}
	}
}
/* functions to trace the variables in a statement */

tr_vars(start, end)
int	start;
register int	end;	/* register variables are in order of use */
{
	register int	i, j;
	enum	 bool	simple_trace();
	
	/* don't trace a statement that was too long to fit in the buffer */
	if (too_long) {
		return;
	}
	/* mark the significant characters */
	for (i = start; i < end; ++i)
		marked[i] = 0;
	for (i = 0; i < vtrace_count; ++i) {
		marked[vtrace[i].exp.start] |= EXP_START;
		marked[vtrace[i].exp.end]   |= EXP_END;
		marked[vtrace[i].var.end]   |= VAR_END;
	}
	/* for each character in the statement */
	for (i = start; i < end; ++i) {
		if (marked[i] & EXP_END)

			/* check for the end of a traced expression */
			for (j = 0; j < vtrace_count; ++j)
				if (vtrace[j].exp.end == i) {
					if (!simple_trace(j)) {
						printf(", ");
						dump_code(j);
						transvar(vtrace[j].var);
					}
					printf("), ");
					if (vtrace[j].ttype == strres)
						printf("_ct");
					else
						transvar(vtrace[j].var);
	
					/* check for a postfix ++/-- operator */
					if (vtrace[j].ttype == postfix) {
						if (yytext[vtrace[j].exp.end - 1] == '+')
							putchar('-');
						else
							putchar('+');
						putchar('1');
					}
					putchar(')');
				}
		if (marked[i] & EXP_START)
	
			/* check for the start of a traced expression */
			for (j = 0; j < vtrace_count; ++j)
				if (vtrace[j].exp.start == i) {
					putchar('(');
					if (simple_trace(j))
						dump_code(j);
					else if (vtrace[j].ttype == strres)
						printf("_ct = ");
				}
		/* output this character */
		putchar(yytext[i]);
	}
}
/* check for a simple variable trace */

static enum bool
simple_trace(j)
register int	j;
{
	register int	k;
	
	if (vtrace[j].exp.start == vtrace[j].var.start &&
	    vtrace[j].exp.end == vtrace[j].var.end) {
		for (k = 0; k < vtrace_count; ++k)
			if (k != j) {	/* if var is not this var */
				if (vtrace[k].exp.start >= vtrace[j].var.start &&
				    vtrace[k].exp.end <= vtrace[j].var.end)
				break;	/* this var has an embedded var */
				if (vtrace[k].exp.start == vtrace[j].var.start ||
				    vtrace[k].exp.end == vtrace[j].var.end)
				break;	/* this var is part of a larger var */
			}
		if (k == vtrace_count)
			return(yes);
	}
	return(no);
}
/* dump routine call */

static
dump_code(j)
register int	j;
{
	register int	k;
	register char	c;
	
	if (vtrace[j].ttype == string || vtrace[j].ttype == strres)
		printf("s_ct_(\"");
	else	/* unknown type */
		printf("u_ct_(\"");
	for (k = 0; (c = indentation[k]) != '\0'; ++k)
		transchar(c);
	printf("/* ");
	putvar(vtrace[j].var);
	printf("\",");
	
	/* size of variable code */
	if (vtrace[j].ttype != string && vtrace[j].ttype != strres) {
		printf("sizeof(");
		transvar(vtrace[j].var);
		printf("),");
	}
}
/* output the variable's name */

static
putvar(var)
struct	text var;
{
	register int	i;
	
	for (i = var.start; i < var.end; ++i)
		transchar(yytext[i]);
}
/* translate sub-exps containing '=', '++', and '--' ops into repeatable exps */

static
transvar(var)
struct	text var;
{
	register int	i, j;
	
	/* don't check the first character for start of an expression */
	putchar(yytext[var.start]);

	for (i = var.start + 1; i < var.end; ++i) {
		if (marked[i] & VAR_END)
		
			/* check for the end of a traced variable */
			for (j = 0; j < vtrace_count; ++j)
				if (vtrace[j].var.end == i) {

					/* transform a postfix ++/-- operator into -/+ 1 */
					if (vtrace[j].ttype == postfix) {
						if (yytext[vtrace[j].exp.end - 1] == '+')
							putchar('-');
						else
							putchar('+');
						printf("1)");

						/* skip the postfix operator */
						i = max(i, vtrace[j].exp.end - 1);
						goto skip;
					}
					/* skip the rest of a assignment expression */
					if (vtrace[j].ttype == assign) {
						i = max(i, vtrace[j].exp.end - 1);
						goto skip;
					}
				}
		if (marked[i] & EXP_START)

			/* check for the start of a traced expression */
			for (j = 0; j < vtrace_count; ++j)
				if (vtrace[j].exp.start == i) {

					/* insert a '(' in front of a postfix ++/-- expression */
					if (vtrace[j].ttype == postfix)
						putchar('(');

					/* skip a prefix ++/-- operator */
					if (vtrace[j].ttype == prefix) {
						i = max(i, vtrace[j].var.start - 1);
						goto skip;
					}
				}
		/* output this character */
		putchar(yytext[i]);
skip:		;
	}
}
/* trace the statement text */

tr_stmt(lineno, stmt, putsemicolon)
int	lineno;
char	*stmt;
enum	bool putsemicolon;
{
	register int	c, i;

	/* see if this function is being traced and */
	/* don't trace a statement that was too long to fit in the buffer */
	if (!trace || too_long) {
		return;
	}
	/* output the trace function call */
	printf("t_ct_(\"");

	/* if the line number field should be blank */
	if (lineno == NO_LINENO) {

		/* print a blank line number */
		printf("\\n    ");

		/* copy the leading blanks and tabs in the loop statement */
		if (stmt[0] == '\n') /* skip a leading newline */
			i = 1;
		else
			i = 0;
		for ( ; isspace(c = stmt[i]); ++i)
			transchar(c);

		i = 0; /* there isn't a newline in the statement text */
	}
	/* if the statement starts on a new line */
	else if (stmt[0] == '\n') {

		/* adjust the line number for any embedded new lines */
		for (i = 1; (c = stmt[i]) != '\0'; ++i)
			if (c == '\n')
				--lineno;

		/* print the line number */
		printf("\\n%3d ", lineno);
		i = 1; /* skip the newline in the statement text */
	}
	else
		i = 0;

	/* convert special characters in the statement text */
	for ( ; (c = stmt[i]) != '\0'; ++i)
		transchar(c);

	/* terminate the trace function call */
	printf("\")");

	/* see if this is to be a statement */
	if (putsemicolon)
		printf("; ");
}

/* translate a character for a string constant */

static
transchar(c)
register int	c;
{
	switch (c) {
	case '"':
		printf("\\\"");
		break;
	case '\\':
		printf("\\\\");
		break;
	case '\t':
		printf("\\t");
		break;
	case '\n':
		printf("\\n");
		break;
	default:
		putchar(c);
	}
}
