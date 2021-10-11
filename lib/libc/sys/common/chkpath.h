#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chkpath.h 1.1 92/07/30 SMI";
#endif

/*
 * return ENOENT if they handed us ""
 */
# include <sys/types.h>
# include <sys/syscall.h>
# include <sys/errno.h>
# define  CHK(p) 		if ((p) != (char*)0  &&  *(p) == 0) { \
					errno = ENOENT; \
					return -1; \
				}
extern int      syscall();
extern          errno;
