/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_temp.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

#include "ex.h"
#include "ex_temp.h"
#include "ex_vis.h"
#include "ex_tty.h"

/*
 * Editor temporary file routines.
 * Very similar to those of ed, except uses 2 input buffers.
 */
#define	READ	0
#define	WRITE	1

char	tfname[40];
char	rfname[40];
int	havetmp;
short	tfile = -1;
short	rfile = -1;

fileinit()
{
	register char *p;
	register int i, j;
	struct stat stbuf;

	if (tline == INCRMT * (HBLKS+2))
		return;
	cleanup(0);
	close(tfile);
	tline = INCRMT * (HBLKS+2);
	blocks[0] = HBLKS;
	blocks[1] = HBLKS+1;
	blocks[2] = -1;
	dirtcnt = 0;
	iblock = -1;
	iblock2 = -1;
	oblock = -1;
	CP(tfname, svalue(vi_DIRECTORY));
	if (stat(tfname, &stbuf)) {
dumbness:
		if (setexit() == 0)
			filioerr(tfname);
		else
			putNFL();
		cleanup(1);
		exit(++errcnt);
	}
	if (!ISDIR(stbuf)) {
		errno = ENOTDIR;
		goto dumbness;
	}
	ichanged = 0;
	ichang2 = 0;
	ignore(strcat(tfname, "/ExXXXXX"));
	for (p = strend(tfname), i = 5, j = getpid(); i > 0; i--, j /= 10)
		*--p = j % 10 | '0';
	tfile = creat(tfname, 0600);
	if (tfile < 0)
		goto dumbness;
#ifdef VMUNIX
	{
		extern stilinc;		/* see below */
		stilinc = 0;
	}
#endif
	havetmp = 1;
	close(tfile);
	tfile = open(tfname, 2);
	if (tfile < 0)
		goto dumbness;
/* 	brk((char *)fendcore); */
}

cleanup(all)
	bool all;
{
	if (all) {
		putpad(exit_ca_mode);
		flush();
		resetterm();
		normtty--;
	}
	if (havetmp)
		unlink(tfname);
	havetmp = 0;
	if (all && rfile >= 0) {
		unlink(rfname);
		close(rfile);
		rfile = -1;
	}
	if (all == 1)
		exit(errcnt);
}

getline(tl)
	line tl;
{
	register char *bp, *lp;
	register int nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~OFFMSK;
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, READ);
			nl = nleft;
		}
}

putline()
{
	register char *bp, *lp;
	register int nl;
	line tl;

	dirtcnt++;
	lp = linebuf;
	change();
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~OFFMSK;
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl += INCRMT, WRITE);
			nl = nleft;
		}
	}
	tl = tline;
	tline += (((lp - linebuf) + BNDRY - 1) >> SHFT) & 077776;
	return (tl);
}

int	read();
int	write();

char *
getblock(atl, iof)
	line atl;
	int iof;
{
	register int bno, off;
        register char *p1, *p2;
        register int n;
	line *tmpptr;
	
	bno = (atl >> OFFBTS) & BLKMSK;
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS) {
		/*
		 * When we overflow tmpfile buffers,
		 * throw away line which could not be 
		 * put into buffer.
		 */
		for(tmpptr = dot; tmpptr < unddol; tmpptr++)
			*tmpptr = *(tmpptr+1);
		if (dot == dol)
			dot--;
		dol--;
		unddol--;
		error(" Tmp file too large");
	}
	nleft = BUFSIZ - off;
	if (bno == iblock) {
		ichanged |= iof;
		hitin2 = 0;
		return (ibuff + off);
	}
	if (bno == iblock2) {
		ichang2 |= iof;
		hitin2 = 1;
		return (ibuff2 + off);
	}
	if (bno == oblock)
		return (obuff + off);
	if (iof == READ) {
		if (hitin2 == 0) {
			if (ichang2) {
				blkio(iblock2, ibuff2, write);
			}
			ichang2 = 0;
			iblock2 = bno;
			blkio(bno, ibuff2, read);
			hitin2 = 1;
			return (ibuff2 + off);
		}
		hitin2 = 0;
		if (ichanged) {
			blkio(iblock, ibuff, write);
		}
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
		return (ibuff + off);
	}
	if (oblock >= 0) {
			blkio(oblock, obuff, write);
	}
	oblock = bno;
	return (obuff + off);
}

#ifdef	VMUNIX
#define	INCORB	64
char	incorb[INCORB+1][BUFSIZ];
#define	pagrnd(a)	((char *)(((int)a)&~(BUFSIZ-1)))
int	stilinc;	/* up to here not written yet */
#endif

blkio(b, buf, iofcn)
	short b;
	char *buf;
	int (*iofcn)();
{

#ifdef VMUNIX
	if (b < INCORB) {
		if (iofcn == read) {
			bcopy(pagrnd(incorb[b+1]), buf, BUFSIZ);
			return;
		}
		bcopy(buf, pagrnd(incorb[b+1]), BUFSIZ);
		if (laste) {
			if (b >= stilinc)
				stilinc = b + 1;
			return;
		}
	} else if (stilinc)
		tflush();
#endif
	lseek(tfile, (long) (unsigned) b * BUFSIZ, 0);
	if ((*iofcn)(tfile, buf, BUFSIZ) != BUFSIZ)
		filioerr(tfname);
}

#ifdef VMUNIX
tlaste()
{

	if (stilinc)
		dirtcnt = 0;
}

tflush()
{
	int i = stilinc;
	
	stilinc = 0;
	lseek(tfile, (long) 0, 0);
	if (write(tfile, pagrnd(incorb[1]), i * BUFSIZ) != (i * BUFSIZ))
		filioerr(tfname);
}
#endif

/*
 * Synchronize the state of the temporary file in case
 * a crash occurs.
 */
synctmp()
{
	register int cnt;
	register line *a;
	register short *bp;
        register char *p1, *p2;
        register int n;

#ifdef VMUNIX
	if (stilinc)
		return;
#endif
	if (dol == zero)
		return;
	/*
	 * In theory, we need to encrypt iblock and iblock2 before writing
	 * them out, as well as oblock, but in practice ichanged and ichang2
	 * can never be set, so this isn't really needed.  Likewise, the
	 * code in getblock above for iblock+iblock2 isn't needed.
	 */
	if (ichanged)
		blkio(iblock, ibuff, write);
	ichanged = 0;
	if (ichang2)
		blkio(iblock2, ibuff2, write);
	ichang2 = 0;
	if (oblock != -1)
		blkio(oblock, obuff, write);
	time(&H.Time);
	uid = getuid();
		H.encrypted = 0;
	*zero = (line) H.Time;
	for (a = zero, bp = blocks; a <= dol; a += BUFSIZ / sizeof *a, bp++) {
		if (bp >= &H.Blocks[LBLKS-1])
			error(" Tmp file too large - block overflow");
		if (*bp < 0) {
			tline = (tline + OFFMSK) &~ OFFMSK;
			*bp = ((tline >> OFFBTS) & BLKMSK);
			if (*bp > NMBLKS) 
				error(" Tmp file too large");
			tline += INCRMT;
			oblock = *bp + 1;
			bp[1] = -1;
		}
		lseek(tfile, (long) (unsigned) *bp * BUFSIZ, 0);
		cnt = ((dol - a) + 2) * sizeof (line);
		if (cnt > BUFSIZ)
			cnt = BUFSIZ;
		if (write(tfile, (char *) a, cnt) != cnt) {
oops:
			*zero = 0;
			filioerr(tfname);
		}
		*zero = 0;
	}
	flines = lineDOL();
	lseek(tfile, 0l, 0);
	if (write(tfile, (char *) &H, sizeof H) != sizeof H)
		goto oops;
}

TSYNC()
{

	if (dirtcnt > MAXDIRT) {	
#ifdef VMUNIX
		if (stilinc)
			tflush();
#endif
		dirtcnt = 0;
		synctmp();
	}
}

/*
 * Named buffer routines.
 * These are implemented differently than the main buffer.
 * Each named buffer has a chain of blocks in the register file.
 * Each block contains roughly 508 chars of text,
 * and a previous and next block number.  We also have information
 * about which blocks came from deletes of multiple partial lines,
 * e.g. deleting a sentence or a LISP object.
 *
 * We maintain a free map for the temp file.  To free the blocks
 * in a register we must read the blocks to find how they are chained
 * together.
 *
 * BUG:		The default savind of deleted lines in numbered
 *		buffers may be rather inefficient; it hasn't been profiled.
 */
struct	strreg {
	short	rg_flags;
	short	rg_nleft;
	short	rg_first;
	short	rg_last;
} strregs[('z'-'a'+1) + ('9'-'0'+1)], *strp;

struct	rbuf {
	short	rb_prev;
	short	rb_next;
	char	rb_text[BUFSIZ - 2 * sizeof (short)];
} *rbuf, KILLrbuf, putrbuf, YANKrbuf, regrbuf;
#ifdef VMUNIX
short	rused[256];
#else
short	rused[32];
#endif
short	rnleft;
short	rblock;
short	rnext;
char	*rbufcp;

regio(b, iofcn)
	short b;
	int (*iofcn)();
{

	if (rfile == -1) {
		CP(rfname, tfname);
		*(strend(rfname) - 7) = 'R';
		rfile = creat(rfname, 0600);
		if (rfile < 0)
oops:
			filioerr(rfname);
		close(rfile);
		rfile = open(rfname, 2);
		if (rfile < 0)
			goto oops;
	}
	lseek(rfile, (long) b * BUFSIZ, 0);
	if ((*iofcn)(rfile, rbuf, BUFSIZ) != BUFSIZ)
		goto oops;
	rblock = b;
}

REGblk()
{
	register int i, j, m;

	for (i = 0; i < sizeof rused / sizeof rused[0]; i++) {
		m = (rused[i] ^ 0177777) & 0177777;
		if (i == 0)
			m &= ~1;
		if (m != 0) {
			j = 0;
			while ((m & 1) == 0)
				j++, m >>= 1;
			rused[i] |= (1 << j);
#ifdef RDEBUG
			printf("allocating block %d\n", i * 16 + j);
#endif
			return (i * 16 + j);
		}
	}
	error("Out of register space (ugh)");
	/*NOTREACHED*/
}

struct	strreg *
mapreg(c)
	register int c;
{

	if (isupper(c))
		c = tolower(c);
	return (isdigit(c) ? &strregs[('z'-'a'+1)+(c-'0')] : &strregs[c-'a']);
}

int	shread();

KILLreg(c)
	register int c;
{
	register struct strreg *sp;

	rbuf = &KILLrbuf;
	sp = mapreg(c);
	rblock = sp->rg_first;
	sp->rg_first = sp->rg_last = 0;
	sp->rg_flags = sp->rg_nleft = 0;
	while (rblock != 0) {
#ifdef RDEBUG
		printf("freeing block %d\n", rblock);
#endif
		rused[rblock / 16] &= ~(1 << (rblock % 16));
		regio(rblock, shread);
		rblock = rbuf->rb_next;
	}
}

/*VARARGS*/
shread()
{
	struct front { short a; short b; };

	if (read(rfile, (char *) rbuf, sizeof (struct front)) == sizeof (struct front))
		return (sizeof (struct rbuf));
	return (0);
}

int	getREG();

putreg(c)
	char c;
{
	register line *odot = dot;
	register line *odol = dol;
	register int cnt;

	deletenone();
	appendnone();
	rbuf = &putrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext == 0) {
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		vreg = -1;
		error("Nothing in register %c", c);
	}
	if (inopen && partreg(c)) {
		if (!FIXUNDO) {
			splitw++; vclean(); vgoto(WECHO, 0); vreg = -1;
			error("Can't put partial line inside macro");
		}
		squish();
		addr1 = addr2 = dol;
	}
	cnt = append(getREG, addr2);
	if (inopen && partreg(c)) {
		unddol = dol;
		dol = odol;
		dot = odot;
		pragged(0);
	}
	killcnt(cnt);
	notecnt = cnt;
}

partreg(c)
	char c;
{

	return (mapreg(c)->rg_flags);
}

notpart(c)
	register int c;
{

	if (c)
		mapreg(c)->rg_flags = 0;
}

getREG()
{
	register char *lp = linebuf;
	register int c;

	for (;;) {
		if (rnleft == 0) {
			if (rnext == 0)
				return (EOF);
			regio(rnext, read);
			rnext = rbuf->rb_next;
			rbufcp = rbuf->rb_text;
			rnleft = sizeof rbuf->rb_text;
		}
		c = *rbufcp;
		if (c == 0)
			return (EOF);
		rbufcp++, --rnleft;
		if (c == '\n') {
			*lp++ = 0;
			return (0);
		}
		*lp++ = c;
	}
}

YANKreg(c)
	register int c;
{
	register line *addr;
	register struct strreg *sp;
	char savelb[LBSIZE];

	if (isdigit(c))
		kshift();
	if (islower(c))
		KILLreg(c);
	strp = sp = mapreg(c);
	sp->rg_flags = inopen && cursor && wcursor;
	rbuf = &YANKrbuf;
	if (sp->rg_last) {
		regio(sp->rg_last, read);
		rnleft = sp->rg_nleft;
		rbufcp = &rbuf->rb_text[sizeof rbuf->rb_text - rnleft];
	} else {
		rblock = 0;
		rnleft = 0;
	}
	CP(savelb,linebuf);
	for (addr = addr1; addr <= addr2; addr++) {
		getline(*addr);
		if (sp->rg_flags) {
			if (addr == addr2)
				*wcursor = 0;
			if (addr == addr1)
				strcpy(linebuf, cursor);
		}
		YANKline();
	}
	rbflush();
	killed();
	CP(linebuf,savelb);
}

kshift()
{
	register int i;

	KILLreg('9');
	for (i = '8'; i >= '0'; i--)
		copy(mapreg(i+1), mapreg(i), sizeof (struct strreg));
}

YANKline()
{
	register char *lp = linebuf;
	register struct rbuf *rp = rbuf;
	register int c;

	do {
		c = *lp++;
		if (c == 0)
			c = '\n';
		if (rnleft == 0) {
			rp->rb_next = REGblk();
			rbflush();
			rblock = rp->rb_next;
			rp->rb_next = 0;
			rp->rb_prev = rblock;
			rnleft = sizeof rp->rb_text;
			rbufcp = rp->rb_text;
		}
		*rbufcp++ = c;
		--rnleft;
	} while (c != '\n');
	if (rnleft)
		*rbufcp = 0;
}

rbflush()
{
	register struct strreg *sp = strp;

	if (rblock == 0)
		return;
	regio(rblock, write);
	if (sp->rg_first == 0)
		sp->rg_first = rblock;
	sp->rg_last = rblock;
	sp->rg_nleft = rnleft;
}

/* Register c to char buffer buf of size buflen */
regbuf(c, buf, buflen)
char c;
char *buf;
int buflen;
{
	register char *p, *lp;

	rbuf = &regrbuf;
	rnleft = 0;
	rblock = 0;
	rnext = mapreg(c)->rg_first;
	if (rnext==0) {
		*buf = 0;
		error("Nothing in register %c",c);
	}
	p = buf;
	while (getREG()==0) {
		for (lp=linebuf; *lp;) {
			if (p >= &buf[buflen])
				error("Register too long@to fit in memory");
			*p++ = *lp++;
		}
		*p++ = '\n';
	}
	if (partreg(c)) p--;
	*p = '\0';
	getDOT();
}

/*
 * Encryption routines.  These are essentially unmodified from ed.
 */


#ifdef TRACE

/* 
 * Test code for displaying named registers.
 */

shownam()
{
	int k;

	printf("\nRegister   Contents\n");
	printf("========   ========\n");
	for (k = 'a'; k <= 'z'; k++) {
		rbuf = &putrbuf;
		rnleft = 0;
		rblock = 0;
		rnext = mapreg(k)->rg_first;
		printf(" %c:",k);
		if (rnext == 0)
			printf("\t\tNothing in register.\n");
		while (getREG() == 0) {
			printf("\t\t%s\n", linebuf);
		}
	}
	return (0);
}

/*
 * Test code for displaying numbered registers.
 */

shownbr()
{
	int k;

	printf("\nRegister   Contents\n");
	printf("========   ========\n");
	for (k = '1'; k <= '9'; k++) {
		rbuf = &putrbuf;
		rnleft = 0;
		rblock = 0;
		rnext = mapreg(k)->rg_first;
		printf(" %c:",k);
		if (rnext == 0)
			printf("\t\tNothing in register.\n");
		while (getREG() == 0) {
			printf("\t\t%s\n", linebuf);
		}
	}
	return (0);
}
#endif
