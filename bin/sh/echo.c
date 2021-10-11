/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)echo.c 1.1 92/07/30 SMI"; /* from S5R3 1.6 */
#endif

/*
 *	UNIX shell
 */
#include	"defs.h"

#define	exit(a)	flushb();return(a)

extern int exitval;

echo(argc, argv)
char **argv;
{
	if (s5builtins)
	{
		register char	*cp;
		register int	i, wd;
		int	j;
	
		if(--argc == 0) {
			prc_buff('\n');
			exit(0);
		}

		for(i = 1; i <= argc; i++) 
		{
			sigchk();
			for(cp = argv[i]; *cp; cp++) 
			{
				if(*cp == '\\')
				switch(*++cp) 
				{
					case 'b':
						prc_buff('\b');
						continue;

					case 'c':
						exit(0);

					case 'f':
						prc_buff('\f');
						continue;

					case 'n':
						prc_buff('\n');
						continue;

					case 'r':
						prc_buff('\r');
						continue;

					case 't':
						prc_buff('\t');
						continue;

					case 'v':
						prc_buff('\v');
						continue;

					case '\\':
						prc_buff('\\');
						continue;
					case '0':
						j = wd = 0;
						while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
							wd <<= 3;
							wd |= (*cp - '0');
						}
						prc_buff(wd);
						--cp;
						continue;

					default:
						cp--;
				}
				prc_buff(*cp);
			}
			prc_buff(i == argc? '\n': ' ');
		}
		exit(0);
	}
	else
	{
		register int i, nflg;

		nflg = 0;
		if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n') {
			nflg++;
			argc--;
			argv++;
		}
		for(i=1; i<argc; i++) {
			sigchk();
			prs_buff(argv[i]);
			if (i < argc-1)
				prc_buff(' ');
		}
		if(nflg == 0)
			prc_buff('\n');
		exit(0);
	}
}
