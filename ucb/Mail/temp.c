#ifndef lint
static	char *sccsid = "@(#)temp.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Give names to all the temporary files that we will need.
 */

char	tempMail[14];
char	tempQuit[14];
char	tempEdit[14];
char	tempResid[14];
char	tempMesg[14];
char	tempZedit[14];

tinit()
{
	char uname[PATHSIZE];
	register int pid;

	pid = getpid();
	sprintf(tempMail, "/tmp/Rs%-d", pid);
	sprintf(tempResid, "/tmp/Rq%-d", pid);
	sprintf(tempQuit, "/tmp/Rm%-d", pid);
	sprintf(tempEdit, "/tmp/Re%-d", pid);
	sprintf(tempMesg, "/tmp/Rx%-d", pid);
	sprintf(tempZedit, "/tmp/Rz%-d", pid);

	if (strlen(myname) != 0) {
		uid = getuserid(myname);
		if (uid == -1) {
			printf("\"%s\" is not a user of this system\n",
			    myname);
			exit(1);
		}
	}
	else {
		uid = getuid() & UIDMASK;
		if (username(uid, uname) < 0) {
			copy("ubluit", myname);
			if (rcvmode) {
				printf("Who are you!?\n");
				exit(1);
			}
		}
		else
			copy(uname, myname);
	}
	strcpy(homedir, Getf("HOME"));
	findmail();
	assign("MBOX", Getf("MBOX"));
	assign("MAILRC", Getf("MAILRC"));
	assign("DEAD", Getf("DEAD"));
	assign("save", "");
	assign("asksub", "");
	assign("header", "");
}
