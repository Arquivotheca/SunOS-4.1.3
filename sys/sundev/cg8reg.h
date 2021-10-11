/*  @(#)cg8reg.h	1.1 of 7/30/92, SMI */

/*
 *  cg8reg.h:   
 *      This file has two incarnations, one for the original P4 TC 
 *      (IBIS frame) buffer and one for various flavors of the 
 *      SBus-based TC Card. Which incarnation is presented is 
 *      dependent upon "sun4c" being defined, or not:
 *          if !sun4c use Sun IBIS definitions
 *          if sun4c use Sbus TC definitions
 */

#ifndef CG8REG_INCLUDED
#define CG8REG_INCLUDED

#if !defined(sun4c) && !defined(sun4m)
/* --------- Board Layout of Ibis 24-bit frame buffer ------------------*/

#include <machine/param.h>
#include <sundev/p4reg.h>
#include <sundev/ramdac.h>

/*
 * These are the physical offset from the beginning of the P4.
 */
#define P4IDREG     0x300000
/* constants define in p4reg.h */
#define DACBASE     (P4IDREG + P4_COLOR_OFF_LUT)
#define OVLBASE     (P4IDREG + P4_COLOR_OFF_OVERLAY)
#define ENABASE     (P4IDREG + P4_COLOR_OFF_ENABLE)
#define FBMEMBASE   (P4IDREG + P4_COLOR_OFF_COLOR)
#define PROMBASE    (P4IDREG + 0x8000)

/*
 * The device as presented by the "mmap" system call.  It seems to the mmap
 * user that the board begins, at its 0 offset, with the overlay plane,
 * followed by the enable plane and the color framebuffer.  At 8MB, there
 * is the ramdac followed by the p4 register and the boot prom.
 */
#define CG8_VADDR_FB    0
#define CG8_VADDR_DAC   0x800000
#define CG8_VADDR_P4REG (CG8_VADDR_DAC + ctob(1))
#define CG8_VADDR_PROM  (CG8_VADDR_P4REG + ctob(1))
#define PROMSIZE    0x40000

/*
 * Some sizes constants for reference only.  No one actually use them.
 */
#define CG8_WIDTH   1152           /* default width */
#define CG8_HEIGHT  900     /* default height */
#define PIXEL_SIZE  4              /* # of bytes per pixel in frame
			* buffer */
#define BITPERBYTE  8
#define FBSCAN_SIZE (CG8_WIDTH * PIXEL_SIZE)
#define OVLSCAN_SIZE (CG8_WIDTH / BITPERBYTE)

/* screen size in bytes */
#define FBMEM_SIZE  (FBSCAN_SIZE * CG8_HEIGHT)
#define OVL_SIZE    (OVLSCAN_SIZE * CG8_HEIGHT)
#define CG8_RAMDAC_OMAPSIZE     4
#define CG8_RAMDAC_CMAPSIZE     256

/*
 * Constants from <sundev/ramdac.h> which define the structure of 3
 * Brooktree 458 ramdac packed into one 32-bit register.
*/
#define CG8_RAMDAC_READMASK     RAMDAC_READMASK
#define CG8_RAMDAC_BLINKMASK        RAMDAC_BLINKMASK
#define CG8_RAMDAC_COMMAND      RAMDAC_COMMAND
#define CG8_RAMDAC_CTRLTEST     RAMDAC_CTRLTEST

/*
 * The following sessions describe the physical device.  No software
 * actually uses this model which for initial board bring-up and debugging
 * only.  Since the definitions of the structure take no space, we leave
 * them here for future references.
 */
union ovlplane {
    u_short         pixel[OVLSCAN_SIZE / sizeof (u_short)][CG8_HEIGHT];
    u_short         bitplane[OVL_SIZE / sizeof (u_short)];
};


struct overlay {
    union ovlplane  color;
    u_char          pad[ENABASE - OVLBASE - OVL_SIZE];
    union ovlplane  enable;
};


/* The whole board.  We defined fb to be linearly addressable, instead of a
two dimensional array.  Maybe we should use union? */
struct cg8_board {
    struct ramdac   lut;           /* start at P4BASE + DACBASE */
    u_char          pad1[P4IDREG - DACBASE - sizeof (struct ramdac)];
    u_int           p4reg;         /* p4 bus register */
    u_char          pad2[OVLBASE - P4IDREG - sizeof (u_int)];
    struct overlay  ovl;           /* overlay planes */
    u_char          pad3[FBMEMBASE - OVLBASE - sizeof (struct overlay)];
    union fbunit    fb[FBMEM_SIZE / sizeof (union fbunit)];
};


#else  
/* ------------------------ SBus TC card --------------------------------*/

/*
 *  Some initial definitions common to all versions of SBus TC Card
 */

#ifdef TC1REG_DEBUG
#include <sys/types.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#endif  TC1REG_DEBUG

#include <machine/param.h>

/*  Frame Buffer Attributes Description (from SBus TC Card on-board PROMs)
 */
typedef struct  {                   /* Fb attributes (from on-board PROM): */
    u_int   selection_enable:1;     /* 1 if there's a selection enable bit. */
    u_int   pip_possible:1;         /* 1 if pip option possible. */
    u_int   simultaneous_8_24:1;    /* 1 if separate luts for 8-bit & 24-bit. */ 
    u_int   monochrome:1;           /* 1 if monochrome frame buffer present. */
    u_int   selection:1;            /* 1 if selection memory present. */
    u_int   true_color:1;           /* 1 if true color frame buffer present. */
    u_int   eight_bit_hardware:1;   /* 1 if eight bit frame buffer present. */
    u_int   unused:9;               /* Unused flag area. */
    u_int   model:8;                /* Frame buffer model. */
    u_int   depth:8;                /* Default frame buffer's depth (= 1). */
}   FB_Attribute_Flags;

#define RASTEROPS_TC    1           /* Value "model": TC 1152x900 card. */
#define RASTEROPS_TCP   2           /* Value "model": TCP 1152x900 card. */
#define RASTEROPS_TCS   3           /* Value "model": TCS 640x480 card. */
#define RASTEROPS_TCL   4           /* Value "model": TCL 1280x1024 card. */

typedef union {                     /* Construction for fb attributes: */
    FB_Attribute_Flags  flags;      /* Representation as flags. */
    int                 integer;    /* Representation as integer. */
}   FB_Attributes;


/***************************************************************************
 * *********************************************************************** *
 * *                                                                     * *
 * * Board layout of SBus TCP Card  (24-bit frame buffer with            * *
 * * Picture in a Picture option.                                        * *
 * *                                                                     * *
 * *********************************************************************** *
 ***************************************************************************/

#define TC_OMAP_SIZE       4      /* Monochrome lookup table size. */
#define TC_CMAP_SIZE       256    /* 24-bit lookup table sizes. */
/*#define TC_RAMDAC_CMAPSIZE TC_CMAP_SIZE   /* Alias used by pixrect code. */

#define TCP_NPLL 16             /* Number of pll registers. */

typedef union {                   /* PLL register map: */
    u_char      vector[TCP_NPLL]; /* ... registers as a vector. */
    struct      {                 /* ... individual register names. */
	u_char  nh_low;           /* ... ... low 8 bits of NH register. */
	u_char  nh_mid_low;       /* ... ... next to low order 8 bits of NH */
	u_char  nh_mid_high;      /* ... ... next to high order 8 bits of NH */
	u_char  nh_high;          /* ... ... high order 8 bits of NH. */
	u_char  r_low;            /* ... ... low 8 bits of R register. */
	u_char  r_middle;         /* ... ... middle 8 bits of R register. */
	u_char  r_high;           /* ... ... high order 8 bits of R. */
	u_char  s;                /* ... ... s register. */
	u_char  l;                /* ... ... l register. */
	u_char  p;                /* ... ... p register. */
	u_char  ttl_lpf;          /* ... ... ttl/lpf register. */
	u_char  enable;           /* ... ... pll enable register. */
	u_char  vco_iring;        /* ... ... vco/iring register. */
	u_char  rl_clock;         /* ... ... rl clock register. */
	u_char  s_delay;          /* ... ... s delay register. */
	u_char  g_delay;          /* ... ... g delay register. */
    }           registers;
}   PLL_Regs;

/*
 *  Brooktree RAMDAC address layouts:
 */
typedef struct {                /* Brooktree 457 layout: */
    u_char  all_address;        /* All 3 RAMDACs' address registers. */
    u_char  all_color;          /* All 3 RAMDACs' color value registers. */
    u_char  all_control;        /* All 3 RAMDACs' control registers. */
    u_char  all_overlay;        /* All 3 RAMDACs' overlay value registers. */

    u_char  red_address;        /* Red gun RAMDAC's address register. */
    u_char  red_color;          /* Red gun RAMDAC's color value register. */
    u_char  red_control;        /* Red gun RAMDAC's control register. */
    u_char  red_overlay;        /* Red gun RAMDAC's overlay value register. */

    u_char  green_address;      /* Green gun RAMDAC's address register. */
    u_char  green_color;        /* Green gun RAMDAC's color value register. */
    u_char  green_control;      /* Green gun RAMDAC's control register. */
    u_char  green_overlay;      /* Green gun RAMDAC's overlay value register. */
    u_char  blue_address;       /* Blue gun RAMDAC's address register. */
    u_char  blue_color;         /* Blue gun RAMDAC's color value register. */
    u_char  blue_control;       /* Blue gun RAMDAC's control register. */
    u_char  blue_overlay;       /* Blue gun RAMDAC's overlay value register. */
} Bt457_Regs;

typedef struct {                        /* Brooktree 463 layout: */
    u_char  address_low;        /* Address register low order bits. */
    u_char  address_high;       /* Address register high order bits. */
    u_char  control;            /* Control/data register. */
    u_char  color;              /* Color palette register. */
    u_char  fill[12];           /* Match up to size of Bt 457 layout. */
} Bt463_Regs;

typedef struct {                /* Brooktree 473 layout: */
    u_char  ram_write_addr;     /* Address register: writes to color palette. */
    u_char  color;              /* Data register for color palette. */
    u_char  read_mask;          /* Pixel read mask register. */
    u_char  ram_read_addr;      /* Address register: reads from color palette */
    u_char  overlay_write_addr; /* Address register: writes overlay palette. */
    u_char  overlay;            /* Data register for overlay. */
    u_char  control;            /* Control register. */
    u_char  overlay_read_addr;  /* Address register: reads overlay palette. */
} Bt473_Regs;

typedef struct {                /* Venus frame buffer controller chip layout: */
    u_char  control1;           /* 00: control register 1. */
    u_char  control2;           /* 01: control register 2. */
    u_char  control3;           /* 02: control register 3. */
    u_char  control4;           /* 03: control register 4. */
    u_char  status;             /* 04: status register. */
    u_char  refresh_interval;   /* 05: refresh interval: */
    u_short io_config;          /* 06: general i/o config. */

    u_int   display_start;      /* 08: display start. */
    u_int   half_row_incr;      /* 0C: half row increment. */

    u_int   display_pitch;      /* 10: display pitch. */
    u_char  cas_mask;           /* 14: CAS mask. */
    u_char  horiz_latency;      /* 15: horizontal latency. */

    u_short horiz_end_sync;     /* 16: horizontal end sync. */
    u_short horiz_end_blank;    /* 18: horizontal end blank. */
    u_short horiz_start_blank;  /* 1A: horizontal start blank. */
    u_short horiz_total;        /* 1C: horizontal total. */
    u_short horiz_half_line;    /* 1E: horizontal half_line. */
    u_short horiz_count_load;   /* 20: horizontal count_load. */

    u_char  vert_end_sync;      /* 22: vertical end_sync. */
    u_char  vert_end_blank;     /* 23: vertical end_blank. */
    u_short vert_start_blank;   /* 24: vertical start blank. */
    u_short vert_total;         /* 26: vertical total. */
    u_short vert_count_load;    /* 28: vertical count_load. */
    u_short vert_interrupt;     /* 2a: vertical interrupt line. */

    u_short io_general;         /* 2c: general i/o. */
    u_char  y_zoom;             /* 2e: Y Zoom register. */
    u_char  soft_register;      /* 2f: soft register. */
} Venus_Regs;

#define VENUS_TCS_MODEL_A   0x8000 /* io_general: version A of tcs card. */
#define VENUS_TIMING_MASK   0x6000 /* io_general: timing dip switch values: */
#define VENUS_TIMING_NATIVE 0x0000 /* ...         native (apple) timing. */
#define VENUS_TIMING_NTSC   0x2000 /* ...         ntsc timing. */
#define VENUS_TIMING_PAL    0x4000 /* ...         pal timing. */
#define VENUS_NO_GENLOCK    0x0020 /* io_general: no genlock source. */
#define VENUS_SOFT_RESET    0x0010 /* io_general: software reset. */

#define VENUS_VERT_INT_ENA  1      /* control4: vertical interrupt enabled. */
#define VENUS_VERT_INT      1      /* status: vertical interrupt pending. */

/*
 * The device as presented by the "mmap" system call.  It seems to the mmap
 * user that the board begins, at its 0 offset, with the overlay plane,
 * followed by the enable plane and the color framebuffer.  At 8MB, there
 * is the ramdac followed by the p4 register and the boot prom.
 */
#define CG8_VADDR_FB    0
#define CG8_VADDR_DAC   0x800000

/*
 *  Layout of device registers for controlling the SBus TCS Card:
 */
typedef struct  {
    Venus_Regs  tc_venus;   /* Venus chip controlling true color. */
    u_char      fill1[0x040-sizeof(Venus_Regs)];    
    Venus_Regs  mono_venus; /* Venus chip controlling selection/monochrome. */
    u_char      fill2[0x040-sizeof(Venus_Regs)];    
    Bt473_Regs  dac;        /* Brootree 473 RAMDAC. */
} Tcs_Device_Map;

/*
 *
 *  Layout of device registers for controlling the SBus TC, TCP, and 
 *  TCL Cards
 *
 */
typedef struct  {
    struct mfb_reg  sun_mfb_reg;/* Standard Sun memory frame buffer registers */
    union                       /* RAMDAC support: */
    {
	Bt457_Regs  bt457;      /* ... Brooktree 457 (TC, TCP cards.) */
	Bt463_Regs  bt463;      /* ... Brooktree 463 (TCL cards.) */
    }        dacs;
    u_char   control_status;    /* TC control/status register 0. */
    u_char   control_status1;   /* TC control/status register 1. */
    u_char   fill1[14];         /* Unused. */
    u_short  x_source_start;    /* X signal start (subset of incoming signal) */
    u_short  x_source_end;      /* X signal end (subset of incoming signal). */
    u_short  x_source_scale;    /* X signal scaling:pixel drop rate multiplier*/
    u_short  fill2;             /* Unused. */
    u_short  y_source_start;    /* Y signal start (subset of incoming signal) */
    u_short  y_source_end;      /* Y signal end (subset of incoming signal). */
    u_short  y_source_scale;    /* Y signal scaling:scan line drop rate mult. */
    u_short  fb_pitch;          /* # pixels(visible & not) between scan lines */
    u_long   pip_start_offset;  /* Starting offset in fb to start PIP at. */
    u_char   control_status2;   /* TC control/status register 2. */
    u_char   fill3[11];         /* Unused. */
    PLL_Regs pll;               /* PLL registers. */
}   Tc1_Device_Map;

/*  Register "addresses" for Brooktree internal registers. These are written to 
 *  the address register (xxx_address) to select the internal register to be 
 *  accessed by reads or writes to the control register (xxx_control).
 */
#define BT457_BLINK_MASK    5   /* Blink mask register. */
#define BT457_COMMAND       6   /* Command register. */
#define BT457_CONTROL       7   /* Control / test register. */
#define BT457_READ_MASK     4   /* Read mask register. */

/*  Old style TC card control and status register bit definitions. 
 */
#define TC1_SELECTION_ENABLE    0x80    /* Turn on selection memory. */


/*  Control Status Register 0 definitions:
 */
#define TCP_CSR0_PIP_ONE_SHOT     0x40  /* Set to 1 for one-shot frame capture*/
#define TCP_CSR0_PIP_IS_ON        0x40  /* 1 on read: pip is turned on. */
#define TCP_CSR0_PIP_IS_ACTIVE    0x20  /* 1 on read: actively generat. images*/
#define TCP_CSR0_TURN_PIP_ON      0x20  /* Set to 1 for pip active, 0 to stop */
#define TCP_CSR0_PIP_INSTALLED    0x10  /* = 1 if pip present in system. */
#define TCP_CSR0_SOURCE_TYPE_MASK 0x03  /* Mask to get video source value. */
#define TCP_CSR0_COMPOSITE_SOURCE 0x00  /* ... source is composite (1 wire.) */
#define TCP_CSR0_S_VIDEO_SOURCE   0x01  /* ... source is s-video (2 wire.) */
#define TCP_CSR0_RGB_SOURCE       0x02  /* ... source is rgb (3 wire.) */

/*  Control Status Register 1 definitions:
 */
#define TCP_CSR1_I2C_DATA         0x80  /* I2C bus interface serial data bit. */
#define TCP_CSR1_I2C_CLOCK        0x40  /* I2C bus interface clock bit. */
#define TCP_CSR1_FIELD_ONLY       0x10  /* 1 to make image w/only 1 field. */ 
#define TCP_CSR1_NO_PIP           0x02  /* 1 if there is no pip present. */
#define TCP_CSR1_INPUT_CONNECTED  0x01  /* 1 if pip input connected. */

/*  Control Status Register 2 definitions:
 */
#define TCP_CSR2_ALTERNATE_PLL    0x01  /* 1 to use VCR PLL (slow timing.) */
#define TCP_CSR2_COUNT_DOWN       0x02  /* 1 if addr counts down (hor. flip.) */
#define TCP_CSR2_FIELD_INVERT     0x04  /* 1 to show odd before even field. */

/*  I-squared C bus device addresses and command definitions:
 */
#define I2C_ADDR_EEPROM (u_int)0xa0  /* Device address: eeprom for config. */
#define I2C_ADDR_DACS   (u_int)0x88  /* Device address: d to a converters. */

#define I2C_DAC0_CMD    (u_int)0x00  /* Command: d to a converter 0 control. */
#define I2C_DAC1_CMD    (u_int)0x01  /* Command: d to a converter 1 control. */
#define I2C_DAC2_CMD    (u_int)0x02  /* Command: d to a converter 2 control. */
#define I2C_DAC3_CMD    (u_int)0x03  /* Command: d to a converter 3 control. */

#define I2C_POD_CMD     (u_int)0x08  /* Command: port output data control. */
#define I2C_POD_NTSC    (u_int)0x00  /* Data: ntsc timing - port output data. */
#define I2C_POD_PAL     (u_int)0x14  /* Data: pal timing - port output data. */

#define I2C_BRIGHTNESS  I2C_DAC0_CMD /* Command: access brightness control. */
#define I2C_CONTRAST    I2C_DAC1_CMD /* Command: access contrast control. */
#define I2C_HUE         I2C_DAC3_CMD /* Command: access hue control. */
#define I2C_SATURATION  I2C_DAC2_CMD /* Command: access saturation control. */
#define I2C_HALF_LEVEL  (u_int)0x20  /* Data: half level for d/a converters. */

/*
 * 
 *  Layout of EEPROM Storage and Related Structures: 
 *
 */
typedef struct {                /* PIP Initialization record: */ 
    u_char      timing_mode;    /* ... timing mode specification. */ 
     
    u_char      brightness;     /* ... d to a converter brightness value. */ 
    u_char      contrast;       /* ... d to a converter contrast value. */ 
    u_char      saturation;     /* ... d to a converter saturation value. */ 
    u_char      hue;            /* ... d to a converter hue value. */ 
     
    u_char      fill;           /* ... *** UNUSED *** */
     
    u_short  x_source_start; /* ... starting pixel of signal in x direction. */
    u_short  x_source_end;   /* ... ending pixel of signal in x direction. */
    u_short  y_source_start; /* ... starting pixel of signal in y direction. */
    u_short  y_source_end;   /* ... ending pixel of signal in y direction. */
     
    u_char   rgb_brightness; /* ... d to a converter rgb brightness. */
    u_char   rgb_contrast;   /* ... d to a converter rgb brightness. */
    
    PLL_Regs pll;            /* ... phase lock loop initialization values. */
}   Pip_Init_Record; 
 
#define EEPROM_FACTORY -1   /* Use factory timing default (loc. 0 in eeprom). */
#define EEPROM_NTSC 0       /* Default timing mode is NTSC. */
#define EEPROM_PAL  1       /* Default timing mode is PAL. */
#define EEPROM_NUM_MODES EEPROM_PAL+1

typedef struct {                            /* EEPROM storage layout: */
    u_char          default_mode;           /* ... Default mode selection. */
    u_char          fill[15];               /* ... *** UNUSED *** */
    Pip_Init_Record mode[EEPROM_NUM_MODES]; /* ... NTSC, PAL timing defaults. */
    PLL_Regs        d1[EEPROM_NUM_MODES];   /* ... Digital 1 pll timing info. */
    PLL_Regs        d2[EEPROM_NUM_MODES];   /* ... Digital 2 pll timing info. */
}   EEPROM_Record;
 
#endif /* !sun4c */

#endif CG8REG_INCLUDED

