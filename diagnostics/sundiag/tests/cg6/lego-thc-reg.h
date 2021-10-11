
/*
 * TEC Hardware Configuration registers.
 *
 * Hardware register offsets from base address. These offsets are
 * intended to be added to a pointer-to-integer whose value is the
 * base address of the Lego memory mapped register area.
 */

/* hardware configuration registers */

#ifndef lint
static char	sccsid_lego_thc_reg_h[] = { "@(#)lego-thc-reg.h 3.2 88/09/22" };
#endif lint

#define L_THC_HCHS		( 0x800 / sizeof(int) )
#define L_THC_HCHSDVB		( 0x804 / sizeof(int) )
#define L_THC_HCHD		( 0x808 / sizeof(int) )
#define L_THC_HCVS		( 0x80C / sizeof(int) )
#define L_THC_HCVD		( 0x810 / sizeof(int) )
#define L_THC_HCREFRESH		( 0x814 / sizeof(int) )
#define L_THC_HCMISC		( 0x818 / sizeof(int) )

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
	L_THC_HCMISC_VID_IGNORE, L_THC_HCMISC_VID_BLANK,
	L_THC_HCMISC_VID_DISPLAY
	} l_thc_hcmisc_vid_t;

typedef enum {
	L_THC_HCMISC_INTR_IGNORE, L_THC_HCMISC_INTR_CLEAR,
	L_THC_HCMISC_INTRI_SET
	} l_thc_hcmisc_intr_t;

struct l_thc_hcmisc {
	unsigned	l_thc_hcmisc_rev : 4;		/* chip revision */
	unsigned	: 20;				/* not used */
	unsigned	l_thc_hcmisc_res : 1;		/* reset */
	l_thc_hcmisc_vid_t	l_thc_hcmisc_vid : 2;	/* enable video */
	l_thc_hcmisc_intr_t	l_thc_hcmisc_intr : 2;	/* enable interrupt */
	unsigned	l_thc_hcmisc_cycles : 4;	/* cycles before xfer */
	};

/*
 * define THC registers as a structure.
 */

struct l_thc {
	int	l_thc_pad_0[512-0];
	struct l_thc_hchs	l_thc_hchs;		/* 512 */
	struct l_thc_hchsdvs	l_thc_hchsdvs;		/* 513 */
	struct l_thc_hchd	l_thc_hchd;		/* 514 */
	struct l_thc_hcvs	l_thc_hcvs;		/* 515 */
	struct l_thc_hcvd	l_thc_hcvd;		/* 516 */
	struct l_thc_hcr	l_thc_hcr;		/* 517 */
	struct l_thc_hcmisc	l_thc_hcmisc;		/* 518 */
	int	l_thc_pad_519[575-519];
	struct l_thc_cursor	l_thc_cursor;		/* 575 */
	int	l_thc_cursora00;			/* 576 */
	int	l_thc_cursora01;			/* 577 */
	int	l_thc_cursora02;			/* 578 */
	int	l_thc_cursora03;			/* 579 */
	int	l_thc_cursora04;			/* 580 */
	int	l_thc_cursora05;			/* 581 */
	int	l_thc_cursora06;			/* 582 */
	int	l_thc_cursora07;			/* 583 */
	int	l_thc_cursora08;			/* 584 */
	int	l_thc_cursora09;			/* 585 */
	int	l_thc_cursora10;			/* 586 */
	int	l_thc_cursora11;			/* 587 */
	int	l_thc_cursora12;			/* 588 */
	int	l_thc_cursora13;			/* 589 */
	int	l_thc_cursora14;			/* 590 */
	int	l_thc_cursora15;			/* 591 */
	int	l_thc_cursora16;			/* 592 */
	int	l_thc_cursora17;			/* 593 */
	int	l_thc_cursora18;			/* 594 */
	int	l_thc_cursora19;			/* 595 */
	int	l_thc_cursora20;			/* 596 */
	int	l_thc_cursora21;			/* 597 */
	int	l_thc_cursora22;			/* 598 */
	int	l_thc_cursora23;			/* 599 */
	int	l_thc_cursora24;			/* 600 */
	int	l_thc_cursora25;			/* 601 */
	int	l_thc_cursora26;			/* 602 */
	int	l_thc_cursora27;			/* 603 */
	int	l_thc_cursora28;			/* 604 */
	int	l_thc_cursora29;			/* 605 */
	int	l_thc_cursora30;			/* 606 */
	int	l_thc_cursora31;			/* 607 */
	int	l_thc_cursorb00;			/* 608 */
	int	l_thc_cursorb01;			/* 609 */
	int	l_thc_cursorb02;			/* 610 */
	int	l_thc_cursorb03;			/* 611 */
	int	l_thc_cursorb04;			/* 612 */
	int	l_thc_cursorb05;			/* 613 */
	int	l_thc_cursorb06;			/* 614 */
	int	l_thc_cursorb07;			/* 615 */
	int	l_thc_cursorb08;			/* 616 */
	int	l_thc_cursorb09;			/* 617 */
	int	l_thc_cursorb10;			/* 618 */
	int	l_thc_cursorb11;			/* 619 */
	int	l_thc_cursorb12;			/* 620 */
	int	l_thc_cursorb13;			/* 621 */
	int	l_thc_cursorb14;			/* 622 */
	int	l_thc_cursorb15;			/* 623 */
	int	l_thc_cursorb16;			/* 624 */
	int	l_thc_cursorb17;			/* 625 */
	int	l_thc_cursorb18;			/* 626 */
	int	l_thc_cursorb19;			/* 627 */
	int	l_thc_cursorb20;			/* 628 */
	int	l_thc_cursorb21;			/* 629 */
	int	l_thc_cursorb22;			/* 630 */
	int	l_thc_cursorb23;			/* 631 */
	int	l_thc_cursorb24;			/* 632 */
	int	l_thc_cursorb25;			/* 633 */
	int	l_thc_cursorb26;			/* 634 */
	int	l_thc_cursorb27;			/* 635 */
	int	l_thc_cursorb28;			/* 636 */
	int	l_thc_cursorb29;			/* 637 */
	int	l_thc_cursorb30;			/* 638 */
	int	l_thc_cursorb31;			/* 639 */
	};
