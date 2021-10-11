#ifndef lint
static	char *sccsid = "@(#)send.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "rcv.h"
#ifdef VMUNIX
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <ctype.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Mail to others.
 */

/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Return -1 on error, but normally
 * the number of lines written.  Adjust the status: field
 * if need be.  If doign is set, suppress ignored header fields.
 * Call (*fp)(line, obuf) to print the line.
 */
msend(mailp, obuf, doign, fp)
	struct message *mailp;
	FILE *obuf;
	int doign;
	int (*fp)();
{
	register struct message *mp;
	long c;
	FILE *ibuf;
	char line[LINESIZE], field[BUFSIZ];
	int lc, ishead, infld, fline, dostat;
	register char *cp, *cp2;
	int oldign = 0;	/* previous line was ignored */

	mp = mailp;
	ibuf = setinput(mp);
	c = mp->m_size;
	ishead = 1;
	dostat = 1;
	infld = 0;
	fline = 1;
	lc = 0;
	clearerr(obuf);
	while (c > 0L) {
		if (fgets(line, LINESIZE, ibuf) == NULL)
			return(-1);
		c -= (long)strlen(line);
		lc++;
		if (ishead) {
			/* 
			 * First line is the From line, so no headers
			 * there to worry about
			 */
			if (fline) {
				fline = 0;
				goto writeit;
			}
			/*
			 * If line is blank, we've reached end of
			 * headers, so force out status: field
			 * and note that we are no longer in header
			 * fields
			 */
			if (line[0] == '\n') {
				if (dostat) {
					statusput(mailp, obuf, doign, fp);
					dostat = 0;
				}
				ishead = 0;
				goto writeit;
			}
			/*
			 * If this line is a continuation
			 * of a previous header field, just echo it.
			 */
			if (isspace(line[0]) && infld)
				if (oldign)
					continue;
				else
					goto writeit;
			infld = 0;
			/*
			 * If we are no longer looking at real
			 * header lines, force out status:
			 * This happens in uucp style mail where
			 * there are no headers at all.
			 */
			if (!headerp(line)) {
				if (dostat) {
					statusput(mailp, obuf, doign, fp);
					dostat = 0;
				}
				(*fp)("\n", obuf);
				ishead = 0;
				goto writeit;
			}
			infld++;
			/*
			 * Pick up the header field.
			 * If it is an ignored field and
			 * we care about such things, skip it.
			 */
			cp = line;
			cp2 = field;
			while (*cp && *cp != ':' && !isspace(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			oldign = doign && isign(field);
			if (oldign)
				continue;
			/*
			 * If the field is "status," go compute and print the
			 * real Status: field
			 */
			if (icequal(field, "status")) {
				if (dostat) {
					statusput(mailp, obuf, doign, fp);
					dostat = 0;
				}
				continue;
			}
		}
writeit:
		(*fp)(line, obuf);
		if (ferror(obuf))
			return(-1);
	}
	if (ferror(obuf))
		return(-1);
	if (ishead && (mailp->m_flag & MSTATUS))
		printf("failed to fix up status field\n");
	return(lc);
}

/*
 * Test if the passed line is a header line, RFC 733 style.
 */
headerp(line)
	register char *line;
{
	register char *cp = line;

	if (*cp=='>' && strncmp(cp+1, "From", 4)==0)
		return(1);
	while (*cp && !isspace(*cp) && *cp != ':')
		cp++;
	while (*cp && isspace(*cp))
		cp++;
	return(*cp == ':');
}

/*
 * Output a reasonable looking status field.
 * But if "status" is ignored and doign, forget it.
 */
statusput(mp, obuf, doign, fp)
	register struct message *mp;
	register FILE *obuf;
	int doign;
	int (*fp)();
{
	char statout[12];

	if (doign && isign("status"))
		return;
	if ((mp->m_flag & (MNEW|MREAD)) == MNEW)
		return;
	strcpy(statout, "Status: ");
	if (mp->m_flag & MREAD)
		strcat(statout, "R");
	if ((mp->m_flag & MNEW) == 0)
		strcat(statout, "O");
	strcat(statout, "\n");
	(*fp)(statout, obuf);
}


/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */

mail(people)
	char **people;
{
	register char *cp2;
	register int s;
	char *buf, **ap;
	struct header head;
	char recfile[128];

	for (s = 0, ap = people; *ap != NULL; ap++)
		s += strlen(*ap) + 1;
	buf = salloc(s+1);
	cp2 = buf;
	for (ap = people; *ap != NULL; ap++) {
		cp2 = copy(*ap, cp2);
		*cp2++ = ' ';
	}
	if (cp2 != buf)
		cp2--;
	*cp2 = '\0';
	head.h_to = buf;
	if (Fflag) {
		char *tp, *bp, tbuf[128];
		struct name *to;
		/*
		 * peel off the first recipient and expand a possible alias
		 */
		for (tp=tbuf, bp=buf; *bp && *bp!=' '; bp++, tp++)
			*tp = *bp;
		*tp = '\0';
		if (debug)
			fprintf(stderr, "First recipient is '%s'\n", tbuf);
		if ((to = usermap(extract(tbuf, GTO)))==NIL) {
			printf("No recipients specified\n");
			return(1);
		}
		getrecf(to->n_name, recfile, Fflag);
	} else
		getrecf(buf, recfile, Fflag);
	head.h_subject = NOSTR;
	head.h_cc = NOSTR;
	head.h_bcc = NOSTR;
	head.h_headers = (char **)NULL;
	head.h_seq = 0;
	mail1(&head, recfile);
	return(0);
}

/*
 * Interface to the mail1 routine for the -t flag
 * (read headers from text).
 */

tmail()
{
	struct header head;
	char recfile[128];

	getrecf("", recfile, 0);
	head.h_to = NOSTR;
	head.h_subject = NOSTR;
	head.h_cc = NOSTR;
	head.h_bcc = NOSTR;
	head.h_headers = (char **)NULL;
	head.h_seq = 0;
	mail1(&head, recfile);
	return(0);
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail routine below.
 */

sendmail(str)
	char *str;
{
	struct header head;

	if (blankline(str))
		head.h_to = NOSTR;
	else
		head.h_to = str;
	head.h_subject = NOSTR;
	head.h_cc = NOSTR;
	head.h_bcc = NOSTR;
	head.h_headers = (char **)NULL;
	head.h_seq = 0;
	mail1(&head, value("record"));
	return(0);
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */

mail1(hp, recfile)
	struct header *hp;
	char *recfile;
{
	int pid, i, s, p, gotcha;
	char **namelist, *deliver;
	struct name *to, *np;
	FILE *mtf;
	int remote = rflag != NOSTR || rmail;
	char **t;
	char *deadletter = Getf("DEAD");
	char *unuucp();

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */

	pid = -1;
	if ((mtf = collect(hp)) == NULL)
		return;
	hp->h_seq = 1;
	if (hp->h_subject == NOSTR)
		hp->h_subject = sflag;
	if (fsize(mtf) == 0 && hp->h_subject == NOSTR) {
		printf("No message !?!\n");
		goto out;
	}

	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */

	senderr = 0;
	to = usermap(cat(extract(hp->h_bcc, GBCC),
	    cat(extract(hp->h_to, GTO), extract(hp->h_cc, GCC))));
	if (to == NIL) {
		printf("No recipients specified\n");
		goto topdog;
	}

	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */

	to = outof(to, mtf, hp);
	rewind(mtf);
	to = verify(to);
	if (senderr && !remote) {
topdog:

		if (fsize(mtf) != 0) {
			remove(deadletter);
			exwrite(deadletter, mtf, 1);
			rewind(mtf);
		}
	}
	for (gotcha = 0, np = to; np != NIL; np = np->n_flink)
		if ((np->n_type & GDEL) == 0) {
			gotcha++;
			np->n_name = unuucp(np->n_name);
		}
	if (!gotcha)
		goto out;
	to = elide(to);
	mechk(to);
	if (count(to) > 1)
		hp->h_seq++;
	if (hp->h_seq > 0 && !remote) {
		fixhead(hp, to);
		if (fsize(mtf) == 0 && intty)
			printf("Null message body; hope that's ok\n");
		if ((mtf = infix(hp, mtf)) == NULL) {
			fprintf(stderr, ". . . message lost, sorry.\n");
			return;
		}
	}
	namelist = unpack(to);
	if (debug) {
		fprintf(stderr, "Recipients of message:\n");
		for (t = namelist; *t != NOSTR; t++)
			fprintf(stderr, " \"%s\"", *t);
		fprintf(stderr, "\n");
		return;
	}
	if (recfile != NOSTR && *recfile)
		savemail(expand(recfile), mtf);

	/*
	 * Wait, to absorb a potential zombie, then
	 * fork, set up the temporary mail file as standard
	 * input for "mail" and exec with the user list we generated
	 * far above. Return the process id to caller in case he
	 * wants to await the completion of mail.
	 */

#ifdef VMUNIX
	while (wait3((int *)0, WNOHANG, (struct rusage *)0) > 0)
		;
#else
	wait((int *)0);
#endif
	rewind(mtf);
	pid = fork();
	if (pid == -1) {
		perror("fork");
		remove(deadletter);
		exwrite(deadletter, mtf, 1);
		goto out;
	}
	if (pid == 0) {
		sigchild();
#ifdef SIGTSTP
		if (remote == 0) {
			sigset(SIGTSTP, SIG_IGN);
			sigset(SIGTTIN, SIG_IGN);
			sigset(SIGTTOU, SIG_IGN);
		}
#endif
		sigset(SIGHUP, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGQUIT, SIG_IGN);
		s = fileno(mtf);
		for (i = 3; i < 15; i++)
			if (i != s)
				close(i);
		close(0);
		dup(s);
		close(s);
#ifdef CC
		submit(getpid());
#endif CC
#ifdef SENDMAIL
		if ((deliver = value("sendmail")) == NOSTR)
			deliver = SENDMAIL;
		execvp(deliver, namelist);
		perror(deliver);
#else SENDMAIL
		execv(MAIL, namelist);
		perror(MAIL);
#endif SENDMAIL
		exit(1);
	}

	if (value("sendwait")!=NOSTR)
		remote++;
out:
	if (remote) {
		while ((p = wait(&s)) != pid && p != -1)
			;
		if (s != 0)
			senderr++;
		pid = 0;
	}
	fclose(mtf);
	return;
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 * If there are any ARPA net recipients in the message,
 * we must insert commas, alas.
 */

fixhead(hp, tolist)
	struct header *hp;
	struct name *tolist;
{
	register int f;
	register struct name *np;

	for (f = 0, np = tolist; np != NIL; np = np->n_flink)
		if (any('@', np->n_name)) {
			f |= GCOMMA;
			break;
		}

	if (debug && (f & GCOMMA))
		fprintf(stderr, "Should be inserting commas in recip lists\n");
	hp->h_to = detract(tolist, GTO|f);
	hp->h_cc = detract(tolist, GCC|f);
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */

FILE *
infix(hp, fi)
	struct header *hp;
	FILE *fi;
{
	extern char tempMail[];
	register FILE *nfo, *nfi;
	register int c;

	rewind(fi);
	if ((nfo = fopen(tempMail, "w")) == NULL) {
		perror(tempMail);
		return(fi);
	}
	if ((nfi = fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		fclose(nfo);
		return(fi);
	}
	remove(tempMail);
	puthead(hp, nfo, GTO|GSUBJECT|GCC|GNL);
	c = getc(fi);
	while (c != EOF) {
		putc(c, nfo);
		c = getc(fi);
	}
	if (ferror(fi)) {
		perror("read");
		return(fi);
	}
	fflush(nfo);
	if (ferror(nfo)) {
		perror(tempMail);
		fclose(nfo);
		fclose(nfi);
		return(fi);
	}
	fclose(nfo);
	fclose(fi);
	rewind(nfi);
	return(nfi);
}

/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */

puthead(hp, fo, w)
	struct header *hp;
	FILE *fo;
{
	register int gotcha;
	char **p;

	gotcha = 0;
	if (hp->h_to != NOSTR && w & GTO)
		fprintf(fo, "To: "), fmt(hp->h_to, fo), gotcha++;
	if (w & GSUBJECT)
		if (hp->h_subject != NOSTR && *hp->h_subject)
			fprintf(fo, "Subject: %s\n", hp->h_subject), gotcha++;
		else
			if (sflag && *sflag)
				fprintf(fo, "Subject: %s\n", sflag), gotcha++;
	if (hp->h_cc != NOSTR && w & GCC)
		fprintf(fo, "Cc: "), fmt(hp->h_cc, fo), gotcha++;
	if (hp->h_bcc != NOSTR && w & GBCC)
		fprintf(fo, "Bcc: "), fmt(hp->h_bcc, fo), gotcha++;
	if (hp->h_headers != (char **)NULL) {
		for (p = hp->h_headers; p && *p; p++) {
			fputs(*p, fo);
			putc('\n', fo);
			gotcha++;
		}
	}

	if (!tflag && gotcha && w & GNL)
		putc('\n', fo);
}

/*
 * Format the given text to not exceed 72 characters.
 */

fmt(str, fo)
	register char *str;
	register FILE *fo;
{
	register int col;
	register char *cp;

	cp = str;
	col = 0;
	while (*cp) {
		if (*cp == ' ' && col > 65) {
			fprintf(fo, "\n    ");
			col = 4;
			cp++;
			continue;
		}
		putc(*cp++, fo);
		col++;
	}
	putc('\n', fo);
}

/*
 * Save the outgoing mail on the passed file.
 */

savemail(name, fi)
	char name[];
	FILE *fi;
{
	register FILE *fo;
	long now;
	char *n;
	char line[LINESIZE];

	if (debug)
		fprintf(stderr, "save in '%s'\n", name);
	if ((fo = fopen(name, "a")) == NULL) {
		perror(name);
		return;
	}
	/*
	 * Make sure the record file is not publicly readable.
	 */
	(void) chmod(name, 0600);	/* rw------- */
	time(&now);
	n = rflag;
	if (n == NOSTR)
		n = myname;
	fprintf(fo, "From %s %s", n, ctime(&now));
	rewind(fi);
	while (fgets(line, LINESIZE, fi) != NULL) {
		if (strncmp(line, "From ", 5) == 0)
			fputc('>', fo);		/* hide any From lines */
		fputs(line, fo);
	}
	fprintf(fo, "\n");
	fflush(fo);
	if (ferror(fo))
		perror(name);
	fclose(fo);
}
