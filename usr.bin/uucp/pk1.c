/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)pk1.c 1.1 92/07/30"	/* from SVR3.2 uucp:pk1.c 2.5 */

#include "uucp.h"

#include "pk.h"
#include <sys/buf.h>

#define PKMAXSTMSG 40
#define CONNODATA	20	/* Max Continuous Non Valid Data Count */
#define NTIMEOUT	50	/* Experimental */

int Connodata = 0;		/* Continuous Non Valid Data Count */
int Ntimeout = 0;		/* Experimental */
extern int Retries;

/*
 * Translation of states from numbers to letters, in such a way as to be
 * meaningful to John Q. UUCPadministrator
 */
static struct {
	int state;
	char *msg;
} st_trans[] = {
	DEAD,	"Dead!",
	INITa,	"INIT code a",
	INITb,	"INIT code b",
	LIVE,	"O.K.",
	RXMIT,	"Rcv/Xmit",
	RREJ,	"RREJ?",
	PDEBUG,	"PDEBUG?",
	DRAINO,	"Draino...",
	WAITO,	"Waiting",
	DOWN,	"Link down",
	RCLOSE,	"RCLOSE?",
	BADFRAME,	"Bad frame",
	-1,	"End of the line",
};

/*
 * start initial synchronization.
 */
struct pack *
pkopen(ifn, ofn)
int ifn, ofn;
{
	register struct pack *pk;
	register char **bp;
	register int i;
	char *calloc();

	if ((pk = (struct pack *) calloc(1, sizeof (struct pack))) == NULL)
		return(NULL);
	pk->p_ifn = ifn;
	pk->p_ofn = ofn;
	pk->p_xsize = pk->p_rsize = PACKSIZE;
	pk->p_rwindow = pk->p_swindow = WINDOWS;

	/*
	 * allocate input window
	 */
	for (i = 0; i < pk->p_rwindow; i++) {
		if ((bp = (char **) malloc((unsigned) pk->p_xsize)) == NULL)
			break;
		*bp = (char *) pk->p_ipool;
		pk->p_ipool = bp;
	}
	if (i == 0)
		return(NULL);
	pk->p_rwindow = i;

	/*
	 * start synchronization
	 */
	pk->p_msg = pk->p_rmsg = M_INITA;
	pkoutput(pk);

	for (i = 0; i < PKMAXSTMSG; i++) {
		pkgetpack(pk);
		if ((pk->p_state & LIVE) != 0)
			break;
	}
	if (i >= PKMAXSTMSG)
		return(NULL);

	pkreset(pk);
	return(pk);
}

/*
 * input framing and block checking.
 * frame layout for most devices is:
 *	
 *	S|K|X|Y|C|Z|  ... data ... |
 *
 *	where 	S	== initial synch byte
 *		K	== encoded frame size (indexes pksizes[])
 *		X, Y	== block check bytes
 *		C	== control byte
 *		Z	== XOR of header (K^X^Y^C)
 *		data	== 0 or more data bytes
 *
 */
#define GETRIES 10

/*
 * Byte collection.
 */
pkgetpack(ipk)
struct pack *ipk;
{
	register char *p;
	register struct pack *pk;
	register struct header *h;
	unsigned short sum;
	int k, tries, ifn, noise;
	char **bp, hdchk;

	pk = ipk;
	/*
	 * If we are known to be DOWN, or if we've received too many garbage
	 * packets or timeouts, give up without a fight.
	 */
	if ((pk->p_state & DOWN) || Connodata > CONNODATA  || Ntimeout > NTIMEOUT)
		pkfail();
	ifn = pk->p_ifn;
	h = &pk->p_ihbuf;

	/*
	 * Attempt no more than GETRIES times to read a packet.  The only valid
	 * exit from this loop is a return.  Break forces a failure.
	 */
	for (tries = 0; tries < GETRIES; tries++) {
		/*
		 * Read header.
		 * First look for SYN.  If more than 3 * packetsize characters
		 * go by w/o a SYN, request a retransmit.
		 */
		p = (caddr_t) h;
		noise = 0;
		for ( ; ; ) {
			if (pkcget(ifn, p, 1) != SUCCESS) {
				DEBUG(7, "Alarm while looking for SYN -- request RXMIT\n", 0);
				goto retransmit;
			}
			if (*p == SYN)
				break;		/* got it */
			if (noise++ > 3 * pk->p_rsize) {
				DEBUG(7, "No SYN in %d characters -- request RXMIT\n", noise);
				goto retransmit;
			}
		}
		/* Now look for remainder of header */
		if (pkcget(ifn, p+1, HDRSIZ - 1) != SUCCESS) {
			DEBUG(7, "Alarm while looking for header -- request RXMIT\n", 0);
			goto retransmit;
		}
		/* Validate the header */		
		Connodata++;
		hdchk = p[1] ^ p[2] ^ p[3] ^ p[4];
		sum = ((unsigned) p[2] & 0377) | ((unsigned) p[3] << 8);
		h->sum = sum;
		k = h->ksize;
		if (hdchk != h->ccntl) {
			/* bad header */
			DEBUG(7, "bad header checksum\n", 0);
			return;
		}

		if (k == 9) {	/* control packet */
			if (((h->sum + h->cntl) & 0xffff) == CHECK) {
				pkcntl(h->cntl, pk);
				xlatestate(pk, 7);
			} else {
				/* bad header */
				DEBUG(7, "bad header (k == 9) 0%o\n", h->cntl&0xff);
				pk->p_state |= BADFRAME;
			}
			return;
		}
		/* data packet */
		if (k && pksizes[k] != pk->p_rsize)
			return;
		pk->p_rpr = h->cntl & MOD8;
		pksack(pk);
		if ((bp = pk->p_ipool) == NULL) {
			DEBUG(7, "bp NULL\n", 0);
			return;
		}
		pk->p_ipool = (char **) *bp;
		/* Header checks out, go for data */
		if (pkcget(pk->p_ifn, (char *) bp, pk->p_rsize) == SUCCESS) {
			pkdata(h->cntl, h->sum, pk, bp);
			Ntimeout = 0;
			return;
		}
		DEBUG(7, "Alarm while reading data -- request RXMIT\n", 0);
retransmit:
		/*
		 * Transmission error or excessive noise.  Send a RXMIT
		 * and try again.
		 */
		Retries++;
		pk->p_msg |= pk->p_rmsg;
		if (pk->p_msg == 0)
			pk->p_msg |= M_RR;
		if ((pk->p_state & LIVE) == LIVE)
			pk->p_state |= RXMIT;
		pkoutput(pk);
	}
	DEBUG(7, "pkgetpack failed after %d tries\n", tries);
	pkfail();
}

/*
 * Translate pk->p_state into something printable.
 */
xlatestate(pk, dbglvl)
register struct pack *pk;
{
	register int i;
	char delimc = ' ', msgline[80], *buf = msgline;

	if (Debug < dbglvl)
		return;
	sprintf(buf, "state -");
	buf += strlen(buf);
	for(i = 0; st_trans[i].state != -1; i++) {
		if (pk->p_state&st_trans[i].state){
			sprintf(buf, "%c[%s]", delimc, st_trans[i].msg);
			buf += strlen(buf);
			delimc = '&';
		}
	}
	sprintf(buf, " (0%o)\n", pk->p_state);
	DEBUG(dbglvl, "%s", msgline);
}

pkdata(c, sum, pk, bp)
register struct pack *pk;
unsigned short sum;
char c;
char **bp;
{
	register x;
	int t;
	char m;

	if (pk->p_state & DRAINO || !(pk->p_state & LIVE)) {
		pk->p_msg |= pk->p_rmsg;
		pkoutput(pk);
		goto drop;
	}
	t = next[pk->p_pr];
	for(x=pk->p_pr; x!=t; x = (x-1)&7) {
		if (pk->p_is[x] == 0)
			goto slot;
	}
drop:
	*bp = (char *)pk->p_ipool;
	pk->p_ipool = bp;
	return;

slot:
	m = mask[x];
	pk->p_imap |= m;
	pk->p_is[x] = c;
	pk->p_isum[x] = sum;
	pk->p_ib[x] = (char *)bp;
}

#define PKMAXBUF 128

/*
 * Start transmission on output device associated with pk.
 * For asynch devices (t_line==1) framing is
 * imposed.  For devices with framing and crc
 * in the driver (t_line==2) the transfer is
 * passed on to the driver.
 */
pkxstart(pk, cntl, x)
register struct pack *pk;
int x;
char cntl;
{
	register char *p;
	register short checkword;
	register char hdchk;

	p = (caddr_t) &pk->p_ohbuf;
	*p++ = SYN;
	if (x < 0) {
		*p++ = hdchk = 9;
		checkword = cntl;
	} else {
		*p++ = hdchk = pk->p_lpsize;
		checkword = pk->p_osum[x] ^ (unsigned)(cntl & 0377);
	}
	checkword = CHECK - checkword;
	*p = checkword;
	hdchk ^= *p++;
	*p = checkword>>8;
	hdchk ^= *p++;
	*p = cntl;
	hdchk ^= *p++;
	*p = hdchk;

 /*
  * writes
  */
	if (Debug >= 9)
		xlatecntl(1, cntl);

	p = (caddr_t) & pk->p_ohbuf;
	if (x < 0) {
		if ((*Write)(pk->p_ofn, p, HDRSIZ) != HDRSIZ) {
			DEBUG(4, "pkxstart, write failed, %s\n",
			    sys_errlist[errno]);
			logent(sys_errlist[errno], "PKXSTART WRITE");
			pkfail();
			/* NOT REACHED */
		}
	} else {
		char buf[PKMAXBUF + HDRSIZ]; 

		memcpy(buf, p, HDRSIZ);
		memcpy(buf+HDRSIZ, pk->p_ob[x], pk->p_xsize);
		if ((*Write)(pk->p_ofn, buf, pk->p_xsize + HDRSIZ) !=
		    pk->p_xsize + HDRSIZ) {
			DEBUG(4, "pkxstart, write failed, %s\n",
			    sys_errlist[errno]);
			logent(sys_errlist[errno], "PKXSTART WRITE");
			pkfail();
			/* NOT REACHED */
		}
		Connodata = 0;
	}
	if (pk->p_msg)
		pkoutput(pk);
}

/*
 * get n characters from input
 *	b	-> buffer for characters
 *	fn	-> file descriptor
 *	n	-> requested number of characters
 * return: 
 *	SUCCESS	-> n chars successfully read
 *	FAIL	-> o.w.
 */
jmp_buf Getjbuf;
cgalarm() { longjmp(Getjbuf, 1); }

pkcget(fn, b, n)
register int n;
register char *b;
register int fn;
{
	register int ret;
	register int donap;

	if (n == 0)
		return(SUCCESS);
	donap = (linebaudrate > 0 && linebaudrate < 4800);
	if (setjmp(Getjbuf)) {
		Ntimeout++;
		DEBUG(4, "pkcget: alarm %d\n", Ntimeout);
		return(FAIL);
	}
	(void) signal(SIGALRM, cgalarm);

	(void) alarm((unsigned) (n < HDRSIZ ? 10 : 20));
	while (1) {
		ret = (*Read)(fn, b, n);
		(void) alarm(0);
		if (ret == 0) {
			DEBUG(4, "pkcget, read failed, EOF\n", 0);
			return(FAIL);
		}
		if (ret < 0) {
			DEBUG(4, "pkcget, read failed, %s\n",
			    sys_errlist[errno]);
			logent(sys_errlist[errno], "PKCGET READ");
			pkfail();
			/* NOT REACHED */
		}
		if ((n -= ret) <= 0)
			break;
#ifdef PKSPEEDUP
		if (donap) {
#if defined(BSD4_2) /* && !defined(ATTSVTTY) && 0??? */
			/* wait for more chars to come in */
			nap((n * HZ * 10) / linebaudrate); /* n char times */
#else
			sleep(1);
#endif
		}
#endif /*  PKSPEEDUP  */
		b += ret;
	}
	(void) alarm(0);
	return(SUCCESS);
}

/*
 * role == 0: receive
 * role == 1: send
 */
xlatecntl(role, cntl)
{
	static char *cntltype[4] = {"CNTL, ", "ALT, ", "DATA, ", "SHORT, "};
	static char *cntlxxx[8] = {"ZERO, ", "CLOSE, ", "RJ, ", "SRJ, ",
				   "RR, ", "INITC, ", "INITB, ", "INITA, "};
	char dbgbuf[128];
	register char *ptr;

	ptr = dbgbuf;
	strcpy(ptr, role ? "send " : "recv ");
	ptr += strlen(ptr);

	strcpy(ptr, cntltype[(cntl&0300)>>6]);
	ptr += strlen(ptr);

	if (cntl&0300) {
		/* data packet */
		if (role)
			sprintf(ptr, "loc %o, rem %o\n", (cntl & 070) >> 3, cntl & 7);
		else
			sprintf(ptr, "loc %o, rem %o\n", cntl & 7, (cntl & 070) >> 3);
	} else {
		/* control packet */
		strcpy(ptr, cntlxxx[(cntl&070)>>3]);
		ptr += strlen(ptr);
		sprintf(ptr, "val %o\n", cntl & 7);
	}

	DEBUG(1, dbgbuf, 0);
}
