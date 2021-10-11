%{
#include <stdio.h>
#include "defs.h"
#include "y.tab.h"
#ifdef LEXDEBUG
# define TOKEN(t)	printf("(%s=%s)\n", "t", yytext); return(t)
#else
# define TOKEN(t)	return(t)
#endif

#define OFF	0
#define ON	1
#undef YYLMAX 
#define YYLMAX  4096

char	*strcat(),
	*subst(),
	*strcpy();
%}

%START	cmds

%%

"#".*$		{ BEGIN 0; }

":"		{ TOKEN(COLON); }
"\n"		{ BEGIN 0; }

^[ \t]+		{ BEGIN cmds; }
<cmds>[-@]+	{ TOKEN(FLAGS); }
<cmds>[^-@].*\n { BEGIN 0; string(); TOKEN(SHELLCMD); }
[ \t]*"=".*	{ BEGIN 0; TOKEN(EQSTRING); }
[ \t]*"=".*\n	{ BEGIN 0; string(); TOKEN(EQSTRING); }

^[A-Za-z_][A-Za-z_.0-9]*/[ \t]*: { TOKEN(STATE); }
^[A-Za-z_][A-Za-z_.0-9]* { TOKEN(NAME); }
[A-Za-z_][A-Za-z_.0-9]* { TOKEN(NAMELIST); }
[ \t]+		;

.		{ TOKEN(GARBAGE) ; }

%%

string()
{
	if( yyleng > 2 && yytext[yyleng-2] == '\\' )
	{
		register char	*p;
		register int	c;

		yytext[--yyleng] = '\0';
		yytext[--yyleng] = '\n';
		p = yytext + yyleng;
		while( yyleng < YYLMAX )
		{
			if( (c = input()) == 0 )
				break;
			if( c == '\n' )
			{
				if( p[-1] == '\\' )
				{
					--yyleng;
					--p;
				}
				else
				{
					*p++ = c;
					*p = '\0';
					break;
				}
			}
			++yyleng;
			*p++ = c;
		}
		if( yyleng >= YYLMAX )
			fatal("line too long");
	}
	if( yytext[yyleng-1] == '\n' )
		yytext[--yyleng] = '\0';
}

char zzlinebuf[YYLMAX+1] = { '\0' };
char	*zzlineptr = zzlinebuf;

int
nextchar()
{
	if( *zzlineptr == '\0' )
	{
		if( nextline() == EOF )
			return(EOF);
		zzlineptr = zzlinebuf;
	}
	if( *zzlineptr == '\0' )
		return(EOF);
	else
		return(*zzlineptr++);
}

int
nextline()
{
	register char	*p;
	register int	c,
			n;


again:
	p = zzlinebuf;
	n = 0;

	/* fill up the line buffer */
	while((((c = getc(yyin)) == EOF) ? (c=stack_getc()) : c) != EOF && c != '\n' )
	{
		*p++ = c;
		if( ++n > sizeof(zzlinebuf) - 2 )
			fatal("line too long");
	}
	*p = '\0';
	if( p == zzlinebuf && c == EOF )
		return(EOF);
	*p++ = '\n';
	*p = '\0';

	/* are we supposed to be expanding macros? */
	if( zzlinebuf[0] != ' ' && zzlinebuf[0] != '\t' )
	{
		char	tempbuf[sizeof(zzlinebuf)];

		subst(zzlinebuf, tempbuf);	/* this could blow up */
		strcpy(zzlinebuf, tempbuf);
	}

/*
 *	DO include FILES HERE.
 */
	if(sindex(zzlinebuf, "include") == 0 && (zzlinebuf[7] == ' ' || zzlinebuf[7] == '\t'))
	{
		char	*pfile;

		for(p = &zzlinebuf[8]; *p != '\0'; p++)
			if(*p != '\t' || *p != ' ')
				break;

		pfile = p;
		for( ; (*p != '\0' && *p != '\n' && *p != '\t' && *p != ' '); p++)
			continue;
		if(*p)
			*p = '\0';

/*
 *	Start using new file.
 */
		fstack(pfile, &yyin, &yylineno);
		goto again;
	}
	return(0);
}

/*
 *	GETC() automatically unravels stacked include files. That is,
 *	during include file processing, when a new file is encountered
 *	fstack will stack the FILE pointer argument. Subsequent
 *	calls to GETC() with the new FILE pointer will get characters
 *	from the new file. When an EOF is encountered, GETC() will
 *	check to see if the file pointer has been stacked. If so,
 *	a character from the previous file will be returned.
 *	The external references are "GETC()" and "fstack(fname,stream,lno)".
 *	"Fstack(stfname,ream,lno)" is used to stack an old file pointer before
 *	the new file is assigned to the same variable. Also stacked are the
 *	file name and the old current lineno, generally, yylineno.
 */



static int morefiles;
static struct sfiles
{
	char sfname[64];
	FILE *sfilep;
	int syylno;
} sfiles[20];

stack_getc()
{
	register int c;

	c = EOF;
	while(c == EOF && morefiles)
	{
		(void)fclose(yyin);
		yylineno = sfiles[--morefiles].syylno;
		yyin = sfiles[morefiles].sfilep;
		c = getc(yyin);
	}
	return(c);
}

fstack(newname, oldfp, oldlno)
	register char	*newname;
	register FILE	**oldfp;
	register int	*oldlno;
{
	if(access(newname, 4) != 0)
		fatal1("Cannot read %s", newname);
/*
 *	Stack the new file name, the old file pointer and the
 *	old yylineno;
 */
	strcat(sfiles[morefiles].sfname, newname);
	sfiles[morefiles].sfilep = *oldfp;
	sfiles[morefiles++].syylno = *oldlno;
	yylineno = 0;
	if((*oldfp=fopen(newname, "r")) == NULL)
		fatal1("Cannot open %s", newname);
}

#undef input

input()
{
	if( yysptr > yysbuf )
		yytchar = *--yysptr;
	else
		yytchar = nextchar();

	if( yytchar == '\n' )
		++yylineno;

	if( yytchar == EOF )
		return(0);
	else
		return(yytchar);
}


