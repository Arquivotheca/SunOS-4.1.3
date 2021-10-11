/*
 * @(#)cg6thc.h 1.4 89/03/30 SMI
 */

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#ifndef	pixrect_cg6thc_h
#define	pixrect_cg6thc_h

/*
 * TEC Hardware Configuration registers.
 *
 * Hardware register offsets from base address. These offsets are
 * intended to be added to a pointer-to-integer whose value is the
 * base address of the CG6 memory mapped register area.
 */

/* hardware configuration registers */

#define L_THC_HCHS		( 0x800 / sizeof(int) )
#define L_THC_HCHSDVB		( 0x804 / sizeof(int) )
#define L_THC_HCHD		( 0x808 / sizeof(int) )
#define L_THC_HCVS		( 0x80C / sizeof(int) )
#define L_THC_HCVD		( 0x810 / sizeof(int) )
#define L_THC_HCREFRESH		( 0x814 / sizeof(int) )
#define L_THC_HCMISC		( 0x818 / sizeof(int) )

#define	THC_HCMISC_REV_SHIFT		16
#define	THC_HCMISC_REV_MASK		15
#define THC_HCMISC_RESET		0x1000
#define	THC_HCMISC_VIDEO		0x0400
#define	THC_HCMISC_SYNC			0x0200
#define	THC_HCMISC_VSYNC		0x0100
#define	THC_HCMISC_SYNCEN		0x0080
#define	THC_HCMISC_CURSOR_RES		0x0040
#define	THC_HCMISC_INTEN		0x0020
#define	THC_HCMISC_INT			0x0010

#define	THC_HCMISC_INIT			0x009f

#define	thc_set_video(thc, on) \
	((thc)->l_thc_hcmisc = \
		(thc)->l_thc_hcmisc & ~THC_HCMISC_VIDEO | \
		((on) ? THC_HCMISC_VIDEO : 0))

#define	thc_get_video(thc) \
	((thc)->l_thc_hcmisc & THC_HCMISC_VIDEO)

#define thc_int_enable(thc) \
	((thc)->l_thc_hcmisc |= THC_HCMISC_INTEN)

#define thc_int_disable(thc) \
	((thc)->l_thc_hcmisc = \
		(thc)->l_thc_hcmisc & ~THC_HCMISC_INTEN | THC_HCMISC_INT)

#define	thc_int_pending(thc) \
	((thc)->l_thc_hcmisc & THC_HCMISC_INT)

/* cursor address register */
#define L_THC_ADDRESS		( 0x8FC / sizeof(int) )

/* cursor data registers, plane A */
#define L_THC_CURSORA00		( 0x900 / sizeof(int) )
#define L_THC_CURSORA01		( 0x904 / sizeof(int) )
#define L_THC_CURSORA02		( 0x908 / sizeof(int) )
#define L_THC_CURSORA03		( 0x90C / sizeof(int) )
#define L_THC_CURSORA04		( 0x910 / sizeof(int) )
#define L_THC_CURSORA05		( 0x914 / sizeof(int) )
#define L_THC_CURSORA06		( 0x918 / sizeof(int) )
#define L_THC_CURSORA07		( 0x91C / sizeof(int) )
#define L_THC_CURSORA08		( 0x920 / sizeof(int) )
#define L_THC_CURSORA09		( 0x924 / sizeof(int) )
#define L_THC_CURSORA10		( 0x928 / sizeof(int) )
#define L_THC_CURSORA11		( 0x92C / sizeof(int) )
#define L_THC_CURSORA12		( 0x930 / sizeof(int) )
#define L_THC_CURSORA13		( 0x934 / sizeof(int) )
#define L_THC_CURSORA14		( 0x938 / sizeof(int) )
#define L_THC_CURSORA15		( 0x93C / sizeof(int) )
#define L_THC_CURSORA16		( 0x940 / sizeof(int) )
#define L_THC_CURSORA17		( 0x944 / sizeof(int) )
#define L_THC_CURSORA18		( 0x948 / sizeof(int) )
#define L_THC_CURSORA19		( 0x94C / sizeof(int) )
#define L_THC_CURSORA20		( 0x950 / sizeof(int) )
#define L_THC_CURSORA21		( 0x954 / sizeof(int) )
#define L_THC_CURSORA22		( 0x958 / sizeof(int) )
#define L_THC_CURSORA23		( 0x95C / sizeof(int) )
#define L_THC_CURSORA24		( 0x960 / sizeof(int) )
#define L_THC_CURSORA25		( 0x964 / sizeof(int) )
#define L_THC_CURSORA26		( 0x968 / sizeof(int) )
#define L_THC_CURSORA27		( 0x96C / sizeof(int) )
#define L_THC_CURSORA28		( 0x970 / sizeof(int) )
#define L_THC_CURSORA29		( 0x974 / sizeof(int) )
#define L_THC_CURSORA30		( 0x978 / sizeof(int) )
#define L_THC_CURSORA31		( 0x97C / sizeof(int) )

/* cursor data registers, plane B */
#define L_THC_CURSORB00		( 0x980 / sizeof(int) )
#define L_THC_CURSORB01		( 0x984 / sizeof(int) )
#define L_THC_CURSORB02		( 0x988 / sizeof(int) )
#define L_THC_CURSORB03		( 0x98C / sizeof(int) )
#define L_THC_CURSORB04		( 0x990 / sizeof(int) )
#define L_THC_CURSORB05		( 0x994 / sizeof(int) )
#define L_THC_CURSORB06		( 0x998 / sizeof(int) )
#define L_THC_CURSORB07		( 0x99C / sizeof(int) )
#define L_THC_CURSORB08		( 0x9A0 / sizeof(int) )
#define L_THC_CURSORB09		( 0x9A4 / sizeof(int) )
#define L_THC_CURSORB10		( 0x9A8 / sizeof(int) )
#define L_THC_CURSORB11		( 0x9AC / sizeof(int) )
#define L_THC_CURSORB12		( 0x9B0 / sizeof(int) )
#define L_THC_CURSORB13		( 0x9B4 / sizeof(int) )
#define L_THC_CURSORB14		( 0x9B8 / sizeof(int) )
#define L_THC_CURSORB15		( 0x9BC / sizeof(int) )
#define L_THC_CURSORB16		( 0x9C0 / sizeof(int) )
#define L_THC_CURSORB17		( 0x9C4 / sizeof(int) )
#define L_THC_CURSORB18		( 0x9C8 / sizeof(int) )
#define L_THC_CURSORB19		( 0x9CC / sizeof(int) )
#define L_THC_CURSORB20		( 0x9D0 / sizeof(int) )
#define L_THC_CURSORB21		( 0x9D4 / sizeof(int) )
#define L_THC_CURSORB22		( 0x9D8 / sizeof(int) )
#define L_THC_CURSORB23		( 0x9DC / sizeof(int) )
#define L_THC_CURSORB24		( 0x9E0 / sizeof(int) )
#define L_THC_CURSORB25		( 0x9E4 / sizeof(int) )
#define L_THC_CURSORB26		( 0x9E8 / sizeof(int) )
#define L_THC_CURSORB27		( 0x9EC / sizeof(int) )
#define L_THC_CURSORB28		( 0x9F0 / sizeof(int) )
#define L_THC_CURSORB29		( 0x9F4 / sizeof(int) )
#define L_THC_CURSORB30		( 0x9F8 / sizeof(int) )
#define L_THC_CURSORB31		( 0x9FC / sizeof(int) )

/*
 * THC Cursor ADDRESS register bits.
 */

struct l_thc_cursor {
	unsigned	l_thc_cursor_x : 16;	/* X co-ordinate */
	unsigned	l_thc_cursor_y : 16;	/* Y co-ordinate */
};

/*
 * THC Video Timing registers bits.
 */

struct l_thc_hchs {
	unsigned	: 9;			/* not used */
	unsigned	l_thc_hchs_hss : 7;	/* hor. sync start */
	unsigned	: 9;			/* not used */
	unsigned	l_thc_hchs_hse : 7;	/* hor. sync end */
};

struct l_thc_hchsdvs {
	unsigned	: 9;			/* not used */
	unsigned	l_thc_hchsdvs_hss : 7;	/* hor. sync end DVS */
	unsigned	: 5;			/* not used */
	unsigned	l_thc_hchsdvs_hse : 11;	/* current vert. line */
};

struct l_thc_hchd {
	unsigned	: 9;			/* not used */
	unsigned	l_thc_hchd_hds : 7;	/* hor. display start */
	unsigned	: 9;			/* not used */
	unsigned	l_thc_hchd_hde : 7;	/* hor. display end */
};

struct l_thc_hcvs {
	unsigned	: 5;			/* not used */
	unsigned	l_thc_hcvs_vss : 11;	/* vert. sync start */
	unsigned	: 5;			/* not used */
	unsigned	l_thc_hcvs_hse : 11;	/* vert. sync end */
};

struct l_thc_hcvd {
	unsigned	: 5;			/* not used */
	unsigned	l_thc_hcvd_vds : 11;	/* vert. display start */
	unsigned	: 5;			/* not used */
	unsigned	l_thc_hcvd_hde : 11;	/* vert. display end */
};

struct l_thc_hcr {
	unsigned	: 21;			/* not used */
	unsigned	l_thc_hcr_clk : 11;	/* refresh counter */
};

/*
 * THC HCMISC register bits.
 */

typedef enum {
        L_THC_HCMISC_VID_BLANK, L_THC_HCMISC_VID_DISPLAY
} l_thc_hcmisc_vid_t;

typedef enum {
	L_THC_HCMISC_INTR_IGNORE, L_THC_HCMISC_INTR_CLEAR,
	L_THC_HCMISC_INTR_SET
} l_thc_hcmisc_intr_t;

struct l_thc_hcmisc {
   unsigned                                 : 12;      /* unused */
   unsigned             l_thc_hcmisc_rev    : 4;       /* chip revision */
   unsigned                                 : 3;       /* unused */
   unsigned             l_thc_hcmisc_reset  : 1;       /* reset */
   unsigned                                 : 1;       /* unused */
   l_thc_hcmisc_vid_t   l_thc_hcmisc_vid    : 1;       /* enable video */
   unsigned             l_thc_hcmisc_sync   : 1;       /* sync */
   unsigned             l_thc_hcmisc_vsync  : 1;       /* vsync */
   unsigned             l_thc_hcmisc_ensync : 1;       /* enable/disable sync */
   unsigned             l_thc_hcmisc_cures  : 1;       /* cursor resolution */
   l_thc_hcmisc_intr_t  l_thc_hcmisc_intr   : 2;       /* enable interrupt */
   unsigned             l_thc_hcmisc_cycles : 4;       /* cycles before xfer */
 };
     
/*
 * define THC registers as a structure.
 */

struct thc {
	u_int	l_thc_pad_0[512-0];
#ifdef tec_structures
	struct l_thc_hchs	l_thc_hchs;		/* 512 */
	struct l_thc_hchsdvs	l_thc_hchsdvs;		/* 513 */
	struct l_thc_hchd	l_thc_hchd;		/* 514 */
	struct l_thc_hcvs	l_thc_hcvs;		/* 515 */
	struct l_thc_hcvd	l_thc_hcvd;		/* 516 */
	struct l_thc_hcr	l_thc_hcr;		/* 517 */
	struct l_thc_hcmisc	l_thc_hcmisc;		/* 518 */
	u_int	l_thc_pad_519[575-519];
	struct l_thc_cursor	l_thc_cursor;		/* 575 */
#else
	u_int 	l_thc_hchs;				/* 512 */
	u_int 	l_thc_hchsdvs;				/* 513 */
	u_int 	l_thc_hchd;				/* 514 */
	u_int 	l_thc_hcvs;				/* 515 */
	u_int 	l_thc_hcvd;				/* 516 */
	u_int 	l_thc_hcr;				/* 517 */
	u_int 	l_thc_hcmisc;				/* 518 */
	u_int	l_thc_pad_519[575-519];
	u_int 	l_thc_cursor;				/* 575 */
#endif
	u_int	l_thc_cursora00;			/* 576 */
	u_int	l_thc_cursora01;			/* 577 */
	u_int	l_thc_cursora02;			/* 578 */
	u_int	l_thc_cursora03;			/* 579 */
	u_int	l_thc_cursora04;			/* 580 */
	u_int	l_thc_cursora05;			/* 581 */
	u_int	l_thc_cursora06;			/* 582 */
	u_int	l_thc_cursora07;			/* 583 */
	u_int	l_thc_cursora08;			/* 584 */
	u_int	l_thc_cursora09;			/* 585 */
	u_int	l_thc_cursora10;			/* 586 */
	u_int	l_thc_cursora11;			/* 587 */
	u_int	l_thc_cursora12;			/* 588 */
	u_int	l_thc_cursora13;			/* 589 */
	u_int	l_thc_cursora14;			/* 590 */
	u_int	l_thc_cursora15;			/* 591 */
	u_int	l_thc_cursora16;			/* 592 */
	u_int	l_thc_cursora17;			/* 593 */
	u_int	l_thc_cursora18;			/* 594 */
	u_int	l_thc_cursora19;			/* 595 */
	u_int	l_thc_cursora20;			/* 596 */
	u_int	l_thc_cursora21;			/* 597 */
	u_int	l_thc_cursora22;			/* 598 */
	u_int	l_thc_cursora23;			/* 599 */
	u_int	l_thc_cursora24;			/* 600 */
	u_int	l_thc_cursora25;			/* 601 */
	u_int	l_thc_cursora26;			/* 602 */
	u_int	l_thc_cursora27;			/* 603 */
	u_int	l_thc_cursora28;			/* 604 */
	u_int	l_thc_cursora29;			/* 605 */
	u_int	l_thc_cursora30;			/* 606 */
	u_int	l_thc_cursora31;			/* 607 */
	u_int	l_thc_cursorb00;			/* 608 */
	u_int	l_thc_cursorb01;			/* 609 */
	u_int	l_thc_cursorb02;			/* 610 */
	u_int	l_thc_cursorb03;			/* 611 */
	u_int	l_thc_cursorb04;			/* 612 */
	u_int	l_thc_cursorb05;			/* 613 */
	u_int	l_thc_cursorb06;			/* 614 */
	u_int	l_thc_cursorb07;			/* 615 */
	u_int	l_thc_cursorb08;			/* 616 */
	u_int	l_thc_cursorb09;			/* 617 */
	u_int	l_thc_cursorb10;			/* 618 */
	u_int	l_thc_cursorb11;			/* 619 */
	u_int	l_thc_cursorb12;			/* 620 */
	u_int	l_thc_cursorb13;			/* 621 */
	u_int	l_thc_cursorb14;			/* 622 */
	u_int	l_thc_cursorb15;			/* 623 */
	u_int	l_thc_cursorb16;			/* 624 */
	u_int	l_thc_cursorb17;			/* 625 */
	u_int	l_thc_cursorb18;			/* 626 */
	u_int	l_thc_cursorb19;			/* 627 */
	u_int	l_thc_cursorb20;			/* 628 */
	u_int	l_thc_cursorb21;			/* 629 */
	u_int	l_thc_cursorb22;			/* 630 */
	u_int	l_thc_cursorb23;			/* 631 */
	u_int	l_thc_cursorb24;			/* 632 */
	u_int	l_thc_cursorb25;			/* 633 */
	u_int	l_thc_cursorb26;			/* 634 */
	u_int	l_thc_cursorb27;			/* 635 */
	u_int	l_thc_cursorb28;			/* 636 */
	u_int	l_thc_cursorb29;			/* 637 */
	u_int	l_thc_cursorb30;			/* 638 */
	u_int	l_thc_cursorb31;			/* 639 */
};

#endif	pixrect_cg6thc_h
