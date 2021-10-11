#include "defs.h"
#include <sys/stat.h>

docom(q)
	SHBLOCK         q;
{
	extern char	*subst();
	char            string[OUTMAX];

	if (IS_ON(QUEST))
		return (0);

	for (; q != 0; q = q->nextsh)
	{
		char		makecall;
		int             ign,
				nopr;

		/*
		 * Allow recursive makes to execute only if the
		 * NOEX flag set 
		 */
/*
		if (sindex(q->shbp, "$(MAKE)") != -1 && IS_ON(NOEX))
			makecall = YES;
		else
*/
			makecall = NO;

		(void) subst(q->shbp, string);


		if( (q->flags & SH_NOECHO) == SH_NOECHO )
			nopr = YES;
		else
			nopr = NO;

		if( IS_ON(IGNERR) || (q->flags & SH_NOSTOP) == SH_NOSTOP )
			ign = YES;
		else
			ign = NO;

		if (docom1(string, ign, nopr, makecall) && !ign)
			if (IS_ON(KEEPGO))
				return (1);
			else
				fatal((char *) 0);
	}
	return (0);
}

static
docom1(comstring, nohalt, noprint, makecall)
	register char	*comstring;
	int             nohalt,
	                noprint;
	char		makecall;
{
	register int    status;

	if (comstring[0] == '\0')
		return (0);

	if (IS_OFF(SIL) && (!noprint || IS_ON(NOEX)))
	{
		char	*p1,
			*ps;

		ps = p1 = comstring;
		while (1)
		{
			/* make sure that the command is echoed with a '\n'
			 * at the end.
			 */
			while (*p1 && *p1 != '\n')
				p1++;
			if (*p1)
			{
				*p1 = 0;
				(void) printf("%s\n", ps);
				*p1 = '\n';
				ps = p1 + 1;
				p1 = ps;
			}
			else
			{
				(void) printf("%s\n", ps);
				break;
			}
		}

		(void) fflush(stdout);
	}

	if (IS_ON(NOEX) && makecall == NO)
		status = 0;
	else
		status = dosys(comstring, nohalt);

	if (status != 0)
	{
		if (status >> 8)
			(void) printf("*** Error code %d", status >> 8);
		else
		{
			extern char    *sys_siglist[];
			int             coredumped;

			coredumped = status & 0200;
			status &= 0177;
			if (status > NSIG)
				(void) printf("*** Signal %d", status);
			else
				(void) printf("*** %s", sys_siglist[status]);
			if (coredumped)
				(void) printf(" - core dumped");
		}

		if (nohalt)
			(void) printf(" (ignored)\n");
		else
			(void) printf("\n");
		(void) fflush(stdout);
	}

	return (status);
}
