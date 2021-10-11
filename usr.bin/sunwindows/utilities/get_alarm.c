#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)get_alarm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif
#endif

#include	<stdio.h>

/*
 *  get_alarm.c
 *  This program gets the WIN_ALARM env var.
 */
main ()
{
	char		*wa_envstr;
	extern	char	*getenv();

	if ( (wa_envstr = getenv ("WINDOW_ALARM")) == NULL ) {
                fprintf (stdout, "get_alarm:  the WINDOW_ALARM environment variable is NULL.\n");
	exit(1);
        }
	else 
		fprintf(stdout, "WINDOW_ALARM=%s\n", wa_envstr);
	exit(0);
}
