#ifndef lint
static	char *sccsid = "@(#)fio.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include "rcv.h"
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * File I/O.
 */

/*
 * Set up the input pointers while copying the mail file into
 * /tmp.
 */

setptr(ibuf, append)
	register FILE *ibuf;
	int append;
{
	register int c;
	register char *cp, *cp2;
	register int count, l;
	register long s;
	off_t offset, lastoffset, saveoffset;
	char linebuf[LINESIZE];
	register int maybe, inhead;
	int flag, first;
	register struct message *mp;
	int seennulls = 0, seengarbage = 0;
	int savemsgCount;
	static int maxmsg = 20;

	if (!append) {
		msgCount = 0;
		maxmsg = 20;
		if (message != (struct message *) 0)
			free((char *) message);
		message = (struct message *) malloc(maxmsg * sizeof (*message));
	}
	fseek(otf, 0, 2);	/* seek to end of file */
	saveoffset = lastoffset = offset = ftell(otf);
	savemsgCount = msgCount;	/* to recover, in case of error */
	s = 0L;
	l = 0;
	first = 1;
	maybe = 1;
	flag = MUSED|MNEW;
	for (;;) {
		c = getc(ibuf);
		for (cp = linebuf; c != EOF && c != '\n'; c = getc(ibuf)) {
			if (c == 0) {
				if (!seennulls) {
					fprintf(stderr,
			"Mail: ignoring NULL characters in mail file\n");
					seennulls++;
				}
				continue;
			}
			if (cp - linebuf >= LINESIZE - 1) {
				ungetc(c, ibuf);
				*cp = 0;
				break;
			}
			*cp++ = c;
		}
		*cp = 0;
		if (cp == linebuf && c == EOF) {
			if (!first) {
				if (msgCount + 1 > maxmsg) {
					maxmsg += 20;
					message = (struct message *)
					    realloc((char *)message,
					    maxmsg * sizeof (*message));
					if (message == (struct message *) 0) {
						printf(
					"Insufficient memory for %d messages\n",
						    msgCount);
						goto err;
					}
				}
				mp = &message[msgCount - 1];
				mp->m_flag = flag;
				mp->m_offset = lastoffset;
				mp->m_size = s;
				mp->m_lines = l;
			}
			bzero((char *)&message[msgCount], sizeof (*message));
			fclose(ibuf);
			dot = message;
			return;
		}
		count = cp - linebuf + 1;
		for (cp = linebuf; *cp;)
			putc(*cp++, otf);
		putc('\n', otf);
		if (ferror(otf)) {
			perror("/tmp");
			goto err;
		}
		if (maybe && linebuf[0] == 'F' && ishead(linebuf)) {
			if (!first) {
				if (msgCount + 1 > maxmsg) {
					maxmsg += 20;
					message = (struct message *)
					    realloc((char *)message,
					    maxmsg * sizeof (*message));
					if (message == (struct message *) 0) {
						printf(
					"Insufficient memory for %d messages\n",
						    msgCount);
						goto err;
					}
				}
				mp = &message[msgCount - 1];
				mp->m_flag = flag;
				mp->m_offset = lastoffset;
				mp->m_size = s;
				mp->m_lines = l;
			}
			msgCount++;
			first = 0;
			flag = MUSED|MNEW;
			inhead = 1;
			s = 0L;
			l = 0;
			lastoffset = offset;
		} else if (first) {
			if (!seengarbage) {
				fprintf(stderr,
				    "Mail: skipping garbage before messages\n");
				seengarbage++;
			}
			offset += count;
			continue;
		}
		if (maybe = (linebuf[0] == 0))
			inhead = 0;
		else
		/*
		 * We're looking for a Status: header line.  Do a quick
		 * test of the second character (first character conflicts
		 * with Subject:) before investing much effort.
		 */
		if (inhead && ((c = linebuf[1]) == 't' || c == 'T') &&
		    index(linebuf, ':')) {
			cp = linebuf;
			cp2 = "status";
			while (*cp2 && isalpha(*cp) && (*cp|040) == *cp2) {
				cp++;
				cp2++;
			}
			if (*cp2 == '\0' && *cp == ':') {
				if (index(cp, 'R'))
					flag |= MREAD;
				if (index(cp, 'O'))
					flag &= ~MNEW;
				inhead = 0;
			}
		}
		offset += count;
		s += (long)count;
		l++;
	}
	/* NOTREACHED */

	/*
	 * If an error occurred, put things back to
	 * the way they were before we started.
	 */
err:
	msgCount = savemsgCount;
	fflush(otf);
	clearerr(otf);
	ftruncate(fileno(otf), saveoffset);
	fseek(otf, 0, 2);	/* seek to end of file */
}

/*
 * Drop the passed line onto the passed output buffer.
 * If a write error occurs, return -1, else the count of
 * characters written, including the newline.
 */

putline(obuf, linebuf)
	FILE *obuf;
	char *linebuf;
{
	register int c;

	c = strlen(linebuf);
	fputs(linebuf, obuf);
	putc('\n', obuf);
	if (ferror(obuf))
		return(-1);
	return(c+1);
}

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;
	int seennulls = 0;

	clearerr(ibuf);
	c = getc(ibuf);
	for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
		if (c == 0) {
			if (!seennulls) {
				fprintf(stderr,
			"Mail: ignoring NULL characters in mail\n");
				seennulls++;
			}
			continue;
		}
		if (cp - linebuf < LINESIZE-2)
			*cp++ = c;
	}
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */

FILE *
setinput(mp)
	register struct message *mp;
{
	fflush(otf);
	if (fseek(itf, mp->m_offset, 0) < 0) {
		perror("fseek");
		panic("temporary file seek");
	}
	return(itf);
}

/*
 * Delete a file, but only if the file is a plain file.
 */

remove(name)
	char name[];
{
	struct stat statb;
	extern int errno;

	if (stat(name, &statb) < 0)
		if (errno == ENOENT)
			return(0);	/* it's already gone, no error */
		else
			return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(name));
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
edstop(noremove)
	int noremove;	/* don't allow the file to be removed, trunc instead */
{
	register int gotcha, c;
	register struct message *mp;
	FILE *obuf, *ibuf, *readstat;
	struct stat statb;
	char tempname[30], *id;

	if (readonly)
		return;
	holdsigs();
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			if ((id = hfield("article-id", mp)) != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (!gotcha || Tflag != NOSTR)
		goto done;
	ibuf = NULL;
	if (stat(editfile, &statb) >= 0 && statb.st_size > mailsize) {
		strcpy(tempname, "/tmp/mboxXXXXXX");
		mktemp(tempname);
		if ((obuf = fopen(tempname, "w")) == NULL) {
			perror(tempname);
			relsesigs();
			reset(0);
		}
		if ((ibuf = fopen(editfile, "r")) == NULL) {
			perror(editfile);
			fclose(obuf);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		fseek(ibuf, mailsize, 0);
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		fclose(ibuf);
		fclose(obuf);
		if ((ibuf = fopen(tempname, "r")) == NULL) {
			perror(tempname);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		remove(tempname);
	}
	if ((obuf = fopen(editfile, "r+")) == NULL) {
		if ((obuf = fopen(editfile, "w")) == NULL) {
			perror(editfile);
			relsesigs();
			reset(0);
		}
	} else
		trunc(obuf);
	printf("\"%s\" ", editfile);
	fflush(stdout);
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (msend(mp, obuf, 0, fputs) < 0) {
			perror(editfile);
			relsesigs();
			reset(0);
		}
	}
	gotcha = (c == 0 && ibuf == NULL);
	if (ibuf != NULL) {
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		fclose(ibuf);
	}
	fflush(obuf);
	if (ferror(obuf)) {
		perror(editfile);
		relsesigs();
		reset(0);
	}
	fclose(obuf);
	if (gotcha && !noremove) {
		remove(editfile);
		printf("removed\n");
	}
	else
		printf("complete\n");
	fflush(stdout);

done:
	relsesigs();
}

# ifdef VMUNIX
static int sigdepth = 0;		/* depth of holdsigs() */
static int omask = 0;
# endif
/*
 * Hold signals SIGHUP - SIGQUIT.
 */
holdsigs()
{
# ifdef VMUNIX
	if (sigdepth++ == 0)
		omask = sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT));
# else
	sighold(SIGHUP);
	sighold(SIGINT);
	sighold(SIGQUIT);
# endif
}

/*
 * Release signals SIGHUP - SIGQUIT
 */
relsesigs()
{
# ifdef VMUNIX
	if (--sigdepth == 0)
		sigsetmask(omask);
# else
	sigrelse(SIGHUP);
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
# endif
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */

off_t
fsize(iob)
	FILE *iob;
{
	register int f;
	struct stat sbuf;

	f = fileno(iob);
	if (fstat(f, &sbuf) < 0)
		return(0);
	return(sbuf.st_size);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 */

char *
expand(name)
	char name[];
{
	char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	register int pid, l;
	register char *cp, *Shell;
	int s, pivec[2];
	struct stat sbuf;

	if (debug) fprintf(stderr, "expand(%s)=", name);
	if (name[0] == '+' && getfold(cmdbuf) >= 0) {
		sprintf(xname, "%s/%s", cmdbuf, name + 1);
		return(expand(savestr(xname)));
	}
	if (!anyof(name, "~{[*?$`'\"\\")) {
		if (debug) fprintf(stderr, "%s\n", name);
		return(name);
	}
	if (pipe(pivec) < 0) {
		perror("pipe");
		return(name);
	}
	sprintf(cmdbuf, "echo %s", name);
	if ((pid = vfork()) == 0) {
		sigchild();
		Shell = value("SHELL");
		if (Shell == NOSTR || *Shell=='\0')
			Shell = SHELL;
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		execlp(Shell, Shell, "-c", cmdbuf, (char *)0);
		_exit(1);
	}
	if (pid == -1) {
		perror("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		fprintf(stderr, "\"Echo\" failed\n");
		goto err;
	}
	if (l < 0) {
		perror("read");
		goto err;
	}
	if (l == 0) {
		fprintf(stderr, "\"%s\": No match\n", name);
		goto err;
	}
	if (l == BUFSIZ) {
		fprintf(stderr, "Buffer overflow expanding \"%s\"\n", name);
		goto err;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	if (any(' ', xname) && stat(xname, &sbuf) < 0) {
		fprintf(stderr, "\"%s\": Ambiguous\n", name);
		goto err;
	}
	if (debug) fprintf(stderr, "%s\n", xname);
	return(savestr(xname));

err:
	fflush(stderr);
	return(NOSTR);
}

/*
 * Determine the current folder directory name.
 */
getfold(name)
	char *name;
{
	char *folder;

	if ((folder = value("folder")) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(name, folder);
	else
		sprintf(name, "%s/%s", homedir, folder);
	return(0);
}

/*
 * A nicer version of Fdopen, which allows us to fclose
 * without losing the open file.
 */

FILE *
Fdopen(fildes, mode)
	char *mode;
{
	register int f;
	FILE *fdopen();

	f = dup(fildes);
	if (f < 0) {
		perror("dup");
		return(NULL);
	}
	return(fdopen(f, mode));
}

/*
 * return the filename associated with "s".  This function always
 * returns a non-null string (no error checking is done on the receiving end)
 */
char *
Getf(s)
register char *s;
{
	register char *cp;
	static char defbuf[PATHSIZE];

	if ((cp = value(s)) && *cp) {
		return(cp);
	} else if (strcmp(s, "MBOX")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "mbox");
		return(defbuf);
	} else if (strcmp(s, "DEAD")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "dead.letter");
		return(defbuf);
	} else if (strcmp(s, "MAILRC")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, ".mailrc");
		return(defbuf);
	} else if (strcmp(s, "HOME")==0) {
		/* no recursion allowed! */
		return(".");
	}
	return("DEAD");	/* "cannot happen" */
}
