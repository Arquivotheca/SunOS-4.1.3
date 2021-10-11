/*	@(#)fbio.h 1.1 92/07/30 SMI	*/

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#ifndef	_sun_fbio_h
#define	_sun_fbio_h
#include <sys/types.h>

#ifndef ASM
/*
 * Frame buffer descriptor.
 * Returned by FBIOGTYPE ioctl on frame buffer devices.
 */
struct	fbtype {
	int	fb_type;	/* as defined below */
	int	fb_height;	/* in pixels */
	int	fb_width;	/* in pixels */
	int	fb_depth;	/* bits per pixel */
	int	fb_cmsize;	/* size of color map (entries) */
	int	fb_size;	/* total size in bytes */
};

#define	FBIOGTYPE _IOR(F, 0, struct fbtype)

#ifdef  KERNEL
struct  fbpixrect {
	struct  pixrect *fbpr_pixrect;  /* Pixrect of dev returned here */
};

#define	FBIOGPIXRECT _IOWR(F, 1, struct fbpixrect)
#endif	KERNEL

/*
 * General purpose structure for passing info in and out of frame buffers
 * (used for gp1)
 */
struct	fbinfo {
	int		fb_physaddr;	/* physical frame buffer address */
	int		fb_hwwidth;	/* fb board width */
	int		fb_hwheight;	/* fb board height */
	int		fb_addrdelta;	/* phys addr diff between boards */
	unsigned char	*fb_ropaddr;	/* fb va thru kernelmap */
	int		fb_unit;	/* minor devnum of fb */
};

#define	FBIOGINFO _IOR(F, 2, struct fbinfo)

/*
 * Color map I/O.  See also fbcmap_i below.
 */
struct	fbcmap {
	int		index;		/* first element (0 origin) */
	int		count;		/* number of elements */
	unsigned char	*red;		/* red color map elements */
	unsigned char	*green;		/* green color map elements */
	unsigned char	*blue;		/* blue color map elements */
};

#define	FBIOPUTCMAP _IOW(F, 3, struct fbcmap)
#define	FBIOGETCMAP _IOW(F, 4, struct fbcmap)

/*
 * Set/Get attributes
 */
#define	FB_ATTR_NDEVSPECIFIC	8	/* no. of device specific values */
#define	FB_ATTR_NEMUTYPES	4	/* no. of emulation types */

struct fbsattr {
	int	flags;			/* misc flags */
#define	FB_ATTR_AUTOINIT	1	/* emulation auto init flag */
#define	FB_ATTR_DEVSPECIFIC	2	/* dev. specific stuff valid flag */
	int	emu_type;		/* emulation type (-1 if unused) */
	int	dev_specific[FB_ATTR_NDEVSPECIFIC];	/* catchall */
};

struct fbgattr {
	int	real_type;		/* real device type */
	int	owner;			/* PID of owner, 0 if myself */
	struct fbtype fbtype;		/* fbtype info for real device */
	struct fbsattr sattr;		/* see above */
	int	emu_types[FB_ATTR_NEMUTYPES];	/* possible emulations */
						/* (-1 if unused) */
};

#define	FBIOSATTR	_IOW(F, 5, struct fbsattr)
#define	FBIOGATTR	_IOR(F, 6, struct fbgattr)


/*
 * Video control
 * (the unused bits are reserved for future use)
 */
#define	FBVIDEO_OFF	0
#define	FBVIDEO_ON	1

#define	FBIOSVIDEO	_IOW(F, 7, int)
#define	FBIOGVIDEO	_IOR(F, 8, int)

/* Vertical retrace support. */
#define	FBIOVERTICAL	_IOW(F, 9, int)
#define	GRABPAGEALLOC   _IOR(F, 10, caddr_t)
#define	GRABPAGEFREE    _IOW(F, 11, caddr_t)
#define	GRABATTACH	_IOW(F, 12, caddr_t)

#define	FBIOGPLNGRP	_IOR(F, 13, int)
#define	FBIOGCMSIZE	_IOR(F, 14, int)
#define	FBIOSCMSIZE	_IOW(F, 15, int)
#define	FBIOSCMS	_IOW(F, 16, int)
#define	FBIOAVAILPLNGRP	_IOR(F, 17, int)


/*
 * Structure to pass double buffering state back and forth the device.
 */

/* used in devstate */
#define	FBDBL_AVAIL	0x80000000
#define	FBDBL_DONT_BLOCK 0x40000000
#define	FBDBL_AVAIL_PG	0x20000000

/* used in read/write/display */
#define	FBDBL_A	 0x1
#define	FBDBL_B	 0x2
#define	FBDBL_BOTH	(FBDBL_A | FBDBL_B)
#define	FBDBL_NONE	0x4

struct fbdblinfo {
	unsigned int	dbl_devstate;
	unsigned int	dbl_read;
	unsigned int	dbl_write;
	unsigned int	dbl_display;
	int		dbl_depth;
	char		dbl_wid;
};

#define	FBIODBLGINFO    _IOR(F, 18, struct fbdblinfo)
#define	FBIODBLSINFO    _IOW(F, 19, struct fbdblinfo)

/* 8-bit emulation in 24-bit ioctls */

#define	FBIOSWINFD	_IOR(F, 20, int)
#define	FBIOSAVWINFD	_IOR(F, 21, int)
#define	FBIORESWINFD	_IOR(F, 22, int)
#define	FBIOSRWINFD	_IOR(F, 23, int)

/*
 * hardware cursor control
 */
 
struct fbcurpos {
        short x, y;
};
 
struct fbcursor {
        short set;              /* what to set */
#define FB_CUR_SETCUR   0x01
#define FB_CUR_SETPOS   0x02
#define FB_CUR_SETHOT   0x04
#define FB_CUR_SETCMAP  0x08
#define FB_CUR_SETSHAPE 0x10
#define FB_CUR_SETALL   0x1F
        short enable;           /* cursor on/off */
        struct fbcurpos pos;    /* cursor position */
        struct fbcurpos hot;    /* cursor hot spot */
        struct fbcmap cmap;     /* color map info */
        struct fbcurpos size;   /* cursor bit map size */
        char *image;            /* cursor image bits */
        char *mask;             /* cursor mask bits */
};
 
/* set/get cursor attributes/shape */
#define FBIOSCURSOR     _IOW(F, 24, struct fbcursor)
#define FBIOGCURSOR     _IOWR(F, 25, struct fbcursor)
 
/* set/get cursor position */
#define FBIOSCURPOS     _IOW(F, 26, struct fbcurpos)
#define FBIOGCURPOS     _IOW(F, 27, struct fbcurpos)
 
/* get max cursor size */
#define FBIOGCURMAX     _IOR(F, 28, struct fbcurpos)
 
/* Window Grabber info ioctl */
#define GRABLOCKINFO    _IOR(F, 29, caddr_t)
 
/*
 * Window Identification (wid) defines, structures, and ioctls.
 *
 * Some wids need to be unique when used for things such as double
 * buffering or rendering clipping.  Some wids can be shared when
 * used for display attributes only.  What can be shared and how
 * may be device dependent.  The fb_wid_alloc.wa_type and fb_wid_item
 * structure members will be left to device specific interpretation.
 */

#define	FB_WID_SHARED_8		0
#define	FB_WID_SHARED_24	1
#define	FB_WID_DBL_8		2
#define	FB_WID_DBL_24		3

struct fb_wid_alloc
{
    unsigned int	wa_type;	/* special attributes		*/
    int			wa_index;	/* base wid returned		*/
    unsigned int	wa_count;	/* how many contiguous wids	*/
};

struct fb_wid_item
{
    unsigned int	wi_type;	/* special attributes		*/
    int			wi_index;	/* which lut			*/
    unsigned int	wi_attrs;	/* which attributes		*/
    unsigned int	wi_values[NBBY*sizeof(int)]; /* the attr values	*/
};

struct fb_wid_list
{
    unsigned int	wl_flags;
    unsigned int	wl_count;
    struct fb_wid_item	*wl_list;
};

struct fb_wid_dbl_info
{
    struct fb_wid_alloc	dbl_wid;
    char		dbl_fore;
    char		dbl_back;
    char		dbl_read_state;
    char		dbl_write_state;
};

#define	FBIO_WID_ALLOC	_IOWR(F, 30, struct fb_wid_alloc)
#define FBIO_WID_FREE	_IOW(F, 31, struct fb_wid_alloc)
#define	FBIO_WID_PUT	_IOW(F, 32, struct fb_wid_list)
#define	FBIO_WID_GET	_IOW(F, 33, struct fb_wid_list)

#define	FBIO_DEVID	_IOR(F, 34, int)
#define	FBIO_U_RST	_IOW(F, 35, int)
#define	FBIO_FULLSCREEN_ELIMINATION_GROUPS	_IOR(F, 36, unsigned char *)
#define FBIO_WID_DBL_SET        _IO(F, 37)
#define	FBIOVRTOFFSET	_IOR(F, 38, int)

struct cg6_info {
      u_short     accessible_width;       /* accessible bytes in scanline */
      u_short     accessible_height;      /* number of accessible scanlines */
      u_short     line_bytes;             /* number of bytes/scanline */
      u_short     hdb_capable;            /* can this thing hardware db? */
      u_short     vmsize;                 /* this is Mb of video memory */
      u_char      boardrev;               /* board revision # */
      u_char      slot;                   /* sbus slot # */
      u_long      pad1;                   /* expansion */
} ;
 
#define MON_TYPE_STEREO         0x8     /* stereo display */
#define MON_TYPE_0_OFFSET       0x4     /* black level 0 ire instead of 7.5 */
#define MON_TYPE_OVERSCAN       0x2     /* overscan */
#define MON_TYPE_GRAY           0x1     /* greyscale monitor */
 
struct mon_info {
      u_long      mon_type;               /* bit array: defined above */
      u_long      pixfreq;                /* pixel frequency in Hz */
      u_long      hfreq;                  /* horizontal freq in Hz */
      u_long      vfreq;                  /* vertical freq in Hz */
      u_long      vsync;                  /* vertical sync in scanlines */
      u_long      hsync;                  /* horizontal sync in pixels */
                                          /* these are in pixel units */
      u_short     hfporch;                /* horizontal front porch */
      u_short     hbporch;                /* horizontal back porch */
      u_short     vfporch;                /* vertical front porch */
      u_short     vbporch;                /* vertical back porch */
} ;
 

#define FBIOGXINFO      _IOR(F, 39, struct cg6_info)
#define FBIOMONINFO     _IOR(F, 40, struct mon_info)

/*
 * Color map I/O.  
 */
struct	fbcmap_i {
	unsigned int	flags;		/* see below */
	int		id;		/* colormap id for multiple cmaps */
	int		index;		/* first element (0 origin) */
	int		count;		/* number of elements */
	unsigned char	*red;		/* red color map elements */
	unsigned char	*green;		/* green color map elements */
	unsigned char	*blue;		/* blue color map elements */
};

#define	FB_CMAP_BLOCK	0x1	/* wait for vrt before returning */
#define	FB_CMAP_KERNEL	0x2	/* called within kernel */

#define	FBIOPUTCMAPI _IOW(F, 41, struct fbcmap_i)
#define	FBIOGETCMAPI _IOW(F, 42, struct fbcmap_i)

/* assigning a given window id to a pixrect - special for PHIGS */
#define FBIO_ASSIGNWID          _IOWR(F, 43, struct fb_wid_alloc)

/* assigning a given window to be stereo */
#define FBIO_STEREO         _IOWR(F, 44, int)
#define	FB_WIN_STEREO	    0x2

#endif !ASM

/* frame buffer type codes */
#define	FBTYPE_SUN1BW		0	/* Multibus mono */
#define	FBTYPE_SUN1COLOR	1	/* Multibus color */
#define	FBTYPE_SUN2BW		2	/* memory mono */
#define	FBTYPE_SUN2COLOR	3	/* color w/rasterop chips */
#define	FBTYPE_SUN2GP		4	/* GP1/GP2 */
#define	FBTYPE_SUN5COLOR	5	/* RoadRunner accelerator */
#define	FBTYPE_SUN3COLOR	6	/* memory color */
#define	FBTYPE_MEMCOLOR		7	/* memory 24-bit */
#define	FBTYPE_SUN4COLOR	8	/* memory color w/overlay */

#define	FBTYPE_NOTSUN1		9	/* reserved for customer */
#define	FBTYPE_NOTSUN2		10	/* reserved for customer */
#define	FBTYPE_NOTSUN3		11	/* reserved for customer */

#define	FBTYPE_SUNFAST_COLOR	12	/* accelerated 8bit */
#define	FBTYPE_SUNROP_COLOR	13	/* MEMCOLOR with rop h/w */
#define	FBTYPE_SUNFB_VIDEO	14	/* Simple video mixing */
#define	FBTYPE_SUNGIFB		15	/* medical image */
#define	FBTYPE_SUNGPLAS		16	/* plasma panel */
#define	FBTYPE_SUNGP3		17	/* cg12 running gpsi microcode */
#define	FBTYPE_SUNGT		18	/* gt graphics accelerator */
#define	FBTYPE_RESERVED1	19	/* reserved, do not use */

#define	FBTYPE_LASTPLUSONE	20	/* max number of fbs (change as add) */

#endif	/*!_sun_fbio_h*/
