#ifndef lint
static	char *sccsid = "@(#)v7.local.c 1.1 92/07/30 SMI"; /* from UCB 2.2 07/28/82 */
#endif

/*
 * Mail -- a mail program
 *
 * Version 7
 *
 * Local routines that are installation dependent.
 */

#include "rcv.h"

/*
 * Locate the user's mailbox file (ie, the place where new, unread
 * mail is queued).  In Version 7, it is in /usr/spool/mail/name.
 */

findmail()
{
	register char *cp;

	/*
	 * If we're looking at our own mailbox,
	 * use MAIL environment variable to locate it.
	 */
	if ((cp = getenv("USER")) != NULL && strcmp(cp, myname) == 0) {
		if ((cp = getenv("MAIL")) && access(cp, 4) == 0) {
			copy(cp, mailname);
			return;
		}
	}
	cp = copy("/usr/spool/mail/", mailname);
	copy(myname, cp);
	if (isdir(mailname)) {
		stradd(mailname, '/');
		strcat(mailname, myname);
	}
}

/*
 * Get rid of the queued mail.
 */

demail(noremove)
	int noremove;	/* don't remove system mailbox, trunc it instead */
{

	if (value("keep") != NOSTR || noremove)
		close(creat(mailname, 0600));
	else {
		if (remove(mailname) < 0)
			close(creat(mailname, 0600));
	}
}

/*
 * Discover user login name.
 */

username(uid, namebuf)
	char namebuf[];
{
	register char *np;

	if (uid == getuid() && (np = getenv("USER")) != NOSTR) {
		strncpy(namebuf, np, PATHSIZE);
		return(0);
	}
	return(getname(uid, namebuf));
}
