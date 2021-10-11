/*
 * @(#)ipvar.h 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 */

#ifndef _IPIVAR_
#define _IPIVAR_


/* controller structure */
#define	NLPART		NDKMAP
#define LPART(dev)	(dev & 07)
#define MAXUNITS	16

/*
 * Packet control Block
 */
struct ippcb  {
	struct	ipictlr	    *pcb_c;		    /* pointer to controller struct */
	struct	ipiunit	    *pcb_un;		    /* ptr to unit struct	    */

	struct	buf	    *pcb_bp;		    /* know my buffer		    */
	struct	ippcb	    *pcb_prev;		    /* prev pcb			    */
	struct	ippcb	    *pcb_next;
	int		    pcb_retry;

	int		    pcb_mbcookie;

	u_char		    pcb_flags;
	u_char		    pcb_mode;
	u_char		    pcb_inuse;
	u_char		    pcb_busy;

	short		    pcb_timecount;
	short		    pcb_errcnt;

	struct	ipi3pkt	    *pcb_ppkt;		/* pointer to pkt		*/
};

/*
 * Controller structure One per controller
 */
struct ipictlr {
	struct	mb_ctlr		*c_mc;	/* Back pointer to mb_ctlr structure	*/
	struct	ipiunit 	*c_units[MAXUNITS];/* max units on controller	*/
	struct	ipidevice 	*c_io;		/* ptr to I/O space data	*/
	struct	ipi3pkt		*c_commandpkts;
	int			 c_busy;	/* controller is busy		*/
	int			 c_pend;	/* set - pending for DVMA alloc */
	int			 c_intpri;	/* interrupt priority		*/
	int			 c_intvec;	/* interrupt vector		*/
	u_long			 c_flags;	/* state information		*/
	struct	ippcb		 c_pcbhdr;	/* pcb header link list manip	*/
	struct	buf		 c_bufhdr;	/* buf hdr link list manip	*/
	struct	ippcb		*c_ppcbsent;	/* pcbs sent this far		*/
};


/*
 * Unit structure One per drive
 */
struct ipiunit {
	struct	ipictlr	    *c;		/* controller				*/
	struct	dk_map	    *un_map;	/* logical partitions			*/
	struct	dk_geom	    *un_g;	/* disk geometry			*/
	struct	buf	    *un_rtab;	/* for physio				*/
	int		     un_ltick;	/* last time active			*/
	struct	mb_device   *un_md;	/* generic unit				*/
	struct	mb_ctlr	    *un_mc;	/* generic controller			*/
	struct	dkbad	    *un_bad;	/* bad sector info			*/
	int		     un_errsect;/* sector in error			*/
	u_char		     un_errno;	/* error number				*/
	u_char		     un_errsevere; /* error severity			*/
	u_short		     un_errcmd;	/* command in error			*/
	u_char		     un_flags;	/* state information			*/
};


#define	IATTACHED	0x10
#define	ISLAVED 	0x20
#define IOPEN		0x40

/* OPCODES */
#define	IP_ATTRIBUTES	0x02	    /* Initialize, report, or load slave attribute  */
#define	IP_ALLOC_RESTOR	0x34	    /* Allocate restore				    */
#define	IP_SLAVE_DIAG	0x80	    /* Perform Slave Diagnostics		    */
#define	IP_FACIL_DIAG	0x81	    /* Perform Facility Diagnostics		    */
#define	IP_FORMAT	0x28	    /* Format					    */
#define	IP_NOP		0x00	    /* Nop					    */
#define	IP_OPER_MODE	0x07	    /* Set Operating mode of facility		    */
#define	IP_PORT_ADDR	0x04	    /* Port Address				    */
#define	IP_POS_CTL	0x41	    /* Position Control				    */
#define	IP_READ		0x10	    /* Read					    */
#define	IP_READ_BUF	0x52	    /* Read Buffer				    */
#define	IP_WRITE_BUF	0x62	    /* Write Buffer				    */
#define	IP_READ_DEFLIST	0x82	    /* Read Defect List				    */
#define	IP_WRITE_DEFLIST 0x83	    /* Read Defect List				    */
#define	IP_READ_ERRLOG	0x84	    /* Read Error Log				    */
#define	IP_REALLOC	0x33	    /* Reallocate Defect			    */
#define	IP_REPORT_STAT	0x03	    /* Report Addressee Status			    */
#define	IP_REPORT_POS	0x42	    /* Report Position				    */
#define	IP_WRITE	0x20	    /* Write					    */
#define	IP_STAT		0x91	    /* Read Statistics				    */

/* Bitdefs for Opcode Modifiers */

#define RWMOD 1         /* read: only read good data. write: */

#define	IP_SYNC         0x1
#define	IP_ASYNC        0x2
#define	IP_SLEEP        0x3

#define IP_IN		0x1
#define IP_OUT		0x2

/* opcodes for ioctl */
#define IP_DIAG_ENTER	0x77
#define IP_DIAG_EXIT	0x78
#define IP_MAPDVMA	0x79
#define	IP_MAPREGS	0x7a

#define PCB_GOT_DVMA	0x4
#define PCB_FAKEBP	0x8		/* pcb carries a fake bp */
#endif _IPIVAR_
