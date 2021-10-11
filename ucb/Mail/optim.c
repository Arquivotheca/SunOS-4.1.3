#ifndef lint
static	char *sccsid = "@(#)optim.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Network name modification routines.
 */

#include "rcv.h"
#include "configdefs.h"
#include <ctype.h>

/*
 * Map a name into the correct network "view" of the
 * name.  This is done by prepending the name with the
 * network address of the sender, then optimizing away
 * nonsense.
 */

char *
netmap(name, from)
	char name[], from[];
{
	char nbuf[BUFSIZ], ret[BUFSIZ];
	register char *cp, *cp2;
	char *unuucp();

	if (debug) fprintf(stderr, "netmap(name '%s', from '%s')\n", name, from);
	if (strlen(from) == 0)
		return(name);	/* "from" is empty - can't do anything */

	if (strcmp(from, name) == 0)
		return(name);	/* "from" and "name" are the same, do nothing */

	/*
	 * If the name contains an "@" or a "%", remove it and the host
	 * following it if that host is "known".
	 */
	if (any('@', name) || any('%', name))
		return(arpafix(name, from));

	/*
	 * If the sender contains a "@" or a "%", make "name" into an
	 * address on that host, on the presumption that it should
	 * really have read "name@from" when we received the message
	 * rather than just "name".
	 */
	if (any('@', from) || any('%', from))
		return(unuucp(makeremote(name, from)));

	if (value("onehop") && any('!', name)) {
		/*
		 * "onehop" is set, meaning all machines are one UUCP
		 * hop away (fat chance, in this day and age), and "name"
		 * is a UUCP path rather than just a name.  Leave it alone.
		 */
		strcpy(nbuf, name);
	} else {
		/*
		 * Why is "revarpa" being called?  We already know that
		 * "from" contains neither an "@" nor a "%", so it should
		 * just leave "from" alone.  Methinks this is a hangover
		 * from "Mail", where "from" was not tested for "@" nor
		 * for "%".  I suspect what it *should* do is do that
		 * test only if requested to do so.  A site that can't
		 * handle domain-style addresses should not request that
		 * it do so (or, at least, should not give up if the test
		 * succeeds), but should go ahead with this stuff.  That
		 * way, you can turn this into a "path" containing hosts
		 * and domains, in the (possibly vain) hope that whoever
		 * gets told to mail to "foo.bar.bletch!mumble" will
		 * understand that "foo.bar.bletch" does not represent
		 * the UUCP name of a host.
		 */
		cp = revarpa(from);
		if (debug) fprintf(stderr, "netmap: revarpa returns cp '%s'\n", cp);

		/*
		 * If we had an error trying to parse that name, give up
		 * and just (potentially) turn "...!bar!bletch" into
		 * "bletch@bar.UUCP".
		 */
		if (cp == NOSTR)
			return(unuucp(name));

		/*
		 * Remove the last component from this funny purely left-
		 * to-right "address", which will be a user name.  This gives
		 * us a funny purely-left-to-right "path".
		 */
		strcpy(nbuf, cp);
		cp = &nbuf[strlen(nbuf) - 1];
		while (!any(*cp, metanet) && cp > nbuf)
			cp--;

		/*
		 * If there are no components (i.e., "from" is just a name,
		 * not an address), don't do anything more than possibly
		 * turning "...!bar!bletch" into "bletch@bar.UUCP".
		 */
		if (cp == nbuf)
			return(unuucp(name));

		/*
		 * Turn the "funny" path back around, and glue the
		 * recipient's name onto the end.
		 */
		*++cp = 0;
		cp2 = revarpa(name);
		if (cp2 == NOSTR)
			return(unuucp(name));
		strcat(nbuf, cp2);
	}
	if (debug) fprintf(stderr, "before optim, nbuf '%s'\n", name);
#ifdef	OPTIM
	if ((cp = value("conv"))==NOSTR || strcmp(cp, "optimize") != 0)
		strcpy(ret, nbuf);
	else
		optim(nbuf, ret);
#else
	strcpy(ret, nbuf);
#endif	OPTIM
	if (debug) fprintf(stderr, "after  optim, nbuf '%s', ret '%s'\n", nbuf, ret);
	cp = revarpa(ret);
	if (debug) fprintf(stderr, "wind up with '%s'\n", name);
	if (!icequal(name, cp))
		return(unuucp((char *) savestr(cp)));
	return(unuucp(name));
}

/*
 * Optionally translate an old format uucp name into a new one, e.g.
 * "mach1!mach2!user" becomes "user@mach2.UUCP".  This optional because
 * some information is necessarily lost (e.g. the route it got here
 * via) and if we don't have the host in our routing tables, we lose.
 * XXX THIS IS NO LONGER VALID WITH THE NEW UUCP PROJECT PLANS TO
 * REGISTER UUCP HOSTS IN THE STANDARD INTERNET NAMESPACE, E.G.
 * ihnp4 BECOMES "ihnp4.att.com".
 */
char *
unuucp(name)
char *name;
{
	register char *np, *hp, *cp;
	char result[100];
	char tname[300];

	if (UnUUCP==0 &&
	    ((cp = value("conv"))==NOSTR || strcmp(cp, "internet")))
		return name;
	if (debug) fprintf(stderr, "unuucp(%s)\n", name);
	strcpy(tname, name);
	np = rindex(tname, '!');
	if (np == NOSTR)
		return name;
	*np++ = 0;
	hp = rindex(tname, '!');
	if (hp == NOSTR)
		hp = tname;
	else
		*hp++ = 0;
	cp = index(np, '@');
	if (cp == NOSTR)
		cp = index(np, '%');
	if (cp)
		*cp = 0;
	if (debug) fprintf(stderr, "host %s, name %s\n", hp, np);
	sprintf(result, "%s@%s.UUCP", np, hp);
	if (debug) fprintf(stderr, "unuucp returns %s\n", result);
	return savestr(result);
}

/*
 * Turn a network machine name into a unique character
 */
netlook(machine, attnet)
	char machine[];
{
	register struct netmach *np;
	register char *cp, *cp2;
	char nbuf[BUFSIZ];

	/*
	 * Make into lower case.
	 */

	for (cp = machine, cp2 = nbuf; *cp; *cp2++ = little(*cp++))
		if (cp2 >= &nbuf[sizeof(nbuf)-1])
			break;
	*cp2 = 0;

	/*
	 * If a single letter machine, look through those first.
	 */

	if (strlen(nbuf) == 1)
		for (np = netmach; np->nt_mid != 0; np++)
			if (np->nt_mid == nbuf[0])
				return(nbuf[0]);

	/*
	 * Look for usual name
	 */

	for (np = netmach; np->nt_mid != 0; np++)
		if (strcmp(np->nt_machine, nbuf) == 0)
			return(np->nt_mid);

	/*
	 * Look in side hash table.
	 */

	return(mstash(nbuf, attnet));
}

/*
 * Make a little character.
 */

little(c)
	register int c;
{

	if (c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	return(c);
}

#ifdef OPTIM
/*
 * Turn a network unique character identifier into a network name.
 */

char *
netname(mid)
{
	register struct netmach *np;
	char *mlook();

	if (mid & 0200)
		return(mlook(mid));
	for (np = netmach; np->nt_mid != 0; np++)
		if (np->nt_mid == mid)
			return(np->nt_machine);
	return(NOSTR);
}
#endif

/*
 * Deal with arpa net addresses.  The way this is done is strange.
 * name contains an "@" or "%".  Look up the machine after it in
 * the hash table.  If it isn't found, return name unmolested.
 * If ???, return name unmolested.
 * Otherwise, delete the "@" or "%" and the machine after it from
 * name, and return the new string.
 */
char *
arpafix(name, from)
	char name[];
	char from[];
{
	register char *cp;
	register int arpamach;
	char newname[BUFSIZ];

	if (debug) {
		fprintf(stderr, "arpafix(%s, %s)\n", name, from);
	}
	cp = rindex(name, '@');
	if (cp == NOSTR)
		cp = rindex(name, '%');
	if (cp == NOSTR) {
		fprintf(stderr, "Something's amiss -- no @ or % in arpafix\n");
		return(name);
	}
	cp++;
	arpamach = netlook(cp, '@');
	if (debug)
		fprintf(stderr,
		    "cp '%s', arpamach %o, nettypes arpamach %o LOCAL %o\n",
		    cp, arpamach, nettype(arpamach), nettype(LOCAL));
	if (arpamach == 0) {
		if (debug)
			fprintf(stderr, "machine %s unknown, uses: %s\n",
			    cp, name);
		return(name);
	}
	if (((nettype(arpamach) & nettype(LOCAL)) & ~AN) == 0) {
		if (debug)
			fprintf(stderr, "machine %s known but remote, uses: %s\n",
			    cp, name);
		return(name);
	}
	strcpy(newname, name);
	cp = rindex(newname, '@');
	if (cp == NOSTR)
		cp = rindex(newname, '%');
	*cp = 0;
	if (debug) fprintf(stderr, "local address, return '%s'\n", newname);
	return(savestr(newname));
}

/*
 * We have name with no @'s in it, and from with @'s.
 * Assume that name is meaningful only on the site in from,
 * and return "name@site_in_from".
 */
char *
makeremote(name, from)
	char name[];
	char from[];
{
	register char *cp;
	char rbuf[BUFSIZ];

	if (debug) fprintf(stderr, "makeremote(%s, %s) returns ", name, from);
	strcpy(rbuf, name);
	cp = rindex(from, '@');
	if (cp == NOSTR)
		cp = rindex(from, '%');
	strcat(rbuf, cp);
	if (debug) fprintf(stderr, "%s\n", rbuf);
	return(savestr(rbuf));
}

/*
 * Take a network machine descriptor and find the types of connected
 * nets and return it.
 */

nettype(mid)
{
	register struct netmach *np;

	if (mid & 0200)
		return(mtype(mid));
	for (np = netmach; np->nt_mid != 0; np++)
		if (np->nt_mid == mid)
			return(np->nt_type);
	return(0);
}

/*
 * Hashing routines to salt away machines seen scanning
 * networks paths that we don't know about.
 */

#define	XHSIZE		97		/* Size of extra hash table */
#define	NXMID		(XHSIZE*3/4)	/* Max extra machines */

struct xtrahash {
	char	*xh_name;		/* Name of machine */
	short	xh_mid;			/* Machine ID */
	short	xh_attnet;		/* Attached networks */
} xtrahash[XHSIZE];

struct xtrahash	*xtab[XHSIZE];		/* F: mid-->machine name */

short	midfree;			/* Next free machine id */

/*
 * Initialize the extra host hash table.
 * Called by sreset.
 */

minit()
{
	register struct xtrahash *xp, **tp;

	midfree = 0;
	tp = &xtab[0];
	for (xp = &xtrahash[0]; xp < &xtrahash[XHSIZE]; xp++) {
		xp->xh_name = NOSTR;
		xp->xh_mid = 0;
		xp->xh_attnet = 0;
		*tp++ = (struct xtrahash *) 0;
	}
}

/*
 * Stash a net name in the extra host hash table.
 * If a new entry is put in the hash table, deduce what
 * net the machine is attached to from the net character.
 *
 * If the machine is already known, add the given attached
 * net to those already known.
 */

mstash(name, attnet)
	char name[];
{
	register struct xtrahash *xp;
	struct xtrahash *xlocate();
	int x;

	xp = xlocate(name);
	if (xp == (struct xtrahash *) 0) {
		printf("Ran out of machine id spots\n");
		return(0);
	}
	if (xp->xh_name == NOSTR) {
		if (midfree >= XHSIZE) {
			printf("Out of machine ids\n");
			return(0);
		}
		xtab[midfree] = xp;
		xp->xh_name = savestr(name);
		xp->xh_mid = 0200 + midfree++;
	}
	x = ntype(attnet);
	if (x == 0)
		xp->xh_attnet |= AN;
	else
		xp->xh_attnet |= x;
	return(xp->xh_mid);
}

/*
 * Search for the given name in the hash table
 * and return the pointer to it if found, or to the first
 * empty slot if not found.
 *
 * If no free slots can be found, return 0.
 */

struct xtrahash *
xlocate(name)
	char name[];
{
	register int h, q, i;
	register char *cp;
	register struct xtrahash *xp;

	for (h = 0, cp = name; *cp; h = (h << 2) + *cp++)
		;
	if (h < 0 && (h = -h) < 0)
		h = 0;
	h = h % XHSIZE;
	cp = name;
	for (i = 0, q = 0; q < XHSIZE; i++, q = i * i) {
		xp = &xtrahash[(h + q) % XHSIZE];
		if (xp->xh_name == NOSTR)
			return(xp);
		if (strcmp(cp, xp->xh_name) == 0)
			return(xp);
		if (h - q < 0)
			h += XHSIZE;
		xp = &xtrahash[(h - q) % XHSIZE];
		if (xp->xh_name == NOSTR)
			return(xp);
		if (strcmp(cp, xp->xh_name) == 0)
			return(xp);
	}
	return((struct xtrahash *) 0);
}

#ifdef OPTIM
/*
 * Return the name from the extra host hash table corresponding
 * to the passed machine id.
 */

char *
mlook(mid)
{
	register int m;

	if ((mid & 0200) == 0)
		return(NOSTR);
	m = mid & 0177;
	if (m >= midfree) {
		printf("Use made of undefined machine id\n");
		return(NOSTR);
	}
	return(xtab[m]->xh_name);
}
#endif

/*
 * Return the bit mask of net's that the given extra host machine
 * id has so far.
 */

mtype(mid)
{
	register int m;

	if ((mid & 0200) == 0)
		return(0);
	m = mid & 0177;
	if (m >= midfree) {
		printf("Use made of undefined machine id\n");
		return(0);
	}
	return(xtab[m]->xh_attnet);
}

#ifdef	OPTIM
/*
 * Take a network name and optimize it.  This gloriously messy
 * operation takes place as follows:  the name with machine names
 * in it is tokenized by mapping each machine name into a single
 * character machine id (netlook).  The separator characters (network
 * metacharacters) are left intact.  The last component of the network
 * name is stripped off and assumed to be the destination user name --
 * it does not participate in the optimization.  As an example, the
 * name "res!vax!res!uvax!bill" becomes, tokenized,
 * "r!x!r!v!" and "bill"  A low level routine, optim1, fixes up the
 * network part (eg, "r!x!r!v!"), then we convert back to network
 * machine names and tack the user name on the end.
 *
 * The result of this is copied into the parameter "name"
 */

optim(net, name)
	char net[], name[];
{
	char netcomp[BUFSIZ], netstr[40], xfstr[40];
	register char *cp, *cp2;
	register int c;

	if (debug) fprintf(stderr, "optim(%s, %s) called\n", net, name);
	strcpy(netstr, "");
	cp = net;
	for (;;) {
		/*
		 * Rip off next path component into netcomp
		 */
		cp2 = netcomp;
		while (*cp && !any(*cp, metanet))
			*cp2++ = *cp++;
		*cp2 = 0;
		/*
		 * If we hit null byte, then we just scanned
		 * the destination user name.  Go off and optimize
		 * if its so.
		 */
		if (*cp == 0)
			break;
		if ((c = netlook(netcomp, *cp)) == 0) {
			printf("No host named \"%s\"\n", netcomp);
err:
			strcpy(name, net);
			return;
		}
		stradd(netstr, c);
		stradd(netstr, *cp++);
		/*
		 * If multiple network separators given,
		 * throw away the extras.
		 */
		while (any(*cp, metanet))
			cp++;
	}
	if (strlen(netcomp) == 0) {
		printf("net name syntax\n");
		goto err;
	}
	if (debug) fprintf(stderr, "optim1(%s,%s) called\n", netstr, xfstr);
	optim1(netstr, xfstr);
	if (debug) fprintf(stderr, "optim1(%s,%s) returns\n", netstr, xfstr);

	/*
	 * Convert back to machine names.
	 */

	cp = xfstr;
	strcpy(name, "");
	while (*cp) {
		if ((cp2 = netname(*cp++)) == NOSTR) {
			printf("Made up bad net name\n");
			printf("Machine code %c (0%o)\n", cp[-1], cp[-1]);
			printf("Sorry.\n");
			goto err;
		}
		strcat(name, cp2);
		stradd(name, *cp++);
	}
	strcat(name, netcomp);
	if (debug) fprintf(stderr, "optim returns %s in name\n", name);
}

/*
 * Take a string of network machine id's and separators and
 * optimize them.  We process these by pulling off maximal
 * leading strings of the same type, passing these to the appropriate
 * optimizer and concatenating the results.
 */

optim1(netstr, name)
	char netstr[], name[];
{
	char path[40], rpath[40];
	register char *cp, *cp2;
	register int tp, nc;

	cp = netstr;
	prefer(cp);
	strcpy(name, "");
	/*
	 * If the address ultimately points back to us,
	 * just return a null network path.
	 */
	if (strlen(cp) > 1 && cp[strlen(cp) - 2] == LOCAL)
		return;
	while (*cp != 0) {
		strcpy(path, "");
		tp = ntype(cp[1]);
		nc = cp[1];
		while (*cp && tp == ntype(cp[1])) {
			stradd(path, *cp++);
			cp++;
		}
		switch (netkind(tp)) {
		default:
			strcpy(rpath, path);
			break;

		case IMPLICIT:
			optimimp(path, rpath);
			break;

		case EXPLICIT:
			optimex(path, rpath);
			break;
		}
		for (cp2 = rpath; *cp2 != 0; cp2++) {
			stradd(name, *cp2);
			stradd(name, nc);
		}
	}
	optiboth(name);
	prefer(name);
}
#endif	OPTIM

/*
 * Return the network of the separator --
 *	AN for arpa net
 *	BN for Bell labs net	(e.g. UUCP, NOT Berknet)
 *	SN for Schmidt net	(Berknet)
 *	0 if we don't know.
 */

ntype(nc)
	register int nc;
{
	register struct ntypetab *np;

	for (np = ntypetab; np->nt_char != 0; np++)
		if (np->nt_char == nc)
			return(np->nt_bcode);
	return(0);
}

#ifdef	OPTIM
/*
 * Return the kind of routing used for the particular net
 * EXPLICIT means explicitly routed
 * IMPLICIT means implicitly routed
 * 0 means don't know
 */

netkind(nt)
	register int nt;
{
	register struct nkindtab *np;

	for (np = nkindtab; np->nk_type != 0; np++)
		if (np->nk_type == nt)
			return(np->nk_kind);
	return(0);
}

/*
 * Do name optimization for an explicitly routed network (eg uucp).
 */

optimex(net, name)
	char net[], name[];
{
	register char *cp, *rp;
	register int m;
	char *rindex();

	strcpy(name, net);
	cp = name;
	if (strlen(cp) == 0)
		return(-1);
	if (cp[strlen(cp)-1] == LOCAL) {
		name[0] = 0;
		return(0);
	}
	for (cp = name; *cp; cp++) {
		m = *cp;
		rp = rindex(cp+1, m);
		if (rp != NOSTR)
			strcpy(cp, rp);
	}
	return(0);
}

/*
 * Do name optimization for implicitly routed network (eg, arpanet).
 */

optimimp(net, name)
	char net[], name[];
{
	register char *cp;
	register int m;

	cp = net;
	if (strlen(cp) == 0)
		return(-1);
	m = cp[strlen(cp) - 1];
	if (m == LOCAL) {
		strcpy(name, "");
		return(0);
	}
	name[0] = m;
	name[1] = 0;
	return(0);
}

/*
 * Perform global optimization on the given network path.
 * The trick here is to look ahead to see if there are any loops
 * in the path and remove them.  The interpretation of loops is
 * more strict here than in optimex since both the machine and net
 * type must match.
 */

optiboth(net)
	char net[];
{
	register char *cp, *cp2;
	char *rpair();

	cp = net;
	if (strlen(cp) == 0)
		return;
	if ((strlen(cp) % 2) != 0) {
		printf("Strange arg to optiboth\n");
		return;
	}
	while (*cp) {
		cp2 = rpair(cp+2, *cp);
		if (cp2 != NOSTR)
			strcpy(cp, cp2);
		cp += 2;
	}
}

/*
 * Find the rightmost instance of the given (machine, type) pair.
 */

char *
rpair(str, mach)
	char str[];
{
	register char *cp, *last;

	cp = str;
	last = NOSTR;
	while (*cp) {
		if (*cp == mach)
			last = cp;
		cp += 2;
	}
	return(last);
}

/*
 * Change the network separators in the given network path
 * to the preferred network transmission means.
 */

prefer(name)
	char name[];
{
	register char *cp;
	register int state, n;

	state = LOCAL;
	for (cp = name; *cp; cp += 2) {
		n = best(state, *cp);
		if (n)
			cp[1] = n;
		state = *cp;
	}
}

/*
 * Return the best network separator for the given machine pair.
 */

best(src, dest)
{
	register int dtype, stype;
	register struct netorder *np;

	stype = nettype(src);
	dtype = nettype(dest);
	fflush(stdout);
	if (stype == 0 || dtype == 0) {
		printf("ERROR:  unknown internal machine id\n");
		return(0);
	}
	if ((stype & dtype) == 0)
		return(0);
	np = &netorder[0];
	while ((np->no_stat & stype & dtype) == 0)
		np++;
	return(np->no_char);
}
#endif	OPTIM

/*
 * Code to twist around arpa net names.
 */

#define WORD 257			/* Token for a string */

static	char netbuf[256];
static	char *yylval;

/*
 * Reverse all of the arpa net addresses in the given name to
 * be of the form "host @ user" instead of "user @ host"
 * This function is its own inverse.
 */

char *
revarpa(str)
	char str[];
{

	if (yyinit(str) < 0)
		return(NOSTR);
	if (name())
		return(NOSTR);
	if (strcmp(str, netbuf) == 0)
		return(str);
	return(savestr(netbuf));
}

/*
 * Parse (by recursive descent) network names, using the following grammar:
 *	name:
 *		term {':' term}
 *		term {'^' term}
 *		term {'!' term}
 *		term '@' name
 *		term '%' name
 *
 *	term:
 *		string of characters.
 */

name()
{
	register int t;
	register char *cp;

	for (;;) {
		t = yylex();
		if (t != WORD)
			return(-1);
		cp = yylval;
		t = yylex();
		switch (t) {
		case 0:
			strcat(netbuf, cp);
			return(0);

		case '@':
		case '%':
			if (name())
				return(-1);
			stradd(netbuf, '@');
			strcat(netbuf, cp);
			return(0);

		case WORD:
			return(-1);

		default:
			strcat(netbuf, cp);
			stradd(netbuf, t);
		}
	}
}

/*
 * Scanner for network names.
 */

static	char *charp;			/* Current input pointer */
static	int nexttok;			/* Salted away next token */

/*
 * Initialize the network name scanner.
 */

yyinit(str)
	char str[];
{
	static char lexbuf[BUFSIZ];

	netbuf[0] = 0;
	if (strlen(str) >= sizeof lexbuf - 1)
		return(-1);
	nexttok = 0;
	strcpy(lexbuf, str);
	charp = lexbuf;
	return(0);
}

/*
 * Scan and return a single token.
 * yylval is set to point to a scanned string.
 */

yylex()
{
	register char *cp, *dotp;
	register int s;

	if (nexttok) {
		s = nexttok;
		nexttok = 0;
		return(s);
	}
	cp = charp;
	while (*cp && isspace(*cp))
		cp++;
	if (*cp == 0)
		return(0);
	if (any(*cp, metanet)) {
		charp = cp+1;
		return(*cp);
	}
	dotp = cp;
	while (*cp && !any(*cp, metanet) && !any(*cp, " \t"))
		cp++;
	if (any(*cp, metanet))
		nexttok = *cp;
	if (*cp == 0)
		charp = cp;
	else
		charp = cp+1;
	*cp = 0;
	yylval = dotp;
	return(WORD);
}

/*
 * Add a single character onto a string.
 */

stradd(str, c)
	register char *str;
	register int c;
{
	while (*str)
		str++;
	*str++ = c;
	*str = '\0';
}
