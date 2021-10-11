#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getnetent.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <rpcsvc/ypclnt.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Internet version.
 */
static struct netdata {
	FILE	*netf;
	char	*current;
	int	currentlen;
	int	stayopen;
#define	MAXALIASES	20
	char	*net_aliases[MAXALIASES];
	struct	netent net;
	char	line[BUFSIZ+1];
	char	*map;
	char	*domain;
	int	usingyellow;
	char	ntoabuf[16];
} *netdata, *_netdata();

static	struct netent *interpret();
struct	netent *getnetent();
char	*inet_ntoa();
char	*strpbrk();

static	char *NETWORKS = "/etc/networks";
static	char *nettoa();

static struct netdata *
_netdata()
{
	register struct netdata *d = netdata;

	if (d == 0) {
		d = (struct netdata *)calloc(1, sizeof (struct netdata));
		netdata = d;
		(void) yp_get_default_domain(&d->domain);
                if (d->domain) 
			d->usingyellow = 1;
                else 
			d->usingyellow = 0;
	}
	return (d);
}

struct netent *
getnetbyaddr(anet, type)
	int anet, type;
{
	register struct netdata *d = _netdata();
	register struct netent *p;
	char *adrstr, *val = NULL;
	int vallen;
	int reason;

	if (d == 0)
		return (0);
	d->map = "networks.byaddr";
	setnetent(0);
	if (!d->usingyellow) {
		while (p = getnetent()) {
			if (p->n_addrtype == type && p->n_net == anet)
				break;
		}
	} else {
		adrstr = nettoa(anet);
		if ((reason = yp_match(d->domain, d->map,
		    adrstr, strlen(adrstr), &val, &vallen))) {
                    if (yp_ismapthere(reason)==0) {
                        d->usingyellow = 0;
                        return(getnetbyaddr(anet,type));
                    }

		    p = NULL;
		} else {
			p = interpret(val, vallen);
			free(val);
		}
	}
	endnetent();
	return (p);
}

struct netent *
getnetbyname(name)
	register char *name;
{
	register struct netdata *d = _netdata();
	register struct netent *p;
	register char **cp;
	char *val = NULL;
	int vallen;
	int reason;

	if (d == 0)
		return (0);
	d->map = "networks.byname";
	setnetent(0);
	if (!d->usingyellow) {
		while (p = getnetent()) {
			if (strcasecmp(p->n_name, name) == 0)
				break;
			for (cp = p->n_aliases; *cp != 0; cp++)
				if (strcasecmp(*cp, name) == 0)
					goto found;
		}
	} else {
		if ((reason = yp_match(d->domain, d->map,
		    name, strlen(name), &val, &vallen))) {
			    if (yp_ismapthere(reason)==0) {
				d->usingyellow = 0;
				return(getnetbyname(name));
			    }
			p = NULL;
		}
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
found:
	endnetent();
	return (p);
}

setnetent(f)
	int f;
{
	register struct netdata *d = _netdata();

	if (d == 0)
		return;
	if (!d->usingyellow) {
		if (d->netf == NULL)
			d->netf = fopen(NETWORKS, "r");
		else
			rewind(d->netf);
	}
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

endnetent()
{
	register struct netdata *d = _netdata();

	if (d == 0)
		return;
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->netf && !d->stayopen) {
		(void) fclose(d->netf);
		d->netf = NULL;
	}
}

char	*BYADDR = "networks.byaddr";

struct netent *
getnetent()
{
	register struct netdata *d = _netdata();
	char *key = NULL, *val = NULL;
	int keylen, vallen;
	struct netent *np;
	int reason;

	if (d == 0)
		return (0);
	d->map = BYADDR;
	if (!d->usingyellow) {
		if (d->netf == NULL && (d->netf = fopen(NETWORKS, "r")) == NULL)
			return (NULL);
	        if (fgets(d->line, BUFSIZ, d->netf) == NULL)
			return (NULL);
		return interpret(d->line, strlen(d->line));
	}

	if (d->current == NULL) {
		if ((reason=yp_first(d->domain, d->map,
		    &key, &keylen, &val, &vallen)))
			{
			    if (yp_ismapthere(reason)==0) {
				d->usingyellow = 0;
				return(getnetent());
			    }
			return (NULL);
			}	
	} else {
		if ((reason=yp_next(d->domain, d->map,
		    d->current, d->currentlen, &key, &keylen, &val, &vallen)))
			{
			    if (yp_ismapthere(reason)==0) {
				d->usingyellow = 0;
				return(getnetent());
			    }
			return (NULL);
			}
	}
	if (d->current)
		free(d->current);
	d->current = key;
	d->currentlen = keylen;
	np = interpret(val, vallen);
	free(val);
	return (np);
}

static struct netent *
interpret(val, len)
	char *val;
{
	register struct netdata *d = _netdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return (0);
	(void) strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (getnetent());
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (getnetent());
	*cp = '\0';
	d->net.n_name = p;
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (getnetent());
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = strpbrk(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	d->net.n_net = inet_network(cp);
	d->net.n_addrtype = AF_INET;
	q = d->net.n_aliases = d->net_aliases;
	if (p != NULL) 
		cp = p;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &d->net_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->net);
}

/*
 * Takes an unsigned integer in host order, and returns a printable
 * string for it as a network number.  To allow for the possibility of
 * naming subnets, only trailing dot-zeros are truncated.
 */
static
char *
nettoa(anet)
	int anet;
{
	register struct netdata *d = _netdata();
	char *p, *index(), *rindex();
	struct in_addr in;
	int addr;

	if (d == 0)
		return (NULL);
	in = inet_makeaddr(anet, INADDR_ANY);
	addr = in.s_addr;
	(void) strcpy(d->ntoabuf, inet_ntoa(in));
	if ((IN_CLASSA_HOST & htonl(addr))==0) {
		p = index(d->ntoabuf, '.');
		if (p == NULL)
			return (NULL);
		*p = 0;
	} else if ((IN_CLASSB_HOST & htonl(addr))==0) {
		p = index(d->ntoabuf, '.');
		if (p == NULL)
			return (NULL);
		p = index(p+1, '.');
		if (p == NULL)
			return (NULL);
		*p = 0;
	} else if ((IN_CLASSC_HOST & htonl(addr))==0) {
		p = rindex(d->ntoabuf, '.');
		if (p == NULL)
			return (NULL);
		*p = 0;
	}
	return (d->ntoabuf);
}
