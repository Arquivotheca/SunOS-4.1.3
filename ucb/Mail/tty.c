#ifndef lint
static	char *sccsid = "@(#)tty.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Generally useful tty stuff.
 */

#include "rcv.h"

#ifdef	USG_TTY

#include <ctype.h>

#define	CTRL(x)	(('x')&037)		/* control character */

static	int	c_erase;		/* Current erase char */
static	int	c_kill;			/* Current kill char */
static	int	c_intr;			/* interrupt char */
static	int	c_quit;			/* quit character */
static	int	c_word;			/* Current word erase char */
static	int	Col;			/* current output column */
static	int	Pcol;			/* end column of prompt string */
static	int	Out;			/* file descriptor of stdout */
static	struct termio savtty, ttybuf;
static	char canonb[BUFSIZ];		/* canonical buffer for input */
					/* processing */
static	int	erasing;		/* we are erasing characters */

/*
 * Read all relevant header fields.
 */

grabh(hp, gflags)
register struct header *hp;
{
	void (*savesigs[2])();
	register int s;

	Out = fileno(stdout);
	if(ioctl(fileno(stdin), TCGETA, &savtty) < 0) {
		perror("ioctl");
		return;
	}
	c_erase = savtty.c_cc[VERASE];
	c_kill = savtty.c_cc[VKILL];
	c_intr = savtty.c_cc[VINTR];
	c_quit = savtty.c_cc[VQUIT];
	for (s = SIGINT; s <= SIGQUIT; s++)
		if ((savesigs[s-SIGINT] = sigset(s, SIG_IGN)) == SIG_DFL)
			sigset(s, SIG_DFL);
	if (gflags & GTO) {
		hp->h_to = readtty("To: ", hp->h_to);
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT) {
		hp->h_subject = readtty("Subject: ", hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
		hp->h_cc = readtty("Cc: ", hp->h_cc);
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
		hp->h_bcc = readtty("Bcc: ", hp->h_bcc);
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
	for (s = SIGINT; s <= SIGQUIT; s++)
		sigset(s, savesigs[s-SIGINT]);
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 */

char *
readtty(pr, src)
	char pr[], src[];
{
	int c;
	register char *cp, *cp2;

	erasing = 0;
	c_word = CTRL(W);	/* erase word character */
	fflush(stdout);
	Col = 0;
	outstr(pr);
	Pcol = Col;
	if (src != NOSTR && strlen(src) > BUFSIZ - 1) {
		printf("too long to edit\n");
		return(src);
	}
	if (setty(Out))
		return(src);
	cp2 = src==NOSTR ? "" : src;
	for (cp=canonb; *cp2; cp++, cp2++)
		*cp = *cp2;
	*cp = '\0';
	outstr(canonb);

	for (;;) {
		c = getc(stdin) & 0177;

		if (c==c_erase) {
			if (cp > canonb)
				if (cp[-1]=='\\' && !erasing) {
					*cp++ = c;
					Echo(c);
				} else {
					rubout(--cp);
				}
		} else if (c==c_kill) {
			if (cp > canonb && cp[-1]=='\\') {
				*cp++ = c;
				Echo(c);
			} else while (cp > canonb) {
				rubout(--cp);
			}
		} else if (c==c_word) {
			if (cp > canonb)
				if (cp[-1]=='\\' && !erasing) {
					*cp++ = c;
					Echo(c);
				} else {
					while (--cp >= canonb)
						if (isalnum(*cp))
							break;
						else
							rubout(cp);
					while (cp >= canonb)
						if (isalnum(*cp))
							rubout(cp--);
						else
							break;
					if (cp < canonb)
						cp = canonb;
					else if (*cp)
						cp++;
				}
		} else if (c==EOF || ferror(stdin) || c==c_intr || c==c_quit) {
			resetty(Out);
			write(Out, "\n", 1);
			xhalt();
		} else switch (c) {
			case '\n':
			case '\r':
				resetty(Out);
				write(Out, "\n", 1);
				if (canonb[0]=='\0')
					return(NOSTR);
				else
					return(savestr(canonb));
				break; /* not reached */
			default:
				erasing = 0;
				*cp++ = c;
				*cp = '\0';
				Echo(c);
		}
	}
}

setty(f)
{
	if (ioctl(f, TCGETA, &savtty) < 0) {
		perror("ioctl");
		return(-1);
	}
	ttybuf = savtty;
#ifdef	u370
	ttybuf.c_cflag &= ~PARENB;	/* disable parity */
	ttybuf.c_cflag |= CS8;		/* character size = 8 */
#endif	u370
	ttybuf.c_cc[VTIME] = 01;
	ttybuf.c_cc[VINTR] = 0;
	ttybuf.c_cc[VQUIT] = 0;
	ttybuf.c_lflag &= ~(ICANON|ECHO);
	if (ioctl(f, TCSETA, &ttybuf) < 0) {
		perror("ioctl");
		return(-1);
	}
	return(0);
}

resetty(f)
{
	if (ioctl(f, TCSETA, &savtty) < 0)
		perror("ioctl");
}

outstr(s)
register char *s;
{
	while (*s)
		Echo(*s++);
}

rubout(cp)
register char *cp;
{
	register int oldcol;
	register int c = *cp;

	erasing = 1;
	*cp = '\0';
	switch (c) {
	case '\t':
		oldcol = countcol();
		do
			write(Out, "\b", 1);
		while (--Col > oldcol);
		break;
	case '\b':
		if (isprint(cp[-1]))
			write(Out, cp-1, 1);
		else
			write(Out, " ", 1);
		Col++;
		break;
	default:
		if (isprint(c)) {
			write(Out, "\b \b", 3);
			Col--;
		}
	}
}

countcol()
{
	register int col;
	register char *s;

	for (col=Pcol, s=canonb; *s; s++)
		switch (*s) {
		case '\t':
			while (++col % 8)
				;
			break;
		case '\b':
			col--;
			break;
		default:
			if (isprint(*s))
				col++;
		}
	return(col);
}

Echo(cc)
{
	char c = cc;

	switch (c) {
	case '\t':
		do
			write(Out, " ", 1);
		while (++Col % 8);
		break;
	case '\b':
		if (Col > 0) {
			write(Out, "\b", 1);
			Col--;
		}
		break;
	case '\r':
	case '\n':
		Col = 0;
		write(Out, "\r\n", 2);
		break;
	default:
		if (isprint(c)) {
			Col++;
			write(Out, &c, 1);
		}
	}
}
#else

static	int	c_erase;		/* Current erase char */
static	int	c_kill;			/* Current kill char */
static	int	hadcont;		/* Saw continue signal */
static	jmp_buf	rewrite;		/* Place to go when continued */
#ifndef TIOCSTI
static	int	ttyset;			/* We must now do erase/kill */
#endif

/*
 * Read all relevant header fields.
 */

grabh(hp, gflags)
	struct header *hp;
{
	struct sgttyb ttybuf;
	void ttycont(), signull();
#ifndef TIOCSTI
	int (*savesigs[2])();
	register int s;
#endif
	void (*savecont)();

# ifdef VMUNIX
	savecont = sigset(SIGCONT, signull);
# endif VMUNIX
#ifndef TIOCSTI
	ttyset = 0;
#endif
	if (gtty(fileno(stdin), &ttybuf) < 0) {
		perror("gtty");
		return;
	}
	c_erase = ttybuf.sg_erase;
	c_kill = ttybuf.sg_kill;
#ifndef TIOCSTI
	ttybuf.sg_erase = 0;
	ttybuf.sg_kill = 0;
	for (s = SIGINT; s <= SIGQUIT; s++)
		if ((savesigs[s-SIGINT] = sigset(s, SIG_IGN)) == SIG_DFL)
			sigset(s, SIG_DFL);
#endif
	if (gflags & GTO) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_to != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_to = readtty("To: ", hp->h_to);
		if (hp->h_to != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GSUBJECT) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_subject != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_subject = readtty("Subject: ", hp->h_subject);
		if (hp->h_subject != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_cc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_cc = readtty("Cc: ", hp->h_cc);
		if (hp->h_cc != NOSTR)
			hp->h_seq++;
	}
	if (gflags & GBCC) {
#ifndef TIOCSTI
		if (!ttyset && hp->h_bcc != NOSTR)
			ttyset++, stty(fileno(stdin), &ttybuf);
#endif
		hp->h_bcc = readtty("Bcc: ", hp->h_bcc);
		if (hp->h_bcc != NOSTR)
			hp->h_seq++;
	}
# ifdef VMUNIX
	sigset(SIGCONT, savecont);
# endif VMUNIX
#ifndef TIOCSTI
	ttybuf.sg_erase = c_erase;
	ttybuf.sg_kill = c_kill;
	if (ttyset)
		stty(fileno(stdin), &ttybuf);
	for (s = SIGINT; s <= SIGQUIT; s++)
		sigset(s, savesigs[s-SIGINT]);
#endif
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 *
 */

char *
readtty(pr, src)
	char pr[], src[];
{
	char ch, canonb[BUFSIZ];
	int c;
	void signull();
	register char *cp, *cp2;

	fputs(pr, stdout);
	fflush(stdout);
	if (src != NOSTR && strlen(src) > BUFSIZ - 2) {
		printf("too long to edit\n");
		return(src);
	}
#ifndef TIOCSTI
	if (src != NOSTR)
		cp = copy(src, canonb);
	else
		cp = copy("", canonb);
	fputs(canonb, stdout);
	fflush(stdout);
#else
	cp = src == NOSTR ? "" : src;
	while (c = *cp++) {
		if (c == c_erase || c == c_kill) {
			ch = '\\';
			ioctl(0, TIOCSTI, &ch);
		}
		ch = c;
		ioctl(0, TIOCSTI, &ch);
	}
	cp = canonb;
	*cp = 0;
#endif
	cp2 = cp;
	while (cp2 < canonb + BUFSIZ)
		*cp2++ = 0;
	cp2 = cp;
	if (setjmp(rewrite))
		goto redo;
# ifdef VMUNIX
	sigset(SIGCONT, ttycont);
# endif VMUNIX
	clearerr(stdin);
	while (cp2 < canonb + BUFSIZ) {
		c = getc(stdin);
		if (c == EOF || c == '\n')
			break;
		*cp2++ = c;
	}
	*cp2 = 0;
# ifdef VMUNIX
	sigset(SIGCONT, signull);
# endif VMUNIX
	if (c == EOF && ferror(stdin) && hadcont) {
redo:
		hadcont = 0;
		cp = strlen(canonb) > 0 ? canonb : NOSTR;
		clearerr(stdin);
		return(readtty(pr, cp));
	}
	clearerr(stdin);
#ifndef TIOCSTI
	if (cp == NOSTR || *cp == '\0')
		return(src);
	cp2 = cp;
	if (!ttyset)
		return(strlen(canonb) > 0 ? savestr(canonb) : NOSTR);
	while (*cp != '\0') {
		c = *cp++;
		if (c == c_erase) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2--;
			continue;
		}
		if (c == c_kill) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2 = canonb;
			continue;
		}
		*cp2++ = c;
	}
	*cp2 = '\0';
#endif
	if (equal("", canonb))
		return(NOSTR);
	return(savestr(canonb));
}

# ifdef VMUNIX
/*
 * Receipt continuation.
 */
/*ARGSUSED*/
void
ttycont(s)
{

	hadcont++;
	longjmp(rewrite, 1);
}
# endif VMUNIX

/*
 * Null routine to satisfy
 * silly system bug that denies us holding SIGCONT
 */
/*ARGSUSED*/
void
signull(s)
{}
#endif	USG_TTY
