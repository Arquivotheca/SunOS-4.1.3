#ifndef lint
static	char *sccsid = "@(#)collect.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Collect input from standard input, handling
 * ~ escapes.
 */

#include "rcv.h"
#include <ctype.h>
#include <sys/stat.h>

/*
 * Read a message from standard output and return a read file to it
 * or NULL on error.
 */

/*
 * The following hokiness with global variables is so that on
 * receipt of an interrupt signal, the partial message can be salted
 * away on dead.letter.  The output file must be available to flush,
 * and the input to read.  Several open files could be saved all through
 * mailx if stdio allowed simultaneous read/write access.
 */

static	void	(*savesig)();		/* Previous SIGINT value */
static	void	(*savehup)();		/* Previous SIGHUP value */
# ifdef VMUNIX
static	void	(*savecont)();		/* Previous SIGCONT value */
# endif VMUNIX
static	FILE	*newi;			/* File for saving away */
static	FILE	*newo;			/* Output side of same */
static	int	ignintr;		/* Ignore interrupts */
static	int	hadintr;		/* Have seen one SIGINT so far */

static	jmp_buf	coljmp;			/* To get back to work */
extern	char	tempMail[], tempEdit[];

FILE *
collect(hp)
	struct header *hp;
{
	FILE *ibuf, *fbuf, *obuf;
	int lc, cc, escape, eof;
	void collrub(), intack();
# ifdef VMUNIX
	void collcont();
# endif
	register int c, t;
	char *cp, *cp2, **fldp, **curr_hdr;
	char linebuf[LINESIZE], field[LINESIZE];
	int getsub, inhead;
	extern char tempMail[];
	extern void collintsig(), collhupsig();

	noreset++;
	ibuf = obuf = NULL;
	if (value("ignore") != NOSTR)
		ignintr = 1;
	else
		ignintr = 0;
	hadintr = 0;
	inhead = 1;
	fldp = curr_hdr = (char **)NULL;
# ifdef VMUNIX
	if ((savesig = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
		sigset(SIGINT, ignintr ? intack : collrub), sigblock(sigmask(SIGINT));
	if ((savehup = sigset(SIGHUP, SIG_IGN)) != SIG_IGN)
		sigset(SIGHUP, collrub), sigblock(sigmask(SIGHUP));
	savecont = sigset(SIGCONT, collcont);
# else VMUNIX
	savesig = sigset(SIGINT, SIG_IGN);
	savehup = sigset(SIGHUP, SIG_IGN);
# endif VMUNIX
	newi = NULL;
	newo = NULL;
	if ((obuf = fopen(tempMail, "w")) == NULL) {
		perror(tempMail);
		goto err;
	}
	newo = obuf;
	if ((ibuf = fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		newo = NULL;
		fclose(obuf);
		goto err;
	}
	newi = ibuf;
	remove(tempMail);

	/*
	 * If we are going to prompt for a subject,
	 * refrain from printing a newline after
	 * the headers (since some people mind).
	 */

	t = GTO|GSUBJECT|GCC|GNL;
	getsub = 0;
	if (hp->h_subject == NOSTR) {
		hp->h_subject = sflag;
		sflag = NOSTR;
	}
	if (intty && !tflag && hp->h_subject == NOSTR && value("asksub"))
		t &= ~GNL, getsub++;
	if (hp->h_seq != 0) {
		puthead(hp, stdout, t);
		fflush(stdout);
	}
	escape = ESCAPE;
	if ((cp = value("escape")) != NOSTR)
		escape = *cp;
	eof = 0;
	for (;;) {
# ifdef VMUNIX
		int omask = sigblock(0) &~ (sigmask(SIGINT)|sigmask(SIGHUP));
# endif

		setjmp(coljmp);
# ifdef VMUNIX
		sigset(SIGCONT, collcont);
		sigsetmask(omask);
# else VMUNIX
		if (savesig != SIG_IGN)
			signal(SIGINT, ignintr ? intack : collintsig);
		if (savehup != SIG_IGN)
			signal(SIGHUP, collhupsig);
# endif VMUNIX
		fflush(stdout);
		if (getsub) {
			grabh(hp, GSUBJECT);
			getsub = 0;
			continue;
		}
		if (readline(stdin, linebuf) <= 0) {
			if (intty && value("ignoreeof") != NOSTR) {
				if (++eof > 35)
					break;
				printf("Use \".\" to terminate letter\n",
				    escape);
				continue;
			}
			break;
		}
		eof = 0;
		hadintr = 0;
		if (intty && equal(".", linebuf) &&
		    (value("dot") != NOSTR || value("ignoreeof") != NOSTR))
			break;
		/*
		 * If -t, scan text for headers.
		 */
		if (tflag) {
			if (!inhead)
				goto writeit;
			if (linebuf[0] == 0) {
				inhead = 0;
				goto writeit;
			}
			if (isspace(linebuf[0])) {
				if (fldp) {
					*fldp = addto(*fldp, linebuf);
					continue;
				} else if (curr_hdr) {
					/*
					 * Tack on to end of current header,
					 * which is pointed to by "curr_hdr".
					 */
					char *new_hdr, *salloc();

					if ((new_hdr =
					    salloc(strlen(*curr_hdr) +
					    strlen(linebuf) + 2)) == NULL) {
						fprintf(stderr,
					"Mail: Out of memory in collect\n");
						goto err;
					}
					strcpy(new_hdr, *curr_hdr);
					strcat(new_hdr, "\n");
					strcat(new_hdr, linebuf);
					*curr_hdr = new_hdr;
					continue;
				}
				inhead = 0;
				goto writeit;
			}
			if (!headerp(linebuf)) {
				putline(obuf, "");
				inhead = 0;
				goto writeit;
			}
			cp = linebuf;
			cp2 = field;
			fldp = curr_hdr = (char **)NULL;
			while (*cp && *cp != ':' && !isspace(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			cp = index(linebuf, ':') + 1;
			if (icequal(field, "to")) {
				hp->h_to = addto(hp->h_to, cp);
				fldp = &hp->h_to;
				hp->h_seq++;
			} else if (icequal(field, "subject")) {
				while (any(*cp, " \t"))
					cp++;
				hp->h_subject = savestr(cp);
				fldp = &hp->h_subject;
				hp->h_seq++;
			} else if (icequal(field, "cc")) {
				hp->h_cc = addto(hp->h_cc, cp);
				fldp = &hp->h_cc;
				hp->h_seq++;
			} else if (icequal(field, "bcc")) {
				hp->h_bcc = addto(hp->h_bcc, cp);
				fldp = &hp->h_bcc;
				hp->h_seq++;
			} else {
			writeit:
				if (putline(obuf, linebuf) < 0)
					goto err;
			}
			continue;
		}
		if (linebuf[0] != escape || rflag != NOSTR) {
			if ((t = putline(obuf, linebuf)) < 0)
				goto err;
			continue;
		}
		c = linebuf[1];
		switch (c) {
		default:
			/*
			 * On double escape, just send the single one.
			 * Otherwise, it's an error.
			 */

			if (c == escape) {
				if (putline(obuf, &linebuf[1]) < 0)
					goto err;
				else
					break;
			}
			printf("Unknown tilde escape.\n");
			break;

		case 'a':
		case 'A':
			/*
			 * autograph; sign the letter.
			 */

			if (cp = value(c=='a' ? "sign":"Sign")) {
			      cpout(cp, obuf);
			      if (isatty(fileno(stdin)))
				    cpout(cp, stdout);
			}

			break;

		case 'i':
			/*
			 * insert string
			 */
			for (cp = &linebuf[2]; any(*cp, " \t"); cp++)
				;
			if (*cp)
				cp = value(cp);
			if (cp != NOSTR) {
				cpout(cp, obuf);
				if (isatty(fileno(stdout)))
					cpout(cp, stdout);
			}
			break;

		case '!':
			/*
			 * Shell escape, send the balance of the
			 * line to sh -c.
			 */

			shell(&linebuf[2]);
			break;

		case ':':
		case '_':
			/*
			 * Escape to command mode, but be nice!
			 */

			execute(&linebuf[2], 1);
			printf("(continue)\n");
			break;

		case '.':
			/*
			 * Simulate end of file on input.
			 */
			goto eofl;

		case 'q':
		case 'Q':
			/*
			 * Force a quit of sending mail.
			 * Act like an interrupt happened.
			 */

			hadintr++;
			collrub(SIGINT);
			exit(1);

		case 'x':
			xhalt();
			break; 	/* not reached */

		case 'h':
			/*
			 * Grab a bunch of headers.
			 */
			if (!intty || !outtty) {
				printf("~h: no can do!?\n");
				break;
			}
			grabh(hp, GTO|GSUBJECT|GCC|GBCC);
			printf("(continue)\n");
			break;

		case 't':
			/*
			 * Add to the To list.
			 */

			hp->h_to = addto(hp->h_to, &linebuf[2]);
			hp->h_seq++;
			break;

		case 's':
			/*
			 * Set the Subject list.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			hp->h_subject = savestr(cp);
			hp->h_seq++;
			break;

		case 'c':
			/*
			 * Add to the CC list.
			 */

			hp->h_cc = addto(hp->h_cc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'b':
			/*
			 * Add stuff to blind carbon copies list.
			 */
			hp->h_bcc = addto(hp->h_bcc, &linebuf[2]);
			hp->h_seq++;
			break;

		case 'd':
			copy(Getf("DEAD"), &linebuf[2]);
			/* fall into . . . */

		case '<':
		case 'r': {
			int	ispip;
			/*
			 * Invoke a file:
			 * Search for the file name,
			 * then open it and copy the contents to obuf.
			 *
			 * if name begins with '!', read from a command
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				printf("Interpolate what file?\n");
				break;
			}
			if (*cp=='!') {
				/* take input from a command */
				ispip = 1;
				if ((fbuf = popen(++cp, "r"))==NULL) {
					perror("");
					break;
				}
			} else {
				ispip = 0;
				cp = expand(cp);
				if (cp == NOSTR)
					break;
				if (isdir(cp)) {
					printf("%s: directory\n", cp);
					break;
				}
				if ((fbuf = fopen(cp, "r")) == NULL) {
					perror(cp);
					break;
				}
			}
			printf("\"%s\" ", cp);
			fflush(stdout);
			lc = 0;
			cc = 0;
			while (readline(fbuf, linebuf) > 0) {
				lc++;
				if ((t = putline(obuf, linebuf)) < 0) {
					if (ispip)
						pclose(fbuf);
					else
						fclose(fbuf);
					goto err;
				}
				cc += t;
			}
			if (ispip)
				pclose(fbuf);
			else
				fclose(fbuf);
			printf("%d/%d\n", lc, cc);
			break;
			}

		case 'w':
			/*
			 * Write the message on a file.
			 */

			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\0') {
				fprintf(stderr, "Write what file!?\n");
				break;
			}
			if ((cp = expand(cp)) == NOSTR)
				break;
			fflush(obuf);
			rewind(ibuf);
			exwrite(cp, ibuf, 1);
			break;

		case 'm':
		case 'f':
			/*
			 * Interpolate the named messages, if we
			 * are in receiving mail mode.  Does the
			 * standard list processing garbage.
			 * If ~f is given, we don't shift over.
			 */

			if (!rcvmode) {
				printf("No messages to send from!?!\n");
				break;
			}
			cp = &linebuf[2];
			while (any(*cp, " \t"))
				cp++;
			if (forward(cp, obuf, c) < 0)
				goto err;
			printf("(continue)\n");
			break;

		case '?':
			if ((fbuf = fopen(THELPFILE, "r")) == NULL) {
				printf("No help just now.\n");
				break;
			}
			t = getc(fbuf);
			while (t != -1) {
				putchar(t);
				t = getc(fbuf);
			}
			fclose(fbuf);
			break;

		case 'p': {
			/*
			 * Print out the current state of the
			 * message without altering anything.
			 */
			FILE *pbuf;
			int nlines;
			void (*saveint)();
			extern jmp_buf pipestop;
			extern void brokpipe();

			fflush(obuf);
			rewind(ibuf);
			saveint = signal(SIGINT, SIG_IGN);
			pbuf = stdout;
			if (setjmp(pipestop))
				goto ret0;
			if (intty && outtty && (cp = value("crt")) != NOSTR) {
				nlines = atoi(cp) - 7;	/* 7 for hdr lines */
				while ((t = getc(ibuf)) != EOF) {
					if (t == '\n')
						if (--nlines <= 0)
							break;
				}
				rewind(ibuf);
				if (nlines <= 0) {
					pbuf = popen(MORE, "w");
					if (pbuf == NULL) {
						perror(MORE);
						pbuf = stdout;
					} else {
						pipef = pbuf;
						sigset(SIGPIPE, brokpipe);
					}
				} else
					signal(SIGINT, saveint);
			} else
				signal(SIGINT, saveint);
			fprintf(pbuf, "-------\nMessage contains:\n");
			puthead(hp, pbuf, GTO|GSUBJECT|GCC|GBCC|GNL);
			while ((t = getc(ibuf))!=EOF)
				putc(t, pbuf);
		ret0:
			if (pbuf != stdout) {
				pipef = NULL;
				pclose(pbuf);
			}
			sigset(SIGPIPE, SIG_DFL);
			signal(SIGINT, saveint);
			printf("(continue)\n");
			break;
		}

		case '^':
		case '|':
			/*
			 * Pipe message through command.
			 * Collect output as new message.
			 */

			obuf = mespipe(ibuf, obuf, &linebuf[2]);
			newo = obuf;
			ibuf = newi;
			newi = ibuf;
			printf("(continue)\n");
			break;

		case 'v':
		case 'e':
			/*
			 * Edit the current message.
			 * 'e' means to use EDITOR
			 * 'v' means to use VISUAL
			 * 'E' and 'V' mean edit the whole message, including
			 * headers.
			 */

			if ((obuf = mesedit(ibuf, obuf, c, hp)) == NULL)
				goto err;
			newo = obuf;
			ibuf = newi;
			printf("(continue)\n");
			break;
		}
	}
eofl:
	fclose(obuf);
	rewind(ibuf);
	if (intty && value("askcc") != NOSTR) {
		setjmp(coljmp);
		grabh(hp, GCC);
	} else if (intty) {
		printf("EOT\n");
		fflush(stdout);
	}
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
	sigsetmask(0);
# endif VMUNIX
	noreset = 0;
	return(ibuf);

err:
	if (ibuf != NULL)
		fclose(ibuf);
	if (obuf != NULL)
		fclose(obuf);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
	sigsetmask(0);
# endif VMUNIX
	noreset = 0;
	return(NULL);
}

/*
 * Write a file, ex-like if f set.
 */

exwrite(name, ibuf, f)
	char name[];
	FILE *ibuf;
{
	register FILE *of;
	register int c;
	long cc;
	int lc;
	struct stat junk;

	if (f) {
		printf("\"%s\" ", name);
		fflush(stdout);
	}
	if (stat(name, &junk) >= 0 && (junk.st_mode & S_IFMT) == S_IFREG) {
		if (!f)
			fprintf(stderr, "%s: ", name);
		fprintf(stderr, "File exists\n", name);
		return;
	}
	if ((of = fopen(name, "w")) == NULL) {
		perror("");
		return;
	}
	lc = 0;
	cc = 0;
	while ((c = getc(ibuf)) != EOF) {
		cc++;
		if (c == '\n')
			lc++;
		putc(c, of);
		if (ferror(of)) {
			perror(name);
			fclose(of);
			return;
		}
	}
	fclose(of);
	printf("%d/%ld\n", lc, cc);
	fflush(stdout);
}

/*
 * Edit the message being collected on ibuf and obuf.
 * Write the message out onto some poorly-named temp file
 * and point an editor at it.
 *
 * On return, make the edit file the new temp file.
 */

FILE *
mesedit(ibuf, obuf, c, hp)
	FILE *ibuf, *obuf;
	struct header *hp;
{
	FILE *fbuf;
	register int t;
	void (*sig)();
# ifdef VMUNIX
	void (*scont)(), foonly();
# endif VMUNIX
	struct stat sbuf;
	register char *editor;
	int edit_header;
	char ecmd[BUFSIZ];

	sig = sigset(SIGINT, SIG_IGN);
# ifdef VMUNIX
	scont = sigset(SIGCONT, foonly);
# endif VMUNIX
	if (stat(tempEdit, &sbuf) >= 0) {
		printf("%s: file exists\n", tempEdit);
		goto out;
	}
	close(creat(tempEdit, 0600));
	if ((fbuf = fopen(tempEdit, "w")) == NULL) {
		perror(tempEdit);
		goto out;
	}
	fflush(obuf);
	rewind(ibuf);

	/*
	 * Optionally add the headers to the file to be edited.
	 */
	edit_header = (int)value("editheaders");
	if (edit_header)
		puthead(hp, fbuf, GTO|GSUBJECT|GCC|GBCC|GNL);

	while ((t = getc(ibuf)) != EOF)
		putc(t, fbuf);
	fflush(fbuf);
	if (ferror(fbuf)) {
		perror(tempEdit);
		remove(tempEdit);
		goto fix;
	}
	fclose(fbuf);
	if ((editor = value(c == 'e' ? "EDITOR" : "VISUAL")) == NOSTR)
		editor = c == 'e' ? EDITOR : VISUAL;
	sprintf(ecmd, "exec %s %s", editor, tempEdit);
	if (system(ecmd) & 0377) {
		printf("Fatal error in \"%s\"\n", editor);
		remove(tempEdit);
		goto out;
	}

	/*
	 * Now switch to new file.
	 */

	if ((fbuf = fopen(tempEdit, "a")) == NULL) {
		perror(tempEdit);
		remove(tempEdit);
		goto out;
	}
	if ((ibuf = fopen(tempEdit, "r")) == NULL) {
		perror(tempEdit);
		fclose(fbuf);
		remove(tempEdit);
		goto out;
	}
	remove(tempEdit);

	/*
	 * Re-parse the header.  we must arrange to leave
	 * only the text of the article in ibuf/fbuf, so
	 * parse_header also has to copy those files to new ones, 
	 * without the header.
	 */
	if (edit_header)
		parse_header(&ibuf, &fbuf, hp);

	fclose(obuf);
	fclose(newi);
	obuf = fbuf;
	goto out;
fix:
	perror(tempEdit);
out:
# ifdef VMUNIX
	sigset(SIGCONT, scont);
# endif VMUNIX
	sigset(SIGINT, sig);
	newi = ibuf;
	return(obuf);
}

#ifdef VMUNIX
/*
 * Currently, Berkeley virtual VAX/UNIX will not let you change the
 * disposition of SIGCONT, except to trap it somewhere new.
 * Hence, sigset(SIGCONT, foonly) is used to ignore continue signals.
 */
void
foonly() {}
#endif

/*
 * Pipe the message through the command.
 * Old message is on stdin of command;
 * New message collected from stdout.
 * Sh -c must return 0 to accept the new message.
 */

FILE *
mespipe(ibuf, obuf, cmd)
	FILE *ibuf, *obuf;
	char cmd[];
{
	register FILE *ni, *no;
	int pid, s;
	void (*saveint)();
	char *Shell;

	newi = ibuf;
	if ((no = fopen(tempEdit, "w")) == NULL) {
		perror(tempEdit);
		return(obuf);
	}
	if ((ni = fopen(tempEdit, "r")) == NULL) {
		perror(tempEdit);
		fclose(no);
		remove(tempEdit);
		return(obuf);
	}
	remove(tempEdit);
	saveint = sigset(SIGINT, SIG_IGN);
	fflush(obuf);
	rewind(ibuf);
	if ((Shell = value("SHELL")) == NULL || *Shell=='\0')
		Shell = "/bin/sh";
	if ((pid = vfork()) == -1) {
		perror("fork");
		goto err;
	}
	if (pid == 0) {
		/*
		 * stdin = current message.
		 * stdout = new message.
		 */

		sigchild();
		close(0);
		dup(fileno(ibuf));
		close(1);
		dup(fileno(no));
		for (s = 4; s < 15; s++)
			close(s);
		execlp(Shell, Shell, "-c", cmd, (char *)0);
		perror(Shell);
		_exit(1);
	}
	while (wait(&s) != pid)
		;
	if (s != 0 || pid == -1) {
		fprintf(stderr, "\"%s\" failed!?\n", cmd);
		goto err;
	}
	if (fsize(ni) == 0) {
		fprintf(stderr, "No bytes from \"%s\" !?\n", cmd);
		goto err;
	}

	/*
	 * Take new files.
	 */

	newi = ni;
	fclose(ibuf);
	fclose(obuf);
	sigset(SIGINT, saveint);
	return(no);

err:
	fclose(no);
	fclose(ni);
	sigset(SIGINT, saveint);
	return(obuf);
}

static char *indentprefix;	/* used instead of tab by tabputs */

/*
 * Interpolate the named messages into the current
 * message, preceding each line with a tab.
 * Return a count of the number of characters now in
 * the message, or -1 if an error is encountered writing
 * the message temporary.  The flag argument is 'm' if we
 * should shift over and 'f' if not.
 */
forward(ms, obuf, f)
	char ms[];
	FILE *obuf;
{
	register int *msgvec, *ip;
	extern char tempMail[];
	int tabputs();

	msgvec = (int *) salloc((msgCount+1) * sizeof *msgvec);
	if (msgvec == (int *) NOSTR)
		return(0);
	if (getmsglist(ms, msgvec, 0) < 0)
		return(0);
	if (*msgvec == NULL) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf("No appropriate messages\n");
			return(0);
		}
		msgvec[1] = NULL;
	}
	if (f == 'm')
		indentprefix = value("indentprefix");
	printf("Interpolating:");
	for (ip = msgvec; *ip != NULL; ip++) {
		touch(*ip);
		printf(" %d", *ip);
		if (msend(&message[*ip-1], obuf, (int)value("alwaysignore"),
		    f == 'm' ? tabputs : fputs) < 0) {
			perror(tempMail);
			return(-1);
		}
	}
	printf("\n");
	return(0);
}

tabputs(line, obuf)
	char *line;
	FILE *obuf;
{

	if (indentprefix)
		fputs(indentprefix, obuf);
	/* Don't create lines with only a tab on them */
	else if (line[0] != '\n')
		fputc('\t', obuf);
	fputs(line, obuf);
}

# ifdef VMUNIX
/*
 * Print (continue) when continued after ^Z.
 */
/*ARGSUSED*/
void
collcont(s)
{

	printf("(continue)\n");
	fflush(stdout);
}
# endif VMUNIX

/*
 * On interrupt, go here to save the partial
 * message on ~/dead.letter.
 * Then restore signals and execute the normal
 * signal routine.  We only come here if signals
 * were previously set anyway.
 */

# ifndef VMUNIX
void
collintsig()
{
	signal(SIGINT, SIG_IGN);
	collrub(SIGINT);
}

void
collhupsig()
{
	signal(SIGHUP, SIG_IGN);
	collrub(SIGHUP);
}
# endif VMUNIX

void
collrub(s)
{
	register FILE *dbuf;
	register int c;
	register char *deadletter = Getf("DEAD");
	extern void hangup(), stop();

	if (s == SIGINT && hadintr == 0) {
		hadintr++;
		fflush(stdout);
		fprintf(stderr, "\n(Interrupt -- one more to kill letter)\n");
		longjmp(coljmp, 1);
	}
	fclose(newo);
	rewind(newi);
	if (s == SIGINT && value("save")==NOSTR || fsize(newi) == 0)
		goto done;
	if ((dbuf = fopen(deadletter, "w")) == NULL)
		goto done;
	chmod(deadletter, 0600);
	while ((c = getc(newi)) != EOF)
		putc(c, dbuf);
	fclose(dbuf);

done:
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
# endif VMUNIX
	if (rcvmode) {
		if (s == SIGHUP)
			hangup(SIGHUP);
		else
			stop(s);
	}
	else
		exit(1);
}

/*
 * Acknowledge an interrupt signal from the tty by typing an @
 */

/*ARGSUSED*/
void
intack(s)
{
	
	puts("@");
	fflush(stdout);
	clearerr(stdin);
	longjmp(coljmp,1);
}

/*
 * Add a string to the end of a header entry field.
 */

char *
addto(hf, news)
	char hf[], news[];
{
	register char *cp, *cp2, *linebuf;

	if (hf == NOSTR)
		hf = "";
	if (*news == '\0')
		return(hf);
	linebuf = salloc(strlen(hf) + strlen(news) + 2);
	for (cp = hf; any(*cp, " \t"); cp++)
		;
	for (cp2 = linebuf; *cp;)
		*cp2++ = *cp++;
	if (cp2 != linebuf)
		*cp2++ = ' ';
	for (cp = news; any(*cp, " \t"); cp++)
		;
	while (*cp != '\0')
		*cp2++ = *cp++;
	*cp2 = '\0';
	return(linebuf);
}

/*
 * Re-read the header stuff from the newly edited file;
 * take pointers to the file open for reading and appending and
 * use/replace them with a new temp file with the header read/stripped off
 * (since the rest of Mail expects that the temp file contains only the text).
 */
parse_header(pibuf, pfbuf, hp)
	FILE **pibuf;		/* Ptr to file currently open for reading. */
	FILE **pfbuf;		/* Return file opened for writing at end */
	register struct header *hp;
{
	register char *cp, *cp2;
	register int c;
	int fd;
	char **curr_hdr, **fldp, **add_header();
	char *osubject, *oto, *occ, *obcc, **oheaders;
	int gothdr;
	char linebuf[LINESIZE], field[LINESIZE];
	char new_tempfile[14];

	strcpy(new_tempfile, "/tmp/RnXXXXXX");
	if ((fd = mkstemp(new_tempfile)) < 0) {
		perror(new_tempfile);
		/* Nothing changes. Message has header in it, though. */
		return;
	}

	fclose(*pfbuf);
	*pfbuf = fdopen(fd, "a");

	/* save the old headers, in case they are accidentally deleted */
	osubject = hp->h_subject;
	oto = hp->h_to;
	occ = hp->h_cc;
	obcc = hp->h_bcc;
	oheaders = hp->h_headers;
	hp->h_subject = hp->h_to = hp->h_cc = hp->h_bcc = NOSTR;
	hp->h_headers = (char **)NULL;
	gothdr = 0;

	fldp = curr_hdr = (char **)NULL;
	while (readline(*pibuf, linebuf) > 0) {
		if (linebuf[0] == '\0')
			break;		/* Blank line, end of header. */
		
		/*
		 * Is it a continuation of the previous header?
		 */
		if (isspace(linebuf[0])) {
			if (debug)
				fprintf(stderr, "Continuation: %s\n", linebuf);

			if (fldp) {
				*fldp = addto(*fldp, linebuf);
			} else if (curr_hdr) {
				/*
				 * Tack on to end of current header,
				 * which is pointed to by "curr_hdr".
				 */
				char *new_hdr, *salloc();

				if ((new_hdr = salloc(strlen(*curr_hdr) +
				    strlen(linebuf) + 2)) == NULL) {
					fprintf(stderr,
				"Mail: Out of memory in parse_header\n");
				} else {
					strcpy(new_hdr, *curr_hdr);
					strcat(new_hdr, "\n");
					strcat(new_hdr, linebuf);
					*curr_hdr = new_hdr;
				}
			} else {
				/*
				 * Looks like a continuation, but no
				 * previous header.  Assume this is
				 * part of the text - somehow the
				 * user deleted the header.
				 */
				putline(*pfbuf, linebuf);
				break;
			}
		} else if (!headerp(linebuf)) {
			/*
			 * Not a header.  Get out of this loop.
			 */
			putline(*pfbuf, linebuf);
			break;		/* Non-header text line found */
		} else {
			/*
			 * Is a header line.  Figure out which one
			 * it is, and save it in the appropriate spot.
			 */
			fldp = curr_hdr = (char **)NULL;
			gothdr = 1;
			cp = linebuf;
			cp2 = field;
			while (*cp && *cp != ':' && !isspace(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			cp = index(linebuf, ':') + 1;
			if (icequal(field, "to")) {
				hp->h_to = addto(hp->h_to, cp);
				fldp = &hp->h_to;
				hp->h_seq++;
			} else if (icequal(field, "subject")) {
				while (any(*cp, " \t"))
					cp++;
				hp->h_subject = savestr(cp);
				fldp = &hp->h_subject;
				hp->h_seq++;
			} else if (icequal(field, "cc")) {
				hp->h_cc = addto(hp->h_cc, cp);
				fldp = &hp->h_cc;
				hp->h_seq++;
			} else if (icequal(field, "bcc")) {
				hp->h_bcc = addto(hp->h_bcc, cp);
				fldp = &hp->h_bcc;
				hp->h_seq++;
			} else {
				/*
				 * User supplied header.
				 * add the line to hp->h_headers
				 */
				curr_hdr = add_header(hp, linebuf);
			}
		}
	}

	while ((c = getc(*pibuf)) != EOF)
		putc(c, *pfbuf);

	fclose(*pibuf);

	if (!gothdr) {
		/* if we didn't see any headers, restore the original headers */
		hp->h_subject = osubject;
		hp->h_to = oto;
		hp->h_cc = occ;
		hp->h_bcc = obcc;
		hp->h_headers = oheaders;
	}

	/*
	 * this isn't too robust if it fails
	 */
	if ((*pibuf = fopen(new_tempfile, "r")) == NULL) {
		perror(new_tempfile);
		return;
	}

	remove(new_tempfile);
	rewind(*pibuf);
}

/*
 * Add a new header line to the h_headers part of the header.
 * "line" should be the entire "Header: we want to add".
 * We return a pointer to the (stored) header that we just added.
 */

char **
add_header(hp, line)
	register struct header *hp;
	char *line;
{
	register char **p, **np;
	register int nheaders = 0;

	p = hp->h_headers;
	if (p != (char **)NULL)
		while (*p++)
			nheaders++;

	np = (char **)salloc((nheaders + 2) * sizeof(char *));
	p = hp->h_headers;
	hp->h_headers = np;

	while (nheaders--)
		*np++ = *p++;

	*np = savestr(line);
	np[1] = NOSTR;
	return(np);
}

cpout(str, ofd)
	char *str;
	FILE *ofd;
{
	register char *cp = str;

	while (*cp) {
		if (*cp == '\\') {
			switch (*(cp+1)) {
			case 'n':
				putc('\n', ofd);
				cp++;
				break;
			case 't':
				putc('\t', ofd);
				cp++;
				break;
			default:
				putc('\\', ofd);
			}
		} else {
			putc(*cp, ofd);
		}
		cp++;
	}
	putc('\n', ofd);
}

xhalt()
{
	fclose(newo);
	fclose(newi);
	sigset(SIGINT, savesig);
	sigset(SIGHUP, savehup);
	if (rcvmode)
		stop(0);
	exit(1);
}
