/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)regex.c 1.1 92/07/30 SMI" /* from S5R3 acct:lib/regex.c 1.4 */
#define SSIZE	50
#define TGRP	48
#define A256	01
#define A512	02
#define A768	03
#define	NBRA	10
#define CIRCFL	32
#define	EOF	-1

#define	CBRA	60
#define GRP	40
#define SGRP	56
#define PGRP	68
#define EGRP	44
#define RNGE	03
#define	CCHR	20
#define	CDOT	64
#define	CCL	24
#define	NCCL	8
#define	CDOL	28
#define	CEOF	52
#define	CKET	12

#define	STAR	01
#define PLUS	02
#define MINUS	16
char	*__braslist[NBRA];
char	*__braelist[NBRA];
char	*__loc1;
int	__bravar[NBRA];
int	*__st[SSIZE + 1];
int	*__eptr_,*__lptr_;
int	__cflg;
char *
regex(addrc,addrl,a1,a2,a3,a4,a5) char *addrc, *addrl, *a1, *a2, *a3, *a4, *a5;
{
	int cur,in;
	register int *adx;
	register char *p1, *p2;

	for(in=0;in<NBRA;in++) {
		__braslist[in] = 0;
		__bravar[in] = -1;
	}
	__cflg = 0;
	cur = __execute(addrc,addrl);
	adx = (int *)&a1;
	for(in=0;in<NBRA;in++) {
		if ((p1 = __braslist[in]) && (__bravar[in] >= 0)) {
			p2 = (char *)adx[__bravar[in]];
			while(p1 < __braelist[in]) *p2++ = *p1++;
			*p2 = '\0';
		}
	}
	if (!__cflg) return (addrl==(char *)cur)?(char *)0:(char *)cur;
	else return (char *)cur;
}

__execute(addrc,addrl)
char *addrc,*addrl;
{
	register char *p1, *p2, c;
	int i;

	p1 = addrl;
	p2 = addrc;
	__eptr_ = (int *)&__st[SSIZE];
	__lptr_ = (int *)&__st[0];
	if (*p2==CIRCFL) {
		__loc1 = p1;
		return (i=__advance(p1,++p2))?i:(int)addrl;
	}
	/* fast check for first character */
	if (*p2==CCHR) {
		c = p2[1];
		do {
			if (*p1!=c)
				continue;
			__eptr_ = (int *)&__st[SSIZE];
			__lptr_ = (int *)&__st[0];
			if (i=__advance(p1, p2))  {
				__loc1 = p1;
				return(i);
			}
		} while (*p1++);
		return((int)addrl);
	}
	/* regular algorithm */
	do {
	__eptr_ = (int *)&__st[SSIZE];
	__lptr_ = (int *)&__st[0];
		if (i=__advance(p1, p2))  {
			__loc1 = p1;
			return(i);
		}
	} while (*p1++);
	return((int)addrl);
}

__advance(alp, aep)
{
	register char *lp, *ep, *curlp;
	char *sep,*dp;
	char *nextep;
	int i,lcnt,dcnt,gflg;

	lp = (char *)alp;
	ep = (char *)aep;
	gflg = 0;
	for (;;) {
		switch(*ep++) {

	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return(0);

	case EGRP|RNGE:
		return((int)lp);
	case EGRP:
	case GRP:
		ep++;
		continue;

	case EGRP|STAR:
		__xpop(0);
	case EGRP|PLUS:
		__xpush(0,++ep);
		return((int)lp);

	case CDOT:
		if (*lp++)
			continue;
		return(0);

	case CDOL:
		if (*lp==0)
			continue;
		lp++;
		return(0);

	case CEOF:
		__cflg = 1;
		return((int)lp);

	case TGRP:
	case TGRP|A768:
	case TGRP|A512:
	case TGRP|A256:
		i = (((ep[-1]&03)<<8) + (*ep)&0377);
		ep++;
		__xpush(0,ep + i + 2);
		gflg = 1;
		__getrnge(&lcnt,&dcnt,&ep[i]);
		while(lcnt--)
			if (!(lp=(char *)__advance(lp,ep)))
				return(0);
		__xpush(1,curlp=lp);
		while(dcnt--)
			if(!(dp=(char *)__advance(lp,ep))) break;
			else __xpush(1,lp=dp);
		ep = (char *)__xpop(0);
		goto star;
	case CCHR|RNGE:
		sep = ep++;
		__getrnge(&lcnt,&dcnt,ep);
		while(lcnt--)
			if(*lp++!=*sep) return(0);
		curlp = lp;
		while(dcnt--)
			if(*lp++!=*sep) break;
		if (dcnt < 0) lp++;
		ep += 2;
		goto star;
	case CDOT|RNGE:
		__getrnge(&lcnt,&dcnt,ep);
		while(lcnt--)
			if(*lp++ == '\0') return(0);
		curlp = lp;
		while(dcnt--)
			if(*lp++ == '\0') break;
		if (dcnt < 0) lp++;
		ep += 2;
		goto star;
	case CCL|RNGE:
	case NCCL|RNGE:
		__getrnge(&lcnt,&dcnt,(ep + (*ep&0377)));
		while(lcnt--)
			if(!__cclass(ep,*lp++,ep[-1]==(CCL|RNGE))) return(0);
		curlp = lp;
		while(dcnt--)
			if(!__cclass(ep,*lp++,ep[-1]==(CCL|RNGE))) break;
		if (dcnt < 0) lp++;
		ep += (*ep + 2);
		goto star;
	case CCL:
		if (__cclass(ep, *lp++, 1)) {
			ep += *ep;
			continue;
		}
		return(0);

	case NCCL:
		if (__cclass(ep, *lp++, 0)) {
			ep += *ep;
			continue;
		}
		return(0);

	case CBRA:
		__braslist[*ep++] = lp;
		continue;

	case CKET:
		__braelist[*ep] = lp;
		__bravar[*ep] = ep[1];
		ep += 2;
		continue;

	case CDOT|PLUS:
		if (*lp++ == '\0') return(0);
	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;

	case CCHR|PLUS:
		if (*lp++ != *ep) return(0);
	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case PGRP:
	case PGRP|A256:
	case PGRP|A512:
	case PGRP|A768:
		if (!(lp=(char *)__advance(lp,ep+1))) return(0);
	case SGRP|A768:
	case SGRP|A512:
	case SGRP|A256:
	case SGRP:
		i = (((ep[-1]&03) << 8) + (*ep&0377));
		ep++;
		__xpush(0,ep + i);
		__xpush(1,curlp=lp);
		while (i=__advance(lp,ep))
			__xpush(1,lp=(char *)i);
		ep = (char *)__xpop(0);
		gflg = 1;
		goto star;

	case CCL|PLUS:
	case NCCL|PLUS:
		if (!__cclass(ep,*lp++,ep[-1]==(CCL|PLUS))) return(0);
	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (__cclass(ep, *lp++, ((ep[-1]==(CCL|STAR)) || (ep[-1]==(CCL|PLUS)))));
		ep += *ep;
		goto star;

	star:
		do {
			if(!gflg) lp--;
			else if (!(lp=(char *)__xpop(1))) break;
			if (i=__advance(lp, ep)) 
				return(i);
		} while (lp > curlp);
		return(0);

	default:
		return(0);
	}
	}
}
__cclass(aset, ac, af)
{
	register char *set, c;
	register n;

	set = (char *)aset;
	if ((c = ac) == 0)
		return(0);
	n = *set++;
	while (--n) {
		if (*set == MINUS) {
			if ((set[2] - set[1]) < 0) return(0);
			if (*++set <= c) {
				if (c <= *++set)
					return(af);
			}
			else ++set;
			++set;
			n -= 2;
			continue;
		}
		if (*set++ == c)
			return(af);
	}
	return(!af);
}
__xpush(i,p) int i; char *p;
{
	if (__lptr_ >= __eptr_) {write(2,"stack overflow\n",15);exit(1);}
	if (i) *__lptr_++ = (int)p;
	else   *__eptr_-- = (int)p;
	return(1);
}
__xpop(i) int i;
{
	if (i)
		return (__lptr_ < (int *)&__st[0])?0:*--__lptr_;
	else
		return (__eptr_ > (int *)&__st[SSIZE])?0:*++__eptr_;
}
__getrnge(i,j,k) int *i,*j; char *k;
{
	*i = (*k++&0377);
	if (*k == -1) *j = 20000;
	else *j = ((*k&0377) - *i);
	return(1);
}
