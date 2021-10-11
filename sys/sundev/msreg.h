/*	@(#)msreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Software mouse registers
 */

#ifndef _sundev_msreg_h
#define _sundev_msreg_h

/*
 * Mouse sample.
 */
struct	mouseinfo {
	char	mi_x;		/* current X coordinate */
	char	mi_y;		/* current Y coordinate */
	char	mi_buttons;	/* set of buttons that are currently down */
#define	MS_HW_BUT1	0x4	/* left button position */
#define	MS_HW_BUT2	0x2	/* middle button position */
#define	MS_HW_BUT3	0x1	/* right button position */
	struct	timeval mi_time; /* timestamp */
};

/*
 * Circular buffer storing mouse events.
 */
struct	mousebuf {
	short	mb_size;	/* size (in mouseinfo units) of buf */
	short	mb_off;		/* current offset in buffer */
	struct	mouseinfo mb_info[1];	/* however many samples */
};

struct	ms_softc {
	struct	mousebuf *ms_buf;	/* pointer to mouse buffer */
	short	ms_bufbytes;		/* buffer size (in bytes) */
	short	ms_flags;		/* currently unused */
	short	ms_oldoff;		/* index into mousebuf */
	short	ms_eventstate;		/* current event being generated */
	short	ms_readformat;		/* format of read stream */
#define	MS_3BYTE_FORMAT	VUID_NATIVE	/* 3 byte format (buts/x/y) */
#define	MS_VUID_FORMAT	VUID_FIRM_EVENT	/* vuid Firm_event format */
	short	ms_vuidaddr;		/* vuid addr for MS_VUID_FORMAT */
	char	ms_prevbuttons;		/* button state as of last message sent upstream */
};

#define	EVENT_X		0	/* generating delta-X event */
#define	EVENT_Y		1	/* generating delta-Y event */
#define	EVENT_BUT1	2	/* generating button 1 event */
#define	EVENT_BUT2	3	/* generating button 2 event */
#define	EVENT_BUT3	4	/* generating button 3 event */

#ifdef KERNEL
#define	MSIOGETBUF	_IOWR(m, 1, int)/* MSIOGETBUF is OBSOLETE */
	/* Get mouse buffer ptr so (window system in particular) can chase
	   around buffer to get events. */
#endif

#endif /*!_sundev_msreg_h*/
