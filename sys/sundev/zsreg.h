/*	@(#)zsreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983, 1989 by Sun Microsystems, Inc.
 */

/*
 * Zilog 8530 SCC Serial Communications Controller
 *
 * This is a dual uart chip with on-chip baud rate generators.
 * It is about as brain-damaged as the typical modern uart chip,
 * but it does have a lot of features as well as the usual lot of
 * brain damage around addressing, write-onlyness, etc.
 */

#ifndef _sundev_zsreg_h
#define	_sundev_zsreg_h

/*
 * Specify the SPARC interrupt level that will be used by the
 * device driver to do its low level processing.
 * This has a range of 1..15, where 1 is low and 15 is high.
 * Boxes with only eight interrupt levels should divide this
 * by two before establishing the interrupt.
 */
#define	ZS_SOFT_INT	6

/*
 * Uart registers:
 *
 * There are 16 write registers and 9 read registers in each channel.
 * As usual, the two channels are ALMOST orthogonal, not exactly.  Most regs
 * can only be written to, or read, but not both.  To access one, you must
 * first write to register 0 with the number of the register you
 * are interested in, then read/write the actual value, and hope that
 * nobody interrupts you in between.
 *
 * Note that the register&bit assignment is suspiciously like the Intel 8274.
 * Do you think they read each others' data sheets?  Can they decode them?
 */

/* bits in RR0 */
#define	ZSRR0_RX_READY		0x01	/* received character available */
#define	ZSRR0_TIMER		0x02	/* if R15_TIMER, timer reached 0 */
#define	ZSRR0_TX_READY		0x04	/* transmit buffer empty */
#define	ZSRR0_CD		0x08	/* CD input (latched if R15_CD) */
#define	ZSRR0_SYNC		0x10	/* SYNC input (latched if R15_SYNC) */
#define	ZSRR0_CTS		0x20	/* CTS input (latched if R15_CTS) */
#define	ZSRR0_TXUNDER		0x40	/* (SYNC) Xmitter underran */
#define	ZSRR0_BREAK		0x80	/* received break detected */

/* bits in RR1 */
#define	ZSRR1_ALL_SENT		0x01	/* all chars fully transmitted */
#define	ZSRR1_PE		0x10	/* parity error (latched, must reset) */
#define	ZSRR1_DO		0x20	/* data overrun (latched, must reset) */
#define	ZSRR1_FE		0x40	/* framing/CRC error (not latched) */
#define	ZSRR1_RXEOF		0x80	/* end of recv sdlc frame */

/*
 * bits in R/WR2 -- interrupt vector number.
 *
 * NOTE that RR2 in channel A is unmodified, while in channel B it is
 * modified by the current status of the UARTs.  (This is independent
 * of the setting of WR9_VIS.)  If no interrupts are pending, the modified
 * status is Channel B Special Receive.  It can be written from
 * either channel.
 */

/*
 * bits in RR3 -- Interrupt Pending flags for both channels (this reg can
 * only be read in Channel A, tho.  Thanks guys.)
 */
#define	ZSRR3_IP_B_STAT		0x01	/* Ext/status int pending, chan B */
#define	ZSRR3_IP_B_TX		0x02	/* Transmit int pending, chan B */
#define	ZSRR3_IP_B_RX		0x04	/* Receive int pending, chan B */
#define	ZSRR3_IP_A_STAT		0x08	/* Ditto for channel A */
#define	ZSRR3_IP_A_TX		0x10
#define	ZSRR3_IP_A_RX		0x20

/* bits in RR8 -- this is the same as reading the Data port */

/* bits in RR10 -- DPLL and SDLC Loop Mode status -- not entered */

/* bits in R/WR12 -- lower byte of time constant for baud rate generator */
/*
 * The following macro can be used to generate the baud rate generator's
 * time constants.  The parameters are the input clock to the BRG (eg,
 * 5000000 for 5MHz) and the desired baud rate.  This macro assumes that
 * the clock needed is 16x the desired baud rate.
 */
#define	ZSTimeConst(InputClock, BaudRate) \
	(short)((((long)InputClock+BaudRate*16) / (2*(long)BaudRate*16)) - 2)

/* bits in R/WR13 -- upper byte of time constant for baud rate generator */

/* bits in R/WR15 -- interrupt enables for status conditions */
#define	ZSR15_TIMER		0x02	/* ie if baud rate generator = 0 */
#define	ZSR15_CD		0x08	/* ie transition on CD (car. det.) */
#define	ZSR15_SYNC		0x10	/* ie transition on SYNC (gen purp) */
#define	ZSR15_CTS		0x20	/* ie transition on CTS (clr to send) */
#define	ZSR15_TX_UNDER		0x40	/* (SYNC) ie transmit underrun */
#define	ZSR15_BREAK		0x80	/* ie on start, and end, of break */

/* Write register 0 -- common commands and Register Pointers */
#define	ZSWR0_REG		0x0F	/* mask: next reg to read/write */
#define	ZSWR0_RESET_STATUS	0x10	/* reset status bit latches */
#define	ZSWR0_SEND_ABORT	0x18	/* SDLC: send abort */
#define	ZSWR0_FIRST_RX		0x20	/* in WR1_RX_FIRST_IE, enab next int */
#define	ZSWR0_RESET_TXINT	0x28	/* reset transmitter interrupt */
#define	ZSWR0_RESET_ERRORS	0x30	/* reset read character errors */
#define	ZSWR0_CLR_INTR		0x38	/* Reset Interrupt In Service */
#define	ZSWR0_RESET_RXCRC	0x40	/* Reset Rx CRC generator */
#define	ZSWR0_RESET_TXCRC	0x80	/* Reset Tx CRC generator */
#define	ZSWR0_RESET_EOM		0xC0	/* Reset Tx underrun / EOM */

/* bits in WR1 */
#define	ZSWR1_SIE		0x01	/* status change master int enable */
					/* Also see R15 for individual enabs */
#define	ZSWR1_TIE		0x02	/* transmitter interrupt enable */
#define	ZSWR1_PARITY_SPECIAL	0x04	/* parity err causes special rx int */
#define	ZSWR1_RIE_FIRST_SPECIAL	0x08	/* r.i.e. on 1st char of msg */
#define	ZSWR1_RIE		0x10	/* receiver interrupt enable */
#define	ZSWR1_RIE_SPECIAL_ONLY	0x18	/* rie on special only */
#define	ZSWR1_REQ_IS_RX		0x20	/* REQ pin is for receiver */
#define	ZSWR1_REQ_NOT_WAIT	0x40	/* REQ/WAIT pin is REQ */
#define	ZSWR1_REQ_ENABLE	0x80	/* enable REQ/WAIT */
/* There are other Receive interrupt options defined, see data sheet. */

/* bits in WR2 are defined above as R/WR2. */

/* bits in WR3 */
#define	ZSWR3_RX_ENABLE		0x01	/* receiver enable */
#define	ZSWR3_RXCRC_ENABLE	0x08	/* receiver CRC enable */
#define	ZSWR3_HUNT		0x10	/* enter hunt mode */
#define	ZSWR3_AUTO_CD_CTS	0x20	/* auto-enable CD&CTS rcv&xmit ctl */
#define	ZSWR3_RX_5		0x00	/* receive 5-bit characters */
#define	ZSWR3_RX_6		0x80	/* receive 6 bit characters */
#define	ZSWR3_RX_7		0x40	/* receive 7 bit characters */
#define	ZSWR3_RX_8		0xC0	/* receive 8 bit characters */

/* bits in WR4 */
#define	ZSWR4_PARITY_ENABLE	0x01	/* Gen/check parity bit */
#define	ZSWR4_PARITY_EVEN	0x02	/* Gen/check even parity */
#define	ZSWR4_1_STOP		0x04	/* 1 stop bit */
#define	ZSWR4_1_5_STOP		0x08	/* 1.5 stop bits */
#define	ZSWR4_2_STOP		0x0C	/* 2 stop bits */
#define	ZSWR4_BISYNC		0x10	/* Bisync mode */
#define	ZSWR4_SDLC		0x20	/* SDLC mode */
#define	ZSWR4_X1_CLK		0x00	/* clock is 1x */
#define	ZSWR4_X16_CLK		0x40	/* clock is 16x */
#define	ZSWR4_X32_CLK		0x80	/* clock is 32x */
#define	ZSWR4_X64_CLK		0xC0	/* clock is 64x */

/* bits in WR5 */
#define	ZSWR5_TXCRC_ENABLE	0x01	/* transmitter CRC enable */
#define	ZSWR5_RTS		0x02	/* RTS output */
#define	ZSWR5_CRC16		0x04	/* Use CRC-16 for checksum */
#define	ZSWR5_TX_ENABLE		0x08	/* transmitter enable */
#define	ZSWR5_BREAK		0x10	/* send break continuously */
#define	ZSWR5_TX_5		0x00	/* transmit 5 bit chars or less */
#define	ZSWR5_TX_6		0x40	/* transmit 6 bit characters */
#define	ZSWR5_TX_7		0x20	/* transmit 7 bit characters */
#define	ZSWR5_TX_8		0x60	/* transmit 8 bit characters */
#define	ZSWR5_DTR		0x80	/* DTR output */

/* bits in WR6 -- Sync characters or SDLC address field. */

/* bits in WR7 -- Sync character or SDLC flag */

/* bits in WR8 -- transmit buffer.  Same as writing to data port. */

/*
 * bits in WR9 -- Master interrupt control and reset.  Accessible thru
 * either channel, there's only one of them.
 */
#define	ZSWR9_VECTOR_INCL_STAT	0x01	/* Include status bits in int vector */
#define	ZSWR9_NO_VECTOR		0x02	/* Do not respond to int ack cycles */
#define	ZSWR9_DIS_LOWER_CHAIN	0x04	/* Disable ints lower in daisy chain */
#define	ZSWR9_MASTER_IE		0x08	/* Master interrupt enable */
#define	ZSWR9_STAT_HIGH		0x10	/* Modify ivec bits 6-4, not 1-3 */
#define	ZSWR9_RESET_CHAN_B	0x40	/* Reset just channel B */
#define	ZSWR9_RESET_CHAN_A	0x80	/* Reset just channel A */
#define	ZSWR9_RESET_WORLD	0xC0	/* Force hardware reset */

/* bits in WR10 -- SDLC, NRZI, FM control bits */
#define	ZSWR10_UNDERRUN_ABORT	0x04	/* send abort on TX underrun */
#define	ZSWR10_NRZI		0x20	/* NRZI mode (SDLC) */
#define	ZSWR10_PRESET_ONES	0x80	/* preset CRC to ones (SDLC) */

/* bits in WR11 -- clock mode control */
#define	ZSWR11_TRXC_XTAL	0x00	/* TRxC output = xtal osc */
#define	ZSWR11_TRXC_XMIT	0x01	/* TRxC output = xmitter clk */
#define	ZSWR11_TRXC_BAUD	0x02	/* TRxC output = baud rate gen */
#define	ZSWR11_TRXC_DPLL	0x03	/* TRxC output = Phase Locked Loop */
#define	ZSWR11_TRXC_OUT_ENA	0x04	/* TRxC output enable (unless input) */
#define	ZSWR11_TXCLK_RTXC	0x00	/* Tx clock is RTxC pin */
#define	ZSWR11_TXCLK_TRXC	0x08	/* Tx clock is TRxC pin */
#define	ZSWR11_TXCLK_BAUD	0x10	/* Tx clock is baud rate gen output */
#define	ZSWR11_TXCLK_DPLL	0x18	/* Tx clock is Phase Locked Loop o/p */
#define	ZSWR11_RXCLK_RTXC	0x00	/* Rx clock is RTxC pin */
#define	ZSWR11_RXCLK_TRXC	0x20	/* Rx clock is TRxC pin */
#define	ZSWR11_RXCLK_BAUD	0x40	/* Rx clock is baud rate gen output */
#define	ZSWR11_RXCLK_DPLL	0x60	/* Rx clock is Phase Locked Loop o/p */
#define	ZSWR11_RTXC_XTAL	0x80	/* RTxC uses crystal, not TTL signal */

/* bits in WR12 -- described above as R/WR12 */

/* bits in WR13 -- described above as R/WR13 */

/* bits in WR14 -- misc control bits, and DPLL control */
#define	ZSWR14_BAUD_ENA		0x01	/* enables baud rate counter */
#define	ZSWR14_BAUD_FROM_PCLK	0x02	/* Baud rate gen src = PCLK not RTxC */
#define	ZSWR14_DTR_IS_REQUEST	0x04	/* Changes DTR line to DMA Request */
#define	ZSWR14_AUTO_ECHO	0x08	/* Echoes RXD to TXD */
#define	ZSWR14_LOCAL_LOOPBACK	0x10	/* Echoes TX to RX in chip */
#define	ZSWR14_DPLL_NOP		0x00	/* These 8 commands are mut. exclu. */
#define	ZSWR14_DPLL_SEARCH	0x20	/* Enter search mode in DPLL */
#define	ZSWR14_DPLL_RESET	0x40	/* Reset missing clock in DPLL */
#define	ZSWR14_DPLL_DISABLE	0x60	/* Disable DPLL */
#define	ZSWR14_DPLL_SRC_BAUD	0x80	/* Source for DPLL is baud rate gen */
#define	ZSWR14_DPLL_SRC_RTXC	0xA0	/* Source for DPLL is RTxC pin */
#define	ZSWR14_DPLL_FM		0xC0	/* DPLL should run in FM mode */
#define	ZSWR14_DPLL_NRZI	0xE0	/* DPLL should run in NRZI mode */

/* bits in WR15 -- described above as R/WR15 */

/*
 * UART register addressing
 *
 * It would be nice if they used 4 address pins to address 15 registers,
 * but they only used 1.  So you have to write to the control port then
 * read or write it; the 2nd cycle is done to whatever register number
 * you wrote in the first cycle.
 *
 * The data register can also be accessed as Read/Write register 8.
 */
#ifndef LOCORE
#ifndef sun386
struct zscc_device {
	unsigned char	zscc_control;
	unsigned char	:8;		/* Filler */
	unsigned char	zscc_data;
	unsigned char	:8;		/* Filler */
};
#else sun386
#ifdef SUN386
struct zscc_device {
	unsigned int	zscc_control;
	unsigned int	zscc_data;
};
#endif SUN386
#ifdef AT386
struct zscc_device {
	unsigned char	zscc_control;
	unsigned char	zscc_data;
	unsigned char	:8;		/* Filler */
	unsigned char	:8;		/* Filler */
};
#endif AT386
#endif !sun386

#define	ZS_RINGBITS	8		/* # of bits in ring ptrs */
#define	ZS_RINGSIZE	(1<<ZS_RINGBITS)	/* size of ring */
#define	ZS_RINGMASK	(ZS_RINGSIZE-1)
#define	ZS_RINGFRAC	2		/* fraction of ring to force flush */

#define	ZS_RING_INIT(zap)	((zap)->za_rput = (zap)->za_rget = 0)
#define	ZS_RING_CNT(zap)	(((zap)->za_rput - (zap)->za_rget) & ZS_RINGMASK)
#define	ZS_RING_FRAC(zap)	(ZS_RING_CNT(zap) >= (ZS_RINGSIZE/ZS_RINGFRAC))
#define	ZS_RING_POK(zap, n) 	(ZS_RING_CNT(zap) < (ZS_RINGSIZE-(n)))
#define	ZS_RING_PUT(zap, c) \
	((zap)->za_ring[(zap)->za_rput++ & ZS_RINGMASK] =  (u_char)(c))
#define	ZS_RING_UNPUT(zap)	((zap)->za_rput--)
#define	ZS_RING_GOK(zap, n) 	(ZS_RING_CNT(zap) >= (n))
#define	ZS_RING_GET(zap)	((zap)->za_ring[(zap)->za_rget++ & ZS_RINGMASK])
#define	ZS_RING_EAT(zap, n)	 ((zap)->za_rget += (n))

/*
 * These flags are shared with mcp_async.c and should be kept in sync.
 */
#define	ZAS_WOPEN	0x00000001	/* waiting for open to complete */
#define	ZAS_ISOPEN	0x00000002	/* open is complete */
#define	ZAS_OUT		0x00000004	/* line being used for dialout */
#define	ZAS_CARR_ON	0x00000008	/* carrier on last time we looked */
#define	ZAS_STOPPED	0x00000010	/* output is stopped */
#define	ZAS_DELAY	0x00000020	/* waiting for delay to finish */
#define	ZAS_BREAK	0x00000040	/* waiting for break to finish */
#define	ZAS_BUSY	0x00000080	/* waiting for transmission to finish */
#define	ZAS_DRAINING	0x00000100	/*
					 * waiting for output to drain
					 * from chip
					 */
#define	ZAS_SERVICEIMM	0x00000200	/*
					 * queue soft interrupt as soon as
					 * receiver interrupt occurs
					 */
#define	ZAS_SOFTC_ATTN	0x00000400	/* check soft carrier state in close */
#define	ZAS_PAUSED	0x00000800	/* MCP: dma interrupted and pending */
#define	ZAS_LNEXT	0x00001000	/* MCP: next input char is quoted */

struct zsaline {
	int	za_flags;		/* random flags */
	dev_t	za_dev;			/* major/minor for this device */
	mblk_t	*za_xmitblk;		/* block being transmitted from */
	struct zscom *za_common;	/* address of zs common data struct */
	tty_common_t za_ttycommon;	/* data common to all tty drivers */
	int	za_wbufcid;		/* id of pending write-side bufcall */
	time_t	za_dtrlow;		/* time dtr went low */
	short	za_needsoft;		/* need for software interrupt */
	short	za_break;		/* break occurred */
	union {
		struct {
			u_char  _hw;	/* overrun (hw) */
			u_char  _sw;	/* overrun (sw) */
		} _z;
		u_short uover_overrun;
	} za_uover;
#define	za_overrun	za_uover.uover_overrun
#define	za_hw_overrun   za_uover._z._hw
#define	za_sw_overrun   za_uover._z._sw
	short   za_ext;			/* modem status change */
	short   za_work;		/* work to do */
	u_char	za_rput;		/* producing ptr for input */
	u_char	za_rget;		/* consuming ptr for input */
	u_char  *za_optr;		/* output ptr */
	short   za_ocnt;		/* output count */
	u_char  za_flowc;		/* startc or stopc to transmit */
	u_char  za_rr0;			/* for break detection */
	u_char  za_ring[ZS_RINGSIZE];	/* circular input buffer */
};

#endif /* !LOCORE */

#endif /*!_sundev_zsreg_h*/
