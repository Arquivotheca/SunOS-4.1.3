#ifndef lint
static	char sccsid[] = "@(#)packetfilt.c 1.1 92/07/30 SMI";
#endif

#include <sys/param.h>
#include <net/packetfilt.h>

/* #define DEBUG	1 */
/* #define INNERDEBUG	1 */	/* define only when debugging enDoFilter()
					or enInputDone()  */

#ifdef INNERDEBUG
#define	enprintf(flags)	if (enDebug&(flags)) printf

/*
 * Symbolic definitions for enDebug flag bits
 *	ENDBG_TRACE should be 1 because it is the most common
 *	use in the code, and the compiler generates faster code
 *	for testing the low bit in a word.
 */

#define	ENDBG_TRACE	1	/* trace most operations */
#define	ENDBG_DESQ	2	/* trace descriptor queues */
#define	ENDBG_INIT	4	/* initialization info */
#define	ENDBG_SCAV	8	/* scavenger operation */
#define	ENDBG_ABNORM	16	/* abnormal events */

int	enDebug = /* ENDBG_ABNORM | ENDBG_INIT | ENDBG_TRACE */ -1;
#endif /* INNERDEBUG */

/*
 * Apply the packet filter given by pfp to the packet given by
 * pp.  Return nonzero iff the filter accepts the packet.
 *
 * The packet comes in two pieces, a header and a body, since
 * that's the most convenient form for our caller.  The header
 * is in contiguous memory, whereas the body is in a mbuf.
 * Our caller will have adjusted the mbuf chain so that its first
 * min(MLEN, length(body)) bytes are guaranteed contiguous.  For
 * the sake of efficiency (and some laziness) the filter is prepared
 * to examine only these two contiguous pieces.  Furthermore, it
 * assumes that the header length is even, so that there's no need
 * to glue the last byte of header to the first byte of data.
 */

#define opx(i)	((i) >> ENF_NBPA)

int
FilterPacket(pp, pfp)
    struct packdesc	*pp;
    struct epacketfilt	*pfp;
{
    register int	maxhdr = pp->pd_hdrlen;
    int			maxword = maxhdr + pp->pd_bodylen;
    register u_short	*sp;
    register u_short	*fp;
    register u_short	*fpe;
    register unsigned	op;
    register unsigned	arg;
    u_short		stack[ENMAXFILTERS+1];

    fp = &pfp->pf_Filter[0];
    fpe = pfp->pf_FilterEnd;

#ifdef	INNERDEBUG
    enprintf(ENDBG_TRACE)("FilterPacket(%x, %x, %x, %x):\n", pp, pfp, fp, fpe);
#endif

    /*
     * Push TRUE on stack to start.  The stack size is chosen such
     * that overflow can't occur -- each operation can push at most
     * one item on the stack, and the stack size equals the maximum
     * program length.
     */
    sp = &stack[ENMAXFILTERS];
    *sp = 1;

    for ( ; fp < fpe; ) {
	op = *fp >> ENF_NBPA;
	arg = *fp & ((1 << ENF_NBPA) - 1);
	fp++;

	switch (arg) {
	    default:
		arg -= ENF_PUSHWORD;
		/*
		 * Since arg is unsigned,
		 * if it were less than ENF_PUSHWORD before,
		 * it would now be huge.
		 */
		if (arg < maxhdr)
		    *--sp = pp->pd_hdr[arg];
		else if (arg < maxword)
		    *--sp = pp->pd_body[arg - maxhdr];
		else {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>0(len)\n");
#endif
		    return (0);
		}
		break;
	    case ENF_PUSHLIT:
		*--sp = *fp++;
		break;
	    case ENF_PUSHZERO:
		*--sp = 0;
	    case ENF_NOPUSH:
		break;
	}
	if (sp < &stack[2]) {	/* check stack overflow: small yellow zone */
#ifdef	INNERDEBUG
	    enprintf(ENDBG_TRACE)("=>0(--sp)\n");
#endif
	    return (0);
	}
	if (op == ENF_NOP)
	    continue;
	/*
	 * all non-NOP operators binary, must have at least two operands
	 * on stack to evaluate.
	 */
	if (sp > &stack[ENMAXFILTERS-2]) {
#ifdef	INNERDEBUG
	    enprintf(ENDBG_TRACE)("=>0(sp++)\n");
#endif
	    return (0);
	}
	arg = *sp++;
	switch (op) {
	    default:
#ifdef	INNERDEBUG
		enprintf(ENDBG_TRACE)("=>0(def)\n");
#endif
		return (0);
	    case opx(ENF_AND):
		*sp &= arg;
		break;
	    case opx(ENF_OR):
		*sp |= arg;
		break;
	    case opx(ENF_XOR):
		*sp ^= arg;
		break;
	    case opx(ENF_EQ):
		*sp = (*sp == arg);
		break;
	    case opx(ENF_NEQ):
		*sp = (*sp != arg);
		break;
	    case opx(ENF_LT):
		*sp = (*sp < arg);
		break;
	    case opx(ENF_LE):
		*sp = (*sp <= arg);
		break;
	    case opx(ENF_GT):
		*sp = (*sp > arg);
		break;
	    case opx(ENF_GE):
		*sp = (*sp >= arg);
		break;

	    /* short-circuit operators */

	    case opx(ENF_COR):
		if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return (1);
		}
		break;
	    case opx(ENF_CAND):
		if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>CAND %x\n", *sp);
#endif
		    return (0);
		}
		break;
	    case opx(ENF_CNOR):
		if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return (0);
		}
		break;
	    case opx(ENF_CNAND):
		if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>CNAND %x\n", *sp);
#endif
		    return (1);
		}
		break;
	}
    }
#ifdef	INNERDEBUG
    enprintf(ENDBG_TRACE)("=>%x\n", *sp);
#endif
    return (*sp);
}
