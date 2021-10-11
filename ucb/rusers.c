#ifndef lint
static  char sccsid[] = "@(#)rusers.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <utmp.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/rusers.h>

struct utmp dummy;
#define NMAX sizeof(dummy.ut_name)
#define LMAX sizeof(dummy.ut_line)
#define	HMAX sizeof(dummy.ut_host)

#define MACHINELEN 12		/* length of machine name printed out */
#define NUMENTRIES 200
#define MAXINT 0x7fffffff
#define min(a,b) ((a) < (b) ? (a) : (b))

struct entry {
	int addr;
	int cnt;
	int idle;		/* set to MAXINT if not present */
	char *machine;
	struct utmpidle *users;
} entry[NUMENTRIES];
int curentry;
int hflag;			/* host: sort by machine name */
int iflag;			/* idle: sort by idle time */
int uflag;			/* users: sort by number of users */
int lflag;			/*  print out long form */
int aflag;			/* all: list all machines */
int dflag;			/* debug: list only first n machines */
int debug;
int vers;

int hcompare(), icompare(), ucompare();
int collectnames();

main(argc, argv)
	char **argv;
{
	struct utmpidlearr utmpidlearr;
	enum clnt_stat clnt_stat;
	int single;

	single = 0;
	while (argc > 1) {
		if (argv[1][0] != '-') {
			single++;
			singlehost(argv[1]);
		}
		else {
			switch(argv[1][1]) {
	
			case 'h':
				hflag++;
				break;
			case 'a':
				aflag++;
				break;
			case 'i':
				iflag++;
				break;
			case 'l':
				lflag++;
				break;
			case 'u':
				uflag++;
				break;
			case 'd':
				dflag++;
				if (argc < 3)
					usage();
				debug = atoi(argv[2]);
				argc--;
				argv++;
				break;
			default:
				usage();
			}
		}
		argv++;
		argc--;
	}
	if (iflag + hflag + uflag > 1)
		usage();
	if (single > 0) {
		if (iflag || hflag || uflag)
			printnames();
		exit(0);
	}
	if (iflag || hflag || uflag) {
		printf("collecting responses... ");
		fflush(stdout);
	}
	vers = RUSERSVERS_IDLE;
	utmpidlearr.uia_arr = NULL;
	clnt_stat = clnt_broadcast(RUSERSPROG, RUSERSVERS_IDLE,
	    RUSERSPROC_NAMES, xdr_void, NULL, xdr_utmpidlearr,
	    &utmpidlearr, collectnames);
#ifdef TESTING
	fprintf(stderr, "starting second round of broadcasting\n");
#endif
	vers = RUSERSVERS_ORIG;
	utmpidlearr.uia_arr = NULL;
	clnt_stat = clnt_broadcast(RUSERSPROG, RUSERSVERS_ORIG,
	    RUSERSPROC_NAMES, xdr_void, NULL, xdr_utmparr, &utmpidlearr,
	    collectnames);
	if (iflag || hflag || uflag)
		printnames();
	exit(0);
	/* NOTREACHED */
}

singlehost(name)
	char *name;
{
	struct hostent *hp;
	enum clnt_stat err;
	struct sockaddr_in addr;
	struct utmpidlearr utmpidlearr;

	vers = RUSERSVERS_ORIG;
	utmpidlearr.uia_arr = NULL;
	err = (enum clnt_stat)callrpc(name, RUSERSPROG, RUSERSVERS_IDLE,
	    RUSERSPROC_NAMES, xdr_void, 0, xdr_utmpidlearr, &utmpidlearr);
	if (err == RPC_PROGVERSMISMATCH) {
		if (err = (enum clnt_stat)callrpc(name, RUSERSPROG,
		    RUSERSVERS_ORIG, RUSERSPROC_NAMES, xdr_void, 0,
		    xdr_utmparr, &utmpidlearr)) {
			fprintf(stderr, "%*s: ", name);
			clnt_perrno(err);
			fprintf(stderr, "\n");
			return;
		}
	}
	else if (err == RPC_SUCCESS)
		vers = RUSERSVERS_IDLE;
	else {
		fprintf(stderr, "%s: ", name);
		clnt_perrno(err);
		fprintf(stderr, "\n");
		return;
	}
	/* 
	 * simulate calling from clnt_broadcast
	 */
	hp = gethostbyname(name);
	if (hp == NULL) {
		fprintf("Yikes!: can't find host name!\n");
		exit(1);
	}
	addr.sin_addr.s_addr = *(int *)hp->h_addr;
	collectnames(&utmpidlearr, &addr);
	return;
}

collectnames(resultsp, raddrp)
	char *resultsp;
	struct sockaddr_in *raddrp;
{
	struct utmpidlearr utmpidlearr;
	struct utmpidle *uip;
	struct utmp *up;
	static int debugcnt;
	int i, cnt, minidle;
	register int addr;
	register struct entry *entryp, *lim;
	struct utmpidle *p, *q;
	struct hostent *hp;
	char host[MACHINELEN + 1];

	utmpidlearr = *(struct utmpidlearr *)resultsp;
	if ((cnt = utmpidlearr.uia_cnt) < 1 && !aflag)
		return(0);
	/* 
	 * weed out duplicates
	 */
	addr = raddrp->sin_addr.s_addr;
	lim = entry + curentry;
	for (entryp = entry; entryp < lim; entryp++)
		if (addr == entryp->addr)
			return (0);
	debugcnt++;
	entry[curentry].addr = addr;
	hp = gethostbyaddr(&raddrp->sin_addr.s_addr,
	    sizeof(int),AF_INET);
	if (hp == NULL)
		sprintf(host, "0x%08.8x", addr);
	else
		sprintf(host, "%.*s", MACHINELEN, hp->h_name);



	/*
	 * if raw, print this entry out immediately
	 * otherwise store for later sorting
	 */
	if (!iflag && !hflag && !uflag) {
		if (lflag &&  (cnt > 0) )  
		  	for (i = 0; i < cnt; i++)
				putline(host, utmpidlearr.uia_arr[i], vers);
		else {
			printf("%-*.*s", MACHINELEN, MACHINELEN, host);
			for (i = 0; i < cnt; i++)
				printf(" %.*s", NMAX, 
				    utmpidlearr.uia_arr[i]->ui_utmp.ut_name);
			printf("\n");
		}
	}
	else {
		entry[curentry].cnt = cnt;
		q = (struct utmpidle *)
		    malloc(cnt*sizeof(struct utmpidle));
		p = q;
		minidle = MAXINT;
		for (i = 0; i < cnt; i++) {
			bcopy(utmpidlearr.uia_arr[i], q,
			    sizeof(struct utmpidle));
			if (vers == RUSERSVERS_IDLE)
				minidle = min(minidle, q->ui_idle);
			q++;
		}
		entry[curentry].users = p;
		entry[curentry].idle = minidle;
	}
	if (curentry >= NUMENTRIES) {
		fprintf(stderr, "too many hosts on network\n");
		exit(1);
	}
	curentry++;
	if (dflag && debugcnt >= debug)
		return (1);
	return(0);
}

printnames()
{
	char buf[MACHINELEN+1];
	struct hostent *hp;
	int i, j, v;
	int (*compare)();

	for (i = 0; i < curentry; i++) {
		hp = gethostbyaddr(&entry[i].addr,sizeof(int),AF_INET);
		if (hp == NULL)
			sprintf(buf, "0x%08.8x", entry[i].addr);
		else
			sprintf(buf, "%.*s", MACHINELEN, hp->h_name);
		entry[i].machine = (char *)malloc(MACHINELEN+1);
		strcpy(entry[i].machine, buf);
	}
	if (iflag)
		compare = icompare;
	else if (hflag)
		compare = hcompare;
	else
		compare = ucompare;
	qsort(entry, curentry, sizeof(struct entry), compare);
	printf("\n");
	for (i = 0; i < curentry; i++) {
		if (!lflag || ( entry[i].cnt < 1 )) {
			printf("%-*.*s", MACHINELEN,
			    MACHINELEN, entry[i].machine);
			for (j = 0; j < entry[i].cnt; j++)
				printf(" %.*s", NMAX,
				    entry[i].users[j].ui_utmp.ut_name);
			printf("\n");
		}
		else {
			if (entry[i].idle == MAXINT)
				v = RUSERSVERS_ORIG;
			else
				v = RUSERSVERS_IDLE;
			for (j = 0; j < entry[i].cnt; j++)
				putline(entry[i].machine,
				    &entry[i].users[j], v);
		}
	}
}

hcompare(a,b)
	struct entry *a, *b;
{
	return (strcmp(a->machine, b->machine));
}

ucompare(a,b)
	struct entry *a, *b;
{
	return (b->cnt - a->cnt);
}

icompare(a,b)
	struct entry *a, *b;
{
	return (a->idle - b->idle);
}

putline(host, uip, vers)
	char *host;
	struct utmpidle *uip;
	int vers;
{
	register char *cbuf;
	struct utmp *up;
	struct hostent *hp;
	char buf[100];

	up = &uip->ui_utmp;
	printf("%-*.*s ", NMAX, NMAX, up->ut_name);

	strcpy(buf, host);
	strcat(buf, ":");
	strncat(buf, up->ut_line, LMAX);
	printf("%-*.*s", MACHINELEN+LMAX, MACHINELEN+LMAX, buf);

	cbuf = (char *)ctime(&up->ut_time);
	printf("  %.12s  ", cbuf+4);
	if (vers == RUSERSVERS_IDLE) {
		prttime(uip->ui_idle, "");
	}
	else
		printf("    ??");
	if (up->ut_host[0])
		printf(" (%.*s)", HMAX, up->ut_host);
	putchar('\n');
}

/*
 * prttime prints a time in hours and minutes.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{
	register int didhrs = 0;

	if (tim >= 60) {
		printf("%3d:", tim/60);
		didhrs++;
	} else {
		printf("    ");
	}
	tim %= 60;
	if (tim > 0 || didhrs) {
		printf(didhrs&&tim<10 ? "%02d" : "%2d", tim);
	} else {
		printf("  ");
	}
	printf("%s", tail);
}

/* 
 * for debugging
 */
printit(i)
{
	int j, v;
	
	printf("%12.12s: ", entry[i].machine);
	if (entry[i].cnt) {
		if (entry[i].idle == MAXINT)
			v = RUSERSVERS_ORIG;
		else
			v = RUSERSVERS_IDLE;
		putline(&entry[i].users[0], v);
		for (j = 1; j < entry[i].cnt; j++) {
			printf("\t");
			putline(&entry[i].users[j], vers);
		}
	}
	else
		printf("\n");
}

usage()
{
	fprintf(stderr, "Usage: rusers [-a] [-h] [-i] [-l] [-u] [host ...]\n");
	exit(1);
}
