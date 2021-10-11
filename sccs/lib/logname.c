# include	<pwd.h>
# include	<sys/types.h>
# include	"../hdr/macros.h"

SCCSID(@(#)logname.c 1.1 92/07/30 SMI); /* from System III 5.1 */

char	*logname()
{
	register char *n;
	extern struct passwd *getpwuid();
	register struct passwd *pw;
	static char name[16];

	pw = getpwuid(getuid());
	endpwent();
	if (pw == 0)
		n = "UNKNOWN";
	else
		n = pw->pw_name;
	strcpy(name, n);
	return (name);
}
