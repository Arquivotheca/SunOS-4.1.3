#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)netname.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
/*
 * netname utility routines
 * convert from unix names to network names and vice-versa
 * This module is operating system dependent!
 * What we define here will work with any unix system that has adopted
 * the Sun NIS domain architecture.
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <ctype.h>

#ifndef	S5EMUL
extern char *sprintf();
#endif
extern char *strncpy();

static char *OPSYS = "unix";
static char *NETID = "netid.byname";

/*
 * Convert network-name into unix credential
 */
netname2user(netname, uidp, gidp, gidlenp, gidlist)
	char netname[MAXNETNAMELEN+1];
	int *uidp;
	int *gidp;
	int *gidlenp;
	int *gidlist;
{
	int stat;
	char *val;
	char *p;
	int vallen;
	char *domain;
	int gidlen;

	stat = yp_get_default_domain(&domain);
	if (stat != 0) {
		return (0);
	}
	stat = yp_match(domain, NETID, netname, strlen(netname), &val, &vallen);
	if (stat != 0) {
		return (0);
	}
	val[vallen] = 0;
	p = val;
	*uidp = atois(&p);
	if (p == NULL || *p++ != ':') {
		free(val);
		return (0);
	}
	*gidp = atois(&p);
	if (p == NULL) {
		free(val);
		return (0);
	}
	gidlen = 0;
	for (gidlen = 0; gidlen < NGROUPS; gidlen++) {
		if (*p++ != ',') {
			break;
		}
		gidlist[gidlen] = atois(&p);
		if (p == NULL) {
			free(val);
			return (0);
		}
	}
	*gidlenp = gidlen;
	free(val);
	return (1);
}

/*
 * Convert network-name to hostname
 */
netname2host(netname, hostname, hostlen)
	char netname[MAXNETNAMELEN+1];
	char *hostname;
	int hostlen;
{
	int stat;
	char *val;
	int vallen;
	char *domain;

	stat = yp_get_default_domain(&domain);
	if (stat != 0) {
		return (0);
	}
	stat = yp_match(domain, NETID, netname, strlen(netname), &val, &vallen);
	if (stat != 0) {
		return (0);
	}
	val[vallen] = 0;
	if (*val != '0') {
		free(val);
		return (0);
	}
	if (val[1] != ':') {
		free(val);
		return (0);
	}
	(void) strncpy(hostname, val + 2, hostlen);
	free(val);
	return (1);
}


/*
 * Figure out my fully qualified network name
 */
getnetname(name)
	char name[MAXNETNAMELEN+1];
{
	int uid;

	uid = geteuid();
	if (uid == 0) {
		return (host2netname(name, (char *) NULL, (char *) NULL));
	} else {
		return (user2netname(name, uid, (char *) NULL));
	}
}


/*
 * Convert unix cred to network-name
 */
user2netname(netname, uid, domain)
	char netname[MAXNETNAMELEN + 1];
	int uid;
	char *domain;
{
	char *dfltdom;

#define	MAXIPRINT	(11)	/* max length of printed integer */

	if (domain == NULL) {
		if (yp_get_default_domain(&dfltdom) != 0) {
			return (0);
		}
		domain = dfltdom;
	}
	if (strlen(domain) + 1 + MAXIPRINT > MAXNETNAMELEN) {
		return (0);
	}
	(void) sprintf(netname, "%s.%d@%s", OPSYS, uid, domain);
	return (1);
}


/*
 * Convert host to network-name
 */
host2netname(netname, host, domain)
	char netname[MAXNETNAMELEN + 1];
	char *host;
	char *domain;
{
	char *dfltdom;
	char hostname[MAXHOSTNAMELEN+1];

	if (domain == NULL) {
		if (yp_get_default_domain(&dfltdom) != 0) {
			return (0);
		}
		domain = dfltdom;
	}
	if (host == NULL) {
		(void) gethostname(hostname, sizeof (hostname));
		host = hostname;
	}
	if (strlen(domain) + 1 + strlen(host) > MAXNETNAMELEN) {
		return (0);
	}
	(void) sprintf(netname, "%s.%s@%s", OPSYS, host, domain);
	return (1);
}


static
atois(str)
	char **str;
{
	char *p;
	int n;
	int sign;

	if (**str == '-') {
		sign = -1;
		(*str)++;
	} else {
		sign = 1;
	}
	n = 0;
	for (p = *str; isdigit(*p); p++) {
		n = (10 * n) + (*p - '0');
	}
	if (p == *str) {
		*str = NULL;
		return (0);
	}
	*str = p;
	return (n * sign);
}
