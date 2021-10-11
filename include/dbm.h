/*	@(#)dbm.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03	*/

#ifndef _dbm_h
#define _dbm_h

#define	PBLKSIZ	1024
#define	DBLKSIZ	4096
#define	BYTESIZ	8
/*
 * The following permits stdio.h to be included ahead of this file without
 * generating a compiler warning.
 */
#ifndef	NULL
#define	NULL	((char *) 0)
#endif	/*!NULL*/

long	bitno;
long	maxbno;
long	blkno;
long	hmask;

char	pagbuf[PBLKSIZ];
char	dirbuf[DBLKSIZ];

int	dirf;
int	pagf;
int	dbrdonly;

typedef	struct
{
	char	*dptr;
	int	dsize;
} datum;

datum	fetch();
datum	makdatum();
datum	firstkey();
datum	nextkey();
datum	firsthash();
long	calchash();
long	hashinc();

#endif /*!_dbm_h*/
