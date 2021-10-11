%term COLON DOT EQUAL FLAGS GARBAGE NAME NAMELIST SHELLCMD STATE EQSTRING

%{
#include "defs.h"
#include <ctype.h>

extern char	yytext[];
extern int	yyleng,
		yylineno;

STATEBLOCK	firststate;
%}

%union {
	char		*sval;
	int		ival;
	SHBLOCK		shval;
	STATEBLOCK	stval;
	NAMEBLOCK	nlval;
	}

%type <sval>	name string
%type <stval>	statetag rule statename
%type <shval>	shline command cmdlist
%type <ival>	flags
%type <nlval>	onename namelist
%%

file:
	| file line
	;
line:	 
	  macrodef
	| rule
	| error = { yyerrok; }
	;

macrodef:
	  name string
		{
			setvar($1,$2);
			free((char *)$1);
		}
	;

string:
	  EQSTRING
		{
			register char	*p;

			p = yytext;
			while( isspace(*p) )
				++p;
			if( *p++ != '=' )
				fatal("scanner error");
			while( isspace(*p) )
				++p;

			if( yytext[yyleng-1] == '\n')
				yytext[yyleng-1] = '\0'; /* zap final '\n' */

			$$ = p = copys(p);
			while( *p )
			{
				if( *p == '\n' )
					*p = ' ';	/* '\n' becomes ' ' */
				++p;
			}
		}
	;
rule:	 
	  statetag cmdlist
		{
			$1->commands = $2;
			$1->next = firststate;
			firststate = $1;
		}
	;

statetag:
	  statename COLON namelist
		{
			$1->othercommands = $3;
		}
	;

statename:
	  STATE
		{
			if( findstate(yytext) != (STATEBLOCK)0 )
				fatal("state redefined");

			$$ = ALLOC(stateblock);
			$$->name = copys(yytext);
		}
	;

namelist:
		{
			$$ = (NAMEBLOCK)0;
		}
	| onename namelist
		{
			$1->nextname = $2;
		}
	;

onename:
	  NAMELIST
		{
			$$ = ALLOC(nameblock);
			$$->namep = copys(yytext);
			$$->namelen = yyleng;
		}
	;

cmdlist: 
		{
			$$ = (SHBLOCK)0;
		}
	| command cmdlist
		{
			$1->nextsh = $2;
		}
	;

command: 
	  flags shline =
		{
			$2->flags = $1;
			$$ = $2;
		}
	| shline
	;

flags:	  FLAGS =
		{
			register char	*p;
			int	flags = 0;

			for( p = yytext; *p; ++p )
			{
				switch(*p)
				{
					case '@':
						flags |= SH_NOECHO;
						break;
					case '-':
						flags |= SH_NOSTOP;
						break;
				}
			}
			$$ = flags;
		}

shline:	  SHELLCMD =
		{
			$$ = ALLOC(shblock);
			$$->shbp = copys(yytext);
		}
	;

name:	  NAME =
		{
			$$ = copys(yytext);
		}
	;
%%

yyerror(msg)
	char	*msg;
{
	fprintf(stderr, "parser error: %s\n", msg);
	fprintf(stderr, "error occurred on line %d near \"%s\"\n",
		yylineno, yytext);
	exit(1);
}
