/*	@(#)nl_cxtime.c 1.1 92/07/30 SMI*/


#include <stdio.h>
#include <time.h>
#include <langinfo.h>
#include <locale.h>

#define TBUFSIZE 128
char _tbuf[TBUFSIZE];

static char *
_setfmt(fmt)
	char *fmt;
{
	struct dtconv *dt;

	if (fmt != (char *)NULL) 
		return (fmt);
	dt = localdtconv();
	return (dt->time_format);
}
	
	
char *
nl_cxtime(clk, fmt)
	struct tm *clk;
	char *fmt;
{
	char *nl_ascxtime();
	return (nl_ascxtime(localtime(clk), fmt));
}

char *
nl_ascxtime(tmptr, fmt) 
	struct tm *tmptr;
	char *fmt;
{
	return (strftime (_tbuf, TBUFSIZE, _setfmt(fmt), tmptr) ?
			 _tbuf : asctime(tmptr));
}

