/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)cpmv.c 1.1 92/07/30"	/* from SVR3.2 uucp:cpmv.c 2.6 */

#include "uucp.h"

/*
 * copy f1 to f2 locally
 *	f1	-> source file name
 *	f2	-> destination file name
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 */

xcp(f1, f2)
char *f1, *f2;
{
	register int	fd1, fd2;
	register int	nr, nw;
	char buf[BUFSIZ];
	char full[MAXFULLNAME];

	if ((fd1 = open(f1, O_RDONLY)) == -1)
		return(FAIL);
	(void) strcpy(full, f2);
	if (DIRECTORY(f2)) {
		(void) strcat(full, "/");
		(void) strcat(full, BASENAME(f1, '/'));
		(void) strcpy(f2, full);
	}

	DEBUG(4, "full %s\n", full);
	if ((fd2 = open(full, O_CREAT | O_WRONLY, 0666)) == -1) {
		(void) close(fd1);
		return(FAIL);
	}
	(void) chmod(full, 0666);

	/*	copy, looking for read or write failures */
	while ( (nr = read( fd1, buf, sizeof buf )) > 0  &&
		(nw = write( fd2, buf, nr )) == nr )
		;

	close(fd1);
	close(fd2);

	if( nr != 0  ||  nw == -1 )
		return(FAIL);
	return(0);
}


/*
 * move f1 to f2 locally
 * returns:
 *	0	-> ok
 *	FAIL	-> failed
 */

xmv(f1, f2)
register char *f1, *f2;
{
	register int do_unlink, ret;
	struct stat sbuf;

	if ( stat(f2, &sbuf) == 0 )
		do_unlink = ((sbuf.st_mode & S_IFMT) == S_IFREG);
	else
		do_unlink = 1;

	if ( do_unlink )
		(void) unlink(f2);   /* i'm convinced this is the right thing to do */
	if ( (ret = link(f1, f2)) < 0) {
	    /* copy file */
	    ret = xcp(f1, f2);
	}

	if (ret == 0)
	    (void) unlink(f1);
	return(ret);
}


/* toCorrupt - move file to CORRUPTDIR
 * return - none
 */

void
toCorrupt(file)
char *file;
{
	char corrupt[MAXFULLNAME];

	(void) sprintf(corrupt, "%s/%s", CORRUPTDIR, BASENAME(file, '/'));
	(void) link(file, corrupt);
	ASSERT(unlink(file) == 0, Ct_UNLINK, file, errno);
	return;
}

/*
 * append f1 to f2
 *	f1	-> source FILE pointer
 *	f2	-> destination FILE pointer
 * return:
 *	SUCCESS	-> ok
 *	FAIL	-> failed
 */
xfappend(fp1, fp2)
register FILE	*fp1, *fp2;
{
	register int nc;
	char	buf[BUFSIZ];

	while ((nc = fread(buf, sizeof (char), BUFSIZ, fp1)) > 0)
		(void) fwrite(buf, sizeof (char), nc, fp2);

	return(ferror(fp1) || ferror(fp2) ? FAIL : SUCCESS);
}


/*
 * copy f1 to f2 locally under uid of uid argument
 *	f1	-> source file name
 *	f2	-> destination file name
 *	Uid and Euid are global
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 * NOTES:
 *  for V7 systems, flip-flop between real and effective uid is
 *  not allowed, so fork must be done.  This code will not
 *  work correctly when realuid is root on System 5 because of
 *  a bug in setuid.
 */

uidxcp(f1, f2)
char *f1, *f2;
{
	int status;
	char full[MAXFULLNAME];

	(void) strcpy(full, f2);
	if (DIRECTORY(f2)) {
	    (void) strcat(full, "/");
	    (void) strcat(full, BASENAME(f1, '/'));
	}

	/* create full owned by uucp */
	(void) close(creat(full, 0666));
	(void) chmod(full, 0666);

	/* do file copy as read uid */
#ifndef V7
	(void) setuid(Uid);
	status = xcp(f1, full);
	(void) setuid(Euid);
	return(status);

#else /* V7 */

	if (vfork() == 0) {
	    setuid(Uid);
	    _exit (xcp(f1, full));
	}
	wait(&status);
	return(status);
#endif
}
