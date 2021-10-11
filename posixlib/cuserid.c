#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)cuserid.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <pwd.h>

extern char *strcpy(), *getlogin();
extern int getuid();
extern struct passwd *getpwuid();
static char res[L_cuserid];

char *
cuserid(s)
char	*s;
{
	register struct passwd *pw;
	register char *p;

	if (s == NULL)
		s = res;
	pw = getpwuid(geteuid());
	endpwent();
	if (pw != NULL)
		return (strcpy(s, pw->pw_name));
	p = getlogin();
        if (p != NULL)
                return (strcpy(s, p));
	*s = '\0';
	return (NULL);
}
