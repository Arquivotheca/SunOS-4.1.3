#ifndef lint
static	char *sccsid = "@(#)edit.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include "rcv.h"
#include <stdio.h>
#include <sys/stat.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Perform message editing functions.
 */

/*
 * Edit a message list.
 */

editor(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("EDITOR")) == NOSTR)
		edname = EDITOR;
	return(edit1(msgvec, edname));
}

/*
 * Invoke the visual editor on a message list.
 */

visual(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("VISUAL")) == NOSTR)
		edname = VISUAL;
	return(edit1(msgvec, edname));
}

/*
 * Edit a message by writing the message into a funnily-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */

edit1(msgvec, ed)
	int *msgvec;
	char *ed;
{
	register int c, lastc = '\n';
	int *ip, pid, mesg, lines, blank = 1;
	long ms;
	void (*sigint)(), (*sigquit)();
	FILE *ibuf, *obuf;
	struct message *mp;
	extern char tempZedit[];
	off_t fsize(), size;
	struct stat statb;
	long modtime;

	/*
	 * Set signals; locate editor.
	 */

	sigint = sigset(SIGINT, SIG_IGN);
	sigquit = sigset(SIGQUIT, SIG_IGN);

	/*
	 * Deal with each message to be edited . . .
	 */

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;

		if (!access(tempZedit, 2)) {
			printf("%s: file exists\n", tempZedit);
			goto out;
		}

		/*
		 * Copy the message into the edit file.
		 */

		close(creat(tempZedit, 0600));
		if ((obuf = fopen(tempZedit, "w")) == NULL) {
			perror(tempZedit);
			goto out;
		}
		if (msend(mp, obuf, 0, fputs) < 0) {
			perror(tempZedit);
			fclose(obuf);
			remove(tempZedit);
			goto out;
		}
		fflush(obuf);
		if (ferror(obuf)) {
			remove(tempZedit);
			fclose(obuf);
			goto out;
		}
		fclose(obuf);

		/*
		 * If we are in read only mode, make the
		 * temporary message file readonly as well.
		 */

		if (readonly)
			chmod(tempZedit, 0400);

		/*
		 * Fork/execl the editor on the edit file.
		 */

		if (stat(tempZedit, &statb) < 0)
			modtime = 0;
		else
			modtime = statb.st_mtime;
		pid = vfork();
		if (pid == -1) {
			perror("fork");
			remove(tempZedit);
			goto out;
		}
		if (pid == 0) {
			sigchild();
			if (sigint != SIG_IGN)
				sigsys(SIGINT, SIG_DFL);
			if (sigquit != SIG_IGN)
				sigsys(SIGQUIT, SIG_DFL);
			execlp(ed, ed, tempZedit, (char *)0);
			perror(ed);
			_exit(1);
		}
		while (wait(&mesg) != pid)
			;

		/*
		 * If in read only mode, just remove the editor
		 * temporary and return.
		 */

		if (readonly) {
			remove(tempZedit);
			continue;
		}

		/*
		 * Now copy the message to the end of the
		 * temp file.
		 */

		if (stat(tempZedit, &statb) < 0) {
			perror(tempZedit);
			continue;
		}
		if (modtime == statb.st_mtime) {
			remove(tempZedit);
			continue;
		}
		if ((ibuf = fopen(tempZedit, "r")) == NULL) {
			perror(tempZedit);
			remove(tempZedit);
			goto out;
		}
		remove(tempZedit);
		fseek(otf, (long) 0, 2);
		size = fsize(otf);
		mp->m_flag |= MODIFY;
		mp->m_offset = size;
		ms = 0L;
		lines = 0;
		while ((c = getc(ibuf)) != EOF) {
			if (c == '\n') {
				lines++;
				blank = lastc == '\n';
			}
			lastc = c;
			putc(c, otf);
			if (ferror(otf))
				break;
			ms++;
		}
		if (!blank) {
			putc('\n', otf);
			ms++;
			lines++;
		}
		mp->m_size = ms;
		mp->m_lines = lines;
		if (ferror(otf))
			perror("/tmp");
		fclose(ibuf);
	}

	/*
	 * Restore signals and return.
	 */

out:
	sigset(SIGINT, sigint);
	sigset(SIGQUIT, sigquit);
	return(0);
}
