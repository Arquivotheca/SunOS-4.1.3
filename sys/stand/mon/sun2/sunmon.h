/*      @(#)sunmon.h 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX       */
/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * sunmon.h
 *
 * Header file for Sun MC68010 ROM Monitor
 */

/*
 * Size of the Monitor proms, in bytes
 */
#define	PROMSIZE	0x8000		/* 32KB total avail in 2 27128's */

/*
 * Various memory layout parameters
 */
#define	INITSP		0x1000		/* Initial stack pointer after reset */
#define	USERCODE	0x2000		/* starting address for user programs */

/*
 * STRTDATA is the starting address of monitor global data
 * On the VME system, the "User Interrupt Vectors" are used by hardware,
 * so we have to avoid all locations 0-0x400.  For compatability, the
 * Multibus version does the same; it helps the kernel.
 */
#define	STRTDATA	0x400

/*
 * Size of line input buffer
 */
#define	BUFSIZE		80

/*
 * Mode of the timer chip's NMI timer
 *
 * The timer runs in "Mode D" -- continuously, toggling its output
 * every time the counter runs out.  This would produce a square wave,
 * except that the NMI routine, when it runs, clears the timer's
 * output (the level 7 interrupt to the CPU), so what we get is a
 * series of pulses whose width is determined by the CPU response
 * time to level 7 interrupts.
 *
 * The time constant (divisor) is the input frequency (CLK_BASIC)
 * divided by NMIFREQ, which is how many times we want to
 * run the NMI routine per second, also divided by NMIDIVISOR, which
 * is what the hardware divisor in the timer chip is set to (/1, /16,
 * /256, etc).  Since we toggle the timer output back to off (by hand)
 * after each interrupt, this produces NMIFREQ interrupts per second.
 *
 * The frequency must be often enough to avoid losing characters on
 * the keyboard.  This means on parallel keyboards it must
 * be 500 or more (keyboard only holds value true ~2ms+).  On serial keyboards
 * it is the baud rate divided by the buffering provided by the UART.
 * If no keyboard is in use, it can be any value that will let us detect
 * BREAKs on the console serial port.
 */

#ifdef KEYBS2

#define	NMIMODE		(CLKM_DIV_BY_16 + CLKM_REPEAT + CLKM_TOGGLE)
#define	NMIDIVISOR	16
#define	NMIFREQ		40		/* Times per second */

#else  KEYBS2

#define	NMIMODE		(CLKM_DIV_BY_1 + CLKM_REPEAT + CLKM_TOGGLE)
#define	NMIDIVISOR	1
#define	NMIFREQ		500		/* Times per second */

#endif KEYBS2

/*
 * Size of up/down keyboard's typeahead buffer.  It is scanned
 * by the refresh routine, and keycodes deposited here.  Then later
 * they are picked up by the monitor busywait keyboard routine, or
 * by Unix or other application programs directly.
 *
 * Note that each keystroke, if typed slowly (by computer standards)
 * takes 3 bytes: a key-down, a key-up, and a keyboard-idle.  So allocate
 * three times as much room as you want to be able to type ahead.  (Of
 * course, this typeahead won't echo immediately in the monitor, but will
 * echo in its proper place in the I/O transcript.)
 */
#define	KEYBUFSIZE	90

/*
 * Size of the transparent I/O (terminal emulator) receive-ahead buffer.
 * It needs at least a line or so, for when it scrolls.
 *
 * HALTMAX is the how many chars are in the buffer when we send a ^S
 * to encourage the host to stop sending.  HALTMIN is how many are in
 * when we send a ^Q to encourage the host to resume.  (It should be
 * more than 0, because it takes the host awhile to receive the ^Q
 * and to restart its output.)  HALTCHAR is ^S.  HALTENDCHAR is ^Q.
 *
 * Note that the larger TRANSPBUFSIZE is, the more lines we can scroll up
 * at once as we get behind.  This may be undesirable to some people; it may
 * be very desirable to others, 'coz it makes it run mooch faster.
 *
 * When you change TRANSPBUFSIZE, be sure to change HALTMAX too.  It should
 * lag behind BufSIZE by 100 or so chars, depending on how fast the host
 * responds to ^S.  (100 chars gives .1 second leeway at 9600 baud).
 */
#define	TRANSPBUFSIZE	1000
#define	TRANSPHALTMAX	900
#define	TRANSPHALTMIN	25
#define	TRANSPHALTCHAR	0x13		/* ^S */
#define	TRANSPHALTENDCHAR	0x11	/* ^Q */

/*
 * These are the "erase" and "kill" characters for
 * input processing.
 */
#define	CERASE1		'\b'		/* backspace */
#define	CERASE2		0x7F		/* delete */
#define	CKILL1		'\025'		/* ^U */

/*
 * This character takes you out of transparent mode, from a serial terminal.
 */
#define	CENDTRANSP	'\036'		/* ^^ (ctrl ^) */


#define	UPCASE		0x5F		/* mask to force upper case letters */
#define	NOPARITY	0x7F		/* mask to strip off parity */
