/*
 * @(#)etherfind.h 1.1 92/07/30 
 *
 * Global definitions for the "Etherfind" program
 *
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

# define MAXSPLIT 4		/* maximum number of simultaneous filters */

int timeflag[MAXSPLIT];		/* print timestamp on given index */
int symflag[MAXSPLIT];		/* verbose printout on given index */
int xflag[MAXSPLIT];		/* dump headers in hex on given index */
int trigger_count[MAXSPLIT];	/* only used in tool version */
int rpcflag[MAXSPLIT];		/* dump RPC headers on given index */

int cflag;			/* only print 'cnt' packets */
int dflag;			/* report dropped packets */
int nflag;			/* leave addresses as numbers */
int pflag;			/* don't go promiscuous */
int tflag;			/* timestamp RPC responses */
int vflag;			/* verbose mode */
int trflag;			/* invoke trigger/replay feature */

int cnt;			/* number of packets to dump */
u_short	sp_ts_len;		/* packet length */

/*
 * Data structure used to build filters during the parse phase, and to
 * interpret them during the filter phase.
 */
struct anode {
	int (*F)();
	struct anode *L, *R;
};

struct anode *exlist[MAXSPLIT];

struct addrpair {
	int addr1;
	int addr2;
};

struct colon {
	int byte;
	u_int value;
	char op;
} colon;

char *getname();



