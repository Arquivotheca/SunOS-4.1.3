/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ifndef lint
static	char sccsid[] = "@(#)awk.g.y 1.1 92/07/30 SMI"; /* from S5R3.1 2.5 */
#endif
%}

%token	FIRSTTOKEN	/* must be first */
%token	FATAL
%token	PROGRAM PASTAT PASTAT2 XBEGIN XEND
%token	NL
%token	ARRAY
%token	MATCH NOTMATCH
%token	FINAL DOT ALL CCL NCCL CHAR OR STAR QUEST PLUS
%token	ADD MINUS MULT DIVIDE MOD
%token	ASSIGN ADDEQ SUBEQ MULTEQ DIVEQ MODEQ POWEQ
%token	ELSE INTEST CONDEXPR
%token	POSTINCR PREINCR POSTDECR PREDECR

%right	ASGNOP
%left	'?'
%left	':'
%left	BOR
%left	AND
%left	GETLINE
%nonassoc APPEND EQ GE GT LE LT NE MATCHOP IN '|'
%left	ARG BLTIN BREAK CALL CLOSE CONTINUE DELETE DO EXIT FOR FIELD FUNC 
%left	GSUB IF INDEX LSUBSTR MATCHFCN NEXT NUMBER
%left	PRINT PRINTF RETURN SPLIT SPRINTF STRING SUB SUBSTR
%left	REGEXPR VAR VARNF WHILE '('
%left	CAT
%left	'+' '-'
%left	'*' '/' '%'
%left	NOT UMINUS
%right	POWER
%right	DECR INCR
%left	INDIRECT
%token	LASTTOKEN	/* must be last */

%{
#include "awk.h"
yywrap() { return(1); }
#ifndef	DEBUG
#	define	PUTS(x)
#endif
Node	*beginloc = 0, *endloc = 0;
int	infunc	= 0;	/* = 1 if in arglist or body of func */
uchar	*curfname = 0;
Node	*arglist = 0;	/* list of args for current function */
uchar	*strnode();
%}
%%

program:
	  pas	{ if (errorflag==0)
			winner = (Node *)stat3(PROGRAM, beginloc, $1, endloc); }
	| error	{ yyclearin; bracecheck(); yyerror("bailing out"); }
	;

and:
	  AND | and NL
	;

bor:
	  BOR | bor NL
	;

comma:
	  ',' | comma NL
	;

do:
	  DO | do NL
	;

else:
	  ELSE | else NL
	;

for:
	  FOR '(' opt_simple_stmt ';' pattern ';' opt_simple_stmt rparen stmt
		{ $$ = stat4(FOR, $3, op2(NE,$5,nullnode), $7, $9); }
	| FOR '(' opt_simple_stmt ';'  ';' opt_simple_stmt rparen stmt
		{ $$ = stat4(FOR, $3, 0, $6, $8); }
	| FOR '(' varname IN varname rparen stmt
		{ $$ = stat3(IN, $3, makearr($5), $7); }
	;

funcname:
	  VAR	{ setfname($1); }
	| CALL	{ setfname($1); }
	;

if:
	  IF '(' pattern rparen		{ $$ = op2(NE,$3,nullnode); }
	;

lbrace:
	  '{' | lbrace NL
	;

nl:
	  NL | nl NL
	;

opt_nl:
	  /* empty */
	| nl
	;

opt_pst:
	  /* empty */
	| pst
	;


opt_simple_stmt:
	  /* empty */			{ $$ = (int)0; }
	| simple_stmt
	;

pas:
	  opt_pst			{ $$ = (int)0; }
	| opt_pst pa_stats opt_pst	{ $$ = $2; }
	;

pa_pat:
	  pattern
	| reg_expr	{ $$ = op3(MATCH, 0, rectonode(), makedfa(reparse($1), 0)); }
	| NOT reg_expr	{ $$ = op1(NOT, op3(MATCH, 0, rectonode(), makedfa(reparse($2), 0))); }
	;

pa_stat:
	  pa_pat			{ $$ = stat2(PASTAT, $1, stat2(PRINT, rectonode(), 0)); }
	| pa_pat lbrace stmtlist '}'	{ $$ = stat2(PASTAT, $1, $3); }
	| pa_pat ',' pa_pat		{ $$ = pa2stat($1, $3, stat2(PRINT, rectonode(), 0)); }
	| pa_pat ',' pa_pat lbrace stmtlist '}'	{ $$ = pa2stat($1, $3, $5); }
	| lbrace stmtlist '}'		{ $$ = stat2(PASTAT, 0, $2); }
	| XBEGIN lbrace stmtlist '}'
		{ beginloc = (Node*) linkum(beginloc,(Node *) $3); $$ = (int)0; }
	| XEND lbrace stmtlist '}'
		{ endloc = (Node*) linkum(endloc,(Node *) $3); $$ = (int)0; }
	| FUNC funcname '(' varlist rparen {infunc++;} lbrace stmtlist '}'
		{ infunc--; curfname=0; defn($2, $4, $8); $$ = (int)0; }
	;

pa_stats:
	  pa_stat
	| pa_stats pst pa_stat	{ $$ = linkum($1, $3); }
	;

patlist:
	  pattern
	| patlist comma pattern	{ $$ = linkum($1, $3); }
	;

ppattern:
	  var ASGNOP ppattern		{ $$ = op2($2, $1, $3); }
	| ppattern '?' ppattern ':' ppattern
	 	{ $$ = op3(CONDEXPR, op2(NE,$1,nullnode), $3, $5); }
	| ppattern bor ppattern %prec BOR
		{ $$ = op2(BOR, op2(NE,$1,nullnode), op2(NE,$3,nullnode)); }
	| ppattern and ppattern %prec AND
		{ $$ = op2(AND, op2(NE,$1,nullnode), op2(NE,$3,nullnode)); }
	| ppattern MATCHOP reg_expr	{ $$ = op3($2, 0, $1, makedfa(reparse($3), 0)); }
	| ppattern MATCHOP ppattern
		{ if (constnode($3))
			$$ = op3($2, 0, $1, makedfa(reparse(strnode($3)), 0));
		  else
			$$ = op3($2, 1, $1, $3); }
	| ppattern IN varname		{ $$ = op2(INTEST, $1, makearr($3)); }
	| '(' plist ')' IN varname	{ $$ = op2(INTEST, $2, makearr($5)); }
	| ppattern term %prec CAT	{ $$ = op2(CAT, $1, $2); }
	| term
	;

pattern:
	  var ASGNOP pattern		{ $$ = op2($2, $1, $3); }
	| pattern '?' pattern ':' pattern
	 	{ $$ = op3(CONDEXPR, op2(NE,$1,nullnode), $3, $5); }
	| pattern bor pattern %prec BOR
		{ $$ = op2(BOR, op2(NE,$1,nullnode), op2(NE,$3,nullnode)); }
	| pattern and pattern %prec AND
		{ $$ = op2(AND, op2(NE,$1,nullnode), op2(NE,$3,nullnode)); }
	| pattern EQ pattern		{ $$ = op2($2, $1, $3); }
	| pattern GE pattern		{ $$ = op2($2, $1, $3); }
	| pattern GT pattern		{ $$ = op2($2, $1, $3); }
	| pattern LE pattern		{ $$ = op2($2, $1, $3); }
	| pattern LT pattern		{ $$ = op2($2, $1, $3); }
	| pattern NE pattern		{ $$ = op2($2, $1, $3); }
	| pattern MATCHOP reg_expr	{ $$ = op3($2, 0, $1, makedfa(reparse($3), 0)); }
	| pattern MATCHOP pattern
		{ if (constnode($3))
			$$ = op3($2, 0, $1, makedfa(reparse(strnode($3)), 0));
		  else
			$$ = op3($2, 1, $1, $3); }
	| pattern IN varname		{ $$ = op2(INTEST, $1, makearr($3)); }
	| '(' plist ')' IN varname	{ $$ = op2(INTEST, $2, makearr($5)); }
	| pattern '|' GETLINE var	{ $$ = op3(GETLINE, $4, $2, $1); }
	| pattern '|' GETLINE		{ $$ = op3(GETLINE, 0, $2, $1); }
	| pattern term %prec CAT	{ $$ = op2(CAT, $1, $2); }
	| term
	;

plist:
	  pattern comma pattern		{ $$ = linkum($1, $3); }
	| plist comma pattern		{ $$ = linkum($1, $3); }
	;

pplist:
	  ppattern
	| pplist comma ppattern		{ $$ = linkum($1, $3); }

prarg:
	  /* empty */			{ $$ = rectonode(); }
	| pplist
	| '(' plist ')'			{ $$ = $2; }
	;

print:
	  PRINT | PRINTF
	;

pst:
	  NL | ';' | pst NL | pst ';'
	;

rbrace:
	  '}' | rbrace NL
	;

reg_expr:
	  '/' {startreg();} REGEXPR '/'		{ $$ = $3; }
	;

rparen:
	  ')' | rparen NL
	;

simple_stmt:
	  print prarg '|' term		{ $$ = stat3($1, $2, $3, $4); }
	| print prarg APPEND term	{ $$ = stat3($1, $2, $3, $4); }
	| print prarg GT term		{ $$ = stat3($1, $2, $3, $4); }
	| print prarg			{ $$ = stat3($1, $2, 0, 0); }
	| DELETE varname '[' patlist ']' { $$ = stat2(DELETE, makearr($2), $4); }
	| pattern			{ $$ = exptostat($1); }
	| error				{ yyclearin; yyerror("illegal statement"); }
	;

st:
	  nl | ';' opt_nl
	;

stmt:
	  BREAK st		{ $$ = stat1(BREAK, 0); }
	| CLOSE pattern st	{ $$ = stat1(CLOSE, $2); }
	| CONTINUE st		{ $$ = stat1(CONTINUE, 0); }
	| do stmt WHILE '(' pattern ')' st
		{ $$ = stat2(DO, $2, op2(NE,$5,nullnode)); }
	| EXIT pattern st	{ $$ = stat1(EXIT, $2); }
	| EXIT st		{ $$ = stat1(EXIT, 0); }
	| for
	| if stmt else stmt	{ $$ = stat3(IF, $1, $2, $4); }
	| if stmt		{ $$ = stat3(IF, $1, $2, 0); }
	| lbrace stmtlist rbrace	{ $$ = $2; }
	| NEXT st		{ $$ = stat1(NEXT, 0); }
	| RETURN pattern st	{ $$ = stat1(RETURN, $2); }
	| RETURN st		{ $$ = stat1(RETURN, 0); }
	| simple_stmt st
	| while stmt		{ $$ = stat2(WHILE, $1, $2); }
	| ';' opt_nl		{ $$ = (int)0; }
	;

stmtlist:
	  stmt
	| stmtlist stmt		{ $$ = linkum($1, $2); }
	;

subop:
	  SUB | GSUB
	;

term:
	  term '+' term			{ $$ = op2(ADD, $1, $3); }
	| term '-' term			{ $$ = op2(MINUS, $1, $3); }
	| term '*' term			{ $$ = op2(MULT, $1, $3); }
	| term '/' term			{ $$ = op2(DIVIDE, $1, $3); }
	| term '%' term			{ $$ = op2(MOD, $1, $3); }
	| term POWER term		{ $$ = op2(POWER, $1, $3); }
	| '-' term %prec UMINUS		{ $$ = op1(UMINUS, $2); }
	| '+' term %prec UMINUS		{ $$ = $2; }
	| BLTIN '(' ')'			{ $$ = op2(BLTIN, $1, rectonode()); }
	| BLTIN '(' patlist ')'		{ $$ = op2(BLTIN, $1, $3); }
	| BLTIN				{ $$ = op2(BLTIN, $1, rectonode()); }
	| CALL '(' ')'			{ $$ = op2(CALL, valtonode($1,CVAR), 0); }
	| CALL '(' patlist ')'		{ $$ = op2(CALL, valtonode($1,CVAR), $3); }
	| DECR var			{ $$ = op1(PREDECR, $2); }
	| INCR var			{ $$ = op1(PREINCR, $2); }
	| var DECR			{ $$ = op1(POSTDECR, $1); }
	| var INCR			{ $$ = op1(POSTINCR, $1); }
	| GETLINE var LT term		{ $$ = op3(GETLINE, $2, $3, $4); }
	| GETLINE LT term		{ $$ = op3(GETLINE, 0, $2, $3); }
	| GETLINE var			{ $$ = op3(GETLINE, $2, 0, 0); }
	| GETLINE			{ $$ = op3(GETLINE, 0, 0, 0); }
	| INDEX '(' pattern comma pattern ')'
		{ $$ = op2(INDEX, $3, $5); }
	| '(' pattern ')'		{ $$ = $2; }
	| MATCHFCN '(' pattern comma reg_expr ')'
		{ $$ = op3(MATCHFCN, 0, $3, makedfa(reparse($5), 1)); }
	| MATCHFCN '(' pattern comma pattern ')'
		{ if (constnode($5))
			$$ = op3(MATCHFCN, 0, $3, makedfa(reparse(strnode($5)), 1));
		  else
			$$ = op3(MATCHFCN, 1, $3, $5); }
	| NOT term			{ $$ = op1(NOT, op2(NE,$2,nullnode)); }
	| NUMBER			{ $$ = valtonode($1, CCON); }
	| SPLIT '(' pattern comma varname comma pattern ')'
		{ $$ = op3(SPLIT, $3, makearr($5), $7); }
	| SPLIT '(' pattern comma varname ')'
		{ $$ = op3(SPLIT, $3, makearr($5), 0); }
	| SPRINTF '(' patlist ')'	{ $$ = op1($1, $3); }
	| STRING	 		{ $$ = valtonode($1, CCON); }
	| subop '(' reg_expr comma pattern ')'
		{ $$ = op4($1, 0, makedfa(reparse($3), 1), $5, rectonode()); }
	| subop '(' pattern comma pattern ')'
		{ if (constnode($3))
			$$ = op4($1, 0, makedfa(reparse(strnode($3)), 1), $5, rectonode());
		  else
			$$ = op4($1, 1, $3, $5, rectonode()); }
	| subop '(' reg_expr comma pattern comma pattern ')'
		{ $$ = op4($1, 0, makedfa(reparse($3), 1), $5, $7); }
	| subop '(' pattern comma pattern comma pattern ')'
		{ if (constnode($3))
			$$ = op4($1, 0, makedfa(reparse(strnode($3)), 1), $5, $7);
		  else
			$$ = op4($1, 1, $3, $5, $7); }
	| SUBSTR '(' pattern comma pattern comma pattern ')'
		{ $$ = op3(SUBSTR, $3, $5, $7); }
	| SUBSTR '(' pattern comma pattern ')'
		{ $$ = op3(SUBSTR, $3, $5, 0); }
	| var
	;

var:
	  varname
	| varname '[' patlist ']'	{ $$ = op2(ARRAY, makearr($1), $3); }
	| FIELD				{ $$ = valtonode($1, CFLD); }
	| INDIRECT term	 		{ $$ = op1(INDIRECT, $2); }
	;	

varlist:
	  /* nothing */		{ arglist = (Node *)($$ = 0); }
	| VAR			{ $$ = valtonode($1,CVAR); arglist = (Node*) $$; }
	| varlist comma VAR	{ $$ = linkum($1,valtonode($3,CVAR)); arglist = (Node*) $$; }
	;

varname:
	  VAR			{ $$ = valtonode($1, CVAR); }
	| ARG 			{ $$ = op1(ARG, $1); }
	| VARNF			{ $$ = op1(VARNF, $1); }
	;


while:
	  WHILE '(' pattern rparen	{ $$ = op2(NE,$3,nullnode); }
	;

%%

setfname(p)
	Cell *p;
{
	curfname = p->nval;
}

constnode(p)
	Node *p;
{
	return p->ntype == NVALUE && ((Cell *) (p->narg[0]))->csub == CCON;
}

uchar *strnode(p)
	Node *p;
{
	return ((Cell *)(p->narg[0]))->sval;
}
