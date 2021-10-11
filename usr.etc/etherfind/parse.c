/*
 * Filter parsing module for the "Etherfind" program
 *
 * @(#)parse.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>

#include "etherfind.h"

char	**Argv;
int	Argc,
	Ai;

struct	anode	*exp(),
		*e1(),
		*e2(),
		*e3(),
		*mk();

char	*nxtarg();

extern char *index();
extern char *strcpy();
extern char *malloc();
extern char *sprintf();

struct colon *strtocolon();
struct addrpair *stringtoaddrpair();

int Randlast;

struct anode Node[100];
int Nn;  /* number of nodes */

/*
 * Return true if the given string matches, modulo an optional "-"
 */
EQ(x, y)
    char *x, *y;
{
	if (*x == '-')
		x++;
	return(strcmp(x, y)==0);
}


/* 
 * convert a:b to a struct colon
 */
struct colon *
strtocolon(str)
	char *str;
{
	char *p, *ntoi();
	struct colon *colonp;
	
	colonp = (struct colon *)malloc(sizeof(colon));
	p = ntoi(str, &colonp->byte);
	colonp->op = *p++;
	(void) ntoi(p, (int *)&colonp->value);
	return (colonp);
}

char *
ntoi(cp, valp)
	char *cp;
	int *valp;
{
	register val, base, c;
	
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	*valp = val;
	return (cp);
}


/*
 *  convert two strings to an addrpair
 */
struct addrpair *
stringtoaddrpair(str1, str2)
	char *str1, *str2;
{
	struct addrpair *p;
	
	p = (struct addrpair *)malloc(sizeof(struct addrpair));
	p->addr1 = stringtoaddr(str1);
	p->addr2 = stringtoaddr(str2);
	return (p);
}

parseargs(argc, argv)
	char *argv[];
{

	Argc = argc; Argv = argv;
	Ai = 1;
	if (!(exlist[0] = exp())) { /* parse and compile the arguments */
		fprintf(stderr, "etherfind: parsing error\n");
		exit(1);
	}
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *
exp()		/* parse ALTERNATION (or)  */
{
	int or();
	register struct anode * p1;
	char *a;

	p1 = e1() /* get left operand */ ;
	a = nxtarg();
	if (EQ(a, "or") || EQ(a, "o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if (Ai <= Argc) --Ai;
	return(p1);
}

struct anode *
e1() { /* parse CONCATENATION (formerly -a) */
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if (EQ(a, "and") || EQ(a, "a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if (EQ(a, "(") || EQ(a, "!") || EQ(a, "not") ||
	          ( *a != NULL && !EQ(a, "or") && !EQ(a, "o"))) {
		--Ai;
		goto And;
	} else if (Ai <= Argc) --Ai;
	return(p1);
}

struct anode *
e2()		/* parse NOT (!) */
{
	int not();
	char *a;

	if (Randlast) {
		fprintf(stderr, "etherfind: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	a = nxtarg();
	if (EQ(a, "!") || EQ(a, "not"))
		return(mk(not, e3(), (struct anode *)0));
	else if (Ai <= Argc) --Ai;
	return(e3());
}

struct anode *
e3()		/* parse parens and predicates */
{
	int src(), dst(), srcnet(), dstnet(), less(), greater(), proto(),
	    tcp(), udp(), rpc(), nfs(),
	    src_port(), dst_port(), byte(), between(), host(),
	    broadcast(), arp(), rarp(), ip(), decnet(), apple();
	struct anode *p1;
	register char *a, *b, *c, s;

	a = nxtarg();
	if (EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if (!EQ(a, ")")) goto err;
		return(p1);
	}
	else if (EQ(a, "broadcast"))
		return(mk(broadcast, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "arp"))
		return(mk(arp, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "rarp"))
		return(mk(rarp, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "ip"))
		return(mk(ip, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "decnet"))
		return(mk(decnet, (struct anode *)0, (struct anode *)0));
	else if (EQ(a, "apple"))
		return(mk(apple, (struct anode *)0, (struct anode *)0));
	b = nxtarg();
	s = *b;
	if (s=='+')
		b++;
	if (EQ(a, "proto"))
		return(mk(proto, (struct anode *)getproto(b),
		    (struct anode *)s));
	else if (EQ(a, "src"))
		return(mk(src, (struct anode *)stringtoaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "dst"))
		return(mk(dst, (struct anode *)stringtoaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "dstnet"))
		return(mk(dstnet, (struct anode *)stringtonetaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "srcnet"))
		return(mk(srcnet, (struct anode *)stringtonetaddr(b),
		    (struct anode *)s));
	else if (EQ(a, "srcport"))
		return(mk(src_port, (struct anode *)stringtoport(b),
		    (struct anode *)s));
	else if (EQ(a, "dstport"))
		return(mk(dst_port, (struct anode *)stringtoport(b),
		    (struct anode *)s));
	else if (EQ(a, "greater"))
		return(mk(greater, (struct anode *)atoi(b),
		    (struct anode *)s));
	else if (EQ(a, "less"))
		return(mk(less, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "byte"))
		return(mk(byte,(struct anode *)strtocolon(b),
		    (struct anode *)s));
	else if (EQ(a, "between")) {
		c = nxtarg();
		return(mk(between, (struct anode *)stringtoaddrpair(b, c),
		    (struct anode *)s));
	}
	else if (EQ(a, "host"))
		return(mk(host, (struct anode *)stringtoaddr(b),
		    (struct anode *)s));
err:	fprintf(stderr, "etherfind: bad option or expression < %s >\n", a);
	usage();
	exit(1);
	/*NOTREACHED*/
}


struct anode *
mk(f, l, r)
	int (*f)();
	struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *
nxtarg()			/* get next arg from command line */
{
	static strikes = 0;

	if (strikes==3) {
		fprintf(stderr, "etherfind: incomplete statement\n");
		usage();
		exit(1);
	}
	if (Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

usage()
{
	fprintf(stderr,
"Usage: etherfind [-d] [-n] [-p] [-r] [-t] [-u] [-v] [-x]\n");
	fprintf(stderr,
"	[-i interface] [-c cnt] [-l length] expression\n");
	fprintf(stderr,
"expressions are:\n");
	fprintf(stderr,
"       broadcast arp rarp ip decnet apple\n");
	fprintf(stderr, 
"       dst src dstnet srcnet less greater proto srcport dstport byte\n");
	fprintf(stderr, 
"       between host and or not\n");
}
