#ifndef lint
static	char sccsid[] = "@(#)libzer.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

# include <stdio.h>

yyerror( s ) char *s; {
	fprintf(  stderr, "%s\n", s );
	}
