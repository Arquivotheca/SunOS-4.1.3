%{
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)config.l 1.1 92/07/30 SMI"; /* from UCB 5.2 4/18/86 */
#endif

#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define tprintf if (do_trace) printf

/*
 * Key word table
 */

struct kt {
	char *kt_name;
	int kt_val;
} key_words[] = {
	{ "and",		AND },
	{ "at",			AT },
	{ "config",		CONFIG },
	{ "channel",		CHANNEL },
	{ "controller",		CONTROLLER },
	{ "cpu",		CPU },
	{ "csr",		CSR },
	{ "device",		DEVICE },
	{ "device-driver",	DEVICE_DRIVER },
	{ "disk",		DISK },
	{ "drive",		DRIVE },
	{ "dst",		DST },
	{ "dumps",		DUMPS },
	{ "flags",		FLAGS },
	{ "ident",		IDENT },
	{ "init",		INIT },
	{ "ipi",		IPI },
	{ "ipi_addr",		IPI_ADDR },
	{ "lun",		LUN },
	{ "machine",		MACHINE },
	{ "major",		MAJOR },
	{ "makeoptions",	MAKEOPTIONS },
	{ "master",		MASTER },
	{ "maxusers",		MAXUSERS },
	{ "mba",		MBA },
	{ "minor",		MINOR },
	{ "nexus",		NEXUS },
	{ "on",			ON },
	{ "options",		OPTIONS },
	{ "priority",		PRIORITY },
	{ "pseudo-device",	PSEUDO_DEVICE },
	{ "root",		ROOT },
	{ "scsibus",		SCSIBUS },
	{ "size",		SIZE },
	{ "slave",		SLAVE },
	{ "swap",		SWAP },
	{ "tape",		DEVICE },
	{ "target",		TARGET },
	{ "trace",		TRACE },
	{ "type",		TYPE },
	{ "uba",		UBA },
	{ "vector",		VECTOR },
	{ 0, 0 },
};
%}
%START	FILENAME
WORD	[A-Za-z_][-A-Za-z_]*
ALPHANUM	[A-Za-z_][-A-Za-z0-9_]*
%%
<FILENAME>{ALPHANUM}	{
			int i;

			if ((i = kw_lookup(yytext)) == -1) {
				yylval.str = yytext;
				tprintf("id(%s) ", yytext);
				return ID;
			}
			if (i == ON) {
				BEGIN FILENAME;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
{WORD}		{
			int i;

			if ((i = kw_lookup(yytext)) == -1) {
				yylval.str = yytext;
				tprintf("id(%s) ", yytext);
				return ID;
			}
			if (i == ON) {
				BEGIN FILENAME;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
\"[^"]+\"	{
			yytext[strlen(yytext)-1] = '\0';
			yylval.str = yytext + 1;
			return ID;
		}
0[0-7]*		{
			yylval.val = octal(yytext);
			tprintf("#O:%o ", yylval.val);
			return NUMBER;
		}
0x[0-9a-fA-F]+	{
			yylval.val = hex(yytext);
			tprintf("#X:%x ", yylval.val);
			return NUMBER;
		}
[1-9][0-9]*	{
			yylval.val = atoi(yytext);
			tprintf("#D:%d ", yylval.val);
			return NUMBER;
		}
[0-9]*"."[0-9]*	{
			double atof();
			yylval.val = (int) (60 * atof(yytext) + 0.5);
			return FPNUMBER;
		}
"-"		{
			return MINUS;
		}
"?"		{
			yylval.val = QUES;
			tprintf("? ");
			return NUMBER;
		}
\n/[ \t]	{
			yyline++;
			tprintf("\n... ");
		}
\n		{
			yyline++;
			tprintf("\n");
			BEGIN 0;
			return SEMICOLON;
		}
#.*		{	/* Ignored (comment) */;	}
[ \t]*		{	/* Ignored (white space) */;	}
";"		{	BEGIN 0; return SEMICOLON;	}
","		{	return COMMA;			}
"="		{	return EQUALS;			}
"@"		{	return AT;			}
.		{	return yytext[0];		}

vme16		{	return VME16D16;		}
vme24		{	return VME24D16;		}
vme32		{	return VME32D32;		}

vme16d16	{	return VME16D16;		}
vme24d16	{	return VME24D16;		}
vme32d16	{	return VME32D16;		}
vme16d32	{	return VME16D32;		}
vme24d32	{	return VME24D32;		}
vme32d32	{	return VME32D32;		}

%%

yyline = 1;	/* start line numbering at 1 */

/*
 * kw_lookup
 *	Look up a string in the keyword table.  Returns a -1 if the
 *	string is not a keyword otherwise it returns the keyword number
 */
kw_lookup(word)
register char *word;
{
	register struct kt *kp;

	for (kp = key_words; kp->kt_name != 0; kp++)
		if (eq(word, kp->kt_name))
			return kp->kt_val;
	return -1;
}

/*
 * Number conversion routines
 */

octal(str)
char *str;
{
	int num;

	(void)sscanf(str, "%o", &num);
	return num;
}

hex(str)
char *str;
{
	int num;

	(void)sscanf(str+2, "%x", &num);
	return num;
}
