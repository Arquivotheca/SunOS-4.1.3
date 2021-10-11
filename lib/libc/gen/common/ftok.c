#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ftok.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

key_t
ftok(path, id)
char *path;
char id;
{
	struct stat st;

	return(stat(path, &st) < 0 ? (key_t)-1 :
	    (key_t)((key_t)id << 24 | ((long)(unsigned)minor(st.st_dev)) << 16 |
		(unsigned)st.st_ino));
}
