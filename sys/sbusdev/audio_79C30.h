/* @(#) audio_79C30.h 1.1@(#) Copyright (c) 1989-92 Sun Microsystems, Inc. */

#ifndef _sbusdev_amd_h
#define	_sbusdev_amd_h

/*
 * This file describes the AMD79C30A ISDN chip and declares
 * parameters and data structures used by the audio driver.
 */

#ifdef KERNEL

/* Driver constants for the AM79C30 */
#define	AUD_79C30_PRECISION	(8)		/* Bits per sample unit */
#define	AUD_79C30_CHANNELS	(1)		/* Channels per sample frame */
#define	AUD_79C30_SAMPLERATE	(8000)		/* Sample frames per second */

/*
 * This is the size of the STREAMS buffers we send down the read side.
 *
 * XXX - For now, this is set to 1K so that reader processes like soundtool
 *	that require some real-time response to input don't have to wait
 *	for larger buffers to fill up.  Eventually, there should be an ioctl
 *	that tells the driver to send all available data upstream.  In this
 *	way, the buffering for 'normal' processes can be increased without
 *	sacrificing the performance of real-time display processes.
 */
#define	AUD_79C30_BSIZE		(1024)		/* Record buffer size */

/* N.B. High and low water marks must fit into unsigned shorts */
#define	AUD_79C30_HIWATER	(8192 * 3)	/* ~3 seconds */
#define	AUD_79C30_LOWATER	(8192 * 2)	/* ~2 seconds */

#define	AUD_79C30_CMDPOOL	(7)	/* total command block buffer pool */
#define	AUD_79C30_RECBUFS	(4)	/* number of record command blocks */

#define	AUD_79C30_DEFAULT_GAIN	(128)	/* gain initialization */

#define	AMD_MINOR_RW	(1)
#define	AMD_MINOR_RO	(2)
#define	AMD_MINOR_CTL	(3)
#define AMD_MINOR_CTL_OLD	(128)


/*
 * These are the registers for the ADM79C30 chip.
 *
 * These names are not very descriptive, but they match the names
 * chosen by AMD.
 */
struct aud_79C30_chip {
	char	cr;		/* Command Register */
	char	dr;		/* Data Register */
	char	dsr1;		/* D-channel Status Register 1 */
	char	der;		/* D-channel Error Register */
	char	dctb;		/* D-channel Transmit Buffer */
	char	bbtb;		/* Bb Transmit Buffer */
	char	bctb;		/* Bc Transmit Buffer */
	char	dsr2;		/* D-channel Status Register 2 */
};

#define	ir	cr		/* Interrupt Register */
#define	bbrb	bbtb		/* Bb Receive Buffer */
#define	bcrb	bctb		/* Bc Receive Buffer */

/*
 *
 */
typedef struct amd_stream {
	aud_stream_t		as;
	aud_cmd_t		*cmdptr;
	unsigned int		samples;
	char			active;
	char			error;
} amd_stream_t;

/*
 * This is the control structure used by the AMD79C30-specific driver routines.
 */
typedef struct {
	aud_state_t		hwi_state;
	struct aud_79C30_chip	*chip;		/* addr of device registers */
	amd_stream_t		play;
	amd_stream_t		rec;
	amd_stream_t		control;
	struct dev_info		*amd_dev;

#ifdef AMD_CHIP
	/*
	 * The AUDIOSETREG ioctl may cause the device to be out of sync
	 * with respect to the driver.
	 */
	int				setreg_used;	/* init on close */
#endif /* AMD_CHIP */
} amd_dev_t;

/* Macros to derive control struct and chip addresses from the audio struct */
#define	DEVP(as)	((amd_dev_t *)(as)->v->devdata)
#define	CSR(as)		(DEVP(as)->chip)

#endif /* KERNEL */


/*
 * NOTE WELL:
 * The following ioctl() interface to the AMD 79C30 chip registers
 * is not supported.  It is provided as a convenience for backwards-
 * compatibility and for prototyping applications that access the
 * advanced ISDN features of the chip.  Future versions of the audio
 * driver may cease to implement the AUDIOGETREG and AUDIOSETREG commands
 * altogether.
 *
 * Use of AUDIOSETREG may interfere with normal device driver functions
 * since it allows applications to alter the state of the device.
 */
#ifdef AMD_CHIP

/*
 * Control specifies the register to which we are writing, plus the length
 * of the register, in bytes.  The two values are combined to make it easier
 * to use.  The register is in the high byte, and the length is in the
 * low byte.  Macros in sbusdev/audioreg.h make it easier to create the
 * values for the control member.
 *
 * By setting the register value appropriately, multiple registers can
 * be loaded with one operation.  The maximum number of bytes loaded at
 * one time is 46, when all the MAP registers are set.
 */
struct audio_ioctl {
	short		control;	/* register/length: see macros below */
	unsigned char	data[46];
};

/*
 * GETREG and SETREG are used to read and write the chip's registers,
 * using the audio_ioctl structure to pass data back and forth.
 */
#define	AUDIOGETREG		_IOWR(i, 1, struct audio_ioctl)
#define	AUDIOSETREG		_IOW(i, 2, struct audio_ioctl)

#endif /* AMD_CHIP */



/*
 * These defines facilitate access to the indirect registers.  One
 * of these values is written to the command register, then the
 * appropriate number of bytes are written/read to/from the data
 * register.
 *
 * The indirection values are formed by OR'ing together two quantities.
 * The first three bits of the indirection value specifies the
 * chip subsystem (audio processor, multiplexor, etc.) to be accessed.
 * AMP calles these three bits the DCF, or destination code field.
 * The last five bits specify the register within the subsystem.  AMD
 * calls these bits the OCF, or Operation Code Field.
 *
 * Note that the INIT_INIT value differs from the data sheet.  The
 * data sheet is wrong -- this is right.
 */
#define	AUDIO_PACK(reg, length)	(((reg) << 8) | (length))
#define	AUDIO_UNPACK_REG(x)	(((x) >> 8) & 0xff)
#define	AUDIO_UNPACK_LENGTH(x)	((x) & 0xff)

#define	AUDIO_INIT	0x20
#define	AUDIO_INIT_INIT	AUDIO_PACK((AUDIO_INIT | 0x01), 1)

#define	AUDIO_MUX	0x40
#define	AUDIO_MUX_MCR1	AUDIO_PACK(AUDIO_MUX | 0x01, 1)
#define	AUDIO_MUX_MCR2	AUDIO_PACK(AUDIO_MUX | 0x02, 1)
#define	AUDIO_MUX_MCR3	AUDIO_PACK(AUDIO_MUX | 0x03, 1)
#define	AUDIO_MUX_MCR4	AUDIO_PACK(AUDIO_MUX | 0x04, 1)

#define	AUDIO_MAP	0x60
#define	AUDIO_MAP_X	AUDIO_PACK(AUDIO_MAP | 0x01, 16)
#define	AUDIO_MAP_R	AUDIO_PACK(AUDIO_MAP | 0x02, 16)
#define	AUDIO_MAP_GX	AUDIO_PACK(AUDIO_MAP | 0x03, 2)
#define	AUDIO_MAP_GR	AUDIO_PACK(AUDIO_MAP | 0x04, 2)
#define	AUDIO_MAP_GER	AUDIO_PACK(AUDIO_MAP | 0x05, 2)
#define	AUDIO_MAP_STG	AUDIO_PACK(AUDIO_MAP | 0x06, 2)
#define	AUDIO_MAP_FTGR	AUDIO_PACK(AUDIO_MAP | 0x07, 2)
#define	AUDIO_MAP_ATGR	AUDIO_PACK(AUDIO_MAP | 0x08, 2)
#define	AUDIO_MAP_MMR1	AUDIO_PACK(AUDIO_MAP | 0x09, 1)
#define	AUDIO_MAP_MMR2	AUDIO_PACK(AUDIO_MAP | 0x0a, 1)
#define	AUDIO_MAP_ALL	AUDIO_PACK(AUDIO_MAP | 0x0b, 46)

/*
 * These are minimum and maximum values for various registers, in
 * normal units, *not* as written to the register.  Most are in db,
 * but the FTGR is in Hz.  The FTGR values are derived from the
 * smallest and largest representable frequency and do not necessarily
 * reflect the true capabilities of the chip.
 */
#define	AUDIO_MAP_GX_MIN	0
#define	AUDIO_MAP_GX_MAX	12
#define	AUDIO_MAP_GR_MIN	-12
#define	AUDIO_MAP_GR_MAX	0
#define	AUDIO_MAP_GER_MIN	-10
#define	AUDIO_MAP_GER_MAX	18
#define	AUDIO_MAP_STG_MIN	-18
#define	AUDIO_MAP_STG_MAX	0
#define	AUDIO_MAP_FTGR_MIN	16
#define	AUDIO_MAP_FTGR_MAX	3999
#define	AUDIO_MAP_ATGR_MIN	-18
#define	AUDIO_MAP_ATGR_MAX	0

/*
 * These are the bit assignment in the INIT register.
 *
 * There are several ways to specify dividing the clock by 2.  Since
 * there appears to be no difference between them, only one way is defined.
*/
#define	AUDIO_INIT_BITS_IDLE		0x00
#define	AUDIO_INIT_BITS_ACTIVE		0x01
#define	AUDIO_INIT_BITS_NOMAP		0x20

#define	AUDIO_INIT_BITS_INT_ENABLED	0x00
#define	AUDIO_INIT_BITS_INT_DISABLED	0x04

#define	AUDIO_INIT_BITS_CLOCK_DIVIDE_2	0x00
#define	AUDIO_INIT_BITS_CLOCK_DIVIDE_1	0x08
#define	AUDIO_INIT_BITS_CLOCK_DIVIDE_4	0x10
#define	AUDIO_INIT_BITS_CLOCK_DIVIDE_3	0x20

#define	AUDIO_INIT_BITS_RECEIVE_ABORT	0x40
#define	AUDIO_INIT_BITS_TRANSMIT_ABORT	0x80

/*
 * The MUX (Multiplexor) connects inputs and outputs.  The MUX has
 * three registers.  Each register can specify two ports in the
 * high and low order nibbles.
 *
 * Here is an example.  To connect ports B1 and Ba, you
 * write the following word to a MUX register:
 * 	AUDIO_MUX_PORT_B1 | (AUDIO_MUX_PORT_BA << 4)
 * Connections are bidirectional, so it doesn't matter which port
 * is which nibble.
 */
#define	AUDIO_MUX_PORT_NONE		0x00
#define	AUDIO_MUX_PORT_B1		0x01	/* line interface unit */
#define	AUDIO_MUX_PORT_B2		0x02	/* line interface unit */
#define	AUDIO_MUX_PORT_BA		0x03	/* main audio processor */
#define	AUDIO_MUX_PORT_BB		0x04	/* microprocessor interface */
#define	AUDIO_MUX_PORT_BC		0x05	/* microprocessor interface */
#define	AUDIO_MUX_PORT_BD		0x06	/* sp channel 1 */
#define	AUDIO_MUX_PORT_BE		0x07	/* sp channel 2 */
#define	AUDIO_MUX_PORT_BF		0x08	/* sp channel 3 */

#define	AUDIO_MUX_MCR4_BITS_INT_ENABLE	0x08
#define	AUDIO_MUX_MCR4_BITS_INT_DISABLE	0x00
#define	AUDIO_MUX_MCR4_BITS_REVERSE_BB	0x10
#define	AUDIO_MUX_MCR4_BITS_REVERSE_BC	0x20

/*
 * These are the bit assignments for the mode registers MMR1 and MMR2.
 */
#define	AUDIO_MMR1_BITS_A_LAW		0x01
#define	AUDIO_MMR1_BITS_u_LAW		0x00
#define	AUDIO_MMR1_BITS_LOAD_GX		0x02
#define	AUDIO_MMR1_BITS_LOAD_GR		0x04
#define	AUDIO_MMR1_BITS_LOAD_GER	0x08
#define	AUDIO_MMR1_BITS_LOAD_X		0x10
#define	AUDIO_MMR1_BITS_LOAD_R		0x20
#define	AUDIO_MMR1_BITS_LOAD_STG	0x40
#define	AUDIO_MMR1_BITS_LOAD_DLB	0x80

#define	AUDIO_MMR2_BITS_AINA		0x00
#define	AUDIO_MMR2_BITS_AINB		0x01
#define	AUDIO_MMR2_BITS_EAR		0x00
#define	AUDIO_MMR2_BITS_LS		0x02
#define	AUDIO_MMR2_BITS_DTMF		0x04
#define	AUDIO_MMR2_BITS_TONE		0x08
#define	AUDIO_MMR2_BITS_RINGER		0x10
#define	AUDIO_MMR2_BITS_HIGH_PASS	0x20
#define	AUDIO_MMR2_BITS_AUTOZERO	0x40

#endif /* !_sbusdev_amd_h */
