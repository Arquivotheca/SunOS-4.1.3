/*	@(#)tty_subr.c 1.1 92/07/30 SMI; from UCB 4.20 83/05/27	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/clist.h>

char	cwaiting;

/*
 * Character list get/put
 */
getc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;

	s = spltty();
	if (p->c_cc <= 0) {
		c = -1;
		p->c_cc = 0;
		p->c_cf = p->c_cl = NULL;
	} else {
		c = *p->c_cf++ & 0377;
		if (--p->c_cc<=0) {
			bp = (struct cblock *)(p->c_cf-1);
			bp = (struct cblock *) ((int)bp & ~CROUND);
			p->c_cf = NULL;
			p->c_cl = NULL;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		} else if (((int)p->c_cf & CROUND) == 0){
			bp = (struct cblock *)(p->c_cf);
			bp--;
			p->c_cf = bp->c_next->c_info;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		}
	}
	(void) splx(s);
	return (c);
}

putc(c, p)
	register struct clist *p;
{
	register struct cblock *bp;
	register char *cp;
	register s;

	s = spltty();
	if ((cp = p->c_cl) == NULL || p->c_cc < 0) {
		if ((bp = cfreelist) == NULL) {
			(void) splx(s);
			return (-1);
		}
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		p->c_cf = cp = bp->c_info;
	} else if (((int)cp & CROUND) == 0) {
		bp = (struct cblock *)cp - 1;
		if ((bp->c_next = cfreelist) == NULL) {
			(void) splx(s);
			return (-1);
		}
		bp = bp->c_next;
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		cp = bp->c_info;
	}
	*cp++ = c;
	p->c_cc++;
	p->c_cl = cp;
	(void) splx(s);
	return (0);
}



/*
 * copy buffer to clist.
 * return number of bytes not transfered.
 */
b_to_q(cp, cc, q)
	register char *cp;
	struct clist *q;
	register int cc;
{
	register char *cq;
	register struct cblock *bp;
	register s, acc;

	if (cc <= 0)
		return (0);
	acc = cc;


	s = spltty();
	if ((cq = q->c_cl) == NULL || q->c_cc < 0) {
		if ((bp = cfreelist) == NULL)
			goto out;
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		q->c_cf = cq = bp->c_info;
	}

	while (cc) {
		if (((int)cq & CROUND) == 0) {
			bp = (struct cblock *) cq - 1;
			if ((bp->c_next = cfreelist) == NULL)
				goto out;
			bp = bp->c_next;
			cfreelist = bp->c_next;
			cfreecount -= CBSIZE;
			bp->c_next = NULL;
			cq = bp->c_info;
		}
		*cq++ = *cp++;
		cc--;
	}
out:
	q->c_cl = cq;
	q->c_cc += acc-cc;
	(void) splx(s);
	return (cc);
}

/*
 * Remove the last character in the list and return it.
 */
unputc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;
	struct cblock *obp;

	s = spltty();
	if (p->c_cc <= 0)
		c = -1;
	else {
		c = *--p->c_cl;
		if (--p->c_cc <= 0) {
			bp = (struct cblock *)p->c_cl;
			bp = (struct cblock *)((int)bp & ~CROUND);
			p->c_cl = p->c_cf = NULL;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
		} else if (((int)p->c_cl & CROUND) == sizeof (bp->c_next)) {
			p->c_cl = (char *)((int)p->c_cl & ~CROUND);
			bp = (struct cblock *)p->c_cf;
			bp = (struct cblock *)((int)bp & ~CROUND);
			while (bp->c_next != (struct cblock *)p->c_cl)
				bp = bp->c_next;
			obp = bp;
			p->c_cl = (char *)(bp + 1);
			bp = bp->c_next;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			obp->c_next = NULL;
		}
	}
	(void) splx(s);
	return (c);
}

/*
 * Only some vax drivers require use of getw() and putw()
 * right now, so we ifdef them out for other machine types.
 */
#ifdef vax
/*
 * Integer (short) get/put
 * using clists
 */
typedef	short word_t;
union chword {
	word_t	word;
	struct {
		char	Ch[sizeof (word_t)];
	} Cha;
#define	ch	Cha.Ch
};

getw(p)
	register struct clist *p;
{
	register int i;
	union chword x;

	if (p->c_cc < sizeof (word_t))
		return (-1);
	for (i = 0; i < sizeof (word_t); i++)
		x.ch[i] = getc(p);
	return (x.word);
}

putw(c, p)
	register struct clist *p;
{
	register s;
	register int i;
	union chword x;

	s = spltty();
	if (cfreelist==NULL) {
		(void) splx(s);
		return (-1);
	}
	x.word = c;
	for (i = 0; i < sizeof (word_t); i++)
		(void) putc(x.ch[i], p);
	(void) splx(s);
	return (0);
}
#endif vax
