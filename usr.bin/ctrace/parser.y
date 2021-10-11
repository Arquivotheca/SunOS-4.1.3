/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ifndef lint
static	char sccsid[] = "@(#)parser.y 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif
%}

%{
/*	ctrace - C program debugging tool
 *
 *	C statement parser
 *
 */
#include "global.h"

#define min(X, Y)	(((int) X < (int) Y) ? X : Y)
#define max(X, Y)	(((int) X > (int) Y) ? X : Y)
#define max3(X, Y, Z)	max(X, max(Y, Z))
#define max4(W,X,Y,Z)	max(W, max(X, max(Y, Z)))

#define symbol_info(X, FIRST_SYM, LAST_SYM, TYPE) \
		/*	X.start = FIRST_SYM.start, */ /* default if $1 */ \
			X.end = LAST_SYM.end, \
			X.type = TYPE

#define add_trace(VAR, FIRST_SYM, LAST_SYM, TYPE) \
			add_fcn(VAR, FIRST_SYM.start, LAST_SYM.end, TYPE)

#define expand_trace(VAR, FIRST_SYM, LAST_SYM) \
			expand_fcn(VAR, FIRST_SYM.start, LAST_SYM.end)

enum	bool fcn_body = no;		/* function body indicator */
			
static	int	len;			/* temporary variable */
static	int	fcn_line;		/* function header line number */
static	char	fcn_name[IDMAX + 1];	/* function name */
static	char	fcn_text[SAVEMAX + 1];	/* function header text */
static	char	nf_name[IDMAX + 1];	/* non-function name */
static	enum	bool save_header = yes;	/* start of function */
static	enum	bool flow_break = no;	/* return, goto, break, & continue */
static	enum	bool executable = no;	/* executable statements */
%}
%union {
	struct	symbol_struct symbol;
}
/* operator precedence */

%left  <symbol> ','
%right <symbol> '=' ASSIGNOP /* += =+ etc. */
%right <symbol> '?' ':'
%left  <symbol> OROR
%left  <symbol> ANDAND
%left  <symbol> '|'
%left  <symbol> '^'
%left  <symbol> '&'
%left  <symbol> EQUOP /* == != */
%left  <symbol> RELOP /* < > <= >= */
%left  <symbol> SHIFTOP /* << >> */
%left  <symbol> '+' '-'
%left  <symbol> '*' DIVOP /* / % */
%right <symbol> NOTCOMPL /* ! ~ */
%right <symbol> SIZEOF INCOP /* ++ -- */
%left  <symbol> '[' '(' DOTARROW

%token <symbol> '{' '}' ']' ')' ';'
%token <symbol> CLASS CONSTANT IDENT FUNCTION PP_IF PP_ELSE PP_ENDIF TYPE
/* the above tokens are used in scanner.l */

/* these tokens are used only in lookup.c */
%token <symbol> BREAK_CONT CASE DEFAULT DO ELSE ENUM FOR GOTO IF MACRO
%token <symbol> RETURN STRUCT_UNION SWITCH TYPEDEF WHILE
%token <symbol> STRRES STRCAT STRCMP STRCPY STRLEN STRNCAT STRNCMP
%token <symbol> IOBUF JMP_BUF	/* used only by lookup() */

%type  <symbol> name name_args opt_exp exp term strfcn int_strfcn arg cast ident strfcn_name
%%
/* declarations (copied from the portable compiler) */

ext_def_list:	   ext_def_list external_def
		|  /* empty */
		;
external_def:	   data_def
		|  error
data_def:	   TYPEDEF oattributes init_dcl_list dcl_semicolon {
			add_type(nf_name); }
		|  oattributes dcl_semicolon
		|  oattributes init_dcl_list dcl_semicolon
		|  oattributes fdeclarator {
			/* prevent further function declarations from overwriting this header */
			save_header = no; }
		   function_body {
			save_header = yes; }
		;
function_body:	   arg_dcl_list fcn_stmt_body
		;
arg_dcl_list:	   arg_dcl_list declaration
		|  /* empty */
		;
		;
dcl_stat_list	:  dcl_stat_list attributes dcl_semicolon
		|  dcl_stat_list attributes init_dcl_list dcl_semicolon
		|  /* empty */
		;
declaration:	   attributes declarator_list dcl_semicolon
		|  attributes dcl_semicolon
		|  error  dcl_semicolon
		;
oattributes:	  attributes
		|  /* VOID */
		;
attributes:	   CLASS type
		|  type CLASS
		|  CLASS
		|  type
		;
type:		   TYPE
		|  TYPE TYPE
		|  TYPE TYPE TYPE
		|  struct_dcl
		|  enum_dcl
		;
enum_dcl:	   enum_head '{' moe_list optcomma '}'
		|  ENUM ident
		;
enum_head:	   ENUM
		|  ENUM ident
		;
moe_list:	   moe
		|  moe_list ',' moe
		;
moe:		   ident
		|  ident '=' con_e
		;
		/* the optional semicolon was eliminated so the parser
		   doesn't have to look for a '}' or another declaration
		   after a ';', which prevents the scanner from finding
		   a new type on the next declaration */
struct_dcl:	   str_head '{' {
			/* output this part of the statement so the first 
			   struct element declaration looks like a new 
			   statement so the typedef scanning works */
			if (!executable) {
				puttext($2.end);
			}
			}
		   type_dcl_list '}'
		|  STRUCT_UNION ident
		;
str_head:	   STRUCT_UNION
		|  STRUCT_UNION ident
		;
type_dcl_list:	   type_declaration
		|  type_dcl_list type_declaration
		;
type_declaration:  type declarator_list dcl_semicolon
		|  type dcl_semicolon
		;
declarator_list:   declarator
		|  declarator_list  ','  declarator
		;
declarator:	   fdeclarator
		|  nfdeclarator
		|  nfdeclarator ':' con_e
			%prec ','
		|  ':' con_e
			%prec ','
		|  error
		;
		/* int (a)();   is not a function --- sorry! */
nfdeclarator:	   '*' nfdeclarator		
		|  nfdeclarator  '('   ')'		
		|  nfdeclarator '[' ']'		
		|  nfdeclarator '[' con_e ']'	
		|  name {
			/* save the name because it may be a typedef name */
			len = min($1.end - $1.start, IDMAX);
			strncpy(nf_name, yytext + $1.start, len);
			nf_name[len] = '\0';
			}
		|   '('  nfdeclarator  ')' 		
		;
fdeclarator:	   '*' fdeclarator
		|  fdeclarator  '('   ')'
		|  fdeclarator '[' ']'
		|  fdeclarator '[' con_e ']'
		|   '('  fdeclarator  ')'
		|  name_args {
			/* save the function header */
			if (save_header) {
				fcn_line = yylineno;
				fcn_text[0] = '\n';
				strncpy(fcn_text + 1, yytext + $1.start, SAVEMAX - 1);
				puttext($1.end);
			}
			}
		;
name_args:	   name_lp  name_list  ')' {
			$$.end = $3.end; }
		|  name_lp ')' {
			$$.end = $2.end; }
		;
name_lp:	  name '(' {
			/* save the function name */
			if (save_header) {
				len = min($1.end - $1.start, IDMAX);
				strncpy(fcn_name, yytext + $1.start, len);
				fcn_name[len] = '\0';
			}
			}
		;
name		: ident
		| strfcn_name
		;
name_list:	   ident			
		|  name_list  ','  ident 
		;
		/* always preceeded by attributes */
init_dcl_list:	   init_declarator
			%prec ','
		|  init_dcl_list  ','  init_declarator
		;
		/* always preceeded by attributes */
xnfdeclarator:	   nfdeclarator
		|  error
		;
		/* always preceeded by attributes */
init_declarator:   nfdeclarator
		|  fdeclarator
		|  xnfdeclarator optasgn exp
			%prec ','
		|  xnfdeclarator optasgn '{' init_list optcomma '}'
		;
init_list:	   initializer
			%prec ','
		|  init_list  ',' {
			puttext($2.end); }
		   initializer
		;
initializer:	   exp
			%prec ','
		|  '{' init_list optcomma '}'
		;
optcomma	:	/* VOID */
		|  ','
		;
optasgn		:	/* VOID */
		|  '='
		;
con_e		: exp %prec ','
		;
		;
dcl_semicolon:	  ';' {	/* could be a struct dcl in a cast */
			if (!executable) {
				puttext($1.end);
			}
			}
		;
/* executable statements */

fcn_stmt_body : '{'	{
			puttext(yyleng);
			fcn_body = yes;
			}
	  dcl_stat_list	{
			/* see if this function is to be traced */
			tr_fcn(fcn_name);
			
			/* declare a temporary variable for string function results */
			printf("char *_ct;");	/* char pointer may be bigger than int */
			
			/* trace the function header */
			tr_stmt(fcn_line, fcn_text, yes);
			executable = yes;
			}
	  stmt_list
	  '}'		{
			executable = no;
			fcn_body = no;
			if (flow_break)
				flow_break = no;
			else
				tr_stmt(NO_LINENO, "/* return */", yes);
			puttext(yyleng);
			}
	;
compound_stmt :
	  '{'		{
			puttext(yyleng);

			/* trace the brace after any declarations */
			fcn_line = yylineno;
			strncpy(fcn_text, yytext, SAVEMAX);
			executable = no;
			}
	  dcl_stat_list	{
			/* don't trace the '{' after "switch (a)" */
			if (flow_break)
				flow_break = no;
			else
				tr_stmt(fcn_line, fcn_text, yes);
			executable = yes;
			}
	  stmt_list
	  '}'		{
			if (flow_break)
				flow_break = no;
			else
				tr_stmt(yylineno, yytext, yes);
			puttext(yyleng);
			}
	;
stmt_list : /* no statements */
        | stmt_list stmt
	;
stmt	: compound_stmt
	| pp_if_stmt
	| flow_control_stmt {
			/* there could have been embedded break statements */
			flow_break = no; }
	| flow_break_stmt {
			flow_break = yes; }
	| label		{
			puttext(yyleng);
			tr_stmt(yylineno, yytext, yes);
			}
	  stmt
	| opt_exp ';'	{
			tr_stmt(yylineno, yytext, yes);
			tr_vars(0, yyleng);
			reset();
			}
	| error ';'	{
			puttext(yyleng); }
	| error '}'	{
			puttext(yyleng); }
	;
flow_control_stmt :
	  if_stmt
	| for_stmt
	| while_stmt
	| do_stmt
	| switch_stmt
	;
flow_break_stmt :
	  break_cont_goto {
			tr_stmt(yylineno, yytext, yes);
			puttext(yyleng);
			}
	| RETURN opt_exp ';' {
			tr_stmt(yylineno, yytext, yes);
			tr_vars(0, yyleng);
			reset();
			}
break_cont_goto :
	  BREAK_CONT ';' 
	| GOTO ident ';' 
	;
label	: ident ':'	
	| CASE exp ':'	
	| DEFAULT ':'	
	;
pp_if_stmt :
	  PP_IF 	{
			putpp(); }
	  stmt_list
	  pp_else_part
	  PP_ENDIF	{
			putpp(); }
	;
pp_else_part : /* no #else */
	| PP_ELSE	{
			putpp(); }
	  stmt_list
	;
if_stmt	: IF '(' exp ')' {
			tr_stmt(yylineno, yytext, yes);
			tr_vars(0, yyleng);
			reset();
			putchar('{');
			}
	  stmt		{
			putchar('}'); }
	  else_part
	;
else_part : /* no else */
	| ELSE		{
			puttext(yyleng);
			putchar('{');
			tr_stmt(yylineno, yytext, yes);
			}
	  stmt		{
			putchar('}'); }
	;
for_stmt :
	  FOR '(' opt_exp ';' opt_exp ';' opt_exp ')' {
			/* put the stmt trace before the first and last
			   exp because they may cause an execution error */
			tr_stmt(yylineno, yytext, yes);
			tr_vars(0, $6.end);
			tr_stmt(yylineno, yytext, no);
			if ($7.start != $7.end && trace && !too_long) /* check for a non-null exp */
				putchar(',');
			tr_vars($7.start, yyleng);
			reset();
			putchar('{');
			}
	  stmt		{
			putchar('}'); }
	;
while_stmt :
	  WHILE '(' exp ')' {
			/* insert the statement trace after the "while(" */
			tr_vars(0, $2.end);
			tr_stmt(yylineno, yytext, no);
			if (trace && !too_long)
				putchar(',');
			tr_vars($3.start, yyleng);
			reset();
			putchar('{');
			}
	  stmt		{
			putchar('}'); }
	;
do_stmt : DO		{
			puttext(yyleng);
			putchar('{');
			tr_stmt(yylineno, yytext, yes);
			}
	  stmt
	  WHILE '(' exp ')' ';' {
			tr_stmt(yylineno, yytext, yes);
			putchar('}');
			tr_vars(0, yyleng);
			reset();
			}
	;
switch_stmt :
	  SWITCH '(' exp ')' {
			tr_stmt(yylineno, yytext, yes);
			tr_vars(0, yyleng);
			reset();
			flow_break = yes; /* don't trace the '{' before the first case */
			}
	  stmt		
	;
/* expressions */

opt_exp	: /* no exp */	{
			$$.start = $$.end = $<symbol>0.end; }
	| exp
	;
exp	: exp ',' exp	{ bop:
			symbol_info($$, $1, $3, max3($1.type, $3.type, repeatable)); }
	| exp '=' exp	{
			symbol_info($$, $1, $3, max3($1.type, $3.type, repeatable));
			if ($1.type != side_effect) {

				/* keep inner expressions lower on the stack so *a++ = *b++ is traced properly */
				rm_trace($1);

				/* don't trace a=b=1 */
				if (suppress && $3.type == constant)
					$$.type = constant;
				else {
					/* trace only a in a=b */
					/* note: (p = p->next) cannot be traced as one variable */
					/* don't set to variable because "(a=b)" will expand to
					   a term and then to a normal trace */
					if (suppress && $3.type == variable)
						rm_trace($3);

					/* trace the assigned variable */
					add_trace($1, $1, $3, assign);
				}
			}
			}
	| exp ASSIGNOP exp {
			symbol_info($$, $1, $3, max3($1.type, $3.type, repeatable));
			if ($1.type != side_effect)
				rm_trace($1); /* keep inner expressions lower on the trace stack */
				add_trace($1, $1, $3, assign);
			}
	| exp '?' exp ':' exp {
			symbol_info($$, $1, $5,
			    max4($1.type, $3.type, $5.type, repeatable)); }
	| exp OROR exp	{ goto bop; }
	| exp ANDAND exp { goto bop; }
	| exp '|' exp	{ goto bop; }
	| exp '^' exp	{ goto bop; }
	| exp '&' exp	{ goto bop; }
	| exp EQUOP exp	{ goto bop; }
	| exp RELOP exp	{ goto bop; }
	| exp SHIFTOP exp { goto bop; }
	| exp '+' exp	{ goto bop; }
	| exp '-' exp	{ goto bop; }
	| exp '*' exp	{ goto bop; }
	| exp DIVOP exp	{ goto bop; }
	| term		{
			if ($1.type == variable)
				add_trace($1, $1, $1, normal);
			}
	;
term	: '*' term	{
			symbol_info($$, $1, $2, $2.type);
			if ($2.type == variable) /* don't expand ++a to *++a */
				expand_trace($2, $1, $2);
			}
	| '&' term	{ goto const_op; }
	| SIZEOF term	{ const_op:
			symbol_info($$, $1, $2, constant);
			if ($2.type == side_effect)
				$$.type = side_effect;
			/* don't trace sizeof(a) or &(a) */
			rm_trace($2);
			}
	| SIZEOF cast %prec SIZEOF {
			symbol_info($$, $1, $2, constant); }
	| '-' term	{ goto unop; }
	| NOTCOMPL term { unop:
			symbol_info($$, $1, $2, max($2.type, repeatable));
			if ($2.type == variable)
				add_trace($2, $2, $2, normal);
			}
	| INCOP term	{
			symbol_info($$, $1, $2, max($2.type, repeatable));
			if ($2.type != side_effect)
				add_trace($2, $1, $2, prefix);
			}
	| term INCOP	{
			symbol_info($$, $1, $2, max($1.type, repeatable));
			if ($1.type != side_effect)
				add_trace($1, $1, $2, postfix);
			}
	| cast term %prec INCOP {
			symbol_info($$, $1, $2, $2.type); }
	| term '[' exp ']' {
			symbol_info($$, $1, $4, max($1.type, $3.type));
			if ($$.type != side_effect)
				$$.type = variable;
			}
	| term '(' opt_exp ')' {
			symbol_info($$, $1, $4, side_effect);
			
			/* prevent pcc sizeof function compiler error */
			rm_trace($1);
			}
	| MACRO '(' opt_exp ')' {
			rm_all_trace($3);
			symbol_info($$, $1, $4, side_effect); }
	| MACRO {
			symbol_info($$, $1, $1, side_effect); }
	| strfcn
	| term DOTARROW ident {
			symbol_info($$, $1, $3, $1.type);
			expand_trace($1, $1, $3);
			}
	| IDENT		{
			$$.type = variable; }
	| FUNCTION	{
			$$.type = constant; }
	| strfcn_name	{
			$$.type = constant; }
	| CONSTANT	{
			$$.type = constant; }
	| '(' exp ')'	{
			symbol_info($$, $1, $3, $2.type);
			if ($2.type == variable) /* don't expand ++a to (++a) */
				expand_trace($2, $1, $3);
			}
	;
strfcn_name :
	  STRRES
	| STRCAT
	| STRNCAT
	| STRCPY
	| STRCMP
	| STRNCMP
	| STRLEN
	;
strfcn	: STRRES '(' arg opt_args ')' {
			symbol_info($$, $1, $5, side_effect);
			if ($3.type != side_effect)
				add_trace($3, $1, $5, strres); /* result */
			}
	| STRCAT '(' arg ',' arg ')' {
			symbol_info($$, $1, $6, side_effect);
			if ($3.type != side_effect) /* $5 is evaluated only once so it can have side effects */
				add_trace($3, $1, $6, string); /* result */
			if ($5.type != side_effect && $5.type != constant)
				add_trace($5, $5, $5, string); /* arg 2 */
			}
	| STRNCAT '(' arg ',' arg ',' arg ')' {
			symbol_info($$, $1, $8, side_effect);
			if ($3.type != side_effect)
				add_trace($3, $1, $8, string); /* result */
			}
	| STRCPY '(' arg ',' arg ')' {
			symbol_info($$, $1, $6, side_effect);
			if ($3.type != side_effect) {

				/* don't trace strcpy(a, "b") */
				if (suppress && $5.type == constant) {
					$$.type = constant;
					rm_trace($3);
				}
				else {
					/* don't trace b in strcpy(a, b); */
					if (suppress && $5.type == variable) {
						$$.type = repeatable; /* prevent term rule trace */
						rm_trace($5);
					}
					else if ($5.type != side_effect && $5.type != constant)
						add_trace($5, $5, $5, string); /* arg 2 */
					/* trace the assigned variable */
					add_trace($3, $1, $6, string); /* result */
				}
			}
			}
	| int_strfcn	{ /* trace a repeatable string function result */
			if ($1.type != side_effect)
				add_trace($1, $1, $1, normal); /* result */
			}
	;
int_strfcn :
	  STRCMP '(' arg ',' arg ')' {
			symbol_info($$, $1, $6, max($3.type, $5.type));
			if ($3.type != side_effect && $3.type != constant)
				add_trace($3, $3, $3, string); /* arg 1 */
			if ($5.type != side_effect && $5.type != constant)
				add_trace($5, $5, $5, string); /* arg 2 */
			}
	| STRNCMP '(' arg ',' arg ',' arg ')' {
			symbol_info($$, $1, $8, max3($3.type, $5.type, $7.type));
			}
	| STRLEN '(' arg ')' {
			symbol_info($$, $1, $4, $3.type);
			if ($3.type != side_effect && $3.type != constant)
				add_trace($3, $3, $3, string); /* arg 1 */
			}
	;
arg	: exp %prec ','
	;
opt_args : /* none */
	| ',' exp
	;
cast	: '(' type null_dcl ')' {
			symbol_info($$, $1, $4, constant); }
	;
null_dcl : /* empty */	
	| '*' null_dcl	
	| null_dcl '[' opt_exp ']' 
/* these two rules: */
	| '(' ')'	
	| '(' null_dcl ')' '(' ')' 
/* are needed in place of: */
/*	| null_dcl '(' ')' */
/* to prevent 2 out of 3 shift/reduce conflicts */
	| '(' null_dcl ')' 
	;
ident	: IDENT
	| FUNCTION
	;
%%
static
yyerror(s)
char *s;
{
	if (last_yychar == MACRO    || yychar == MACRO
	 || last_yychar == PP_IF    || yychar == PP_IF
	 || last_yychar == PP_ELSE  || yychar == PP_ELSE
	 || last_yychar == PP_ENDIF || yychar == PP_ENDIF) {
		fatal("cannot handle preprocessor code, use -P option");
	}
	else if (strcmp(s, "yacc stack overflow") == 0) {
		fatal("'if ... else if' sequence too long");
	}
	else if (strcmp(s, "syntax error") == 0) {
		error("possible syntax error, try -P option");
	}
	else {	/* no other known yacc errors, but better be safe than sorry */
		fatal(s);
	}
}
