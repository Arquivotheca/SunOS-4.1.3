/*
 *	@(#)audioreg.h 1.1 92/07/30 SMI
 *	Copyright 1989 (c) Sun Microsystems, Inc.
 */

/*	These defines facilitate access to the indirect registers.  One
	of these values is written to the command register, then the
	appropriate number of bytes are written/read to/from the data
	register.

	The indirection values are formed by od'ing together two quantities.
	The first three bits of the indirection value specifies the
	chip subsystem (audio processor, multiplexor, etc.) to be accessed.
	AMP calles these three bits the DCF, or destination code field.
	The last five bits specify the register within the subsystem.  AMD
	calls these bits the OCF, or Operation Code Field.

	Note that the INIT_INIT value differs from the data sheet.  The
	data sheet is wrong -- this is right.
*/

#define AUDIO_PACK(reg,length)	(((reg) << 8) | (length))
#define AUDIO_UNPACK_REG(x)	(((x) >> 8) & 0xff)
#define AUDIO_UNPACK_LENGTH(x)	((x) & 0xff)

#define AUDIO_INIT	0x20
#define AUDIO_INIT_INIT	AUDIO_PACK((AUDIO_INIT | 0x01),1)

#define AUDIO_MUX	0x40
#define AUDIO_MUX_MCR1	AUDIO_PACK(AUDIO_MUX | 0x01,1)
#define AUDIO_MUX_MCR2	AUDIO_PACK(AUDIO_MUX | 0x02,1)
#define AUDIO_MUX_MCR3	AUDIO_PACK(AUDIO_MUX | 0x03,1)
#define AUDIO_MUX_MCR4	AUDIO_PACK(AUDIO_MUX | 0x04,1)

#define AUDIO_MAP	0x60
#define AUDIO_MAP_X	AUDIO_PACK(AUDIO_MAP | 0x01,16)
#define AUDIO_MAP_R	AUDIO_PACK(AUDIO_MAP | 0x02,16)
#define AUDIO_MAP_GX	AUDIO_PACK(AUDIO_MAP | 0x03,2)
#define AUDIO_MAP_GR	AUDIO_PACK(AUDIO_MAP | 0x04,2)
#define AUDIO_MAP_GER	AUDIO_PACK(AUDIO_MAP | 0x05,2)
#define AUDIO_MAP_STG	AUDIO_PACK(AUDIO_MAP | 0x06,2)
#define AUDIO_MAP_FTGR	AUDIO_PACK(AUDIO_MAP | 0x07,2)
#define AUDIO_MAP_ATGR	AUDIO_PACK(AUDIO_MAP | 0x08,2)
#define AUDIO_MAP_MMR1	AUDIO_PACK(AUDIO_MAP | 0x09,1)
#define AUDIO_MAP_MMR2	AUDIO_PACK(AUDIO_MAP | 0x0a,1)
#define AUDIO_MAP_ALL	AUDIO_PACK(AUDIO_MAP | 0x0b,46)

/*  These are minimum and maximum values for various registers, in
 *  normal units, *not* as written to the register.  Most are in db,
 *  but the FTGR is in Hz.  The FTGR values are derived from the
 *  smallest and largest representable frequency and do not necessarily
 *  reflect the true capabilities of the chip.
 */
#define AUDIO_MAP_GX_MIN	0
#define AUDIO_MAP_GX_MAX	12
#define AUDIO_MAP_GR_MIN	-12
#define AUDIO_MAP_GR_MAX	0
#define AUDIO_MAP_GER_MIN	-10
#define AUDIO_MAP_GER_MAX	18
#define AUDIO_MAP_STG_MIN	-18
#define AUDIO_MAP_STG_MAX	0
#define AUDIO_MAP_FTGR_MIN	16
#define AUDIO_MAP_FTGR_MAX	3999
#define AUDIO_MAP_ATGR_MIN	-18
#define AUDIO_MAP_ATGR_MAX	0

/*	These are the bit assignment in the INIT register.

	There are several ways to specify dividing the clock by 2.  Since
	there appears to be no difference between them, I only define one
	way to set it.
*/

#define AUDIO_INIT_BITS_IDLE		0x00
#define AUDIO_INIT_BITS_ACTIVE		0x01
#define AUDIO_INIT_BITS_NOMAP		0x20

#define AUDIO_INIT_BITS_INT_ENABLED	0x00
#define AUDIO_INIT_BITS_INT_DISABLED	0x04

#define AUDIO_INIT_BITS_CLOCK_DIVIDE_2	0x00
#define AUDIO_INIT_BITS_CLOCK_DIVIDE_1	0x08
#define AUDIO_INIT_BITS_CLOCK_DIVIDE_4	0x10
#define AUDIO_INIT_BITS_CLOCK_DIVIDE_3	0x20

#define AUDIO_INIT_BITS_RECEIVE_ABORT	0x40
#define AUDIO_INIT_BITS_TRANSMIT_ABORT	0x80

/*	The MUX (Multiplexor) connects inputs and outputs.  The MUX has
	three registers.  Each register can specify two ports in the
	high and low order nibbles.

	Here is an example.  To connect ports B1 and Ba, you
	write the following word to a MUX register:
		AUDIO_MUX_PORT_B1 | (AUDIO_MUX_PORT_BA << 4)
	Connections are bidirectional, so it doesn't matter which port
	is which nibble.
*/

#define AUDIO_MUX_PORT_NONE		0x00
#define AUDIO_MUX_PORT_B1		0x01	/* line interface unit */
#define AUDIO_MUX_PORT_B2		0x02	/* line interface unit */
#define AUDIO_MUX_PORT_BA		0x03	/* main audio processor */
#define AUDIO_MUX_PORT_BB		0x04	/* microprocessor interface */
#define AUDIO_MUX_PORT_BC		0x05	/* microprocessor interface */
#define AUDIO_MUX_PORT_BD		0x06	/* sp channel 1 */
#define AUDIO_MUX_PORT_BE		0x07	/* sp channel 2 */
#define AUDIO_MUX_PORT_BF		0x08	/* sp channel 3 */

#define AUDIO_MUX_MCR4_BITS_INT_ENABLE	0x08
#define AUDIO_MUX_MCR4_BITS_INT_DISABLE	0x00
#define AUDIO_MUX_MCR4_BITS_REVERSE_BB	0x10
#define AUDIO_MUX_MCR4_BITS_REVERSE_BC	0x20

/*	These are the bit assignments for the mode registers MMR1 and MMR2.
*/

#define AUDIO_MMR1_BITS_A_LAW		0x01
#define AUDIO_MMR1_BITS_u_LAW		0x00
#define AUDIO_MMR1_BITS_LOAD_GX		0x02
#define AUDIO_MMR1_BITS_LOAD_GR		0x04
#define AUDIO_MMR1_BITS_LOAD_GER	0x08
#define AUDIO_MMR1_BITS_LOAD_X		0x10
#define AUDIO_MMR1_BITS_LOAD_R		0x20
#define AUDIO_MMR1_BITS_LOAD_STG	0x40
#define AUDIO_MMR1_BITS_LOAD_DLB	0x80

#define AUDIO_MMR2_BITS_AINA		0x00
#define AUDIO_MMR2_BITS_AINB		0x01
#define AUDIO_MMR2_BITS_EAR		0x00
#define AUDIO_MMR2_BITS_LS		0x02
#define AUDIO_MMR2_BITS_DTMF		0x04
#define AUDIO_MMR2_BITS_TONE		0x08
#define AUDIO_MMR2_BITS_RINGER		0x10
#define AUDIO_MMR2_BITS_HIGH_PASS	0x20
#define AUDIO_MMR2_BITS_AUTOZERO	0x40

/*	These are the registers for the chip.

	These names are not very descriptive, but they match the names
	chosen by AMD.
*/

struct	audio_chip {
	char	cr;
	char	dr;
	char	dsr1;
	char	der;
	char	dctb;
	char	bbtb;
	char	bctb;
	char	dsr2;
};

#define ir	cr
#define	bbrb	bbtb
#define bcrb	bctb
