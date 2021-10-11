#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)issecure.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <sys/file.h>

#define PWDADJ	"/etc/security/passwd.adjunct"

/*
 * Is this a secure system ?
 */
issecure()
{
	static int	securestate	= -1;

	if (securestate == -1)
		securestate = (access(PWDADJ, F_OK) == 0);
	return(securestate);
}
