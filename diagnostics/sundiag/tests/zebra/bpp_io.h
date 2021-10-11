/******  @(#)bpp_io.h 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc. *****/
/*	@(#)bpp_io.h	1.6	90/04/30	SMI		*/
/*
 * 	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	I/O header file for the bidirectional parallel port
 *	driver (bpp) for the Zebra SBus card.
 *
 */
/*	#includes below		*/

/*	#defines (not struct elements) below */
/*
 * Minor device number encoding:
 */
#define	BPP_UNIT(dev)	minor(dev)

/*
 * ioctl defines for cmd argument
 */
	/* set contents of transfer parms structure	*/
#define	BPPIOC_SETPARMS		_IOW(b, 1, struct bpp_transfer_parms)
	/* read contents of transfer parms structure	*/
#define	BPPIOC_GETPARMS		_IOR(b, 2, struct bpp_transfer_parms)
	/* set contents of output pins structure	*/
#define	BPPIOC_SETOUTPINS	_IOW(b, 3, struct bpp_pins)
	/* read contents of output pins structure	*/
#define	BPPIOC_GETOUTPINS	_IOR(b, 4, struct bpp_pins)
	/* read contents of snapshot error status structure	*/
#define	BPPIOC_GETERR		_IOR(b, 5, struct bpp_error_status)
	/* pretend to attempt a data transfer	*/
#define	BPPIOC_TESTIO		_IO(b, 6)
/* Other test-only ioctls are defined in bpp_var.h	*/
/* 7-11 are used there */


/*	typedefs below		*/

/*	Structure definitions and locals #defines below */

#define	MAX_TIMEOUT	1800000		/* maximum read/write timeout	*/
					/* 30 minutes			*/

/*
 * This structure is used to configure the hardware handshake and
 * timing modes.
 */
struct bpp_transfer_parms {
	int	read_handshake;		/* parallel port read handshake mode */
#define	BPP_NO_HS	1		/* no handshake */
#define	BPP_ACK_HS	2		/* handshake controlled by ack line */
#define	BPP_BUSY_HS	3		/* handshake controlled by bsy line */
#define	BPP_ACK_BUSY_HS	4		/* controlled by ack and bsy lines */
#define	BPP_XSCAN_HS	5		/* xerox scanner mode */
#define	BPP_HSCAN_HS	6		/* HP Scanjet scanner mode */
#define	BPP_CLEAR_MEM	7		/* write zeroes to memory */
#define	BPP_SET_MEM	8		/* write ones to memory */

	int	read_setup_time;	/* DSS register - in nanoseconds */
		/* dstrb- to bsy+ in BPP_NO_HS or BPP_ACK_HS		*/
		/* dstrb- to ack+ in BPP_ACK_HS or BPP_ACK_BUSY_HS	*/
		/* ack- to dstrb+ in BPP_XSCAN_HS			*/

	int	read_strobe_width;	/* DSW register - in nanoseconds */
		/* ack+ to ack- in BPP_ACK_HS or BPP_ACK_BUSY_HS	*/
		/* dstrb+ to dstrb- in BPP_XSCAN_HS			*/

	int	read_timeout;		/* wait this many microseconds */
					/* before aborting a transfer */
	int	write_handshake;	/* parallel port write handshake mode */
#if	NO
/* these are duplicates of the definitions above */
	#define	BPP_NO_HS		/* no handshake */
	#define	BPP_ACK_HS		/* handshake controlled by ack line */
	#define	BPP_BUSY_HS		/* handshake controlled by bsy line */
#endif	NO
/* The two versatec handshakes are valid only in write-only mode */
#define	BPP_VPRINT_HS	9		/* versatec print mode */
#define	BPP_VPLOT_HS	10		/* versatec plot mode */

	int	write_setup_time;	/* DSS register - in nanoseconds */
		/* data valid to dstrb+ in all handshakes		*/

	int	write_strobe_width;	/* DSW register - in nanoseconds */
		/* dstrb+ to dstrb- in non-versatec handshakes	*/
		/* minimum dstrb+ to dstrb- in BPP_VPRINT_HS or BPP_VPLOT_HS */
	int	write_timeout;		/* wait this many microseconds */
					/* before aborting a transfer */
};

static struct	bpp_transfer_parms	default_transfer_parms = {
	BPP_ACK_BUSY_HS,		/* read_handshake */
	1000,				/* read_setup_time - 1 us */
	1000,				/* read_strobe_width - 1 us */
	60,				/* read_timeout - 1 minute */
	BPP_ACK_HS,			/* write_handshake */
	1000,				/* write_setup_time - 1 us */
	1000,				/* write_strobe_width - 1 us */
	60,				/* write_timeout - 1 minute */
};

struct	bpp_pins {
	u_char	output_reg_pins;	/* pins in P_OR register */
#define	BPP_SLCTIN_PIN		0x01	/* Select in pin		*/
#define	BPP_AFX_PIN		0x02	/* Auto feed pin		*/
#define	BPP_INIT_PIN		0x04	/* Initialize pin		*/
#define	BPP_V1_PIN		0x08	/* Versatec pin 1		*/
#define	BPP_V2_PIN		0x10	/* Versatec pin 2		*/
#define	BPP_V3_PIN		0x20	/* Versatec pin 3		*/

#define	BPP_ALL_OUT_PINS	(BPP_SLCTIN_PIN | BPP_AFX_PIN | BPP_INIT_PIN |\
				 BPP_V1_PIN | BPP_V2_PIN | BPP_V3_PIN )

	u_char	input_reg_pins;		/* pins in P_IR register */
#define	BPP_ERR_PIN		0x01	/* Error pin			*/
#define	BPP_SLCT_PIN		0x02	/* Select pin			*/
#define	BPP_PE_PIN		0x04	/* Paper empty pin		*/

#define	BPP_ALL_IN_PINS		(BPP_ERR_PIN | BPP_SLCT_PIN | BPP_PE_PIN)
};

static struct bpp_pins		default_pins = {
	0,				/* output pins	*/
	0,				/* input pins	*/
};

struct	bpp_error_status {
	char	timeout_occurred;	/* 1 if a timeout occurred	*/
	char	bus_error;		/* 1 if an SBus bus error	*/
	u_char	pin_status;		/* status of pins which could	*/
					/* cause an error		*/
#define	BPP_ERR_ERR		0x01	/* Error pin			*/
#define	BPP_SLCT_ERR		0x02	/* Select pin			*/
#define	BPP_PE_ERR		0x04	/* Paper empty pin		*/
#define	BPP_SLCTIN_ERR		0x10	/* Select in pin		*/
#define	BPP_BUSY_ERR		0x40	/* Busy pin 			*/
};

static struct bpp_error_status	default_error_stat = {
	0,				/* no timout		*/
	0,				/* no bus error		*/
	0,				/* no pin status set	*/
};

/*	Function declarations below	*/

/*	External Variable declararations below */
