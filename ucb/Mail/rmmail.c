#ifndef lint
static	char *sccsid = "@(#)rmmail.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * For use with:
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 *	This "library command" removes an empty file with mode
 *	0660 from a directory modifiable and accessable by
 *	the effective userID of the invoker or by group mail,
 *	provided that the effective userID matches the file's
 *	owner or the real groupID of the invoker is mail (and
 *	the file's group is mail).
 *
 *	In particular, this "command"  is used by mailx to remove
 *	mailboxes (presumably from the /usr/mail directory).
 *	It is assumed that mailboxes can be removed from
 *	the /usr/mail directory by running under the effective
 *	group ID mail; thus this program must run setgid mail.
 *	The permission checks herein are to prevent users from
 *	removing other peoples mailboxes by invoking this "command"
 *	directly.
 *
 *	Mailboxes should be successfully removed when all of the
 *	following conditions are met:
 *
 *		1) The file is empty (mailx should assure this).
 *		2) The mode is 0660.
 *		3) The effective uid matches the owner's uid
 *		   or the real gid is mail (as well as the
 *		   file's group).
 *
 *	NOTE: This program would not be needed if mailx ran
 *	setgid mail.  If mailx ran setgid mail, however, it
 *	would have to go to great pains to ensure that the user
 *	did not acquire group mail privileges for any of its
 *	other functions.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define	DONE(x)	done(x,argv[1])

main(argc,argv)
int argc;
char *argv[];
{
	struct stat stbuf;

	if (argc != 2)
		DONE(1);
	if (stat(argv[1],&stbuf) != 0)
		DONE(2);
	if (stbuf.st_size != 0)
		DONE(3);
	if ((stbuf.st_mode & 0777) != 0660)
		DONE(4);

	if (geteuid() == stbuf.st_uid)
		DONE(unlink(argv[1]));
	if (getgid() == stbuf.st_gid)
		DONE(unlink(argv[1]));
	DONE(5);
	/* NOTREACHED */
}

done(status, file)
int status;
char *file;
{
	switch(status) {
	case -1:
		fprintf(stderr,"rmmail: cannot unlink %s\n",file);
		break;
	case 0:
		break;
	case 1:
		fprintf(stderr,"rmmail: invalid arguments\n");
		break;
	case 2:
		fprintf(stderr,"rmmail: cannot stat %s\n",file);
		break;
	case 3:
		fprintf(stderr,"rmmail: %s not empty\n",file);
		break;
	case 4:
		break;
	case 5:
		fprintf(stderr,"rmmail: cannot remove %s - permission denied\n",file);
		break;
	default:
		fprintf(stderr,"rmmail: bad status\n");
		break;
	}
	exit(status);
}
