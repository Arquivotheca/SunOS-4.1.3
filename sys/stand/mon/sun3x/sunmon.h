
/*	@(#)sunmon.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * sunmon.h
 *
 * Header file for Sun 68030 PROM Monitor
 */

/*
 * Size of the Pegasus Monitor PROM, in bytes
 */
#define PROMSIZE	0x20000		/* 128Kb available in two 27512's */
/*
 * Various memory layout parameters
 */
#define	INITSP		STACK_TOP	/* Initial stack pointer after reset */
#define	USERCODE	0x2000		/* starting address for user programs */

/*
 * STRTDATA is the starting address of monitor global data.
 */
#define	STRTDATA	MONSHORTPAGE

/*
 * Size of line input buffer
 */
#define	BUFSIZE		80

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
 * Frequency of the NMI timer used for keyboard scanning.
 *
 * NMIFREQ is how many times we will run the NMI routine per second.
 *
 * Since Sun-3 hardware provides 1/100th, 1/10th, and 1-second interrupts,
 * we have to run it at 1/100th second.
 */
#define	NMIFREQ		100

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
 * This character takes you out of transparent mode, from a serial terminal.
 */
#define	CENDTRANSP	'\036'		/* ^^ (ctrl ^) */

/*
 * These are the "erase" and "kill" characters for
 * input processing.
 */
#define	CERASE1		'\b'		/* backspace */
#define	CERASE2		0x7F		/* delete */
#define	CKILL1		'\025'		/* ^U */

#define	UPCASE		0x5F		/* mask to force upper case letters */
#define	NOPARITY	0x7F		/* mask to strip off parity */

