%{
#ifndef lint
static  char scanner_SID[] =
	"@(#)scanner.l 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>

extern int lineno;
struct tokdope
{
	char *keyword;
	int token;
	int tokentype;
};
struct tokdope *keylook();
static struct tokdope *curcomtok;
#define	STRING		0
#define	DTYPE		1
#define	SPECTOK		2
#define	SIZE		3
#define	STRINGLIST	4
#define	BOOLEAN		5
%}
%Start	EATUP DOSTRING DOSTRINGLIST DOSIZE DOBOOLEAN DODTYPE FKIND
W	[ \t]+
OW	{W}?
FALSE	{OW}false{OW}$
TRUE	{OW}true{OW}$
%%
^[^#\*]		{
			yyerror("Unknown Stuff");
			BEGIN EATUP;
		}
^#.*$		;
\n		{ lineno++; }
^\*=.*\n	{
			lineno++;
			return(SEPTOK);
		}
^\*[-a-zA-Z_]+=	{
			int c;

			curcomtok = keylook(yytext);
			if(!curcomtok)
			{
				yyerror("Unknown Keyword %s",yytext);
				BEGIN EATUP;
			}
			else if((c = input()) == '\n')
			{
				unput(c);
				BEGIN 0;
				makenulltok(&yylval,curcomtok);
				return(curcomtok->token);
			}

			unput(c);
			switch(curcomtok->tokentype)
			{
			case STRING:
				BEGIN DOSTRING;
				break;
			case STRINGLIST:
				BEGIN DOSTRINGLIST;
				break;
			case SIZE:
				BEGIN DOSIZE;
				break;
			case BOOLEAN:
				BEGIN DOBOOLEAN;
				break;
			case SPECTOK:
				BEGIN FKIND;
				break;
			case DTYPE:
				BEGIN DODTYPE;
				break;
			default:
				yyfatal("Unknown tokentype at line %d",
					lineno+1);
				/*NORETURN*/
			}
		}
<DOBOOLEAN>{TRUE}	{
			yylval.ybool = TRUE;
			BEGIN 0;
			return(curcomtok->token);
		}
<DOBOOLEAN>{FALSE}	{
			yylval.ybool = FALSE;
			BEGIN 0;
			return(curcomtok->token);
		}
<DOBOOLEAN>.	{
		yyerror("Did not find required boolean value for command %s",
			curcomtok->keyword);
			yylval.ybool = FALSE;
			BEGIN EATUP;
			return(curcomtok->token);
		}
<DOSIZE>.*$	{
			yylval.ylong = munchval(yytext);
			BEGIN 0;
			return(curcomtok->token);
		}
<DOSTRING>.*$	{
#ifdef	NOT
			if((yytext[yyleng-1] == '\\' && yyleng == 1) ||
				(yyleng > 1 && yytext[yyleng-1] == '\\' &&
				yytext[yyleng-2] != '\\'))
			{
				lineno++;
				yytext[yyleng-2] = yytext[yyleng-1];
				yytext[--yyleng] = 0;
				yymore();
			}
			else
#endif
			{
				char *p;
				for(p = yytext; *p == ' ' || *p == '\t'; p++)
					;
				if(*p)
				{
					char *q;
					for(q = p; *q; q++)
						;
					--q;
					while(*q == ' ' || *q == '\t')
						q--;
					*(++q) = '\0';
					yylval.ystring = newstring(p);
				}
				else
					yylval.ystring = newstring("");
				BEGIN 0;
				return(curcomtok->token);
			}
		}
<DOSTRINGLIST>.*\n	{
			lineno++;
#ifdef	NOT
			if((yytext[yyleng-1] == '\\' && yyleng == 1) ||
				(yyleng > 1 && yytext[yyleng-1] == '\\' &&
				yytext[yyleng-2] != '\\'))
			{
				yytext[yyleng-2] = yytext[yyleng-1];
				yytext[--yyleng] = 0;
				yymore();
			}
			else
#endif
			{
				string_list parselist();
				yylval.ystrings = parselist(yytext);
				BEGIN 0;
				return(curcomtok->token);
			}
		}
<DODTYPE>{OW}tape{OW}$	{
				yylval.ydkind = TAPE;
				BEGIN 0;
				return(DEVTYPTOK);
		}
<DODTYPE>{OW}disk{OW}$	{
				yylval.ydkind = DISK;
				BEGIN 0;
				return(DEVTYPTOK);
		}
<DODTYPE>.	{
			yylval.ydkind = STREAM;
			yyerror("No device specified. Assuming Stream");
			BEGIN EATUP;
			return(DEVTYPTOK);
		}
<FKIND>{OW}amorphous{OW}$	{
		BEGIN 0; yylval.yfkind = AMORPHOUS; return(AMORPHOUSTOK); }
<FKIND>{OW}standalone{OW}$	{ 
	BEGIN 0; yylval.yfkind = STANDALONE; return(STANDALONETOK); }
<FKIND>{OW}executable{OW}$	{ BEGIN 0; yylval.yfkind = EXECUTABLE;
					return(EXECUTABLETOK); }
<FKIND>{OW}installable{OW}$	{ BEGIN 0; yylval.yfkind = INSTALLABLE;
					return(INSTALLABLETOK); }
<FKIND>{OW}install-tool{OW}$	{ BEGIN 0; yylval.yfkind = INSTALL_TOOL;
					return(INSTALLTOOLTOK); }
<FKIND>{OW}.	{
			yyerror("Unknown File-Kind");
			BEGIN EATUP;
			return(0);
		}
<EATUP>.*$	{
			BEGIN 0;
		}
%%

static struct tokdope dopetab[] =
{
	"*Title=",		TITLETOK,	STRING,	
	"*Version=",		VERSIONTOK,	STRING,
	"*Date=",		DATETOK,	STRING,
	"*Source=",		SOURCETOK,	STRING,
	"*Name=",		NAMETOK,	STRING,
	"*Description=",	DESCTOK,	STRING,
	"*File-Type=",		FILTYPTOK,	STRING,
	"*File-Kind=",		FILKINDTOK,	SPECTOK,
	"*Size=",		SIZETOK,	STRING,
	"*Architectures=",	ARCHTOK,	STRINGLIST,
	"*Depends-on=",		DEPENDSTOK,	STRINGLIST,
	"*Required=",		REQUIREDTOK,	BOOLEAN,
	"*Desirable=",		DESIRABLETOK,	BOOLEAN,
	"*Common=",		COMMONTOK,	BOOLEAN,
	"*Loadpoint=",		LOADPTTOK,	STRING,
	"*Moveable=",		MOVEABLETOK,	BOOLEAN,
	"*Pre-install=",	PREINSTALLTOK,	STRING,
	"*Install=",		INSTALLTOK,	STRING,
	"*Belongs-to=",		BELONGSTOK,	STRINGLIST,
	"*Worksize=",		WORKSIZETOK,	SIZE,
	"*Command=",		COMMANDTOK,	STRING,
	"*Device-Type=",	DEVTYPTOK,	DTYPE,
	"*Device-Size=",	DSIZTOK,	SIZE,
	"*Device-Rname=",	DNAMTOK,	STRING,
	0
};

static struct tokdope *
keylook(word)
register char *word;
{
	register struct tokdope *tp = dopetab;

	while(tp->keyword)
		if(strcmp(tp->keyword,word) == 0)
			return(tp);
		else
			tp++;
	return((struct tokdope *) 0);
}

#define is_span(X)	((X) == ' ' || (X) == '\n' || (X) == '\t')
#define is_nspan(X)	((X) && (X) != ' ' && (X) != '\n' && (X) != '\t')
string_list
parselist(string)
register char *string;
{
	extern char *malloc();
	int nstrings;
	register string_list *sptr;
	register char **v, *p;
	char tmp;

	sptr = (string_list *) malloc(sizeof (string_list));

	sptr->string_list_len = 0;
	sptr->string_list_val = v = (char **) malloc (sizeof (char **));
	*v = newstring("");

	while(*string)
	{
		while(is_span(*string))
			string++;
		if(!*string)
			break;
		if(*string == '"')
		{
			string++;
			for(;;)
			{
				for(p = string; *p && *p != '"'; p++)
					;
				if(!*p || p == string)
					break;
				else if(!(*p == '"' && *(p-1) == '\\'))
					break;
			}
		}
		else
			for(p = string; is_nspan(*p); p++)
				;
		tmp = *p; *p = 0;
		sptr->string_list_val = (char **) realloc(sptr->string_list_val,
			(sptr->string_list_len+1) * (sizeof (char **)));
		v = sptr->string_list_val + sptr->string_list_len;
		sptr->string_list_len++;
		*v = malloc(p-string);
		strcpy(*v,string);
		*++v = newstring("");
		*p = tmp;
		string = p;
	}
	return(*sptr);
}

static
makenulltok(yp,tp)
register YYSTYPE *yp;
register struct tokdope *tp;
{
	switch(tp->tokentype)
	{
	case STRING:
		/*
		 * make a zero length string so xdr doesn't choke
		 */
		yp->ystring = newstring("");
		break;
	case STRINGLIST:
		yylval.ystrings = parselist("");
		break;
	case SIZE:
		yyfatal("Missing SIZE specification");
		break;
	case BOOLEAN:
		yyfatal("Missing true/false specification");
		break;
	case SPECTOK:
		yyfatal("Missing keyword");
		break;
	case DTYPE:
		yyfatal("Missing device specification");
		break;
	}
}

int munchval(str)
char *str;
{
	register tmp, rval;
	register char *ptr;
	char *eval();

	while (isspace(*str))
		str++;

	if (!isdigit(*str)) {
		str = eval(str);  /* run a command to evaluate the string */
		while (isspace(*str))
			str++;
	}

	ptr = str;
	while (isdigit(*ptr))
		ptr++;
	tmp = *ptr;
	*ptr = 0;

	rval = atoi(str);
	*ptr = tmp;
	if(*ptr == 'k' && *(ptr+1) == 'b')
		rval <<= 10;
	if(*ptr == 'm' && *(ptr+1) == 'b')
		rval <<= 20;
	return rval;
}

char *
eval(p)
	char *p;
{
	FILE *f;
	static char a[16];

	f = popen(p, "r");
	if (f == NULL) {
		perror(p);
		yyfatal("Couldn't evaluate command");
	}
	if (read(fileno(f), a, 16) <= 0) {
		perror(p);
		yyfatal("Couldn't evaluate command");
	}
	if (pclose(f) < 0) {
		perror(p);
		yyfatal("Couldn't evaluate command");
	}
	a[15] = '\0';
	return a;
}
