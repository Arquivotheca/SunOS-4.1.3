/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
/*	@(#)framebuf.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
 * framebuf.h - constants for the SUN graphics board version 1
 *
 * GXBase contains the starting address, in the current virtual
 * address space, of the frame buffer.  It is typically initialized 
 * from an ioctl() call on /dev/gfx.  Standalones just punt with
 * an external constant selected at link time.
 */ 
extern int GXBase;

/* 
 * GXConfig contains a bunch of configuration bits which are returned
 * by the GXfind/GXprobe routine.  These indicate whether or not the
 * frame buffer is Present, whether it is Landscape mode (1024 wide by
 * 800 high) or portrait mode (800 wide by 1024 high), and if it is
 * a Color frame buffer.  More bits can be added upon request.
 */
extern long GXConfig;
#define GXCPresent	0x00000001
#define GXCLandscape	0x00000002
#define GXCColor	0x00000004

/*
 * GXDeviceName and GXFile are relevant only under Unix.  They indicate
 * the string containing the device name (default is "/dev/gfx" but can
 * be overridden from the environment (see environ(5) ) by specifying
 * environment variable GraphicsDev.  GXFile is the file descriptor
 * returned by open()ing the file.  Right now it is not too useful,
 * but as more graphics system calls are added, it may prove handy.
 */
extern char *GXDeviceName;
extern int GXFile;

/*
 * The address space occupied by the frame buffer is GXaddrRange bytes
 * long.
 */
#define GXaddrRange	0x20000

/*
 * The low order 11 bits consist of the X or Y address times 2.
 * The lowest order bit is ignored, so word addressing works efficiently.
 */

# define GXselectX (0<<11)	/* the address is loaded into an X register */
# define GXselectx (0<<11)	/* the address is loaded into an X register */
# define GXselectY (1<<11)	/* the address is loaded into an Y register */
# define GXselecty (1<<11)	/* the address is loaded into an Y register */

/*
 * There are four sets of X and Y register pairs, selected by the following bits
 */

# define GXaddressSet0  (0<<12)
# define GXaddressSet1  (1<<12)
# define GXaddressSet2  (2<<12)
# define GXaddressSet3  (3<<12)
# define GXset0  (0<<12)
# define GXset1  (1<<12)
# define GXset2  (2<<12)
# define GXset3  (3<<12)

/*
 * The following bits indicate which registers are to be loaded
 */

# define GXnone    (0<<14)
# define GXothers  (1<<14)
# define GXsource  (2<<14)
# define GXmask    (3<<14)
# define GXpat     (3<<14)

# define GXupdate (1<<16)	/* actually update the frame buffer */


/*
 * These registers can appear on the left of an assignment statement.
 * Note they clobber X register 3.
 */

# define GXfunction	*(short *)(GXBase + GXset3 + GXothers + (0<<1) )
# define GXwidth	*(short *)(GXBase + GXset3 + GXothers + (1<<1) )
# define GXcontrol	*(short *)(GXBase + GXset3 + GXothers + (2<<1) )
# define GXintClear	*(short *)(GXBase + GXset3 + GXothers + (3<<1) )

# define GXsetMask	*(short *)(GXBase + GXset3 + GXmask )
# define GXsetSource	*(short *)(GXBase + GXset3 + GXsource )
# define GXpattern	*(short *)(GXBase + GXset3 + GXpat )

/*
 * The following bits are written into the GX control register.
 * It is reset to zero on hardware reset and power-up.
 * The high order three bits determine the Interrupt level (0-7)
 */

# define GXintEnable   (1<<8)
# define GXvideoEnable (1<<9)
# define GXintLevel0	(0<<13)
# define GXintLevel1	(1<<13)
# define GXintLevel2	(2<<13)
# define GXintLevel3	(3<<13)
# define GXintLevel4	(4<<13)
# define GXintLevel5	(5<<13)
# define GXintLevel6	(6<<13)
# define GXintLevel7	(7<<13)

/*
 * The following are "function" encodings loaded into the function register
 */

# define GXnoop			0xAAAA
# define GXinvert		0x5555
# define GXcopy        		0xCCCC
# define GXcopyInverted 	0x3333
# define GXclear		0x0000
# define GXset			0xFFFF
# define GXpaint		0xEEEE
# define GXpaintInverted 	0x2222
# define GXxor			0x6666

/*
 * The following permit functions to be expressed as Boolean combinations
 * of the three primitive functions 'source', 'mask', and 'dest'.  Thus
 * GXpaint is equal to GXSOURCE|GXDEST, while GXxor is GXSOURCE^GXDEST.
 */

# define GXSOURCE		0xCCCC
# define GXMASK			0xF0F0
# define GXDEST			0xAAAA


/*
 * These may appear in statement contexts to just
 * set the X and Y registers of set number zero to the given values.
 */

# define GXsetX(X)	*(short *)(GXBase + GXselectX + (X<<1)) = 1;
# define GXsetY(Y)	*(short *)(GXBase + GXselectY + (Y<<1)) = 1;

