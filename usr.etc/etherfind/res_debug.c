/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * @(#)res_debug.c 1.1 92/07/30 from UCB 5.22 3/7/88
 */

#define DEBUG

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>

extern char *p_cdname(), *p_rr(), *p_type(), *p_class(), *p_time();
extern char *inet_ntoa();

char *_res_opcodes[] = {
	"QUERY",
	"IQUERY",
	"CQUERYM",
	"CQUERYU",
	"4",
	"5",
	"6",
	"7",
	"8",
	"UPDATEA",
	"UPDATED",
	"UPDATEDA",
	"UPDATEM",
	"UPDATEMA",
	"ZONEINIT",
	"ZONEREF",
};

char *_res_resultcodes[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"NOCHANGE",
};

/*
 * Print the contents of a query.
 * This is intended to be primarily a debugging routine.
 */
domainprint(index, msg, size)
	int index;
	char *msg;
	int size;
{
#ifdef DEBUG
	register char *cp;
	register HEADER *hp;
	register int n;

	/*
	 * Print header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + sizeof(HEADER);
	display(index,"\nHEADER:\n");
	display(index,"\topcode = %s", _res_opcodes[hp->opcode]);
	display(index,", id = %d", ntohs(hp->id));
	display(index,", rcode = %s\n", _res_resultcodes[hp->rcode]);
	display(index,"\theader flags: ");
	if (hp->qr)
		display(index," qr");
	if (hp->aa)
		display(index," aa");
	if (hp->tc)
		display(index," tc");
	if (hp->rd)
		display(index," rd");
	if (hp->ra)
		display(index," ra");
	if (hp->pr)
		display(index," pr");
	display(index,"\n\tqdcount = %d", ntohs(hp->qdcount));
	display(index,", ancount = %d", ntohs(hp->ancount));
	display(index,", nscount = %d", ntohs(hp->nscount));
	display(index,", arcount = %d\n\n", ntohs(hp->arcount));
	/*
	 * Print question records.
	 */
	if (n = ntohs(hp->qdcount)) {
		display(index,"QUESTIONS:\n");
		while (--n >= 0) {
			display(index,"\t");
			cp = p_cdname(cp, msg, index);
			if (cp == NULL)
				return;
			display(index,", type = %s", p_type(_getshort(cp)));
			cp += sizeof(u_short);
			display(index,", class = %s\n\n", p_class(_getshort(cp)));
			cp += sizeof(u_short);
		}
	}
	/*
	 * Print authoritative answer records
	 */
	if (n = ntohs(hp->ancount)) {
		display(index,"ANSWERS:\n");
		while (--n >= 0) {
			display(index,"\t");
			cp = p_rr(cp, msg, index);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print name server records
	 */
	if (n = ntohs(hp->nscount)) {
		display(index,"NAME SERVERS:\n");
		while (--n >= 0) {
			display(index,"\t");
			cp = p_rr(cp, msg, index);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print additional records
	 */
	if (n = ntohs(hp->arcount)) {
		display(index,"ADDITIONAL RECORDS:\n");
		while (--n >= 0) {
			display(index,"\t");
			cp = p_rr(cp, msg, index);
			if (cp == NULL)
				return;
		}
	}
#endif
}

char *
p_cdname(cp, msg, index)
	char *cp, *msg;
	int index;
{
#ifdef DEBUG
	char name[MAXDNAME];
	int n;

	if ((n = dn_expand(msg, msg + 512, cp, name, sizeof(name))) < 0)
		return (NULL);
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	display(index, name);
	return (cp + n);
#endif
}

/*
 * Print resource record fields in human readable form.
 */
char *
p_rr(cp, msg, index)
	char *cp, *msg;
	int index;
{
#ifdef DEBUG
	int type, class, dlen, n, c;
	struct in_addr inaddr;
	char *cp1;

	if ((cp = p_cdname(cp, msg, index)) == NULL)
		return (NULL);			/* compression error */
	display(index,"\n\ttype = %s", p_type(type = _getshort(cp)));
	cp += sizeof(u_short);
	display(index,", class = %s", p_class(class = _getshort(cp)));
	cp += sizeof(u_short);
	display(index,", ttl = %s", p_time(_getlong(cp)));
	cp += sizeof(u_long);
	display(index,", dlen = %d\n", dlen = _getshort(cp));
	cp += sizeof(u_short);
	cp1 = cp;
	/*
	 * Print type specific data, if appropriate
	 */
	switch (type) {
	case T_A:
		switch (class) {
		case C_IN:
			bcopy(cp, (char *)&inaddr, sizeof(inaddr));
			if (dlen == 4) {
				display(index,"\tinternet address = %s\n",
					inet_ntoa(inaddr));
				cp += dlen;
			} else if (dlen == 7) {
				display(index,"\tinternet address = %s",
					inet_ntoa(inaddr));
				display(index,", protocol = %d", cp[4]);
				display(index,", port = %d\n",
					(cp[5] << 8) + cp[6]);
				cp += dlen;
			}
			break;
		default:
			cp += dlen;
		}
		break;
	case T_CNAME:
	case T_MB:
#ifdef OLDRR
	case T_MD:
	case T_MF:
#endif /* OLDRR */
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
		display(index,"\tdomain name = ");
		cp = p_cdname(cp, msg, index);
		display(index,"\n");
		break;

	case T_HINFO:
		if (n = *cp++) {
			display(index,"\tCPU=%.*s\n", n, cp);
			cp += n;
		}
		if (n = *cp++) {
			display(index,"\tOS=%.*s\n", n, cp);
			cp += n;
		}
		break;

	case T_SOA:
		display(index,"\torigin = ");
		cp = p_cdname(cp, msg, index);
		display(index,"\n\tmail addr = ");
		cp = p_cdname(cp, msg, index);
		display(index,"\n\tserial=%ld", _getlong(cp));
		cp += sizeof(u_long);
		display(index,", refresh=%s", p_time(_getlong(cp)));
		cp += sizeof(u_long);
		display(index,", retry=%s", p_time(_getlong(cp)));
		cp += sizeof(u_long);
		display(index,",\n\texpire=%s", p_time(_getlong(cp)));
		cp += sizeof(u_long);
		display(index,", min=%s\n", p_time(_getlong(cp)));
		cp += sizeof(u_long);
		break;

	case T_MX:
		display(index,"\tpreference = %ld,",_getshort(cp));
		cp += sizeof(u_short);
		display(index," name = ");
		cp = p_cdname(cp, msg, index);
		break;

	case T_MINFO:
		display(index,"\trequests = ");
		cp = p_cdname(cp, msg, index);
		display(index,"\n\terrors = ");
		cp = p_cdname(cp, msg, index);
		break;

	case T_UINFO:
		display(index,"\t%s\n", cp);
		cp += dlen;
		break;

	case T_UID:
	case T_GID:
		if (dlen == 4) {
			display(index,"\t%ld\n", _getlong(cp));
			cp += sizeof(int);
		}
		break;

	case T_WKS:
		if (dlen < sizeof(u_long) + 1)
			break;
		bcopy(cp, (char *)&inaddr, sizeof(inaddr));
		cp += sizeof(u_long);
		display(index,"\tinternet address = %s, protocol = %d\n\t",
			inet_ntoa(inaddr), *cp++);
		n = 0;
		while (cp < cp1 + dlen) {
			c = *cp++;
			do {
 				if (c & 0200)
					display(index," %d", n);
 				c <<= 1;
			} while (++n & 07);
		}
		display(index, "\n");
		break;

#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		{
			int NumBytes = 8;
			char *DataPtr;
			int i;

			if (dlen < NumBytes) NumBytes = dlen;
			display(index, "\tFirst %d bytes of hex data:",
				NumBytes);
			for (i = 0, DataPtr = cp; i < NumBytes; i++, DataPtr++)
				display(index, " %x", *DataPtr);
			display(index, "\n");
			cp += dlen;
		}
		break;
#endif /* ALLOW_T_UNSPEC */

	default:
		display(index,"\t???\n");
		cp += dlen;
	}
	if (cp != cp1 + dlen)
		display(index,"packet size error (%#x != %#x)\n", cp, cp1+dlen);
	display(index,"\n");
	return (cp);
#endif
}

static	char nbuf[20];
extern	char *sprintf();

/*
 * Return a string for the type
 */
char *
p_type(type)
	int type;
{
	switch (type) {
	case T_A:
		return("A");
	case T_NS:		/* authoritative server */
		return("NS");
#ifdef OLDRR
	case T_MD:		/* mail destination */
		return("MD");
	case T_MF:		/* mail forwarder */
		return("MF");
#endif /* OLDRR */
	case T_CNAME:		/* connonical name */
		return("CNAME");
	case T_SOA:		/* start of authority zone */
		return("SOA");
	case T_MB:		/* mailbox domain name */
		return("MB");
	case T_MG:		/* mail group member */
		return("MG");
	case T_MX:		/* mail routing info */
		return("MX");
	case T_MR:		/* mail rename name */
		return("MR");
	case T_NULL:		/* null resource record */
		return("NULL");
	case T_WKS:		/* well known service */
		return("WKS");
	case T_PTR:		/* domain name pointer */
		return("PTR");
	case T_HINFO:		/* host information */
		return("HINFO");
	case T_MINFO:		/* mailbox information */
		return("MINFO");
	case T_AXFR:		/* zone transfer */
		return("AXFR");
	case T_MAILB:		/* mail box */
		return("MAILB");
	case T_MAILA:		/* mail address */
		return("MAILA");
	case T_ANY:		/* matches any type */
		return("ANY");
	case T_UINFO:
		return("UINFO");
	case T_UID:
		return("UID");
	case T_GID:
		return("GID");
#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		return("UNSPEC");
#endif /* ALLOW_T_UNSPEC */
	default:
		return (sprintf(nbuf, "%d", type));
	}
}

/*
 * Return a mnemonic for class
 */
char *
p_class(class)
	int class;
{

	switch (class) {
	case C_IN:		/* internet class */
		return("IN");
	case C_ANY:		/* matches any class */
		return("ANY");
	default:
		return (sprintf(nbuf, "%d", class));
	}
}

/*
 * Return a mnemonic for a time to live
 */
char *p_time(value)
	unsigned long value;
{
	int secs, mins, hours;
	static char buf[128];

	secs = value % 60;
	value /= 60;
	mins = value % 60;
	value /= 60;
	hours = value % 24;
	value /= 24;

# define PLURALIZE(x) x, (x==1)?"":"s"
	if (value)
		(void) sprintf(buf, "%d day%s", PLURALIZE(value));
	else
		strcpy(buf, "");
	if (hours) {
		(void) sprintf(nbuf, " %d hour%s", PLURALIZE(hours));
		(void) strcat(buf, nbuf);
	}
	if (mins) {
		(void) sprintf(nbuf, " %d min%s", PLURALIZE(mins));
		(void) strcat(buf, nbuf);
	}
	if (secs) {
		(void) sprintf(nbuf, " %d sec%s", PLURALIZE(secs));
		(void) strcat(buf, nbuf);
	}
	return (buf);
}
