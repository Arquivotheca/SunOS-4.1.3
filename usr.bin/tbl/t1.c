#ifndef lint
static	char sccsid[] = "@(#)t1.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/07/22 */
#endif

 /* t1.c: main control and input switching */
#
# include "t..c"
#include <signal.h>
# ifdef gcos
/* required by GCOS because file is passed to "tbl" by troff preprocessor */
# define _f1 _f
extern FILE *_f[];
# endif

# ifdef unix
# define MACROS "/usr/lib/tmac/tmac.s"
# define PYMACS "/usr/lib/tmac/tmac.m"
# endif

# ifdef gcos
# define MACROS "cc/troff/smac"
# define PYMACS "cc/troff/mmac"
# endif

# define ever (;;)

main(argc,argv)
	char *argv[];
{
# ifdef unix
int badsig();
signal(SIGPIPE, badsig);
# endif
# ifdef gcos
if(!intss()) tabout = fopen("qq", "w"); /* default media code is type 5 */
# endif
exit(tbl(argc,argv));
/* NOTREACHED */
}


tbl(argc,argv)
	char *argv[];
{
char line[BUFSIZ];
/* required by GCOS because "stdout" is set by troff preprocessor */
tabin=stdin; tabout=stdout;
setinp(argc,argv);
while (gets1(line, sizeof line))
	{
	fprintf(tabout, "%s\n",line);
	if (prefix(".TS", line))
		tableput();
	}
fclose(tabin);
return(0);
}
int sargc;
char **sargv;
setinp(argc,argv)
	char **argv;
{
	sargc = argc;
	sargv = argv;
	sargc--; sargv++;
	if (sargc>0)
		swapin();
}
swapin()
{
	while (sargc>0 && **sargv=='-') /* Mem fault if no test on sargc */
		{
		if (sargc<=0) return(0);
		if (match("-me", *sargv))
			{
			*sargv = "/usr/lib/tmac/tmac.e";
			break;
			}
		if (match("-ms", *sargv))
			{
			*sargv = MACROS;
			break;
			}
		if (match("-mm", *sargv))
			{
			*sargv = PYMACS;
			break;
			}
		if (match("-TX", *sargv))
			pr1403=1;
		sargc--; sargv++;
		}
	if (sargc<=0) return(0);
# ifdef unix
/* file closing is done by GCOS troff preprocessor */
	if (tabin!=stdin) fclose(tabin);
# endif
	tabin = fopen(ifile= *sargv, "r");
	iline=1;
# ifdef unix
/* file names are all put into f. by the GCOS troff preprocessor */
	fprintf(tabout, ".ds f. %s\n",ifile);
# endif
	if (tabin==NULL)
		error("Can't open file");
	sargc--;
	sargv++;
	return(1);
}
# ifdef unix
badsig()
{
signal(SIGPIPE, SIG_IGN);
 exit(0);
}
# endif
