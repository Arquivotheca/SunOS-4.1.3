/*	@(#)packetfilt.h 1.1 92/07/30 SMI	*/

#ifndef _net_packetfilt_h
#define _net_packetfilt_h

#define ENMAXFILTERS	40		/* maximum filter short words */

/*
 *  filter structure for SETF
 */
struct packetfilt {
    u_char	Pf_Priority;		/* priority of filter */
    u_char	Pf_FilterLen;		/* length of filter cmd list */
    u_short	Pf_Filter[ENMAXFILTERS];
					/* the filter command list */
};

/*
 *  We now allow specification of up to MAXFILTERS (short) words of a filter
 *  command list to be applied to incoming packets to determine if
 *  those packets should be given to a particular open ethernet file.
 *
 *  In this context, "word" means a short (16-bit) integer.
 *  
 *  Each open enet file specifies the filter command list via ioctl.
 *  Each filter command list specifies a sequence of actions that leaves a
 *  boolean value on the top of an internal stack.  Each word of the
 *  command list specifies an action from the set {PUSHLIT, PUSHZERO,
 *  PUSHWORD+N} which respectively push the next word of the filter, zero,
 *  or word N of the incoming packet on the stack, and a binary operator
 *  from the set {EQ, LT, LE, GT, GE, AND, OR, XOR} which operates on the
 *  top two elements of the stack and replaces them with its result.  The
 *  special action NOPUSH and the special operator NOP can be used to only
 *  perform the binary operation or to only push a value on the stack.
 *  
 *  If the final value of the filter operation is true, then the packet is
 *  accepted for the open file which specified the filter.
 */

/*  these must sum to sizeof(u_short)!  */
#define ENF_NBPA	10			/* # bits / action */
#define ENF_NBPO	 6			/* # bits / operator */

/*  binary operators  */
#define ENF_NOP		( 0 << ENF_NBPA)
#define ENF_EQ		( 1 << ENF_NBPA)
#define ENF_LT		( 2 << ENF_NBPA)
#define ENF_LE		( 3 << ENF_NBPA)
#define ENF_GT		( 4 << ENF_NBPA)
#define ENF_GE		( 5 << ENF_NBPA)
#define ENF_AND		( 6 << ENF_NBPA)
#define ENF_OR		( 7 << ENF_NBPA)
#define ENF_XOR		( 8 << ENF_NBPA)
#define ENF_COR		( 9 << ENF_NBPA)
#define ENF_CAND	(10 << ENF_NBPA)
#define ENF_CNOR	(11 << ENF_NBPA)
#define ENF_CNAND	(12 << ENF_NBPA)
#define ENF_NEQ		(13 << ENF_NBPA)

/*  stack actions  */
#define ENF_NOPUSH	 0
#define ENF_PUSHLIT	 1	
#define ENF_PUSHZERO	 2
#define ENF_PUSHWORD	16

#ifdef	KERNEL
/*
 * Expanded version of the Packetfilt structure that includes
 * some additional fields that aid filter execution efficiency.
 */
struct epacketfilt {
    struct packetfilt	pf;
#define pf_Priority	pf.Pf_Priority
#define pf_FilterLen	pf.Pf_FilterLen
#define pf_Filter	pf.Pf_Filter
    u_short		*pf_FilterEnd;	/* pointer to word immediately
					   past end of filter */
    u_short		pf_PByteLen;	/* length in bytes of packet
					   prefix the filter examines */
};

/*
 * (Internal) packet descriptor for FilterPacket
 */
struct packdesc {
	u_short	*pd_hdr;	/* header starting address */
	u_int	pd_hdrlen;	/* header length in shorts */
	u_short	*pd_body;	/* body starting address */
	u_int	pd_bodylen;	/* body length in shorts */
};
#endif	KERNEL

#endif /*!_net_packetfilt_h*/
