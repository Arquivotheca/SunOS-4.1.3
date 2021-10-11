#ifndef lint
static	char *sccsid = "@(#)aux.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "rcv.h"
#include <sys/stat.h>
#include <ctype.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Auxiliary functions.
 */

/*
 * Return a pointer to a dynamic copy of the argument.
 */

char *
savestr(str)
	char *str;
{
	register char *cp, *cp2, *top;

	for (cp = str; *cp; cp++)
		;
	top = salloc(cp-str + 1);
	if (top == NOSTR)
		return(NOSTR);
	for (cp = str, cp2 = top; *cp; cp++)
		*cp2++ = *cp;
	*cp2 = 0;
	return(top);
}

/*
 * Announce a fatal error and die.
 */

panic(str)
	char *str;
{
	fprintf(stderr, "Mail: panic: %s\n", str);
	exit(1);
}

/*
 * Touch the named message by setting its MTOUCH flag.
 * Touched messages have the effect of not being sent
 * back to the system mailbox on exit.
 */

touch(mesg)
{
	register struct message *mp;

	if (mesg < 1 || mesg > msgCount)
		return;
	mp = &message[mesg-1];
	mp->m_flag |= MTOUCH;
	if ((mp->m_flag & MREAD) == 0)
		mp->m_flag |= MREAD|MSTATUS;
}

/*
 * Test to see if the passed file name is a directory.
 * Return true if it is.
 */

isdir(name)
	char name[];
{
	struct stat sbuf;

	if (stat(name, &sbuf) < 0)
		return(0);
	return((sbuf.st_mode & S_IFMT) == S_IFDIR);
}

/*
 * Count the number of arguments in the given string raw list.
 */

argcount(argv)
	char **argv;
{
	register char **ap;

	for (ap = argv; *ap != NOSTR; ap++)
		;	
	return(ap-argv);
}

/*
 * Return the desired header line from the passed message
 * pointer (or NOSTR if the desired header field is not available).
 * Read all the header lines and concatenate multiple instances of
 * the requested header.
 */

char *
hfield(field, mp)
	char field[];
	struct message *mp;
{
	register FILE *ibuf;
	char linebuf[LINESIZE];
	register int lc;
	register char *hdr = NOSTR;

	ibuf = setinput(mp);
	if ((lc = mp->m_lines) <= 0)
		return(NOSTR);
	if (readline(ibuf, linebuf) < 0)
		return(NOSTR);
	lc--;
	do {
		lc = gethfield(ibuf, linebuf, lc);
		if (lc == -1)
			return(hdr);
		if (ishfield(linebuf, field)) {
			if (hdr == NOSTR) {
				hdr = savestr(hcontents(linebuf));
			} else {
				register char *s, *p;

				s = hcontents(linebuf);
				p = salloc(strlen(hdr) + strlen(s) + 2);
				if (p == NOSTR)
					return (NOSTR);
				strcpy(p, hdr);
				strcat(p, " ");
				strcat(p, s);
				hdr = p;
			}
		}
	} while (lc > 0);
	return(hdr);
}

/*
 * Return the next header field found in the given message.
 * Return > 0 if something found, <= 0 elsewise.
 * Must deal with \ continuations & other such fraud.
 */

gethfield(f, linebuf, rem)
	register FILE *f;
	char linebuf[];
	register int rem;
{
	char line2[LINESIZE];
#ifdef CANTELL
	long loc;
#endif
	register char *cp, *cp2;
	register int c;


	for (;;) {
		if (rem <= 0)
			return(-1);
		if (readline(f, linebuf) < 0)
			return(-1);
		rem--;
		if (strlen(linebuf) == 0)
			return(-1);
		if (isspace(linebuf[0]))
			continue;
		if (linebuf[0] == '>')
			continue;
		cp = index(linebuf, ':');
		if (cp == NOSTR)
			continue;
		for (cp2 = linebuf; cp2 < cp; cp2++)
			if (isdigit(*cp2))
				continue;
		
		/*
		 * I guess we got a headline.
		 * Handle wraparounding
		 */
		
		for (;;) {
			if (rem <= 0)
				break;
#ifdef CANTELL
			loc = ftell(f);
			if (readline(f, line2) < 0)
				break;
			rem--;
			if (!isspace(line2[0])) {
				fseek(f, loc, 0);
				rem++;
				break;
			}
#else
			c = getc(f);
			ungetc(c, f);
			if (!isspace(c) || c == '\n')
				break;
			if (readline(f, line2) < 0)
				break;
			rem--;
#endif
			cp2 = line2;
			for (cp2 = line2; *cp2 != 0 && isspace(*cp2); cp2++)
				;
			if (strlen(linebuf) + strlen(cp2) >= LINESIZE-2)
				break;
			cp = &linebuf[strlen(linebuf)];
			while (cp > linebuf &&
			    (isspace(cp[-1]) || cp[-1] == '\\'))
				cp--;
			*cp++ = ' ';
			for (cp2 = line2; *cp2 != 0 && isspace(*cp2); cp2++)
				;
			strcpy(cp, cp2);
		}
		if ((c = strlen(linebuf)) > 0) {
			cp = &linebuf[c-1];
			while (cp > linebuf && isspace(*cp))
				cp--;
			*++cp = 0;
		}
		return(rem);
	}
	/* NOTREACHED */
}

/*
 * Check whether the passed line is a header line of
 * the desired breed.
 */

ishfield(linebuf, field)
	char linebuf[], field[];
{
	register char *cp;
	register int c;

	if ((cp = index(linebuf, ':')) == NOSTR)
		return(0);
	if (cp == linebuf)
		return(0);
	cp--;
	while (cp > linebuf && isspace(*cp))
		cp--;
	c = *++cp;
	*cp = 0;
	if (icequal(linebuf ,field)) {
		*cp = c;
		return(1);
	}
	*cp = c;
	return(0);
}

/*
 * Extract the non label information from the given header field
 * and return it.
 */

char *
hcontents(hf)
	char hf[];
{
	register char *cp;

	if ((cp = index(hf, ':')) == NOSTR)
		return(NOSTR);
	cp++;
	while (*cp && isspace(*cp))
		cp++;
	return(cp);
}

/*
 * Compare two strings, ignoring case.
 */

icequal(s1, s2)
	register char *s1, *s2;
{

	while (raise(*s1++) == raise(*s2))
		if (*s2++ == 0)
			return(1);
	return(0);
}

/*
 * Copy a string, lowercasing it as we go.
 */
istrcpy(dest, src)
	char *dest, *src;
{
	register char *cp, *cp2;

	cp2 = dest;
	cp = src;
	do {
		*cp2++ = little(*cp);
	} while (*cp++ != 0);
}

/*
 * The following code deals with input stacking to do source
 * commands.  All but the current file pointer are saved on
 * the stack.
 */

static	int	ssp = -1;		/* Top of file stack */
struct sstack {
	FILE	*s_file;		/* File we were in. */
	int	s_cond;			/* Saved state of conditionals */
	int	s_loading;		/* Loading .mailrc, etc. */
} sstack[NOFILE];

/*
 * Pushdown current input file and switch to a new one.
 * Set the global flag "sourcing" so that others will realize
 * that they are no longer reading from a tty (in all probability).
 */

source(name)
	char name[];
{
	register FILE *fi;
	register char *cp;

	if ((cp = expand(name)) == NOSTR)
		return(1);
	if ((fi = fopen(cp, "r")) == NULL) {
		perror(cp);
		return(1);
	}
	if (ssp >= NOFILE - 2) {
		printf("Too much \"sourcing\" going on.\n");
		fclose(fi);
		return(1);
	}
	sstack[++ssp].s_file = input;
	sstack[ssp].s_cond = cond;
	sstack[ssp].s_loading = loading;
	loading = 0;
	cond = CANY;
	input = fi;
	sourcing++;
	return(0);
}

/*
 * Pop the current input back to the previous level.
 * Update the "sourcing" flag as appropriate.
 */

unstack()
{
	if (ssp < 0) {
		printf("\"Source\" stack over-pop.\n");
		sourcing = 0;
		return;
	}
	fclose(input);
	if (cond != CANY)
		printf("Unmatched \"if\"\n");
	cond = sstack[ssp].s_cond;
	loading = sstack[ssp].s_loading;
	input = sstack[ssp--].s_file;
	if (ssp < 0)
		sourcing = loading;
}

/*
 * Touch the indicated file.
 * This is nifty for the shell.
 * If we have the utime() system call, this is better served
 * by using that, since it will work for empty files.
 * On non-utime systems, we must sleep a second, then read.
 */

alter(name)
	char name[];
{
#ifdef UTIME
	struct stat statb;
	long time();
	time_t time_p[2];
#else
	register int pid, f;
	char w;
#endif UTIME

#ifdef UTIME
	if (stat(name, &statb) < 0)
		return;
	time_p[0] = time((long *) 0) + 1;
	time_p[1] = statb.st_mtime;
	if (time_p[0] <= time_p[1]) {		/* may be true on nfs */
		time_p[0] = time_p[1] + 1;
	}
	utime(name, time_p);
	sleep(1);		/* really want "sleep until time_p[0]" */
#else
	sleep(1);
	if ((f = open(name, 0)) < 0)
		return;
	read(f, &w, 1);
	exit(0);
#endif
}

/*
 * Examine the passed line buffer and
 * return true if it is all blanks and tabs.
 */

blankline(linebuf)
	char linebuf[];
{
	register char *cp;

	for (cp = linebuf; *cp; cp++)
		if (*cp != ' ' && *cp != '\t')
			return(0);
	return(1);
}

/*
 * Get sender's name from this message.  If the message has
 * a bunch of arpanet stuff in it, we may have to skin the name
 * before returning it.
 */
char *
nameof(mp, reptype)
	register struct message *mp;
{
	register char *cp;

	cp = skin(name1(mp, reptype));
	if (reptype != 0)
		return(cp);
	return(striproute(cp));
}

/*
 * Strip an address of all host names, both UUCP and Internet-style.
 * Assumes that all UUCP names pile up on the left, and all Internet-style
 * names pile up on the right.
 * Modifies the string that it is passed.
 */
char *
striphosts(addr)
	char *addr;
{
	register char *cp, *cp2;

	if ((cp = rindex(addr,'!')) != NULL)
		cp++;
	else
		cp = addr;
	/*
	 * Now strip off all Internet-type
	 * hosts.
	 */
	if ((cp2 = index(cp, '%')) == NOSTR)
		cp2 = index(cp, '@');
	if (cp2 != NOSTR)
		*cp2 = '\0';
	return(cp);
}

/*
 * Strip an address of all but the last UUCP host name.
 * Assumes that all UUCP names pile up on the left.
 */
char *
striproute(addr)
	char *addr;
{
	register char *cp;

	if ((cp = rindex(addr, '!')) != NOSTR) {
		cp--;
		while (cp > addr && *cp != '!')
			cp--;
		if (*cp == '!')
			cp++;
	} else
		cp = addr;
	return(cp);
}

/*
 * Skin an arpa net address according to the RFC 822 interpretation
 * of "host-phrase."
 */
char *
skin(name)
	char *name;
{
	register int c;
	register char *cp, *cp2;
	char *bufend;
	int gotlt, lastsp;
	char nbuf[BUFSIZ];
	int nesting;

	if (name == NOSTR)
		return(NOSTR);
	if (index(name, '(') == NOSTR && index(name, '<') == NOSTR
	&& index(name, ' ') == NOSTR)
		return(name);
	gotlt = 0;
	lastsp = 0;
	bufend = nbuf;
	for (cp = name, cp2 = bufend; c = *cp++; ) {
		switch (c) {
		case '(':
			/*
			 * Start of a "comment".
			 * Ignore it.
			 */
			nesting = 1;
			while ((c = *cp) != 0) {
				cp++;
				switch (c) {
				case '\\':
					if (*cp == 0)
						goto outcm;
					cp++;
					break;
				case '(':
					nesting++;
					break;

				case ')':
					--nesting;
					break;
				}

				if (nesting <= 0)
					break;
			}
		outcm:
			lastsp = 0;
			break;

		case '"':
			/*
			 * Start of a "quoted-string".
			 * Copy it in its entirety.
			 */
			while ((c = *cp) != 0) {
				cp++;
				switch (c) {
				case '\\':
					if ((c = *cp) == 0)
						goto outqs;
					cp++;
					break;
				case '"':
					goto outqs;
				}
				*cp2++ = c;
			}
		outqs:
			lastsp = 0;
			break;

		case ' ':
			lastsp = 1;
			break;

		case ',':
			*cp2++ = ',';
			*cp2++ = ' ';
			bufend = cp2;
			lastsp = 0;
			break;

		case '<':
			cp2 = bufend;
			gotlt++;
			lastsp = 0;
			break;

		case '>':
			if (gotlt) {
				gotlt = 0;
				while (*cp != ',' && *cp != 0)
					cp++;
				if (*cp == 0 )
					goto done;
				*cp2++ = ',';
				*cp2++ = ' ';
				bufend = cp2;
				break;
			}

			/* Fall into . . . */

		default:
			if (lastsp) {
				lastsp = 0;
				*cp2++ = ' ';
			}
			*cp2++ = c;
			break;
		}
	}
done:
	*cp2 = 0;

	return(savestr(nbuf));
}

/*
 * Fetch the sender's name from the passed message.
 * Reptype can be
 *	0 -- get sender's name for display purposes
 *	1 -- get sender's name for reply
 *	2 -- get sender's name for Reply
 */

/*ARGSUSED*/
char *
name1(mp, reptype)
	register struct message *mp;
{
	char namebuf[LINESIZE];
	char linebuf[LINESIZE];
	register char *cp, *cp2;
	register FILE *ibuf;
	int first = 1;

#ifdef SENDMAIL
	if ((cp = hfield("from", mp)) != NOSTR)
		return(cp);
	if (reptype == 0 && (cp = hfield("sender", mp)) != NOSTR)
		return(cp);
#endif
	ibuf = setinput(mp);
	copy("", namebuf);
	if (readline(ibuf, linebuf) <= 0)
		return(savestr(namebuf));
newname:
	for (cp = linebuf; *cp != ' '; cp++)
		;
	while (any(*cp, " \t"))
		cp++;
	for (cp2 = &namebuf[strlen(namebuf)]; *cp && !any(*cp, " \t") &&
	    cp2-namebuf < LINESIZE-1; *cp2++ = *cp++)
		;
	*cp2 = '\0';
nextname:							/* ARH */
	if (readline(ibuf, linebuf) <= 0)
		return(savestr(namebuf));
	if ((cp = index(linebuf, 'F')) == NULL)
		return(savestr(namebuf));
	if (strncmp(cp, "From", 4) != 0)
		return(savestr(namebuf));
	while ((cp = index(cp, 'r')) != NULL) {
		if (strncmp(cp, "remote", 6) == 0) {
			if ((cp = index(cp, 'f')) == NULL)
				break;
			if (strncmp(cp, "from", 4) != 0)
				break;
			if ((cp = index(cp, ' ')) == NULL)
				break;
			cp++;
			if (first) {
				copy(cp, namebuf);
				first = 0;
			} else
				strcpy(rindex(namebuf, '!')+1, cp);
			strcat(namebuf, "!");
			goto newname;
		}
		cp++;
	}
	goto nextname;						/* ARH */
}

/*
 * Are any of the characters in the two strings the same?
 */

anyof(s1, s2)
	register char *s1, *s2;
{
	register int c;

	while (c = *s1++)
		if (any(c, s2))
			return(1);
	return(0);
}

/*
 * See if the given header field is supposed to be ignored.
 */
isign(field)
	char *field;
{
	char realfld[BUFSIZ];

	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	istrcpy(realfld, field);

	if (nretained > 0)
		return (!member(realfld, retain));
	else
		return (member(realfld, ignore));
}

member(realfield, table)
	register char *realfield;
	register struct ignore **table;
{
	register struct ignore *igp;

	for (igp = table[hash(realfield)]; igp != 0; igp = igp->i_link)
		if (equal(igp->i_field, realfield))
			return (1);

	return (0);
}
