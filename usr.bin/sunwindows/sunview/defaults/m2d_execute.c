#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)m2d_execute.c 1.1 92/07/30";
#endif
#endif

#include <stdio.h>
#include "m2d_def.h"
#include <sys/stat.h>
#include <sunwindow/sun.h>

int	d2m_special_cmd();
char	*malloc();

extern struct      grouphead       *m2d_groups[];/* Pointer to active groups */
int	m2d_invoker = NOPROG;

static int	cond = CANY;		/* current state of conditional exc. */

static int	group(), set(), ifcmd(), elsecmd(), endifcmd();

/* minimum number of args to set is 1 to alias definition is 2 */
static struct cmd cmdtab[] = {
	"alias",        group,          0,      2,      1000,
	"set",          set,            0,      1,      1000,
	"if",		ifcmd,		0,	1,	1,
	"else",		elsecmd,	0,	0,	0,
	"endif",	endifcmd,	0,	0,	0,
	0,              0,              0,              0,      0
};


static char	line[LINESIZE];

m2d_commands(mailrc_fd)
FILE	*mailrc_fd;
{

	/* execute each line that is a set or an alias */
	/* adding their contents to an appropriate data structure */
        while(readline(mailrc_fd, line)) 
		(void)execute(line);
}


d2m_commands(mailrc_fd)
FILE	*mailrc_fd;
{
	int	known;

	/* execute each line that is a set or an alias */
	/* adding their contents to an appropriate data structure */
        while(readline(mailrc_fd, line)) {
		known = execute(line);
		d2m_special_cmd(line, known);
	}
}


static
execute(linebuf)
	char linebuf[];
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	struct cmd *com;
	register char *cp, *cp2;
	register int c;
	int edstop(), e;
	int	len;

	/* check for blank lines */
	len = strlen(linebuf);
	if (len == 0)
		return(EX_UNKNOWN);
	if (linebuf[len - 1] == '\n') {
		linebuf[len - 1] = '\0';
		if (len == 1)
			return(EX_UNKNOWN);
	}

	cp = linebuf;
	while (any(*cp, " \t"))
		cp++;
	cp2 = word;
	while (*cp && !any(*cp, " \t0123456789$^.:/-+*'\""))
		*cp2++ = *cp++;
	*cp2 = '\0';

	if (word[0] == '\0')
		return(EX_UNKNOWN);
	if ((com = lex(word)) == NONE)
		return(EX_UNKNOWN);
	if ((c = getrawlist(cp, arglist)) < 0)
		return(EX_UNKNOWN);
	if (c < com->c_minargs) {
		(void)printf("%s requires at least %d arg(s)\n",
			com->c_name, com->c_minargs);
		return(EX_UNKNOWN);
	}
	if (c > com->c_maxargs) {
		(void)printf("%s takes no more than %d arg(s)\n",
			com->c_name, com->c_maxargs);
		return(EX_UNKNOWN);
	}
	e = (*com->c_func)(arglist);
	return((e == 0) ? EX_KNOWN : EX_UNKNOWN);
}

/*
 * Is ch any of the characters in str?
 */

static
any(ch, str)
	char *str;
{
	register char *f;
	register c;

	f = str;
	c = ch;
	while (*f)
		if (c == *f++)
			return(1);
	return(0);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

static struct cmd *
lex(word)
	char word[];
{
	register struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NOSTR; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NONE);
}

/*
 * Return a pointer to a dynamic copy of the argument.
 */

static char *
savestr(str)
	char *str;
{
	register char *cp, *cp2, *top;

	for (cp = str; *cp; cp++)
		;
	top = malloc((unsigned)(cp-str + 1));
	if (top == NOSTR)
		return(NOSTR);
	for (cp = str, cp2 = top; *cp; cp++)
		*cp2++ = *cp;
	*cp2 = 0;
	return(top);
}

/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */

/* ARGSUSED */
static
getrawlist(line_local, argv)
	char line_local[];
	char **argv;
{
	register char **ap, *cp, *cp2;
	char linebuf[BUFSIZ], quotec;

	ap = argv;
	cp = line_local;
	while (*cp != '\0') {
		while (any(*cp, " \t"))
			cp++;
		cp2 = linebuf;
		quotec = 0;
		while (*cp != '\0') {
			if (quotec) {
				if (*cp == quotec)
					quotec=0;
				*cp2++ = *cp++;
			} else {
				if (any(*cp, " \t"))
					break;
				if (any(*cp, "'\""))
					quotec = *cp;
				*cp2++ = *cp++;
			}
		}
		*cp2 = '\0';
		if (cp2 == linebuf)
			break;
		*ap++ = savestr(linebuf);
	}
	*ap = NOSTR;
	return(ap-argv);
}


/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */

static
isprefix(as1, as2)
	char *as1, *as2;
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */

static
ifcmd(argv)
	char **argv;
{
	register char *cp;

	if (cond != CANY) {
		(void)printf("Illegal nested \"if\"\n");
		return(2);
	}
	cond = CANY;
	cp = argv[0];
	switch (*cp) {
	case 'r': case 'R':
		cond = CRCV;
		break;

	case 's': case 'S':
		cond = CSEND;
		break;

	case 't': case 'T':
		cond = CTTY;
		break;

	default:
		(void)printf("Unrecognized if-keyword: \"%s\"\n", cp);
		return(2);
	}
	return(1);
}

/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */

static
elsecmd()
{

	switch (cond) {
	case CANY:
		(void)printf("\"Else\" without matching \"if\"\n");
		return(2);

	case CSEND:
		cond = CRCV;
		break;

	case CRCV:
		cond = CSEND;
		break;

	case CTTY:
		cond = CNOTTY;
		break;

	case CNOTTY:
		cond = CTTY;
		break;

	default:
		(void)printf("invalid condition encountered\n");
		cond = CANY;
		break;
	}
	return(1);
}

/*
 * End of if statement.  Just set cond back to anything.
 */

static
endifcmd()
{

	if (cond == CANY) {
		(void)printf("\"Endif\" without matching \"if\"\n");
		return(2);
	}
	cond = CANY;
	return(1);
}


/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

static
readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;
	int seennulls = 0;

	do {
		clearerr(ibuf);
		c = getc(ibuf);
		for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
			if (c == 0) {
				if (!seennulls) {
					fputs("Mail: ignoring NULL characters in mail\n", stderr);
					seennulls++;
				}
				continue;
			}
			if (cp - linebuf < LINESIZE-2) {
				if (c == '\\') {
					c = getc(ibuf);
					c = ' ';
				}
				*cp++ = c;
			}
		}
	} while (ferror(ibuf) && ibuf == stdin);
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}

static
set(arglist)
	char **arglist;
{
	register char *cp, *cp2;
	char varbuf[BUFSIZ], **ap;
	int errs;

	if (cond != CANY)
		return(-1);
	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++) {
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			(void)printf("Non-null variable name required\n");
			errs++;
			continue;
		}
		if (m2d_invoker == MAILRC_TO_DEFAULTS)
			m2d_assign(varbuf, cp);
	}
	return(errs);
}


/*
 * Put add users to a group.
 */

static
group(argv)
	char **argv;
{
	register struct grouphead *gh;
	register struct group *gp;
	register int h;
	char **ap, *gname;

	if (cond != CANY)
		return(-1);
	if (m2d_invoker == DEFAULTS_TO_MAILRC)
		return(0);
	gname = *argv;
	h = m2d_hash(gname);
	if ((gh = m2d_findgroup(gname)) == NOGRP) {
		gh = (struct grouphead *) (LINT_CAST(calloc(
		    1, (unsigned)(sizeof *gh))));
		gh->g_name = m2d_vcopy(gname);
		gh->g_list = NOGE;
		gh->g_link = m2d_groups[h];
		m2d_groups[h] = gh;
	}

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	for (ap = argv+1; *ap != NOSTR; ap++) {
		gp = (struct group *) (LINT_CAST(
			calloc(1, (unsigned)(sizeof *gp))));
		gp->ge_name = m2d_vcopy(*ap);
		gp->ge_link = gh->g_list;
		gh->g_list = gp;
	}
	return(0);
}

