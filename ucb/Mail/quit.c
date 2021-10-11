#ifndef lint
static	char *sccsid = "@(#)quit.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "rcv.h"
#include <sys/stat.h>
#include <fcntl.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */

quit(noremove)
	int noremove;	/* don't remove system mailbox, trunc it instead */
{
	int mcount, p, modify, autohold, anystat, holdbit, nohold, fd;
	FILE *ibuf, *obuf, *fbuf, *rbuf, *readstat;
	register struct message *mp;
	register int c;
	extern char tempQuit[], tempResid[];
	struct stat minfo;
	char *id;
	int appending;
	char *mbox = expand(Getf("MBOX"));

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */

	mcount = 0;
	if (readonly)
		return;
	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 *
	 * If the luser has sent mail to himself, refuse to do
	 * anything with the mailbox, unless mail locking works.
	 */

	lock(mailname);
#ifndef CANLOCK
	if (selfsent) {
		printf("You have new mail.\n");
		unlock();
		return;
	}
#endif
	rbuf = NULL;
	if (stat(mailname, &minfo) >= 0 && minfo.st_size > mailsize) {
		printf("New mail has arrived.\n");
		rbuf = fopen(tempResid, "w");
		fbuf = fopen(mailname, "r");
		if (rbuf == NULL || fbuf == NULL)
			goto newmail;
#ifdef APPEND
		fseek(fbuf, mailsize, 0);
		while ((c = getc(fbuf)) != EOF)
			putc(c, rbuf);
#else
		p = minfo.st_size - mailsize;
		while (p-- > 0) {
			c = getc(fbuf);
			if (c == EOF)
				goto newmail;
			putc(c, rbuf);
		}
#endif
		fclose(fbuf);
		fclose(rbuf);
		if ((rbuf = fopen(tempResid, "r")) == NULL)
			goto newmail;
		remove(tempResid);
		touchlock();
	}

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NOSTR;
	appending = value("append") != NOSTR;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOXED|MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NOSTR)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	modify = 0;
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (c = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			id = hfield("article-id", mp);
			if (id != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			printf("Held 1 message in %s\n", mailname);
		else
			printf("Held %2d messages in %s\n", p, mailname);
		unlock();
		return;
	}
	if (c == 0) {
		if (p != 0) {
			writeback(rbuf);
			unlock();
			return;
		}
		goto cream;
	}

	/*
	 * Create another temporary file and copy user's mbox file
	 * darin.  If there is no mbox, copy nothing.
	 * If he has specified "append" don't copy his mailbox,
	 * just copy saveable entries at the end.
	 */

	mcount = c;
	if (!appending) {
		if ((obuf = fopen(tempQuit, "w")) == NULL) {
			perror(tempQuit);
			unlock();
			return;
		}
		if ((ibuf = fopen(tempQuit, "r")) == NULL) {
			perror(tempQuit);
			remove(tempQuit);
			fclose(obuf);
			unlock();
			return;
		}
		remove(tempQuit);
		if ((fbuf = fopen(mbox, "r")) != NULL) {
			while ((c = getc(fbuf)) != EOF)
				putc(c, obuf);
			fclose(fbuf);
		}
		if (ferror(obuf)) {
			perror(tempQuit);
			fclose(ibuf);
			fclose(obuf);
			unlock();
			return;
		}
		fclose(obuf);
		if ((fd = open(mbox, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0
		    || (obuf = fdopen(fd, "r+")) == NULL) {
			perror(mbox);
			fclose(ibuf);
			unlock();
			return;
		}
		touchlock();
	} else {	/* we are appending */
		if ((fd = open(mbox, O_RDWR|O_CREAT, 0600)) < 0
		    || (obuf = fdopen(fd, "a")) == NULL) {
			perror(mbox);
			unlock();
			return;
		}
	}
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX) {
			if (msend(mp, obuf, (int)value("alwaysignore"),
			    fputs) < 0) {
				perror(mbox);
				fclose(ibuf);
				fclose(obuf);
				unlock();
				return;
			}
			mp->m_flag &= ~MBOX;
			mp->m_flag |= MBOXED;
			touchlock();
		}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!appending) {
		rewind(ibuf);
		c = getc(ibuf);
		while (c != EOF) {
			putc(c, obuf);
			if (ferror(obuf))
				break;
			c = getc(ibuf);
		}
		fclose(ibuf);
		fflush(obuf);
	}
	trunc(obuf);
	if (ferror(obuf)) {
		perror(mbox);
		fclose(obuf);
		unlock();
		return;
	}
	fclose(obuf);
	if (mcount == 1)
		printf("Saved 1 message in %s\n", mbox);
	else
		printf("Saved %d messages in %s\n", mcount, mbox);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */

	if (p != 0) {
		writeback(rbuf);
		unlock();
		return;
	}

	/*
	 * Finally, remove his /usr/mail file.
	 * If new mail has arrived, copy it back.
	 */

cream:
	mailsize = 0;
	if (rbuf != NULL) {
		fbuf = fopen(mailname, "r+");
		if (fbuf == NULL)
			goto newmail;
		while ((c = getc(rbuf)) != EOF)
			putc(c, fbuf);
		fclose(rbuf);
		trunc(fbuf);
		fclose(fbuf);
		alter(mailname);
		unlock();
		return;
	}
	demail(noremove);
	unlock();
	return;

newmail:
	printf("Thou hast new mail.\n");
	unlock();
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  On any error, just return -1.  Else return 0.
 * Incorporate the any new mail that we found.
 */
writeback(res)
	register FILE *res;
{
	register struct message *mp;
	register int p, c;
	FILE *obuf;

	p = 0;
	if ((obuf = fopen(mailname, "r+")) == NULL) {
		perror(mailname);
		return;
	}
#ifndef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			putc(c, obuf);
#endif
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			p++;
			if (msend(mp, obuf, 0, fputs) < 0) {
				perror(mailname);
				fclose(obuf);
				return;
			}
			touchlock();
		}
	mailsize = fsize(obuf);
#ifdef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			putc(c, obuf);
#endif
	fflush(obuf);
	trunc(obuf);
	if (ferror(obuf)) {
		perror(mailname);
		fclose(obuf);
		return;
	}
	if (res != NULL)
		fclose(res);
	fclose(obuf);
	alter(mailname);
	if (p == 1)
		printf("Held 1 message in %s\n", mailname);
	else
		printf("Held %d messages in %s\n", p, mailname);
}
