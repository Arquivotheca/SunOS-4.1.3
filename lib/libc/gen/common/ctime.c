#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ctime.c 1.1 92/07/30 SMI"; /* from Arthur Olson's 3.1 */
#endif

/*LINTLIBRARY*/

#include <sys/types.h>
#include <time.h>

char *
ctime(timep)
time_t *	timep;
{
	return asctime(localtime(timep));
}
