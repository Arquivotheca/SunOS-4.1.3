#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)gethostent.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/* 
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <rpcsvc/ypclnt.h>
struct hostent *_gethostent();

/*
 * Internet version.
 */
static struct hostdata {
	FILE	*hostf;
	char	*current;
	int	currentlen;
	int	stayopen;
#define	MAXALIASES	20
	char	*host_aliases[MAXALIASES];
#define MAXADDRS	10
#define	HOSTADDRSIZE	4	/* assumed == sizeof u_long */
	char	hostaddr[MAXADDRS][HOSTADDRSIZE];
	char	*addr_list[MAXADDRS+1];
	char	line[BUFSIZ+1];
	struct	hostent host;
	char	*domain;
	char	*map;
	int 	notusingyellow;
} *hostdata, *_hostdata();

static	struct hostent *interpret();
struct	hostent *gethostent();
char	*inet_ntoa();
char	*strpbrk();

static	char *HOSTDB = "/etc/hosts";

/*
 * Internal routine to allocate hostdata on heap, instead of
 * putting lots of stuff into the data segment.
 */
static struct hostdata *
_hostdata()
{
	register struct hostdata *d = hostdata;

	if (d == 0) {
		d = (struct hostdata *)calloc(1, sizeof (struct hostdata));
		hostdata = d;
		(void) yp_get_default_domain(&d->domain);
	}
	return (d);
}

/*
 * NIS use strategy:
 * gethostbyaddr() and gethostbyname() always try NIS first.
 * gethostent() tries NIS first, if it fails with an indication
 * that NIS is not running it sets a flag so that subsequent
 * calls will not try NIS.  If sethostent() is called 
 * the next gethostent() will try NIS first. An intervening
 * gethostbyaddr() or gethostbyname() will also  
 * clear the flag so that gethostent() will go back to trying NIS
 */

struct hostent *
gethostbyaddr(addr, len, type)
	char *addr;
	register int len, type;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;
	char *adrstr, *val = NULL;
	int vallen;
	int stat;

	h_errno = NO_RECOVERY;
	if (d == 0)
		return ((struct hostent*)NULL);
	d->map = "hosts.byaddr";

	sethostent(0);
	if (d->domain ) {
		/*try to yp_match it*/
		adrstr = inet_ntoa(*(struct in_addr *)addr);
		stat = yp_match(d->domain, d->map, adrstr, strlen(adrstr), 
					&val, &vallen);
		switch (stat) {
		    case 0:
			p = interpret(val, vallen);
			free(val);
			h_errno = 0;
			return(p);

		    case YPERR_KEY: 
			h_errno = HOST_NOT_FOUND;	
			return(NULL);

		    case YPERR_NOMORE:
			h_errno = TRY_AGAIN;	
			return(NULL);
		}
	}
	h_errno = HOST_NOT_FOUND;
	_sethostent(0);
	while (p = _gethostent()) {
		if (p->h_addrtype != type || p->h_length != len)
			continue;
		if (bcmp(p->h_addr_list[0], addr, len) == 0) {	
			h_errno = 0;
			break;
		}
	}
	endhostent();
	return (p);
}

struct hostent *
gethostbyname(name)
	register char *name;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;
	register char **cp;
	register char *name2;
	char *val = NULL;
	int vallen,stat;

	h_errno = NO_RECOVERY;
	if (d == 0)
		return ((struct hostent*)NULL);
	d->map = "hosts.byname";

	sethostent(0);
	if (d->domain ) {
		  /*
		   * try to yp_match it first
		   */
		name2 = name;
		for (val=d->line; *name2; name2++)
			*val++ = isupper((unsigned char)*name2) ? tolower(*name2) : *name2;
		*val = '\0';
		stat = yp_match(d->domain, d->map,
			    d->line, val - d->line, &val, &vallen);
		switch (stat) {
		    case 0:
			h_errno = 0;
			p = interpret(val, vallen);
			free(val);
			return(p);

		    case YPERR_KEY: 
			h_errno = HOST_NOT_FOUND;	
			return(NULL);

		    case YPERR_NOMORE:
			h_errno = TRY_AGAIN;	
			return(NULL);
		}
	
	}

	/* yp_match said NIS isn't running or domain is null */
	h_errno = HOST_NOT_FOUND;
	_sethostent(0);
	while (p = _gethostent()) {
		if (strcasecmp(p->h_name, name) == 0) {
			h_errno = 0;
			break;
		}
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (strcasecmp(*cp, name) == 0)	{
				h_errno = 0;
				goto found;
			}
	}

found:
	endhostent();
	return (p);
}

sethostent(f)
int f;
{
/*Doesn't open file if not needed*/
/*f!=0 -> makes it open file     */

	register struct hostdata *d = _hostdata();

	if (d == 0)
		return;
	d->notusingyellow = 0; /*try yellow again if off*/
	if (f) {
		_sethostent(f);
		return; 
	}
	if (d->hostf) 
		rewind(d->hostf); /*if its open rewind it*/
	if (d->current)
		free(d->current);
	d->current = NULL;
}

static _sethostent(f)
	int f;
{
	register struct hostdata *d = _hostdata();

	if (d == 0)
		return;
	if (d->hostf == NULL)
		d->hostf = fopen(HOSTDB, "r");
	else
		rewind(d->hostf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

endhostent()
{
	register struct hostdata *d = _hostdata();

	if (d == 0)
		return;
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->hostf && !d->stayopen) {
		(void) fclose(d->hostf);
		d->hostf = NULL;
	}
}

struct hostent *
gethostent()
{
	register struct hostdata *d = _hostdata();
	struct hostent *hp;
	char *key = NULL, *val = NULL;
	int keylen, vallen, stat;

	if (d == 0)
		return ((struct hostent*)NULL);
	d->map = "hosts.byaddr";

	if (d->domain && !(d->notusingyellow)) {
		if (d->current == NULL) {
			stat = yp_first(d->domain, d->map, &key, &keylen, 
					&val, &vallen);
		} else {
			stat = yp_next(d->domain, d->map,
			    d->current, d->currentlen, &key, &keylen, 
			    		&val, &vallen);
		}
		switch (stat) {
		    case YPERR_NOMORE:
			return(NULL);

		    case YPERR_KEY:
		    	return(NULL); /* first might return this*/

		    case 0:
			if (d->current)
			free(d->current);
			d->current = key;
			d->currentlen = keylen;
			hp = interpret(val, vallen);
			free(val);
			return (hp);
		}
		/*All other cases mean no NIS!*/
		d->notusingyellow = 1;
	}
	return(_gethostent()); /*The hard way*/
}

static struct hostent *
_gethostent()
{
	register struct hostdata *d = _hostdata();

	if (d->hostf == NULL && (d->hostf = fopen(HOSTDB, "r")) == NULL)
		return (NULL);
        if (fgets(d->line, BUFSIZ, d->hostf) == NULL)
		return (NULL);
	return (interpret(d->line, strlen(d->line)));
}

/*
 * Take the line read from /etc/hosts or from the NIS map, and parse it
 * into a hostent structure.  Multiple lines are used to encode multiple
 * addresses for the same name.  This should only happen when NIS talks to
 * the Domain Name Service, but it should not hurt to put it here.
 */
static struct hostent *
interpret(val, len)
char	*val;
int	len;
{
	register struct hostdata *d = _hostdata();
	char *p, *z;
	register char *cp, **q;
	int i;

	if (d == 0)
		return (0);
	(void) strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	d->line[len+1] = '\0';
	/*
	 * Note that these are recursive calls, and thus should only
	 * happen when reading /etc/hosts.
	 */
	if ((*p == '#') || (*p == '\n')) /* Check for special cases */
		return (gethostent());
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (gethostent());
	z = cp;
	if (*z == '#')
		z = strpbrk(cp, "\n");
	*cp = '\0';
	/* 
	 * This is what causes the blank line bug, we find the \n but no
	 * whitespace after it and return NULL. fixed by special casing the
	 * blank lines above.
	 */
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (NULL);
	*cp++ = '\0';

	/* THIS STUFF IS INTERNET SPECIFIC */
	d->addr_list[0] = d->hostaddr[0];
	d->addr_list[1] = NULL;
	d->host.h_addr_list = d->addr_list;
	*((u_long *)d->hostaddr[0]) = inet_addr(p);
	d->host.h_length = HOSTADDRSIZE;
	d->host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->host.h_name = cp;
	q = d->host.h_aliases = d->host_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->host_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	
	/*
	 * Reached end of the line -- see if there are any more lines.
	 * At this point, z points to the END of the first line (\n character),
	 * i is the index of addr_list[]. (We only look at the addresses)
	 */
	i = 1;
	while (z) {
		cp = z+1;
		if (*cp == '\0')
			break;
		while (*cp == '\n' || *cp == ' ' || *cp == '\t')
			cp++;
		p = cp;
		cp = strpbrk(cp, " \t\n");
		if (cp == NULL)
			break;
		z = cp;
		if (*cp != '\n')
			z = strpbrk(cp, "\n");
		*cp = '\0';
		/*
		 * At this point p points to the start of the address,
		 * past any white space, cp points to the end, and
		 * z points to the end of the current line.
		 */
		d->addr_list[i] = d->hostaddr[i];
		*((u_long *)d->hostaddr[i]) = inet_addr(p);
		i++;
		d->addr_list[i] = NULL;
	}
	return (&d->host);
}
