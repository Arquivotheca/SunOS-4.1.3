#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fdopen.c 1.1 92/07/30 SMI"; /* from S5R2 1.4 */
#endif

/*LINTLIBRARY*/
/*
 * Unix routine to do an "fopen" on file descriptor
 * The mode has to be repeated because you can't query its
 * status
 */

#include <stdio.h>
#include <sys/errno.h>

extern int  errno;
extern long lseek();
extern FILE *_findiop();

FILE *
fdopen(fd, mode)
int	fd;
register char *mode;
{
	static int nofile = -1;
	register FILE *iop;

	if(nofile < 0)
		nofile = getdtablesize();

	if(fd < 0 || fd >= nofile) {
		errno = EINVAL;
		return(NULL);
	}

	if((iop = _findiop()) == NULL)
		return(NULL);

	iop->_cnt = 0;
	iop->_file = fd;
	iop->_base = iop->_ptr = NULL;
	iop->_bufsiz = 0;
	switch(*mode) {

		case 'r':
			iop->_flag = _IOREAD;
			break;
		case 'a':
			(void) lseek(fd, 0L, 2);
			/* No break */
		case 'w':
			iop->_flag = _IOWRT;
			break;
		default:
			errno = EINVAL;
			return(NULL);
	}

	if(mode[1] == '+')
		iop->_flag = _IORW;

	return(iop);
}
