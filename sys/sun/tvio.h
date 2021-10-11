
/*	@(#)tvio.h	1.1 92/07/30 SMI	*/

/*
 * Copyright 1989 by Sun Microsystems, Inc.
 */

/*
 * IOCTL definitions for SunVideo(tvone) driver
 */

#ifndef	tvio_DEFINED

#define	tvio_DEFINED

#include <sys/ioccom.h>		/* define _IOR and _IOW */
#include <pixrect/pixrect.h>	/* for struct pr_pos */


#define TVIOGFORMAT	_IOR(X, 1, int)		/* Get input format */
#define TVIOSFORMAT	_IOW(X, 2, int)		/* Set input format */
#define TVIOGCOMPOUT	_IOR(X, 3, int)		/* Get component out */
#define TVIOSCOMPOUT	_IOW(X, 4, int)		/* Set component out */
#define TVIOGSYNC	_IOR(X, 5, int)		/* Get sync select */
#define TVIOSSYNC	_IOW(X, 6, int)	    	/* Set sync select */
#define TVIO_NTSC	0			/*  NTSC format */
#define TVIO_YC		1			/*  YC format */
#define TVIO_YUV	2			/*  YUV component format */
#define TVIO_RGB	3			/*  RGB component format */

#define TVIOGOUT	_IOR(X, 7, int)		/* Get video output select */
#define TVIOSOUT	_IOW(X, 8, int)		/* Set video output select */
#define TVIO_ONSCREEN	1			/*  inserts onto screen */
#define TVIO_VIDEOOUT	2			/*  outputs standard video */

#define TVIOGCOMPRESS	_IOR(X, 9, int)		/* Get compression/expansion */
#define TVIOSCOMPRESS	_IOW(X, 10, int)	/* Set compression/expansion */
/*	0					    2X
 *	1					    1:1
 *	3					    1/2
 *	5					    1/4
 *	7					    1/8
 *	9					    1/16
 */

#define TVIOGCHROMAGAIN	_IOR(X, 11, int)	/* Get chroma gain */
#define TVIOSCHROMAGAIN	_IOW(X, 12, int)	/* Set chroma gain */
#define TVIOGREDGAIN	_IOR(X, 13, int)	/* Get red gain */
#define TVIOSREDGAIN	_IOW(X, 14, int)	/* Set red gain */
#define TVIOGREDBLACK	_IOR(X, 15, int)	/* Get red black level */
#define TVIOSREDBLACK	_IOW(X, 16, int)	/* Set red black level */
#define TVIOGGREENGAIN	_IOR(X, 17, int)	/* Get green gain */
#define TVIOSGREENGAIN	_IOW(X, 18, int)	/* Set green gain */
#define TVIOGGREENBLACK	_IOR(X, 19, int)	/* Get green black level */
#define TVIOSGREENBLACK	_IOW(X, 20, int)	/* Set green Black level */
#define TVIOGBLUEGAIN	_IOR(X, 21, int)	/* Get blue gain */
#define TVIOSBLUEGAIN	_IOW(X, 22, int)	/* Set blue gain */
#define TVIOGBLUEBLACK	_IOR(X, 23, int)	/* Get blue black level */
#define TVIOSBLUEBLACK	_IOW(X, 24, int)	/* Set blue balck level */
#define TVIOGLUMAGAIN	_IOR(X, 25, int)	/* Get luma gain */
#define TVIOSLUMAGAIN	_IOW(X, 26, int)	/* Set Luma gain */
/*						values range from -255 to 255
 *						actual value stored is offset
 *					        from calibration values.
 */

#define TVIOGPOS    _IOR(X, 27, struct pr_pos) /* get video start position */
#define TVIOSPOS    _IOW(X, 28, struct pr_pos) /* set video start position */

#define TVIOGRAB	_IO(X, 29)		/* Grab device */
#define TVIORELEASE	_IO(X, 30)		/* Release device */

#define TVIOGBIND	_IOR(X, 31, dev_t)	/* Get bound device's dev_t */
#define TVIOSBIND	_IOW(X, 32, dev_t)	/* Set bound device's dev_t  */

#define	TVIOGBTYPE	_IOR(X, 33, int)	/* get bound device's type */
#define TVIOREDIRECT	_IO(X, 34)		/* Redirect /dev/fb */

#define TVIOGLIVE	_IOR(X, 35, int)	/* Get live status */
#define TVIOSLIVE	_IOW(X, 36, int)	/* Set live status */

#define TVIOSYNC	_IO(X, 37)		/* Syncronize buffer */

#define TVIOGCHROMASEP	_IOR(X, 39, int)	/* Get chroma separation */
#define TVIOSCHROMASEP	_IOW(X, 40, int)	/* Set chroma separation */
#define TVIOSEP_AUTO	0			/* Auto */
#define TVIOSEP_LOW_PASS 2			/* Low Pass filter */
#define TVIOSEP_COMB	1			/* Comb Filter */

#define TVIOGCHROMADEMOD    _IOR(X, 41, int)	/* Get chroma demod. */
#define TVIOSCHROMADEMOD    _IOW(X, 42, int)	/* Set chroma demod. */
#define TVIO_AUTO	0			/*	auto chroma sep  */
#define	TVIO_ON		1			/*	color = on	 */
#define TVIO_OFF	2			/*	color = off	 */
 

#define TVIOGGENLOCK	_IOR(X, 43, int)	/* Get Genlock status */
#define TVIOSGENLOCK	_IOW(X, 44, int)	/* Set Genlock status */
						/* 1==>on, 0-->off    */

#define TVIOVWAIT	    _IO(X, 45)		/* Wait for vertical sync */
#define TVIOGSYNCABSENT	    _IOR(X, 46, int)	/* Check sync absent */
#define TVIOGBURSTABSENT    _IOR(X, 47, int)	/* Check burst absent */


/* ioctls for calibration */
struct tv1_rgb_gain_black {
    unsigned char red_gain,
	red_black,
	green_gain,
	green_black,
	blue_gain,
	blue_black;
};

union tv1_nvram {
    int packed[16];
    struct {	/* New Calibration */
	unsigned char magic;		/* Magic number, should be A9 */
	unsigned char format;		/* Format code (should be 1) */
	unsigned char reserved[2];
	struct pr_pos offset;		/* X Y offsets to start of screen */

	struct tv1_rgb_gain_black ntsc_yc_cal;
	unsigned char ntsc_chroma_gain;
	unsigned char ntsc_luma_gain;

	unsigned char yc_chroma_gain;
	unsigned char yc_luma_gain;
	
	struct tv1_rgb_gain_black
	    yuv_cal,			/* Calibration for YUV */
	    rgb_cal;			/* Calibration for RGB */

	struct tv1_rgb_gain_black loopback_ntsc_yc_cal;
	unsigned char loopback_ntsc_chroma_gain;
	unsigned char loopback_ntsc_luma_gain;

	unsigned char loopback_yc_chroma_gain;
	unsigned char loopback_yc_luma_gain;
	
	struct tv1_rgb_gain_black
	    loopback_yuv_cal,		/* Calibration for YUV loopback */
	    loopback_rgb_cal;		/* Calibration for RGB loopback */
    } field;
};
#define TVIOGVIDEOCAL	_IOR(X, 49, union tv1_nvram)  /* Get calibration */
#define	TVIOSVIDEOCAL	_IOW(X, 50, union tv1_nvram)  /* Set Calibration */
#define TVIONVREAD	_IOR(X, 51, union tv1_nvram)  /* Read cal from NV */ 
#define	TVIONVWRITE	_IOW(X, 52, union tv1_nvram)  /* Write cal to NV */


/* ioctls for diagnostics */
#define TVIOSIBADVANCE		_IOW(X, 53, int)   /* Advance ibstate */
#define TVIOGIBSTATE		_IOR(X, 55, int)   /* Get ibstate */
#define TVIOSABSTATE		_IOW(X, 56, int)   /* Set ibstate */
#define TVIOGABSTATE		_IOR(X, 57, int)   /* Get abstate */
#define TVIOGCONTROL		_IOR(X, 59, int)   /* Get control register */
#define TVIOSCONTROL		_IOW(X, 60, int)  /* Set control register */
#define TVIOSLOOPBACKCAL	_IO(X, 61)	  /* switch to Loopback cal */


#endif	!tvio_DEFINED
