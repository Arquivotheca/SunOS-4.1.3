/*	@(#)isvar.h	1.1 7/30/92 SMI */

#ifndef _isvar_h
#define _isvar_h 

#include <sundev/ipi_chan.h>

#define IS_RESP_PER_CH	3		/* number of response areas to alloc */
#define IS_NLIST	2		/* number of packet free lists */
#define	IS_SIZE(index)	(64 << (index))	/* size of packet for list 'index' */
#define IS_INIT_ALLOC	8		/* initial allocation */
#define IS_INIT_SIZE	128		/* initial allocation size */
#define IS_MAX_SIZE	IS_SIZE(IS_NLIST-1)	/* max cmd/resp area size */

#define	IS_MAX_ALLOC	8192		/* maximum cmd/response allocation */
#define	IS_MAX_IOPB	(ctob(IOPBMEM)/4) /* maximum alloc from IOPBMEM */


/*
 * Sun VME IPI string controller/channel:  controller and device structure.
 * 
 * This all assumes only one channel or string controller per board.
 */
struct is_ctlr {
	struct is_reg	*ic_reg;	/* I/O registers */
	struct mb_ctlr	*ic_ctlr;	/* MB Controller */
	struct mb_device *ic_dev;	/* MB Device */
	struct ipiq	*ic_resp_q;	/* response queue */

	short		ic_cmd_pend;	/* total commands pending */
	u_short		ic_chan;	/* System Channel Number */
	char		ic_resp_pend;	/* pending response buffers */
	char		ic_flags;	/* flags */
	u_short		ic_alloc[IS_NLIST];	/* current allocation */

	struct ipiq_list ic_wait;	/* wait list */
};

/*
 * Flags in is_ctlr.
 */
#define	IS_ENABLE_RESET	1		/* enable reset */
#define IS_PRESENT	2		/* controller probed successfully */
#define IS_RESET_WAIT	4		/* waiting after reset */

#endif	/* !_isvar_h */ 
