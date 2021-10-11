/*
 * systemV acct() system call
 */
#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)acct.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <stdio.h>

acct(acctfile)
	char *acctfile;
{
	extern int errno;
	int retval;

	/*
	 * If acctfile is NULL, let the real acct() takes care of it
	 */
	if (acctfile == (char *)0)
		goto real_acct;
	/*
	 * Now check if the accounting is already on or not.
	 */
	if (syscall (SYS_acct, (char *)1) != NULL) {	/* accounting is on ? */
		errno = EBUSY;
		return (-1);
	}
real_acct:
	/*
	 * Trap to the real acct() passing the given argument
	 */
	if ((retval = syscall (SYS_acct, acctfile)) != NULL) {
		/*
		 * Map errno to ENOENT if acctfile is null string;
		 * check after acct() system call to avoid segmentation
		 * violation if acctfile is bad address.
		 */
		if (errno == EACCES) {
			if (acctfile != (char *)0 && *acctfile == 0)
				errno = ENOENT;
		}
	}
	return (retval);
}
