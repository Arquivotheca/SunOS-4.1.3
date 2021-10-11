/*********** @(#)lpviio.h 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc. *********/
/*	@(#)lpviio.h 1.4 90/05/14 SMI */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * supported ioctls for the lpvi driver.
 */

#define	LPVIIOC_RESET	_IO(z, 0)	/* reset the interface */
#define	LPVIIOC_TESTIO	_IO(z, 1)	/* test interface readiness */
#define	LPVIIOC_INQ	_IOR(z, 2, struct lpvi_inq)
#define	LPVIIOC_GETERR	_IOR(z, 3, struct lpvi_err)
#define	LPVIIOC_SETPAGE	_IOW(z, 4, struct lpvi_page)
#define	LPVIIOC_GETPAGE	_IOR(z, 5, struct lpvi_page)
#define	LPVIIOC_SETMODE	_IOW(z, 6, struct lpvi_mode)
#define	LPVIIOC_GETMODE	_IOR(z, 7, struct lpvi_mode)
#define	LPVIIOC_SETDSP	_IOW(z, 8, struct lpvi_display)	/* set display */

/* configuration info */
struct lpvi_inq {
	u_long	counters;
	u_char	papersize;	/* size and existence */
	u_char	deve_low:1;	/* deve = toner? */
	u_char	drum_low:1;
	u_char	irev;		/* interface revision */
	char	engine[32];	/* words describing print engine */
};

/* paper size codes */
#define	NOCST	0x00	/* no cassette */
#define	B4SEF	0x01	/* b4 sef */
#define	A4SEF	0x02	/* a4 sef */
#define	LTRSEF	0x03	/* letter sef */
#define	B5SEF	0x05	/* b5 sef */
#define	LGL14	0x06	/* legal 14" */
#define	LGL13	0x07	/* legal 15" */


struct lpvi_err {
	u_char	err_type;
	u_char	err_code;
};
/* error types (bits) */
#define	ENGWARN	0x01		/* need consumables, or warming up (door) */
#define	ENGFATL	0x02		/* major problems */
#define	EDRVR	0x04		/* interface/driver error */
				/* these errors should never happen */
/* error codes */
#define	EMOTOR		0x01
#define	EROS		0x02
#define	EFUSER		0x03
#define	XEROFAIL	0x04
#define	ILCKOPEN	0x05
#define	NOTRAY		0x06
#define	NOPAPR		0x07
#define	XITJAM		0x08
#define	MISFEED		0x09
#define	WDRUMX		0x0a
#define	WDEVEX		0x0b
#define	NODRUM		0x0c
#define	NODEVE		0x0d
#define	EDRUMX		0x0e
#define	EDEVEX		0x0f
#define	ENGCOLD		0x10
#define	TIMEOUT		0x11
#define	EDMA		0x12
#define	ESERIAL		0x13


/* page layout stuff */
struct lpvi_page {
	u_long	bitmap_width;	/* in bytes */
	u_long	page_width;	/* in pixels */
	u_long	page_length;	/* scans (pixels) */
	u_long	top_margin;
	u_long	left_margin;
	u_long	resolution;	/* on set, if zero, don't change it */
				/* on get, is current resolution of printer */
};

/* supported resolutions */
#define	NOCHG	0x00
#define	DPI300	0x01
#define	DPI400	0x02


/* set operating mode */
struct lpvi_mode {
	u_char	image_pol:2;
	u_char	margin_pol:2;
	u_char	clear_pol:2;
	u_char	en_pause:2;
	u_char	tray_sel;
};

/* pause modes */
#define	FUSPAUS	0x01
#define	ROSPAUS	0x02

/* trays */
#define	MAINTRY	0x01
#define	AUXTRY	0x02
#define	MANUAL	0x03


/* set display mode */
struct lpvi_display {
	u_char	control;
	u_char	type0; /* 2nd order digit */
	u_char	data0; /* 2nd order digit */
	u_char	type1; /* 1st order digit */
	u_char	data1; /* 1st order digit */
};

/* control modes */
#define	IOT	0x00;
#define	MASTER	0x01;

/* display mode types */
#define	DSPON	0x01;
#define	DSPBLNK	0x02;
#define	DSPHBLNK	0x03;
#define	DSPRBLNK	0x04;

