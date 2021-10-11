/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)accton.c 1.1 92/07/30 SMI" /* from S5R3 acct:accton.c 1.6 */
/*
 *	accton - calls syscall with super-user privileges
 */
#include	<sys/types.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<stdio.h>

#define	ROOT	0
#define	ERR	(-1)
#define	OK	0
#define	NOGOOD	1
#define	MATCH	0
#define	prmsg(X)	fprintf(stderr, "%s: %s: %s\n", \
				 arg0, X, sys_errlist[errno]);

char		pacct[] = "/usr/adm/pacct";
char		wtmp[] = "/var/adm/wtmp";
char		*arg0;
extern char	*sys_errlist[];

main(argc,argv)
int argc;
char **argv;
{
	register int	uid;

	arg0 = argv[0];
	uid = getuid();
	if(uid == ROOT) {
		if(setuid(ROOT) == ERR) {
			prmsg("cannot setuid (check command mode and owner)");
			exit(1);
		}
		if(ckfile(pacct) == NOGOOD)
			exit(1);

		if(ckfile(wtmp) == NOGOOD)
			exit(1);

		if (argc > 1) {
			if(acct(argv[1]) == ERR) {
				if(errno == EBUSY)
					fprintf(stderr, "%s: %s: %s\n",
						arg0,
						"accounting is busy",
						"cannot turn accounting ON");
				else
					prmsg("cannot turn accounting ON");
				exit(1);
			}
		}
	/*
	 * The following else branch currently never returns
	 * an ERR.  In other words, you may turn the accounting
	 * off to your heart's content.
	 */
		else if(acct((char *)0) == ERR) {
			prmsg("cannot turn accounting OFF");
			exit(1);
		}
		exit(0);

	}
	fprintf(stderr,"%s: permission denied - UID must be root/adm\n", arg0);
	exit(1);
}

ckfile(admfile)
register char	*admfile;
{
	struct stat		stbuf;
	register struct stat	*s = &stbuf;

	if(stat(admfile, s) == ERR)
		if(creat(admfile, 0664) == ERR) {
			prmsg("cannot create");
			return(NOGOOD);
		}

	if(s->st_uid != ROOT || s->st_gid != ROOT)
		if(chown(admfile, ROOT, ROOT) == ERR) {
			prmsg("cannot change owner");
			return(NOGOOD);
		}

	if(s->st_mode & 0777 != 0664)
		if(chmod(admfile, 0664) == ERR) {
			prmsg("cannot chmod");
			return(NOGOOD);
		}

	return(OK);
}
