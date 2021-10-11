#ifndef lint
static	char sccsid[] = "@(#)xmsg.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif


/*
	Call fatal with an appropriate error message
	based on errno.  If no good message can be made up, it makes
	up a simple message.
	The second argument is a pointer to the calling functions
	name (a string); it's used in the manufactured message.
*/

# include	<errno.h>
# include	<sys/types.h>
# include	"../hdr/macros.h"

xmsg(file,func)
char *file, *func;
{
	register char *str;
	extern int errno;
	extern char Error[];
	char buf[1024];

	switch (errno) {
	case ENFILE:
		str = "no file (ut3)";
		break;
	case ENOENT:
		sprintf(str = Error,"`%s' nonexistent (ut4)",
			nse_file_trim(file, 1));
		break;
	case EACCES:
		copy(file,buf);
		sprintf(str = Error,"directory `%s' unwritable (ut2)",
			dname(buf));
		break;
	case ENOSPC:
		str = "no space! (ut10)";
		break;
	case EFBIG:
		str = "write error (ut8)";
		break;
	default:
		sprintf(str = Error,"errno = %d, function = `%s' (ut11)",errno,
			func);
		break;
	}
	return(fatal(str));
}
