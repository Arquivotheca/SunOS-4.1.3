%{
#ifndef lint
static  char sccsid[] = "@(#)parser.y 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "mktp.h"
#include <string.h>

typedef union
{
	char		*ystring;
	string_list	ystrings;
	bool_t		ybool;
	unsigned long	ylong;
	media_type	ydkind;
	file_kind	yfkind;
} YYSTYPE;
extern YYSTYPE yylval;

int yyanyerrors = 0;
int lineno;
%}
%token			FILKINDTOK SEPTOK
%token	<yfkind>	AMORPHOUSTOK STANDALONETOK EXECUTABLETOK INSTALLABLETOK
%token	<yfkind>	INSTALLTOOLTOK
%token	<ydkind>	DEVTYPTOK
%token	<ystring>	TITLETOK VERSIONTOK DATETOK SOURCETOK DLOCTOK
%token	<ystring>	DNAMTOK NAMETOK DESCTOK FILTYPTOK
%token	<ystring>	LOADPTTOK PREINSTALLTOK INSTALLTOK COMMANDTOK
%token	<ystrings>	BELONGSTOK ARCHTOK DEPENDSTOK
%token	<ybool>		REQUIREDTOK DESIRABLETOK COMMONTOK MOVEABLETOK
%token	<ylong>		WORKSIZETOK SIZETOK DSIZTOK

%%

descriptionfile	:	distinfo entryinfo
	| 	error
	{
		fprintf(stderr,"Could not parse input file\n");
		return(-1);
	}
	;

/*
 * Basic distribution Information
 */

distinfo:	TITLETOK VERSIONTOK DATETOK SOURCETOK
	{
		dinfo.Title = $1;
		dinfo.Version = $2;
		dinfo.Date = $3;
		dinfo.Source = $4;
		if($1 == (char *) 0 || $1 == '\0')
			yyerror("You must specify at least a Title");
	}
	;

entryinfo:	/* empty	*/
	|	entryinfo SEPTOK entry
	|	error
	;

entry	:	common1 variant common2
	{
		ep++;
	}
	;

common1	:	NAMETOK DESCTOK FILTYPTOK
	{
		ep->Name = $1;
		ep->LongName = $2;
		if(strcmp($3,"tar") == 0)
			ep->Type = TAR;
		else if(strcmp($3,"cpio") == 0)
			ep->Type = CPIO;
		else if(strcmp($3,"image") == 0)
			ep->Type = IMAGE;
		else if(strcmp($3,"bar") == 0)
			ep->Type = BAR;
		else if(strcmp($3,"tarZ") == 0)
			ep->Type = TARZ;
		else
			yyerror("Unknown File Type: %s",$3);
		free($3);
	}
	;

variant	:	AMORPHOUSTOK SIZETOK
	{
		ep->Info.kind = $1;
		ep->Info.Information_u.File.size = grabval((char *)ep->Name);
	}
	|	STANDALONETOK ARCHTOK SIZETOK
	{
		ep->Info.kind = $1;
		ep->Info.Information_u.Standalone.arch = $2;
		ep->Info.Information_u.Standalone.size =
		    grabval((char *)ep->Name);
	}
	|	EXECUTABLETOK ARCHTOK SIZETOK
	{
		ep->Info.kind = $1;
		ep->Info.Information_u.Standalone.arch = $2;
		ep->Info.Information_u.Standalone.size =
		    grabval((char *)ep->Name);
	}
	| INSTALLABLETOK ARCHTOK DEPENDSTOK REQUIREDTOK DESIRABLETOK
			COMMONTOK LOADPTTOK MOVEABLETOK SIZETOK PREINSTALLTOK
			INSTALLTOK
	{
		ep->Info.kind = $1;
		ep->Info.Information_u.Install.arch_for = $2;
		ep->Info.Information_u.Install.soft_depends = $3;
		ep->Info.Information_u.Install.required = $4;
		ep->Info.Information_u.Install.desirable = $5;
		ep->Info.Information_u.Install.common = $6;
		ep->Info.Information_u.Install.loadpoint = $7;
		ep->Info.Information_u.Install.moveable = $8;
		ep->Info.Information_u.Install.size =
		    grabval((char *)ep->Name);
		ep->Info.Information_u.Install.pre_install = $10;
		ep->Info.Information_u.Install.install = $11;
	}
	|	INSTALLTOOLTOK BELONGSTOK LOADPTTOK MOVEABLETOK SIZETOK
			WORKSIZETOK
	{
		ep->Info.kind = $1;
		ep->Info.Information_u.Tool.belongs_to = $2;
		ep->Info.Information_u.Tool.loadpoint = $3;
		ep->Info.Information_u.Tool.moveable = $4;
		ep->Info.Information_u.Tool.size =
		    grabval((char *)ep->Name);
		ep->Info.Information_u.Tool.workspace = $6;
	}
	;

common2:	COMMANDTOK DEVTYPTOK DSIZTOK DNAMTOK
	{
		extern char *rname;
		extern int volsize;

		ep->Command = $1;
		bzero((char *)&ep->Where,sizeof ep->Where);
		ep->Where.dtype = $2;
		if($2 == TAPE)
		{
			ep->Where.Address_u.tape.volsize = $3;
			ep->Where.Address_u.tape.dname = $4;
			ep->Where.Address_u.tape.label = newstring("VOLUMEX");
			ep->Where.Address_u.tape.volno =
				ep->Where.Address_u.tape.file_number =
				ep->Where.Address_u.tape.size = -1;
		}
		else if($2 == DISK)
		{
			ep->Where.Address_u.disk.volsize = $3;
			ep->Where.Address_u.disk.dname = $4;
			ep->Where.Address_u.disk.label = newstring("VOLUMEX");
			ep->Where.Address_u.disk.volno =
				ep->Where.Address_u.disk.offset =
				ep->Where.Address_u.disk.size = -1;
		}

		if(!rname)
			rname = newstring($4);
		if(volsize == -1)
			volsize = $3;

		if(strcmp($4,rname))
			yyerror("Sorry- only one device allowed for now\n");
		else if(volsize != $3)
			yyerror("Sorry- only one SIZE of device allowed now\n");
	}
	;
%%
#include "scanner.c"

yyfatal(s,a,b,c,d,e,f,g)
char *s;
{
	fprintf(stderr,s,a,b,c,d,e,f,g);
	fprintf(stderr,"\n");
	exit(-1);
}

yyerror(s,a,b,c,d,e,f,g)
char *s;
{
	fprintf(stderr,"Error at line %d: ",lineno+1);
	fprintf(stderr,s,a,b,c,d,e,f,g);
	fprintf(stderr,"\n");
	yyanyerrors = 1;
}

yywrap()
{
	return(1);
}

grabval(name)
char *name;
{
	FILE *fp;
	int sizeinkb;
	char *archenv, *archstring, sizefile[BUFSIZ], *getenv();

	archstring = getenv("ARCH");
	sprintf(sizefile, "./disksizes.%s/%s", archstring, name);

	if ((fp = fopen(sizefile, "r")) == NULL) {
		fprintf(stdout, " Can't open size file %s\n", sizefile);
		return(-1);
	}
	if (fscanf(fp, "%dkb", &sizeinkb) == NULL) {
		fprintf(stdout," Error reading size file %s\n", sizefile);
		return(-1);
	}

	fclose(fp);
	return(sizeinkb*1024);
}	
