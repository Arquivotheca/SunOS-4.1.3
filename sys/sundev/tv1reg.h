/* @(#)tv1reg.h	1.1 92/07/30 SMI */

#ifndef tv1reg_included
#define tv1reg_included

#define TV1_MAGIC	0xA9
#define TV1_INTR_VECTOR	0xA9
#define TV1_INTR_MASK	0xff

/*
 * The meaningful size of the frame buffer is 640 by 612, the bottem 483 lines are video
 * signals, the top ones are data.
 */
#define FB_BASE 0
#define FB_SIZE 0x200000
#define FB_WIDTH    640
#define FB_HEIGHT   512
#define FB_DEPTH     32
#define FB_LINEBYTES 0x1000

#define CP_BASE 0x200000
#define CP_SIZE	0x040000
#define CP_WIDTH  1280
#define CP_HEIGHT 1023
#define CP_DEPTH  1
#define CP_LINEBYTES 0x100
#define CP_XY_REG_OFFSET CP_HEIGHT*CP_LINEBYTES

#define CSR_BASE 0x300000

union light {
    struct {
	unsigned int:   8;
	unsigned int:   8;
	unsigned int    chroma:8;
	unsigned int    luma:8;
    }               field;
    unsigned int    packed;
};

union color {
    struct {
	unsigned int:   8;
	unsigned int:   8;
	unsigned int    black:8;
	unsigned int    gain:8;
    }               field;
    unsigned int    packed;
};

struct knob {
    union light     light;
    union color     red,
                    green,
                    blue;
};

struct tv1_enable_plane {
    unsigned char tv1_enable_data[CP_HEIGHT * CP_LINEBYTES]; /* actual data */
    struct pr_pos tv1_enable_offset; /* where to store the offset */
};
    


#define SYNC_NTSC		1
#define SYNC_Y			2
#define SYNC_GY			4
#define TV1_SYNC_SHIFT		3
#define TV1_SYNC_MASK		(0x7       << TV1_SYNC_SHIFT)
#define TV1_SYNC_NTSC		(SYNC_NTSC << TV1_SYNC_SHIFT)
#define TV1_SYNC_Y		(SYNC_Y    << TV1_SYNC_SHIFT)
#define TV1_SYNC_GY		(SYNC_GY   << TV1_SYNC_SHIFT)

/*
 * There are three sets of connectors for input signals, NTSC, YC and RGB/YUV.
 * The NTSC signal will be separated into Y (luma) and C (chroma).
 * YC will be further demodulated into RGB.
 * RGB and YUV are convertible to each other and share the same input connectors
 */
#define FORMAT_NTSC		0
#define FORMAT_YC		1
#define FORMAT_YUV		2
#define FORMAT_RGB		3
#define TV1_FORMAT_SHIFT	6
#define TV1_FOORMAT_MASK	(0x3  << TV1_FORMAT_SHIFT)
#define TV1_FORMAT_NTSC		(NTSC << TV1_FORMAT_SHIFT)
#define TV1_FORMAT_YC		(YC   << TV1_FORMAT_SHIFT)
#define TV1_FORMAT_YUV		(YUV  << TV1_FORMAT_SHIFT)
#define TV1_FORMAT_RGB		(RGB  << TV1_FORMAT_SHIFT)

/* How is NTSC separated into YC, default AUTO */
#define SEP_AUTO		0
#define SEP_COMB		1
#define SEP_LPF			2
#define TV1_SEP_SHIFT		8
#define TV1_SEP_MASK		(0x3      << TV1_SEP_SHIFT)
#define TV1_SEP_AUTO		(SEP_AUTO << TV1_SEP_SHIFT)
/* combination */
#define TV1_SEP_COMB		(SEP_COMB << TV1_SEP_SHIFT)
/* low pass filter */
#define TV1_SEP_LPF		(SEP_LPF  << TV1_SEP_SHIFT)

/* How is YC demodulated into RGB, default AUTO */
#define DEMOD_AUTO		0
#define DEMOD_ON		1
#define DEMOD_OFF		2
#define TV1_DEMOD_SHIFT		10
#define TV1_DEMOD_MASK		(0x3 << TV1_DEMO_SHIFT)
#define TV1_DEMOD_AUTO		(DEMOD_AUTO << TV1_DEMO_SHIFT)
#define TV1_DEMOD_ON		(DEMOD_ON   << TV1_DEMO_SHIFT)
#define TV1_DEMOD_OFF		(DEMOD_OFF  << TV1_DEMO_SHIFT)


/*
 * abstate: which fields can be accessed, the one being written cannot.
 *         value:   A          B            C
 *          0       write odd  -            -
 *          1       -          write even   -
 *          2       -          -            write odd
 *          3       write even -            -
 *          4       -          write odd    -
 *          5       -          -            write even
 */
union control {
    struct {
	unsigned int:   16;
	unsigned int	ib_advance:1;
	unsigned int    abstate:3;
	unsigned int    demod:2;
	unsigned int    yc_sep:2;
	unsigned int    format:2;
	unsigned int    sync:3;
	unsigned int    compress:3;
    }               field;
    unsigned int    packed;
};

/*
 * yuv: if input comes in thru RGV/YUV connectors, what are them.  Ignored if input is NTSC
 *      or YC.
 * expand: 
 *	1: expanding at 1 to 2, 0: shrinking at 2 ** compress
 * CAVEAT!! It is illegal to have expand 1 and compress not zero.
 */
union display {
    struct {
	unsigned int:   6;
	unsigned int	genlock:1;
	unsigned int    yuv:1;
	unsigned int    expand:1;
	unsigned int    ypos:12;
	unsigned int    xpos:11;
    }               field;
    unsigned int    packed;
};

/*
 * PLL is the pixel frequency.
 * 	92 MHz is effectively 1152 scanlines.
 * 	135 MHz is 1280 scanlines
 * crystal: whether the input is crystalizedly clocked.
 * burst/sync_absent: did we received burst/sync signal.  No burst usually means no color.
 *       No sync usually means no video.
 * ibstate: which field is being filled.  see abstate above.
 * nvram_clock/data: to read nvram (non-volatile memory), get the bits from
 *           the date port, each writing of one to the clock will give a
 *           fresh set of data.  To write to the nvram,
 *           place value to the data port, write one to the clock and repeat.
 *           No, no one likes it and yes, what a drag.
 *
 * sync/vtrace_intr_en : enabling the interruption.
 * showup: show video on sun screen
 * live525: show video to the output port
 *      CAVEAT: it is illegal to have both of them 1's, both 0's is OK.
 * live525: 0 == freeze, 1 == live
 * magic_intr: read from as the magic number, write to as interrupt vector.
 */
#define PLL_92_MHZ		0
#define PLL_135_MHZ		1
#define TV1_PLL_SHIFT		22
#define TV1_PLL_92_MHZ		(PLL_92_MHZ << TV1_PLL_SHIFT)
#define TV1_PLL_135_MHZ		(PLL_135_MHZ << TV1_PLL_SHIFT)

union general {
    struct {
	unsigned int:   7;
	unsigned int	nvram_enable:1;
	unsigned int:	2;
	unsigned int    pll:1;
	unsigned int    crystal:1;
	unsigned int    burst_absent:1;
	unsigned int    sync_absent:1;
	unsigned int	ibstate:3;
	unsigned int    nvram_data:2;
	unsigned int    sync_intr_en:1;
	unsigned int    vtrace_intr_en:1;
	unsigned int    showup:1;
	unsigned int    show525:1;
	unsigned int    live525:1;
	unsigned int    magic_intr:8;
    }               field;
    struct {
	unsigned int:   7;
	unsigned int	nvram_enable:1;
	unsigned int:	2;
	unsigned int    pll:1;
	unsigned int    crystal:1;
	unsigned int    burst_absent:1;
	unsigned int    sync_absent:1;
	unsigned int:	1;
	unsigned int	nvram_alt_enable:1; /* ? */
	unsigned int	nvram_clk:1;
	unsigned int    nvram_data:2;
	unsigned int    sync_intr_en:1;
	unsigned int    vtrace_intr_en:1;
	unsigned int    showup:1;
	unsigned int    show525:1;
	unsigned int    live525:1;
	unsigned int    magic_intr:8;
    }               field_write;
    unsigned int    packed;
};

struct csr {
    struct knob     knob;
    union control   control;
    char            fill1[0x20 - 0x10 - sizeof (union control)];
    union display   display;
    char            fill2[0x30 - 0x20 - sizeof (union display)];
    union general   general;
};

struct tv1 {
    unsigned int    fb[FB_SIZE / sizeof (unsigned int)];
    unsigned int    cp[CP_SIZE / sizeof (unsigned int)];
    char fill[CSR_BASE - CP_BASE - CP_SIZE];
    struct csr      csr;
};

#endif tv1reg_included
