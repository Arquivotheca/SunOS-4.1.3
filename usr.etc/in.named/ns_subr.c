/*
 * Some functions shared by the name server and the xfer program
 *
 * @(#)ns_subr.c 1.1 92/07/30 SMI from BBN & UCB
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>
#include <arpa/nameser.h>

extern char *malloc();

struct valuelist {
	struct valuelist *next, *prev;
	char	*name;
	char	*proto;
	short	port;
} *servicelist, *protolist;

/*
 * Make a copy of a string and return a pointer to it.
 */
static char *
savestr(str)
      char *str;
{
      char *cp;
 
      cp = malloc((unsigned)strlen(str) + 1);
      if (cp == NULL) {
              syslog(LOG_ERR, "savestr: %m");
              exit(2);        /* djw 7/5/88: 1->2 */
      }
      (void) strcpy(cp, str);
      return (cp);
}



/*  These next two routines will be moveing to res_debug.c */
/* They are currently here for ease of distributing the WKS record fix */
char *
p_protocol(num)
int num;
{
	static	char number[8];
	struct protoent *pp;
	extern struct protoent *cgetprotobynumber();

	pp = cgetprotobynumber(num);
	if(pp == 0)  {
		(void) sprintf(number, "%d", num);
		return(number);
	}
	return(pp->p_name);
}

char *
p_service(port, proto)
u_short port;
char *proto;
{
	static	char number[8];
	struct servent *ss;
	extern struct servent *cgetservbyport();

	ss = cgetservbyport(htons(port), proto);
	if(ss == 0)  {
		(void) sprintf(number, "%d", port);
		return(number);
	}
	return(ss->s_name);
}

buildservicelist()
{
	struct servent *sp;
	struct valuelist *slp;

	setservent(1);
	while (sp = getservent()) {
		slp = (struct valuelist *)malloc(sizeof(struct valuelist));
		slp->name = savestr(sp->s_name);
		slp->proto = savestr(sp->s_proto);
		slp->port = ntohs((u_short)sp->s_port);
		slp->next = servicelist;
		slp->prev = NULL;
		if (servicelist) {
			servicelist->prev = slp;
		}
		servicelist = slp;
	}
	endservent();
}


buildprotolist()
{
	struct protoent *pp;
	struct valuelist *slp;

	setprotoent(1);
	while (pp = getprotoent()) {
		slp = (struct valuelist *)malloc(sizeof(struct valuelist));
		slp->name = savestr(pp->p_name);
		slp->port = pp->p_proto;
		slp->next = protolist;
		slp->prev = NULL;
		if (protolist) {
			protolist->prev = slp;
		}
		protolist = slp;
	}
	endprotoent();
}

struct servent *
cgetservbyport(port, proto)
u_short port;
char *proto;
{
	register struct valuelist **list = &servicelist;
	register struct valuelist *lp = *list;
	static struct servent serv;

	port = htons(port);
	for (; lp != NULL; lp = lp->next) {
		if (port != lp->port)
			continue;
		if (strcasecmp(lp->proto, proto) == 0) {
			if (lp != *list) {
				lp->prev->next = lp->next;
				if (lp->next)
					lp->next->prev = lp->prev;
				(*list)->prev = lp;
				lp->next = *list;
				*list = lp;
			}
			serv.s_name = lp->name;
			serv.s_port = htons((u_short)lp->port);
			serv.s_proto = lp->proto;
			return(&serv);
		}
	}
	return(0);
}

struct protoent *
cgetprotobynumber(proto)
register int proto;
{

	register struct valuelist **list = &protolist;
	register struct valuelist *lp = *list;
	static struct protoent prot;

	for (; lp != NULL; lp = lp->next)
		if (lp->port == proto) {
			if (lp != *list) {
				lp->prev->next = lp->next;
				if (lp->next)
					lp->next->prev = lp->prev;
				(*list)->prev = lp;
				lp->next = *list;
				*list = lp;
			}
			prot.p_name = lp->name;
			prot.p_proto = lp->port;
			return(&prot);
		}
	return(0);
}
