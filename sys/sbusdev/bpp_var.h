/*	@(#)bpp_var.h	1.1	92/07/30	SMI		*/
/*
 * 	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Local variables header file for the bidirectional parallel port
 *	driver (bpp) for the Zebra SBus card.
 *
 */
#ifndef	_sbusdev_bpp_var_h
#define	_sbusdev_bpp_var_h


/*	#defines (not struct elements) below */

/* Other external ioctls are defined in bpp_io.h	*/
	/* FOR TEST - request fakeout simulate partial data transfer	*/
#define	BPPIOC_SETBC		_IOW(b, 7, u_int)
	/* FOR TEST - request value of DMA_BCNT at end of last data transfer */
#define	BPPIOC_GETBC		_IOR(b, 8, u_int)
	/* FOR TEST - get contents of device registers */
#define	BPPIOC_GETREGS		_IOR(b, 9, struct bpp_regs)
	/* FOR TEST - set special error code to simulate errors */
#define	BPPIOC_SETERRCODE	_IOW(b, 10, int)
	/* FOR TEST - get pointer to (fakely) "transferred" data */
#define	BPPIOC_GETFAKEBUF	_IOW(b, 11, u_char *)
	/* FOR TEST - test nested timeout  calls */
#define	BPPIOC_TESTTIMEOUT	_IO(b, 12)

#ifdef	notdef
/*  This define will use the hardware simulations in software */
#define	NO_BPP_HW
#endif	notdef


#define MAX_NBPP	10	/* Maximum units allowed to identify */ 
#define	BPP_PROM_NAME	"SUNW,bpp"	/* name string in FCode prom */


/*	Structure definitions and locals #defines below */

#define FAKE_BUF_SIZE (1024 * 128)	/* size of each DVMA transfer */

struct bpp_unit				/* Unit structure - one per unit */
{
	u_char		unit_open;		/* 1 means open.	*/
	u_char		open_inhibit;		/* 1 means inhibit.	*/
	u_char		versatec_allowed;	/* 1 means allow	*/
						/* versatec handshakes	*/
	int		openflags;		/* read-write flags	*/
						/* this unit opened with*/
	u_char		timeouts;		/* encoded pending timeouts */

	u_int		interrupt_pri;		/* Highest int pri (spl)*/
						/* for this unit only.  */
	int		saved_spl;		/* Saved spl while in   */
						/* critical sections    */
						/* for this unit only.  */
						/* At all other times   */
						/* the value is (-1).   */
	long		transfer_remainder;	/* number of bytes which */
						/* were not transferred */
						/* since they were across */
						/* the max DVMA boundary. */
						/* value is (0) when transfer */
						/* is within the boundary */
	struct dev_info 	*devinfo_p;	/* Node for this unit.  */
	struct bpp_regs 	*bpp_regs_p;	/* Device control regs. */
	struct bpp_transfer_parms transfer_parms;
						/* handshake and timing */
	struct bpp_pins		pins;		/* control pins 	*/
	struct bpp_error_status	error_stat;	/* snapshotted error cond */
	struct buf		buf;		/* For physio() calls.  */
};

/* defines for the timeouts field */
#define	NO_TIMEOUTS		0x00		/* No timeouts pending	*/
#define	TRANSFER_TIMEOUT	0x01		/* DVMA transfer */
#define	FAKEOUT_TIMEOUT		0x10		/* Hardware simulation	*/
#define	TEST1_TIMEOUT		0x20		/* Test timeout #1	*/
#define	TEST2_TIMEOUT		0x40		/* Test timeout #2	*/



/*	Function declarations below	*/
extern int	nulldev();
extern int	seltrue();

#endif	_sbusdev_bpp_var_h
