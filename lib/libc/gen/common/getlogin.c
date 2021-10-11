#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getlogin.c 1.1 92/07/30 SMI"; /* from UCB 4.2 82/11/14 */
#endif

#include <utmp.h>

static	char *UTMP	= "/etc/utmp";
static	struct utmp *ubufp;

char *
getlogin()
{
	register int me, uf;
	register char *cp;

#ifdef S5EMUL
	if ((me = ttyslot()) < 0)
#else
	if (!(me = ttyslot()))
#endif
		return(0);
	if (ubufp == 0) {
		ubufp = (struct utmp *)malloc(sizeof (*ubufp));
		if (ubufp == 0)
			return (0);
	}
	if ((uf = open(UTMP, 0)) < 0)
		return (0);
	lseek (uf, (long)(me*sizeof(*ubufp)), 0);
	if (read(uf, (char *)ubufp, sizeof (*ubufp)) != sizeof (*ubufp)) {
		close(uf);
		return (0);
	}
	close(uf);
	if (ubufp->ut_name[0] == '\0')
		return (0);
	ubufp->ut_name[sizeof (ubufp->ut_name)] = ' ';
	for (cp = ubufp->ut_name; *cp++ != ' '; )
		;
	*--cp = '\0';
	if (ubufp->ut_name[0] == '\0')
		return (0);
	return (ubufp->ut_name);
}
