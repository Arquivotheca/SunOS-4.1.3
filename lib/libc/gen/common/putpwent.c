#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)putpwent.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * format a password file entry
 */
#include <stdio.h>
#include <pwd.h>

extern int fprintf();

int
putpwent(p, f)
register struct passwd *p;
register FILE *f;
{
	(void) fprintf(f, "%s:%s", p->pw_name, p->pw_passwd);
	if((*p->pw_age) != '\0')
		(void) fprintf(f, ",%s", p->pw_age);
	(void) fprintf(f, ":%d:%d:%s:%s:%s\n",
		p->pw_uid,
		p->pw_gid,
		p->pw_gecos,
		p->pw_dir,
		p->pw_shell);
	return(ferror(f));
}
