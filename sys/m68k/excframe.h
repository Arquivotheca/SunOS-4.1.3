/*       @(#)excframe.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures related to 68xxx bus error handling.
 */

#ifndef _m68k_excframe_h
#define _m68k_excframe_h

#ifndef LOCORE
/*
 * 68010/020/030 "type 0" normal four word stack.
 */
#define	SF_NORMAL	0x0

/*
 * 68020/030 "type 1" throwaway four word stack.
 */
#define	SF_THROWAWAY	0x1

/*
 * 68020/030 "type 2" normal six word stack frame.
 */
#define	SF_NORMAL6	0x2
struct	bei_normal6 {
	int	bei_pc;		/* pc where error occurred */
};

/*
 * 68010 "type 8" long bus error stack frame.
 */
#define	SF_LONG8	0x8
struct	bei_long8 {
	u_int	bei_rerun : 1;	/* rerun bus cycle */
	u_int		  : 1;
	u_int	bei_ifetch: 1;	/* inst fetch (1=true) */
	u_int	bei_dfetch: 1;	/* data fetch */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_hibyte: 1;	/* high byte transfer */
	u_int	bei_bytex : 1;	/* byte transfer */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int		  : 4;
	u_int	bei_fcode : 4;	/* function code */
	int	bei_accaddr;	/* access address */
	u_int		  : 16;	/* undefined */
	short	bei_dob;		/* data output buffer */
	u_int		  : 16;	/* undefined */
	short	bei_dib;		/* data input buffer */
	u_int		  : 16;	/* undefined */
	short	bei_irc;		/* inst buffer */
	short	bei_maskpc;	/* chip mask # & micropc */
	short	bei_undef[15];	/* undefined */
};

/*
 * 68020/030 "type 9" co-processor mid-instruction exception stack frame.
 */
#define	SF_COPROC	0x9
struct	bei_coproc {
	int	bei_pc;		/* pc where error occured */
	int	bei_internal;	/* an internal register */
	int	bei_eea;	/* evaluated effective address */
};

/*
 * 68020/030 "type A" medium bus cycle fault stack frame.
 */
#define	SF_MEDIUM	0xA
struct	bei_medium {
	short	bei_internal;	/* an internal register */
	u_int	bei_faultc: 1;	/* fault on stage c of instruction pipe */
	u_int	bei_faultb: 1;	/* fault on stage b of instruction pipe */
	u_int	bei_rerunc: 1;	/* rerun stage c */
	u_int	bei_rerunb: 1;	/* rerun stage b */
	u_int		  : 3;
	u_int	bei_dfault: 1;	/* fault/rerun on data */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int	bei_size  : 2;	/* size code for data cycle */
	u_int		  : 1;
	u_int	bei_fcode : 3;	/* function code */
	short	bei_ipsc;	/* instruction pipe stage c */
	short	bei_ipsb;	/* instruction pipe stage b */
	int	bei_fault;	/* fault address */
	int	bei_internal2;	/* another internal register */
	int	bei_dob;	/* data output buffer */
	int	bei_internal3;	/* yet another internal register */
};

/*
 * 68020/030 "type B" long bus cycle fault stack frame.
 */
#define	SF_LONGB	0xB
struct	bei_longb {
	short	bei_internal;	/* an internal register */
	u_int	bei_faultc: 1;	/* fault on stage c of instruction pipe */
	u_int	bei_faultb: 1;	/* fault on stage b of instruction pipe */
	u_int	bei_rerunc: 1;	/* rerun stage c */
	u_int	bei_rerunb: 1;	/* rerun stage b */
	u_int		  : 3;
	u_int	bei_dfault: 1;	/* fault/rerun on data */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int	bei_size  : 2;	/* size code for data cycle */
	u_int		  : 1;
	u_int	bei_fcode : 3;	/* function code */
	short	bei_ipsc;	/* instruction pipe stage c */
	short	bei_ipsb;	/* instruction pipe stage b */
	int	bei_fault;	/* fault address */
	int	bei_internal2;	/* another internal register */
	int	bei_dob;	/* data output buffer */
	int	bei_internal3;	/* yet another internal register */
	int	bei_internal4;	/* yet another internal register */
	int	bei_stageb;	/* stage B address */
	int	bei_internal5;	/* yet another internal register */
	int	bei_dib;	/* data input buffer */
	int	bei_internal6[11];/* more internal registers */
};
#endif !LOCORE

#endif /*!_m68k_excframe_h*/
