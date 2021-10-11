#ifndef lint
static	char sccsid[] = "@(#)token.c 1.1 92/07/30 SMI; from UCB 4.1 82/12/24";
#endif

#include "awk.h"
struct tok
{	char *tnm;
	int yval;
} tok[]	= {
#include "tokendefs"
};
ptoken(n)
{
	if(n<128) printf("lex: %c\n",n);
	else	if(n<=256) printf("lex:? %o\n",n);
	else	if(n<LASTTOKEN) printf("lex: %s\n",tok[n-257].tnm);
	else	printf("lex:? %o\n",n);
	return;
}

char *tokname(n)
{
	if (n<=256 || n >= LASTTOKEN)
		n = 257;
	return(tok[n-257].tnm);
}
