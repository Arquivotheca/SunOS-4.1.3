/*
 * sunmon.h
 *
 * @(#)sunmon.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Size of the Monitor prom, in bytes
 */
#define	PROMSIZE	0x40000		/* 64KB avail in one 27512 */

/*
 * Various memory layout parameters.  
 *
 * The "-(MINFRAME+REG_SAVE_AREA)" in the "INITSP" definition below provides 
 * space for the first window overflow and REG_SAVE_AREA.  "INITSP" is the
 * initial stack pointer after reset.
 */
#define	INITSP   ALIGN_DOWN(((int) (STACK_TOP-(MINFRAME+REG_SAVE_AREA))))

#define	USERCODE 0x2000 /* starting address for user programs */

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
#define ESCAPEKEY       0x1b            /* escape character */
#define	BACKSPACEKEY	'\b'		/* backspace */
#define	DELETEKEY	0x7F		/* delete */
#define	DELETELINE	0x15		/* ^u - delete line */
#define	DELETEWORD	0x17		/* ^w delete word */
#define RETURNKEY       0xd             /* return key */
#define NEWLINEKEY      '\n'            /* newline */
#define TABKEY          '	'       /* tab */
#define SPACEKEY        ' '             /* space */
#define	UPCASE		0x5F		/* mask to force upper case letters */
#define	NOPARITY	0x7F		/* mask to strip off parity */
