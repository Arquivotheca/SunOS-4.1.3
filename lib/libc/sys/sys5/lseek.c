#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)lseek.c 1.1 92/07/30 SMI";
#endif

/*
	lseek -- system call emulation for 4.2BSD and BRL PDP-11 UNIX

	last edit:	28-Aug-1982	D A Gwyn
*/

#include	<errno.h>
#include	<signal.h>

extern int	getpid(), kill();
extern long	_lseek();

long
lseek( fildes, offset, whence ) 	/* returns offset, or -1L */
	int		fildes; 	/* file descriptor */
	long		offset; 	/* byte offset */
	register int	whence; 	/* 0 => relative to BOF
					   1 => relative to CP
					   2 => relative to EOF */
	{
	if ( whence < 0 || whence > 2 )
		{
		(void)kill( getpid(), SIGSYS ); /* so it says */
		errno = EINVAL;
		return -1L;
		}
	else
		return _lseek( fildes, offset, whence );
	}
