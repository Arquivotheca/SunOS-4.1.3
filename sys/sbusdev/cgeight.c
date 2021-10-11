#ifndef lint
static char         sccsid[] = "@(#)cgeight.c 1.1 92/07/30 SMI";
#endif

/******************************************************************************
 * ************************************************************************** *
 * *"cgeight.c                                                              * *
 * *                                                                        * *
 * *    Device driver for the True Color (TC) Family of Frame Buffers       * *
 * *    For The Sun Microsystems SBus.                                      * *
 * *                                                                        * *
 * *    Conventions:                                                        * *
 * *       (1) File is formatted assuming tab stops every four characters.  * *
 * *       (2) Routines are in alphabetical order.                          * *
 * *       (3) Prefix conventions for routine names:                        * *
 * *             (a) cgeightxxx - external entry point for driver.          * *
 * *             (b) cg8_xxx - internal pixrect related code.               * *
 * *             (c) pip_xxx - routine common to all pip implementations.   * *
 * *             (d) tc_xxx  - routine common to all hardware configurations* *
 * *             (e) tcp_xxx - routine for TC and TCP products.             * *
 * *             (f) tcs_xxx - routine for TCS product.                     * *
 * *                                                                        * *
 * *                  Copyright 1991, Sun Microsystems, Inc.                * *
 * ************************************************************************** *
 ******************************************************************************/

#include <sys/types.h>              /* General type defs. */
#include <sys/param.h>              /* General system parameters and limits. */
#include <sys/time.h>               /* General timer parameters. */

#include <sys/buf.h>                /* Input / Output buffer defs. */
#include <sys/errno.h>              /* Kernel error defs. */
#include <sys/ioccom.h>             /* Ioctl registery and macros. */
#include <sys/map.h>                /* Resource allocation map information. */
#include <sys/proc.h>               /* Process data structures and defs. */
#include <sys/user.h>               /* User management and accounting defs. */
#include <sys/vmmac.h>              /* Virtual memory related macros. */

#include <machine/eeprom.h>         /* Non-volatile ram software defs. */
#include <machine/enable.h>         /* Enable register defs. */
#include <machine/mmu.h>            /* Memory management unit defs. */
#include <machine/pte.h>            /* Page table entry defs. */
#include <mon/sunromvec.h>          /* Rom monitor interface definition. */

#include <pixrect/pixrect.h>        /* General pixrect related defs. */
#include <pixrect/pr_impl_util.h>   /* Pixrect implementatino utilities. */
#include <pixrect/pr_planegroups.h> /* Bit plane manipulation defs. */

#include <sun/fbio.h>               /* General frame buffer ioctl defs. */
#include <sundev/mbvar.h>           /* Main bus interface defs. */
#include <sbusdev/memfb.h>          /* SBus memory mapped frame buf layout. */
#include <sunwindow/cms.h>          /* Colormapseg, cms_map */

#include <pixrect/memvar.h>         /* Memory pixrect structural defs. */
#include <pixrect/cg4var.h>         /* Need some defs from this device. */
#include <sundev/cg8reg.h>          /* Device dependent memory layouts, etc. */
#include <pixrect/cg8var.h>         /* Pixrect related variables for our card */
#include <sunwindow/rect.h>         /* Definition of Rect data structure. */

#include    "win.h"                 /* # of windows(non-zero => sunview) */

/*
 *  Debugging equates and macros
 */
#ifndef TC_DEBUG
#define TC_DEBUG 0
#endif TC_DEBUG

#define TEST_NEW_HARDWARE 1

#if TC_DEBUG
int tc1_debug = 5;              /* Debugging level. */
#define DEBUGF(level, args)     _STMT(if (tc1_debug >= (level)) printf args;)
#define DEBUG_LEVEL(level) {extern int tc1_debug; tc1_debug = level;}
#else TC_DEBUG
#define DEBUGF(level, args)     /* nothing */
#define DEBUG_OFF
#define DEBUG_LEVEL(level)
#endif TC_DEBUG

/*
 *  Information telling how and when to memory map a frame buffer:
 */
typedef struct Mmap_Info {      /* Frame buffer memory map region info: */
    int     group;          /* Type of frame buffer to memory map. */
    u_int   bgn_offset;     /* Offset in vm of start of frame buffer. */
    u_int   end_offset;     /* Offset in vm of 1st word not in frame buffer. */
    u_int   sbus_delta;     /* Offset in SBus slot (from device regs) of fb. */
} Mmap_Info;

/*
 *  Per-Unit Device Data Definitions:
 */
typedef struct Tc1_Softc
{
    int     basepage;                 /* Physical page # of mmap base addr. */
#ifdef TEST_NEW_HARDWARE
    int     test_basepage;            /* Base page of slot 3 for testing h/w. */
#endif TEST_NEW_HARDWARE

    int     cmap_begin;               /* Starting index for color map load. */
    int     cmap_count;               /* # of entries to load. */
    u_char  cmap_blue[TC_CMAP_SIZE];  /* Blue components for color map. */
    u_char  cmap_green[TC_CMAP_SIZE]; /* Green components for color map. */
    u_char  cmap_red[TC_CMAP_SIZE];   /* Red components for color map. */

    Tc1_Device_Map  *device_reg;   /* Device register area virt. address. */
    int     dev_reg_mmap_offset;   /* Device regs memory offset from selves. */
    int     dev_reg_mmap_size;     /* Device register area size in bytes. */
    int     dev_reg_sbus_base;     /* Device register area SBus phys. offset */
    int     dev_reg_sbus_delta;    /* SBus offset from dev. regs. (=0). */
    Tcs_Device_Map  *device_tcs;   /* Device register are on TCS card. */

    u_char  em_blue[TC_CMAP_SIZE];  /* Indexed color map emulation : blue. */
    u_char  em_green[TC_CMAP_SIZE]; /* Indexed color map emulation : green. */
    u_char  em_red[TC_CMAP_SIZE];   /* Indexed color map emulation : red. */

    u_char  *fb_8bit;             /* Eight-bit buffer virt. address. */
    int     fb_8bit_mmap_size;    /* Size of region mmapped into virt memory */
    int     fb_8bit_sbus_delta;   /* Eight bit SBus offset from device regs. */

    u_char  fb_model;             /* Frame buffer model. */

    u_char  *fb_mono;             /* Overlay (monochrome) buffer virt. addr. */
    int     fb_mono_mmap_size;    /* Size of region mmapped into virt memory */
    int     fb_mono_sbus_delta;   /* Monochrome SBus offset from device regs */

    u_char  *fb_sel;              /* Enable (selection) memory virt. addr. */
    int     fb_sel_mmap_size;     /* Size of region mmapped into virt memory */
    int     fb_sel_sbus_delta;    /* Selection SBus offset from device regs. */

    long    *fb_tc;               /* True color buffer virt. address. */
    int     fb_tc_mmap_size;      /* Size of region mmapped into virt memory */
    int     fb_tc_sbus_delta;     /* True color SBus offset from device regs */

    u_char  *fb_video;            /* Video enable buffer virt. address. */
    int     fb_video_mmap_size;   /* Size of region mmapped into vir. memory */
    int     fb_video_sbus_delta;  /* SBus offset of video enable. */

    int     flags;                /* Miscellaneous flags. */
    int     height;               /* Height of display in scan lines. */
    int     linebytes1;           /* # of bytes in a monochrome scan line. */
    int     linebytes8;           /* # of bytes in an eight bit scan line. */
    int     linebytes32;          /* # of bytes in a true color scan line. */

    int     memory_map_end;       /* Total size of mapping of regions. */
    int     mmap_count;           /* # of entries in use in mmap_info. */
    Mmap_Info mmap_info[5];       /* Information on how to mmap fbs. */

    int     omap_begin;              /* Starting index for overlay map load. */
    int     omap_count;              /* # of entries to load. */
    u_char  omap_blue[TC_OMAP_SIZE];  /* Monochrome overlay color map: blue */
    u_char  omap_green[TC_OMAP_SIZE]; /* Monochrome overlay color map: green */
    u_char  omap_red[TC_OMAP_SIZE];   /* Monochrome overlay color map: red */

    u_char  pg_possible[FB_NPGS];    /* Plane groups which are possible. */
    u_char  pg_enabled[FB_NPGS];     /* Plane groups enabled for use. */

    int     pip_on_off_semaphore;    /* -1 if on, else # of suspensions */
    u_char  sw_cursor_color_frozen;  /* Berthold: cursor color is frozen. */

#if NWIN > 0
    Pixrect         pr;              /* Kernel pixrect for this unit. */
    struct cg8_data tc1d;            /* Frame buffer specific data. */
#endif NWIN > 0

    int             timing_regimen;  /* Current timing regimen (TCS card.) */
    int             width;           /* Width of display in pixels. */
} Tc1_Softc;

#define SOFTC register Tc1_Softc

/* Forward references for kernel routines used by this device driver: */

addr_t kmem_zalloc();
addr_t map_regs();
void   report_dev();

/* Compilation configuration switches and macros: */

#ifndef OLDDEVSW
#define OLDDEVSW 1
#endif  OLDDEVSW

#ifndef NOHWINIT
#define NOHWINIT 0
#endif NOHWINIT

#if NOHWINIT
int tc1_hwinit = 1;
int tc1_small = 0;
int tc1_on = 1;         /* Must be 1 to enable tc1 */
#endif NOHWINIT

#define TC_CMAP_ENTRIES        MFB_CMAP_ENTRIES

#if OLDDEVSW
#define STATIC /* nothing */
#else OLDDEVSW
#define STATIC static
#endif OLDDEVSW

/* Configuration information (device driver operations vector) */

static int cgeightattach();
STATIC int cgeightclose();
static int cgeightindentify();
STATIC int cgeightioctl();
STATIC int cgeightmmap();
STATIC int cgeightopen();
static int cgeightpoll();

struct dev_ops cgeight_ops = {
    0,                  /* revision */
    cgeightindentify,       /* Device driver indentification routine. */
    cgeightattach,         /* Attach this device as a slave. */
    cgeightopen,           /* Open the device. */
    cgeightclose,          /* Close the device. */
    0, 0, 0, 0, 0,
    cgeightioctl,          /* Perform a control operation. */
    0,
    cgeightmmap,           /* Map a page of the device's address space. */
};

/*  
 *  Information kept on a per-unit basis for our device type. 
 */
static int       num_tc;         /* # of frame buffers present. */
static Tc1_Softc *tc1_softc;     /* Array of per unit data structures. */

#define TC_FB_ATTR 0x9C000001    /* Original SBus TC frame buffer attributes. */

/* Handy macros:
 */
#define SOFTC register Tc1_Softc    /* Short hand for oft-used declaration. */

#define getsoftc(unit)  (&tc1_softc[unit])

#define btob(n)         ctob(btoc(n))           /* XXX */

#define BCOPY(s,d,c)    bcopy((caddr_t) (s), (caddr_t) (d), (u_int) (c))
#define COPYIN(s,d,c)   copyin((caddr_t) (s), (caddr_t) (d), (u_int) (c))
#define COPYOUT(s,d,c)  copyout((caddr_t) (s), (caddr_t) (d), (u_int) (c))

/* Forward  routine references:
 */
static int tcp_attach();
static int tcs_attach();

/*  Default data structures for various ioctl requests. */

static 
struct fbgattr tc1_attrdefault={ /* Default for FBIOGATTR ioctl: */
    FBTYPE_MEMCOLOR,             /* .  Actual type. */
    0,                           /* .  Owner. */
    {                            /* .  Frm buffer type(matchs tc1typedefault)*/
	FBTYPE_MEMCOLOR,         /* .  .  Type. */
	0,                       /* .  .  Height. */
	0,                       /* .  .  Width. */
	32,                      /* .  .  Depth. */
	256,                     /* .  .  Color map size. */
	0                        /* .  .  Size. */
    },                           /* .  .  */
    {                            /* .  Frame buffer attributes (flags): */
	FB_ATTR_AUTOINIT,        /* .  .  Flags. */
	FBTYPE_SUN2BW,           /* .  .  Emu type. */
	{ 0 }                    /* .  .  Device specific information. */
    },                           /* .  . */   
    {                            /* .  Emu types. */ 
      FBTYPE_MEMCOLOR,
      FBTYPE_SUN2BW,
      -1,
      -1
    } 
};

static 
struct fbgattr cg4_attrdefault={ /* Default for FBIOGATTR ioctl in cg4 mode: */
    FBTYPE_SUN4COLOR,            /* .  Actual type. */
    0,                           /* .  Owner. */
    {                            /* .  Frm buffer type(match tc1typedefault) */
	FBTYPE_SUN4COLOR,        /* .  .  Type. */
	0,                       /* .  .  Height. */
	0,                       /* .  .  Width. */
	8,                       /* .  .  Depth. */
	256,                     /* .  .  Color map size. */
	0                        /* .  .  Size. */
    },                           /* .  .  */
    {                            /* .  Frame buffer attributes (flags): */
	FB_ATTR_AUTOINIT,        /* .  .  Flags. */
	FBTYPE_SUN2BW,           /* .  .  Emu type. */
	{ 0 }                    /* .  .  Device specific information. */
    },                           /* .  . */   
    {                            /* .  Emu types. */
	FBTYPE_SUN4COLOR,
	FBTYPE_SUN2BW,
	-1,
	-1
    } 
};

static 
struct fbtype tc1typedefault={  /* Default for FBIOGTYPE ioctl for cg4, cg8: */
    FBTYPE_SUN2BW,              /* .  Type. */
    0,                          /* .  Height. */
    0,                          /* .  Width. */
    1,                          /* .  Depth. */
    0,                          /* .  Color map size. */
    0                           /* .  Size. */
};

static int      cmsize = -1;

#if NWIN > 0

/*  SunWindows related: Kernel pixrects operations vector definition. 
 */
static int cg8_ioctl();
static int cg8_putattributes();     
static int cg8_putcolormap();
static int cg8_rop();

static 
struct pixrectops tcp_ops={/* Pip option present kernel pixrect ops vector: */
    cg8_rop,              /* . Raster operation. */
    cg8_putcolormap,      /* . Write the color map. */
    cg8_putattributes,    /* . Set the attributes. */
    cg8_ioctl             /* . Perform an ioctl operation. */
};


#endif NWIN > 0

/*
 *  Table to translate Sun overlay index into a SBus TC overlay index.
 *
 *  Note that our overlay map works in a different manner from Sun's cg8, so a
 *  translation must be done (using translate_omap data structure):
 *   Function                   Sun LUT index   SBus TC LUT index
 *   --------                   -------------   ------------------
 *   True color                       0                  0
 *   Window system foreground         1                  2
 *   Current window foreground        2                  1
 *   Window system background         3                  3
 *
 */
static 
 u_char translate_omap[] = /* Translate caller's omap index to match hardware */
    { 0, 2, 1, 3};

/*"cgeightattach"
 *
 *  Perform device specific initializations. (Called during autoconfiguration 
 *  process.) This routine allocates the per-unit data structure associated 
 *  with the device and initializes it from the properties exported by the 
 *  on-board PROM. In addition it maps the device's memory regions into 
 *  virtual memory and performs hardware specific initializations.
 *
 *      = 1 if success
 *      = 0 if failure
 *
 */
static cgeightattach(devi)
    struct dev_info *devi; /* ->dev info struct w/node id...(rest filled here)*/
{
    FB_Attributes   fb_attr;      /* Attributes about fb from on-board PROM. */
    SOFTC           *softc;       /* Pointer to per-unit data for this device */
    static int      unit;         /* Unit # to use.(NOTE VALUE INCREMENTS!) */

    DEBUGF(1, ("cgeightattach num_tc=%d unit=%d\n", num_tc, unit));

    
    /*  If this is the first time attach has ever been called, allocate 
     *  softc structures for the frame buffers.
     */       
    if (!tc1_softc)
    {
	tc1_softc = (struct Tc1_Softc *) 
	    kmem_zalloc((u_int) sizeof (Tc1_Softc) * num_tc);
    }
    softc = getsoftc(unit);

    
    
    /*  SET UP PROPERTIES RELATED TO DEVICE REGISTER AREA AND MEMORIES
     *
     *  Get properties from the on-board PROM (set up by Forth code at ipl time)
     *  Fill in the switches related to them. (Note we default enabled to 
     *  possible here, but enabled bits are related to the type of device 
     *  being emulated.)
     */
    fb_attr.integer = (int)getprop(devi->devi_nodeid, "depth", TC_FB_ATTR); 
    if ( fb_attr.integer <= 1 ) fb_attr.integer = TC_FB_ATTR;
    DEBUGF(5, (" PROM frame buffer attributes are %x\n", fb_attr.integer));
    softc->pg_possible[PIXPG_OVERLAY] = fb_attr.flags.monochrome;
    softc->pg_possible[PIXPG_OVERLAY_ENABLE] = fb_attr.flags.selection;
    softc->pg_possible[PIXPG_8BIT_COLOR] = fb_attr.flags.eight_bit_hardware;
    softc->pg_possible[PIXPG_24BIT_COLOR] = fb_attr.flags.true_color;
    softc->pg_possible[PIXPG_VIDEO_ENABLE] = fb_attr.flags.pip_possible;
    BCOPY(softc->pg_possible, softc->pg_enabled, FB_NPGS);

    softc->fb_model = fb_attr.flags.model;
    if ( softc->fb_model == 0 )
    {
	softc->fb_model = ( softc->pg_possible[PIXPG_8BIT_COLOR] ) ? 
				RASTEROPS_TCP : RASTEROPS_TC;
    }
    softc->timing_regimen = NATIVE_TIMING;

    /*  Get display image dimensions, and number of bytes between rows in a 
     *  1-bit pixel scan line. Use this information to generate the inter-row 
     *  spacing for 8-bit and 32-bit scan lines.
     */
    softc->width =       getprop(devi->devi_nodeid, "width", 1152);
    softc->height =      getprop(devi->devi_nodeid, "height", 900);
    softc->linebytes1 =  getprop(devi->devi_nodeid, "linebytes", 
				    mpr_linebytes(1152, 8));
    softc->linebytes8 =  softc->linebytes1 * 8;
    softc->linebytes32 = softc->linebytes1 * 32;

    /*  PERFORM BOARD SPECIFIC PROCESSING BASED ON THE TYPE OF BOARD SPECIFIED
     *  
     *  The depth attribrute from the PROMs specifies the SBus Card model
     *  being used. Use this information to perform model specific 
     *  initializations. A return code of zero indicates an error occurred, in 
     *  which case this routine should signal a failure. 
     */
    switch(softc->fb_model)
    {
      default:
      case RASTEROPS_TC:
      case RASTEROPS_TCP:
	if ( tcp_attach(devi, softc) == 0 ) return 0;
	break;
      case RASTEROPS_TCS:
	if ( tcs_attach(devi, softc) == 0 ) return 0;
	break;
    }

    /*  REGISTER OUR DEVICE
     *
     *  Save unit number in device information area we were passed. 
     *  Attach interrupt routine. Save back pointer to softc in device
     *  information area. Increment the unit number for the next attach,
     *  and report the device.
     */
    devi->devi_unit = unit;
    addintr(devi->devi_intr[0].int_pri, cgeightpoll, devi->devi_name, unit);
    devi->devi_data = (addr_t) softc;

    unit++;
    report_dev(devi);

    return 1;
}


/*"cgeightclose"
 *
 *  Close down operations on a SBus TC board
 *
 *      = 0
 */
/*ARGSUSED*/
STATIC cgeightclose(dev, flag)
    dev_t dev;    /* = device specification for device to be closed. */
    int flag;     /* = read/write flags (unused in this routine. */ 
{
    DEBUGF(2, ("cgeightclose(%d)\n", minor(dev)));

    return 0;
}

  
/*"cgeightindentify"
 *
 *  Check if a name matches that of our device, and if so update the
 *  count of devices we maintain.
 *
 *      = number of devices present 
 *      = 0 if name passed does not match, or this device type is disabled
 */
static cgeightindentify(name)
    char *name; /* -> name to be tested. */
{
    DEBUGF(1, ("cgeightindentify(%s)\n", name));

#   if NOHWINIT
	if (!tc1_on)
	    return 0;
#   endif NOHWINIT

    return (strcmp(name, "cgeight") == 0) ? ++num_tc : 0;
}

/*"cgeightioctl"
 *
 *  Perform an input / output control operation on a SBus TC card
 *  or associated driver code. 
 *
 *    =  0 if successful operation
 *    ~= 0 if error detected
 */
cgeightioctl(dev, cmd, data, flag)
    dev_t   dev;    /* =  device specification for device to operate upon. */
    int     cmd;    /* =  opcode for operation to be performed. */
    caddr_t data;   /* -> data area for the operation . (may be in or out). */
    int     flag;   /* =  modifier info. for operation to be performed. */
{
    SOFTC   *softc = getsoftc(minor(dev));  /* Pointer to per-unit info. */

#   ifdef TC_DEBUG
	DEBUGF(5,  ("tconeioctl: cmd = "));
	switch (cmd) {
	    case FBIOSATTR: 
		DEBUGF(5, ("FBIOSATTR\n")); break;
	    case FBIOGATTR: 
		DEBUGF(5, ("FBIOGATTR\n")); break;
	    case FBIOGETCMAP: 
		DEBUGF(6, ("FBIOGETCMAP\n")); break;
	    case FBIOPUTCMAP: 
		DEBUGF(6, ("FBIOPUTCMAP\n")); break;
	    case FBIOGTYPE: 
		DEBUGF(5, ("FBIOGTYPE\n")); break;
	    case FBIOGPIXRECT: 
		DEBUGF(5, ("FBIOGPIXRECT\n")); break;
	    case FBIOSVIDEO: 
		DEBUGF(5, ("FBIOSVIDEO\n")); break;
	    case FBIOGVIDEO: 
		DEBUGF(5, ("FBIOGVIDEO\n")); break;
	    case PIPIO_G_CURSOR_COLOR_FREEZE: 
		DEBUGF(5, ("PIPIO_G_CURSOR_COLOR_FREEZE\n")); break;
	    case PIPIO_G_EMULATION_MODE: 
		DEBUGF(5, ("PIPIO_G_EMULATION_MODE\n")); break;
	    case PIPIO_G_FB_INFO: 
		DEBUGF(5, ("PIPIO_G_FB_INFO\n")); break;
	    case PIPIO_S_CURSOR_COLOR_FREEZE: 
		DEBUGF(5, ("PIPIO_S_CURSOR_COLOR_FREEZE\n")); break;
	    case PIPIO_S_EMULATION_MODE: 
		DEBUGF(5, ("PIPIO_S_EMULATION_MODE\n")); break;
	    default: 
		DEBUGF(5, ("not supported (0x%x)\n", cmd)); break;
	}
#   endif TC_DEBUG

    /*  Process request based on its type. Cases are alphabetical.
     */
    switch (cmd)
    {
	case FBIOGATTR:               
	    return x_g_attr( softc, (struct  fbgattr *)data );
	case FBIOGETCMAP:             
	    return x_getcmap( softc, (struct fbcmap  *)data );
#if NWIN > 0
	case FBIOGPIXRECT:            
	    return x_g_pixrect( softc, dev, (struct fbpixrect *)data );
#endif NWIN > 0
	case FBIOGTYPE:               
	    return x_g_type( softc, (struct fbtype  *)data );
	case FBIOGVIDEO:
	    switch(softc->fb_model)
	    {
	      default:
		*(int *)data = mfb_get_video(&softc->device_reg->sun_mfb_reg) ? 
		    FBVIDEO_ON : FBVIDEO_OFF;
		break;
	      case RASTEROPS_TCS:
		break;
	    }
	    break;
	case FBIOPUTCMAP:             
	    switch(softc->fb_model)
	    {
	      default:
		return tc_putcmap( softc, (struct fbcmap *)data );
	      case RASTEROPS_TCS:
		return tcs_putcmap( softc, (struct fbcmap *)data );
	    }    
	case FBIOSVIDEO:
	    switch(softc->fb_model)
	    {
	      default:
		mfb_set_video(&softc->device_reg->sun_mfb_reg, 
		    *(int *)data & FBVIDEO_ON);
		break;
	      case RASTEROPS_TCS:
		break;
	    }
	    break;

	/*  Ioctls placed under the guise of the PIP which should always be 
	 *  handled.
	 */
	case PIPIO_G_CURSOR_COLOR_FREEZE:
	    *(int *)data = softc->sw_cursor_color_frozen;
	    break;
	case PIPIO_G_EMULATION_MODE:
	    return x_g_emulation_mode( softc, (Pipio_Emulation *)data );
	case PIPIO_G_FB_INFO:
	    return x_g_fb_info( softc, (Pipio_Fb_Info   *)data );
	case PIPIO_S_CURSOR_COLOR_FREEZE:
	    softc->sw_cursor_color_frozen = *(int *)data;
	    break;
	case PIPIO_S_EMULATION_MODE:
	    return x_s_emulation_mode( softc, (Pipio_Emulation *)data );
#ifdef TEST_NEW_HARDWARE
	case PIPIO_S_MAP_SLOT:
	    return x_s_map_slot( softc, (u_int    *)data );
#endif TEST_NEW_HARDWARE

	/*- Unknown request, see if the pip knows about it... 
	 */
	default:      
#ifndef NO_PIP
	    if ( (pip_ioctl(softc, cmd, data, flag) != 0) )
	    {
		DEBUGF(5, ("not supported (0x%x)\n", cmd)); 
		return EINVAL;
	    }
#else NO_PIP
	    return EINVAL;
#endif !NO_PIP
    } /* switch(cmd) */
    return 0;
}

/*"cgeightmmap"
 *
 *  Provide physical page address associated with a memory map offset. This
 *  routine determines which of the six discontiguous physical memory
 *  regions the offset passed to it is associated with. Based on that 
 *  determination a physical page address is calculated and returned.
 *
 *      = PTE subset for the page in question
 *      = -1 if off is not within the memory space for one of the frame buffers
 *
 *  Notes:
 *      (1) The value given by "off" is relative to the device register area
 *          (since its address was given in the map_regs call to allocate 
 *          virtual memory for the mapping.)
 *      (2) See the description "Physical to virtual memory mapping for devices 
 *          on the tc1" near the end of this file for further details of the 
 *          mapping process.
 */
/*ARGSUSED*/
STATIC cgeightmmap(dev, off, prot)
    dev_t           dev;  /* = device which is to be memory mapped. */
    register off_t  off;  /* = offset into memory map for page to be mapped. */
    int             prot; /* (unused) */
{
    int         mmi;        /* Index: # of mmap_info entry examining. */
    Mmap_Info   *mmp;       /* Cursor: mmap_info entry examining. */
    off_t       phys_off;   /* Physical offset from base page of page. */
    SOFTC   *softc =        /* Pointer to per-unit info. */
	getsoftc(minor(dev));  

    DEBUGF(off ? 9 : 1, ("cgeightmmap(%d, 0x%x)\n", minor(dev), (u_int) off));

#ifdef TEST_NEW_HARDWARE
    if ( off >= 0x00900000 )
    {
	if ( softc->test_basepage == 0 ) return -1;
	phys_off = (off-0x900000);
	printf("Returning offset %x as phys_off %x and address %x\n", 
	    off, phys_off, softc->test_basepage+btop(phys_off));
	return softc->test_basepage + btop(phys_off);
    }
#endif TEST_NEW_HARDWARE

    /* It's not us, it's an interesting approach, but its not us! 
     */
    if ( (off < 0) || (off > softc->memory_map_end) )
    {
	return -1;
    }

    /*  From "off" determine which physical memory is being referenced. 
     *  Recalculate "off" as a physical offset relative to base page (that was 
     *  mapped with map_regs). 
     */
    if ( off >= softc->dev_reg_mmap_offset )                /* DEVICE MAP */
    {
	phys_off = (off-softc->dev_reg_mmap_offset) + softc->dev_reg_sbus_delta;
	return softc->basepage + btop(phys_off);
    }

    for ( mmi=0, mmp=softc->mmap_info; mmi < softc->mmap_count; mmi++, mmp++ )
    {
	if ( off >= mmp->bgn_offset && off < mmp->end_offset )
	{
	    phys_off = (off - mmp->bgn_offset) + mmp->sbus_delta;
#if TC_DEBUG
	    DEBUGF((mmp->group==PIXPG_OVERLAY && 
		    off==mmp->bgn_offset) ? 1 : 9,
		    ("cgeightmmap returning 0x%x (0x%x)\n", 
		    ptob(softc->basepage)+phys_off, 
		    softc->basepage+btop(phys_off)));
	    if ( off == mmp->bgn_offset )
	    {
		switch (mmp->group)
		{
		  case PIXPG_8BIT_COLOR:
		    printf("cgeightmmap returning eight bit color");
		    break;
		  case PIXPG_OVERLAY:
		    printf("cgeightmmap returning monochrome");
		    break;
		  case PIXPG_OVERLAY_ENABLE:
		    printf("cgeightmmap returning selection");
		    break;
		  case PIXPG_24BIT_COLOR:
		    printf("cgeightmmap returning true color");
		    break;
		  case PIXPG_VIDEO_ENABLE:
		    printf("cgeightmmap returning video enable");
		    break;
		}
		printf(" start at 0x%x (0x%x)\n",
		ptob(softc->basepage)+phys_off, 
		softc->basepage+btop(phys_off));
	    }
#endif TC_DEBUG
	    return softc->basepage + btop(phys_off);
	}
    }

    return -1;
}

/*"cgeightopen"
 *
 *  Open a SBus TC board "device" for use.
 *
 *  Parameters:
 *      dev   = device to be opened
 *      flag  = flags indicating access method (e.g., read, write)
 *
 *  Function value:
 *      = return code from frame buffer driver open routine
 */
STATIC cgeightopen(dev, flag)
   dev_t dev;  /* =  device specification giving what device instance to open */
   int   flag; /* =  flags indicating access method (e.g., read, write). */
{
#ifdef LOADABLE
    extern  dev_t   fbdev;      /* Unit # of /dev/fb. */
#endif LOADABLE

    DEBUGF(2, ("cgeightopen(%d,%d)\n", major(dev), minor(dev)));

#ifdef LOADABLE
    /*  This driver is dynamically loaded, so if the value for /dev/fb has
     *  not been set, make ourselves the default frame buffer. NOTE hardwire
     *  of /dev/fb's major device number!
     */
    if ( (fbdev < 1) || (fbdev == 22) )
    {
	fbdev = dev;
    }
#endif LOADABLE

    return fbopen(dev, flag, num_tc);
}

/*"cgeightpoll"
 *
 *  Interrupt service routine for vertical retrace interrupts
 *  on this device type. Loop through each device instance
 *  looking for one with an interrupt pending.
 *
 *      = number of interrupts serviced
 */
static cgeightpoll()
{
    Tcs_Device_Map  *device_tcs; /* Poniter to tcs device register area. */
    int             instance;    /* Index: instance now examining interrupt on*/
    int             serviced;    /* Number of interrupts serviced. */
    SOFTC           *softc;      /* Pointer to characteristics for our unit. */
    
    serviced = 0;
    for (softc = tc1_softc, instance = num_tc; --instance >= 0; softc++)
    {
	switch(softc->fb_model)
	{
	  default:
	    if ( mfb_int_pending((struct mfb_reg *)softc->device_reg) )
	    {
		mfb_int_disable( ((struct mfb_reg *)softc->device_reg) );
		serviced++;
	    }
	    break;
	  case RASTEROPS_TCP:
	  case RASTEROPS_TC:
	    mfb_int_disable( ((struct mfb_reg *)softc->device_reg) );
	    serviced++;
	    break;

	  case RASTEROPS_TCS:
	    device_tcs = softc->device_tcs;
	    if ( device_tcs->tc_venus.status & VENUS_VERT_INT )
	    {
		tcs_update_cmap(softc);
		device_tcs->tc_venus.control4 = 0;
		device_tcs->tc_venus.status = 0;
		serviced++;
	    }
	    break;
	}
    }

    return serviced;
}

/*"cg8_ioctl"
 */
static int cg8_ioctl(pr, cmd, data)
    Pixrect        *pr;
    int             cmd;
    caddr_t         data;
{
    static  int     save_window_fd;

    switch (cmd)
    {
      case FBIOGPLNGRP:
	*(int *) data = PIX_ATTRGROUP(cg8_d(pr)->planes);
	break;

      case FBIOAVAILPLNGRP:
	{
	    static int      cg4groups =
	    MAKEPLNGRP(PIXPG_OVERLAY) |
	    MAKEPLNGRP(PIXPG_OVERLAY_ENABLE) |
	    MAKEPLNGRP(PIXPG_24BIT_COLOR);

	    struct cg8_data *cg8d;
	    int    fbi;
	    int    group_mask = 0;

	    cg8d = cg8_d(pr);
	    for (fbi = 0; fbi < cg8d->num_fbs; fbi++)
	    {
		group_mask |= MAKEPLNGRP(cg8d->fb[fbi].group);
	    }
	    *(int *) data = group_mask;
	    break;
	}

      case FBIO_FULLSCREEN_ELIMINATION_GROUPS:
	{
	    struct cg8_data *cg8d;
	    char            *elim_groups = (char *)data;

	    bzero(elim_groups, PIXPG_LAST_PLUS_ONE);
	    if ( cg8d->flags & CG8_EIGHT_BIT_PRESENT )
	    {
		elim_groups[PIXPG_24BIT_COLOR] = 1;
	    }
	    else
	    {
		elim_groups[PIXPG_8BIT_COLOR] = 1;
	    }
	}
	break;

      case FBIOGCMSIZE:
	if (cmsize < 0)
	return -1;
	*(int *) data = cmsize;
	break;

      case FBIOSCMSIZE:
	cmsize = *(int *) data;
	break;

      case FBIOSCMS:
	cg8_d(pr)->cms = *(struct colormapseg *) data;
	break;

      case FBIOSWINFD:
	cg8_d(pr)->windowfd = *(int *) data;
	break;

      case FBIOSAVWINFD:
	if (cg8_d(pr)->windowfd >= -1)
	{
	save_window_fd = cg8_d(pr)->windowfd; 
	cg8_d(pr)->windowfd = -2; 
	}
	break; 

      case FBIORESWINFD:
	cg8_d(pr)->windowfd = save_window_fd; 
	break; 
			 
      default:
	return ENOTTY;
    }
    return 0;
}

/*"cg8_putattributes"
 */
static int cg8_putattributes (pr, planesp)
    Pixrect        *pr;         /* -> pixrect to set plane group of. */
    u_int          *planesp;    /* -> encoded group and plane mask */
{
    struct cg8_data *cg8d;         /* private data */
    int             dont_set_planes;
    u_int           planes,
		    group;

    dont_set_planes = *planesp & PIX_DONT_SET_PLANES;
    planes = *planesp & PIX_ALL_PLANES;/* the plane mask */
    group = PIX_ATTRGROUP(*planesp);  /* extract the group part */

    cg8d = cg8_d (pr);

    /*
     * User is trying to set the group to something else. We'll see if the
     * group is supported.  If does, do as he wishes.
     */
    if (group != PIXPG_CURRENT || group == PIXPG_INVALID) 
    {
	int   active = 0,
	      found = 0,
	      cm_size;

	/* kernel should not access the color frame buffer */

	if (group == PIXPG_TRANSPARENT_OVERLAY)
	{
	    group = PIXPG_OVERLAY;
	    cg8d->flags |= CG8_COLOR_OVERLAY;
	}
	else 
	{
	    cg8d->flags &= ~CG8_COLOR_OVERLAY;
	}

	for (; active < cg8d->num_fbs && !found; active++)
	{
	    if (found = (group == cg8d->fb[active].group)) 
	    {
		cg8d->mprp = cg8d->fb[active].mprp;
		cg8d->planes = PIX_GROUP (group) | 
			(cg8d->mprp.planes & PIX_ALL_PLANES);
		cg8d->active = active;
		pr->pr_depth = cg8d->fb[active].depth;
		switch (group)
		{
		  default:
		  case PIXPG_8BIT_COLOR:    
		  case PIXPG_24BIT_COLOR:
			cm_size = TC_CMAP_SIZE; 
		    if (cg8d->flags&CG8_PIP_PRESENT) cg8d->flags|=CG8_STOP_PIP; 
		    break;
		  case PIXPG_OVERLAY:       
		    cm_size = 4;    
		    cg8d->flags &= ~CG8_STOP_PIP;
		    break;
		  case PIXPG_OVERLAY_ENABLE:    
		    cm_size = 0;     
		    cg8d->flags &= ~CG8_STOP_PIP;
		    break;
		  case PIXPG_VIDEO_ENABLE:
		    cm_size = 0;    
		    if (cg8d->flags&CG8_PIP_PRESENT) cg8d->flags|=CG8_STOP_PIP; 
		    break;
		}
		cg8_ioctl(pr, FBIOSCMSIZE, (caddr_t) &cm_size);
	    }
	}
    }

    /* group is PIXPG_CURRNT here */
    if (!dont_set_planes) {
    cg8d->planes =
	cg8d->mprp.planes =
	cg8d->fb[cg8d->active].mprp.planes =
	PIX_GROUP(group) | planes;
    }

    return 0;
}

/*"cg8_putcolormap"
 */
static int cg8_putcolormap(pr, index, count, red, green, blue)
    Pixrect        *pr;
    int             index,
		    count;
    unsigned char  *red,
		   *green,
		   *blue;
{
    struct cg8_data     *cg8d;
    struct fbcmap       fbmap;
    int                 cc,i,plane;
 
    /*
     * Set "cg8d" to the cg8 private data structure.  If the plane
     * is not specified in the index, then use the currently active
     * plane.  ### Does this conflict with the cg4 model?
     */
 
    cg8d = cg8_d(pr);
    if (PIX_ATTRGROUP(index))
    {
	plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
	index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;
    }
    else
	plane = cg8d->fb[cg8d->active].group;
 
    /*  If updates are to be forced, or the plane groups are 8 or 24 set
     *  the colormap via ioctl. Otherwise, use the emulation approach.
     */
 
    if (index & PR_FORCE_UPDATE || 
	plane == PIXPG_24BIT_COLOR || plane == PIXPG_8BIT_COLOR)
    {
	cg8d->flags |= CG8_KERNEL_UPDATE;       /* BCOPY vs. COPYIN */
	fbmap.index = index | PIX_GROUP(plane);
	fbmap.count = count;
	fbmap.red = red;
	fbmap.green = green;
	fbmap.blue = blue;
	return cgeightioctl(cg8d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
    }
    else        /* emulated index */
    {
	if (plane == PIXPG_OVERLAY_ENABLE) return 0;
 
	if (plane == PIXPG_OVERLAY)
	{
	    for (cc=i=0; count && !cc; i++,index++,count--)
	    {
		cg8d->flags |= CG8_KERNEL_UPDATE;       /* BCOPY vs. COPYIN */
		/* Index 0 is mapped to 1.  All others mapped to 3. */
		fbmap.index = (index ? 3 : 1) | PIX_GROUP(plane);
		fbmap.count = 1;
		fbmap.red = red+i;
		fbmap.green = green+i;
		fbmap.blue = blue+i;
		cc = cgeightioctl(cg8d->fd, FBIOPUTCMAP, (caddr_t) &fbmap, 0);
	    }
	    return cc;
	}
    }
    return PIX_ERR;
}

/* Macros to change a cg8 pixrect into a memory pixrect */

#define PR_TO_MEM(src, mem)                     \
    if (src && src->pr_ops != &mem_ops)                 \
    {                                   \
	mem.pr_ops  = &mem_ops;                 \
	mem.pr_size = src->pr_size;                 \
	mem.pr_depth    = src->pr_depth;                \
	mem.pr_data = (char *) &cg8_d(src)->mprp;           \
	src     = &mem;                     \
    }

/*"cg8_rop"
 */
static int cg8_rop(dpr, dx, dy, w, h, op, spr, sx, sy)
Pixrect     *dpr, *spr;
int     dx, dy, w, h, op, sx, sy;
{
    struct cg8_data *cg8d;  /* Pointer to cg8 private data. */
    Pixrect         dmempr; /* Scratch area to build mem pixrect for dst. in. */
    Pixrect         smempr; /* Scratch area to build mem pixrect for src in. */

    if (dpr->pr_depth>8 || (spr && spr->pr_depth>8))
    {
	printf("kernel: cg8_rop error: attempt at 32 bit rop\n");
	return 0;
    }

    /*  If the video enable plane group is being written to, ignore the
     *  call since, we do not map it in the kernel.
     */
    cg8d = (dpr->pr_ops != &mem_ops ) ? cg8_d(dpr) : cg8_d(spr);
    if ( cg8d->fb[cg8d->active].group == PIXPG_VIDEO_ENABLE )
    {
	return 0;
    }

    PR_TO_MEM(dpr, dmempr);
    PR_TO_MEM(spr, smempr);
    return pr_rop(dpr, dx, dy, w, h, op, spr, sx, sy);
}

#ifndef NO_PIP

/*  Routines to support Picture In A Picture Option card.
 *
 *
 *  Notes:
 *      (1) Control status register zero has one bit which is shared, that is, 
 *          it has different meanings on input and output. If the bit is written
 *          it directs that a one-shot grab should be done. If the bit is read 
 *          it indicates if the pip has been turned on. Because of this sharing 
 *          it is not possible to use read modify write cycles in "pip_off".
 *      
 *      (2) The TCP_CSR0_TURN_PIP_ON bit is used to set the pip on or off. Its 
 *          value when read (TCP_CSR0_PIP_IS_ACTIVE) indicates if the pip is 
 *          actively writing data, NOT necessarily if the pip is on. The 
 *          TCP_CSR0_PIP_IS_ON bit indicates the the pip has been turned on, or 
 *          is running. The cycle looks like:
 *
 *              PIP_ON set  Field start             PIP_OFF set   Field end
 *                      |      |                            |        |
 *                      |      V                            V        V
 *                      V      ____________...________________________
 *  PIP_IS_ACTIVE     ________/                                       \_________
 *                       __________________...________________________
 *  PIP_IS_ON         __/                                             \_________
 *
 *      (3) In addition, TCP_CSR0_PIP_IS_ACTIVE will drop if the input source is 
 *          not present, even though it is "ON". So TCP_CSR0_PIP_IS_ON should be 
 *          examined to determine the state of the pip, not 
 *          TCP_CSR0_PIP_IS_ACTIVE.
 *
 *      (4) When turning off the pip it will continue until the end of the field
 *          as shown above. So TCP_CSR0_PIP_IS_ACTIVE may be polled to see if 
 *          the pip is off.
 *
 *   2  
 *  I C Bus Programming Considerations And Details:
 *
 *      (1) The i2c bus is implemented on the PIP as two contiguous bits in a 
 *          single register. Thus read-modify writes are used to set and clear 
 *          the bits. This can cause a problem during acknowledgement cycles 
 *          since the receiver of data (one of the devices on the i2c bus) will 
 *          hold the data line low during the acknowledgement cycle, which is 
 *          ended with the clock being set high by us. However, since the clock 
 *          is set high with a read-modify-write we pick up a low setting for 
 *          the data bit and store it back. To get around this we explicitly set
 *          the data bit back to high after, or while, setting the clock. This 
 *          actions are flagged with comments in the code below.
 *
 *      (2) The standard cycle for sending the first data byte is as follows:
 *
 *                  Start
 *                       |
 *                       |
 *                       |
 *                   V      1     2     3     4     5     6     7     8    ack
 *      Clock:     ____    __    __    __    __    __    __    __    __    __  
 *                     \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__
 *
 *      Sender:    __     _____ _____ _____ _____ _____ _____ _____ ____________
 *      Data         \___/_____X_____X_____X_____X_____X_____X_____X_____/
 *
 *      Reciever:  ____________________________________________________       __
 *      Data                                                           \_____/
 *
 *      (3) The standard cycle for sending subsequent bytes is:
 *
 *
 *                           1     2     3     4     5     6     7     8    ack
 *      Clock:          __    __    __    __    __    __    __    __    __  
 *                 ____/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__
 *
 *      Sender:    _________ _____ _____ _____ _____ _____ _____ ___________
 *      Data          \_____X_____X_____X_____X_____X_____X_____X_____/
 *
 *      Reciever:  __________________________________________________      ___
 *      Data                                                         \____/
 *
 *      (4) The standard cycle for stop at the end of data transmission is:
 *
 *
 *      Clock:          ______
 *                 ____/  
 *
 *      Sender:    ___    ____
 *      Data          \__/
 *
 *      (5) The standard cycle for reading a byte is:
 *
 *
 *                           1     2     3     4     5     6     7     8    stop
 *      Clock:          __    __    __    __    __    __    __    __    _____
 *                 ____/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/ 
 *
 *      Sender:    __________________________________________________    ____
 *      Data                                                         \__/
 *
 *      Receiver:  _________ _____ _____ _____ _____ _____ _____ ____  
 *      Data          \_____X_____X_____X_____X_____X_____X_____X____\____
 *
 */

/*"pip_ioctl"
 *
 *  Perform a pip-related input / output control operation on a
 *  SBus TC Card or its associated driver code
 *
 *    =  0 if successful operation
 *    ~= 0 if error detected
 */
/*ARGSUSED*/
pip_ioctl(softc, cmd, data, flag)
    SOFTC   *softc; /* -> instance dependent storage. */
    int     cmd;    /* =  opcode for operation to be performed. */
    caddr_t data;   /* -> data area for the operation (may be in or out). */
    int     flag;   /* =  modifier info. for operation to be performed. */
{

#   ifdef TC_DEBUG
	DEBUGF(5,  ("tconeioctl: cmd = "));
	switch (cmd) {
	  case PIPIO_G_EMULATION_MODE: 
	    DEBUGF(5, ("PIPIO_G_EMULATION_MODE\n")); break;
	  case PIPIO_G_PIP_ON_OFF:  
	    DEBUGF(5, ("PIPIO_G_PIP_ON_OFF\n")); break;
	  case PIPIO_G_PIP_ON_OFF_SUSPEND:  
	    DEBUGF(5, ("PIP_G_PIP_ON_OFF_SUSPEND\n")); break;
	  case PIPIO_G_PIP_ON_OFF_RESUME:   
	    DEBUGF(5, ("PIP_G_PIP_ON_OFF_RESUME\n")); break;

	  case PIPIO_S_EMULATION_MODE: 
	    DEBUGF(5, ("PIPIO_S_EMULATION_MODE\n")); break;
	  case PIPIO_S_PIP_ON_OFF:  
	    DEBUGF(5, ("PIP_S_PIP_ON_OFF\n")); break;
	  default: 
	    DEBUGF(5, ("not supported (0x%x)\n", cmd)); break;
	}
#   endif TC_DEBUG

    /*  Make sure pip operations are possible, then process request based on its 
     *  type. Cases are alphabetical.
     */
    if ( softc->pg_possible[PIXPG_VIDEO_ENABLE] == 0 )
    {
	return EINVAL;
    }
    switch (cmd)
    {
	case PIPIO_G_PIP_ON_OFF:      
	    *(int *)data = 
		(softc->device_reg->control_status&TCP_CSR0_PIP_IS_ON) ? 1 : 0;
	    break;
	case PIPIO_G_PIP_ON_OFF_RESUME: 
	    softc->pip_on_off_semaphore--;
	    if ( softc->pip_on_off_semaphore < 0 )
	    {
		softc->pip_on_off_semaphore = -1; /* In case too many resumes */
		pip_on(softc->device_reg);
		*(int *)data = 1;
	    }
	    else
	    {
		*(int *)data = 0;
	    }
	    break;
	case PIPIO_G_PIP_ON_OFF_SUSPEND: 
	    *(int *)data = pip_off(softc, 0);
	    softc->pip_on_off_semaphore++;
	    break;

	case PIPIO_S_PIP_ON_OFF:      
	    if ( *(int *)data )
	    {
		pip_on(softc->device_reg);
		softc->pip_on_off_semaphore = -1;
	    }
	    else
	    {
		pip_off(softc, 0);
		softc->pip_on_off_semaphore = 0;
	    }
	    break;

	/*- Unknown request, return fact we could not process it. */

	default:  
	    return ENOTTY;

    }                      /* switch(cmd) */
    return 0;
}

/*"pip_off"
 *
 *  Turn off live video generation, returning the previous state of the bit.
 *
 *
 *  Notes:
 *      (1) See notes 2 and 3 at the beginning of this file for a detailed
 *          explanation of all the shenanigans with bits in control status
 *          register 0 that occur here.
 *
 *      = 1 if pip was on
 *      = 0 if pip was off
 */
int pip_off(softc, wait_for_inactive)
    SOFTC   *softc;             /* -> device instance dependent data. */
    int     wait_for_inactive;  /* =  1 if wait for pip to actually turn off */
{
    int             i;          /* Index: # of times gone around wait loop. */
    int             pip_was_on; /* Indicates if pip was active. */
    Tc1_Device_Map  *dmap;      /* -> device area on TCPIP card. */

    dmap = softc->device_reg;
    pip_was_on = dmap->control_status & TCP_CSR0_PIP_IS_ON;
    dmap->control_status&=(u_char)~(TCP_CSR0_TURN_PIP_ON|TCP_CSR0_PIP_ONE_SHOT);
    if ( wait_for_inactive && (dmap->control_status & TCP_CSR0_PIP_IS_ACTIVE) )
    {
	printf("Waiting for inactive\n");
	for (i = 0; i < 10000000; i++)
	{
	    if ( !(dmap->control_status & TCP_CSR0_PIP_IS_ACTIVE) ) break;
	}
	if ( i >=       10000000 )
	{
	    printf(" Pip did not turn off in %d loops!!!!\n", i);
	}
    }
    return (pip_was_on) ? 1 : 0;
}

/*"pip_on"
 *
 *  Turn on live video generation. (Don't do it unless there
 *  is an active data source connected.
 */
int pip_on(dmap)
    Tc1_Device_Map  *dmap;  /* -> device area on TCPIP card. */
{
    if ( dmap->control_status1 & TCP_CSR1_INPUT_CONNECTED )
    {
	dmap->control_status = 
	    ((u_int)dmap->control_status&~TCP_CSR0_PIP_ONE_SHOT) |
	    TCP_CSR0_TURN_PIP_ON;
    }
}

#endif !NO_PIP

/*"tc_putcmap"
 *
 *  Local routine to set the color map values for either the 24-bit or the 
 *  monochrome frame buffer on a TC or TCP card. In addition to real lookup 
 *  table manipulation, emulation of 8-bit indexed color maps is done for 
 *  24-bit mode. (Called by tcone_ioctl).
 *      = 0 if success
 *      = error indication otherwise
 */
 static int tc_putcmap( softc, cmap )
    SOFTC          *softc; /* -> characteristics for our unit. */
    struct fbcmap  *cmap;  /* -> where to get colormap(data area from ioctl). */
{
    u_int          count;             /* # of color map entries to set. */
    Tc1_Device_Map *device_reg;       /* Pointer to the Brooktrees. */
    int            force_update;      /* Indicates if should access real luts */
    u_int          index;             /* Index of first entry to access. */
    int            n_entry;           /* Loop index: current color map entry. */
    int            plane_group;       /* Top 7 bits of index = plane group. */

    device_reg = softc->device_reg;

    /*
     *  Verify we need to actually move some data.Initialize variables 
     *  from caller's parameters, etc. If the plane group is zero, use 
     *  the 24 bit group.
     */
    if ( (cmap->count == 0) || !cmap->red || !cmap->green || !cmap->blue )
    {
	return 0;
    }
    count = cmap->count;
    index = cmap->index & PIX_ALL_PLANES;
    if (index+count > TC_CMAP_SIZE)
    {
	return EINVAL;
    }
    force_update = cmap->index & PR_FORCE_UPDATE;
    if ( !(plane_group = PIX_ATTRGROUP(cmap->index)) )
    {
	plane_group = PIXPG_24BIT_COLOR; 
    }

    /*  Move caller's data, there are 2 cases: kernel space and user space. 
     */
    if (softc->tc1d.flags & CG8_KERNEL_UPDATE)
    {
	DEBUGF(6, ("Kernel putcmap\n", 0) );
	softc->tc1d.flags &= ~CG8_KERNEL_UPDATE;
	BCOPY( cmap->red, softc->cmap_red, count );
	BCOPY( cmap->green, softc->cmap_green, count);
	BCOPY( cmap->blue, softc->cmap_blue, count );
    }
    else
    {   
	DEBUGF(6, ("User putcmap, index=%x, count=%d, pg=%d\n", 
	    index, count, plane_group) );
	if ( COPYIN( cmap->red, softc->cmap_red, count ) ||
	     COPYIN( cmap->green, softc->cmap_green, count) ||
	     COPYIN( cmap->blue, softc->cmap_blue, count ) )
	{
	    return EFAULT;
	}
    }    

    /*  If there is no eight bit buffer and hardware update is not forced,
     *  set 8-bit index emulation color table entries for 24-bit buffer. 
     *  (The check for the 8-bit plane group is here in case the kernel
     *   knows 8-bit h'ware exists, but the driver didn't export it to Sunview.)
     */
    if ( !force_update && (softc->pg_enabled[PIXPG_8BIT_COLOR] == 0) &&
	((plane_group==PIXPG_24BIT_COLOR) || (plane_group==PIXPG_8BIT_COLOR)) )
    {
	for (n_entry = 0; n_entry < count; n_entry++, index++)
	{
	    softc->em_red[index] = softc->cmap_red[n_entry];
	    softc->em_green[index] = softc->cmap_green[n_entry];
	    softc->em_blue[index] = softc->cmap_blue[n_entry];
	}
    }    

    /*  Set entries in actual Brooktree lookup tables for 8/24-bit buffer.
     */
    else if ( plane_group == PIXPG_24BIT_COLOR || 
	      plane_group == PIXPG_8BIT_COLOR )
    {
	DEBUGF(6, ("Real pcmap: group=%d flags=%x", 
	    plane_group, softc->tc1d.flags) );
	DEBUGF(6, (" first = %d", index) );
	DEBUGF(6, (" count = %d\n", count) );

	device_reg->dacs.bt457.all_address = index;
	for (n_entry = 0; n_entry < count; n_entry++)
	{
	    device_reg->dacs.bt457.red_color = softc->cmap_red[n_entry];
	    device_reg->dacs.bt457.green_color = softc->cmap_green[n_entry];
	    device_reg->dacs.bt457.blue_color = softc->cmap_blue[n_entry];
	}
    }    

    else if (plane_group != PIXPG_OVERLAY )
    {
	printf("Illegal plane group %d encountered\n", plane_group);
	return EINVAL;
    }

    /*  Set entries in actual Brooktree lookup tables for monochrome overlay.
     *  At the same time write the shadow color map which fbiogetcmap gets 
     *  values from. Note that our overlay map works in a different manner from 
     *  Sun's cg8, so a translation must be done (using translate_omap data 
     *  structure):
     *   Function                   Sun LUT index   SBus TC LUT index
     *   --------                   -------------   ------------------
     *   True color                       0                  0 
     *   Window system foreground         1                  2
     *   Current window foreground        2                  1
     *   Window system background         3                  3
     *
     *  Note we only write the overlay if 24-bit enabled to get around
     *  a problem with cg4 emulation. Also we implement cursor color
     *  freezing here for true color!
     */
    else if ( softc->pg_enabled[PIXPG_24BIT_COLOR] )
    {
	DEBUGF(6, ("Overlay pcmap: flags=%x", softc->tc1d.flags) );
	DEBUGF(6, (" first = %d", index) );
	DEBUGF(6, (" count = %d\n", count) );

	for (n_entry = 0; n_entry < count; n_entry++, index++)
	{
	    if ( !softc->sw_cursor_color_frozen || index != 2 ) 
	    {
		device_reg->dacs.bt457.all_address = translate_omap[index];
		device_reg->dacs.bt457.red_overlay = 
		    softc->omap_red[index] = softc->cmap_red[n_entry];
		device_reg->dacs.bt457.green_overlay = 
		    softc->omap_green[index] = softc->cmap_green[n_entry];
		device_reg->dacs.bt457.blue_overlay = 
		    softc->omap_blue[index] = softc->cmap_blue[n_entry];
	    }
	}
    }
    

    return 0;
}

/*"tcp_attach"
 *
 *  Perform device specific initializations for TC or TCP frame buffer. 
 *  (Called from cgeightattach().) This routine initializes parts of the
 *  per-unit data structure.  In addition it maps the device's memory regions 
 *  into virtual memory and performs hardware specific initializations.
 *  A number of per-unit data items have already been initialized from PROMs
 *  in cgeightattach().
 *
 *      = 1 if success
 *      = 0 if failure
 */
static int tcp_attach(devi, softc)
    struct dev_info *devi;  /* -> device info structure w/node id... (rest filled here). */
    SOFTC           *softc; /* -> per-unit data structure to be filled in. */
{
    int             color;        /* Index: next color to set in lut ramp. */
    Tc1_Device_Map  *dmap;        /* Fast pointer to device register area. */
    int             fb_map_bytes; /* # of bytes needed to map frame buffers. */
    addr_t          reg;          /* Virt. addr of device map(&optionally fbs)*/
    register u_long *sel_addr;    /* Cursor: selection memory word to init. */
    register int    sel_cnt;      /* Index: # of words initialized in sel mem */
    int             sel_max;      /* # of words in sel. mem. to initialize. */

    /*  PROCESS REMAINING PROM PARAMETERS:
     *
     *  Pick up the device specific parameters exported by the on-board PROMs.
     *  Use this information to set up memory mapping characteristics of the 
     *  board.
     */
    DEBUGF(5, ("tcp_attach softc at %x\n", softc));

    /*  Device register area parameters:
     */
    softc->dev_reg_sbus_base = 
	getprop(devi->devi_nodeid, "device-map-offset", 0x400000);
    softc->dev_reg_mmap_offset = CG8_VADDR_DAC;
    softc->dev_reg_sbus_delta = 0;
    softc->dev_reg_mmap_size = ptob(btopr(NBPG));
    softc->memory_map_end = 
	softc->dev_reg_mmap_offset + softc->dev_reg_mmap_size;

    DEBUGF(5, (" dev_reg_sbus_base is %x\n", softc->dev_reg_sbus_base));

    /*  Monochrome frame buffer parameters:
     */
    softc->fb_mono_sbus_delta = 
	getprop(devi->devi_nodeid, "monochrome-offset", 0x00C00000);
    softc->fb_mono_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_mono_mmap_size = 
	getprop(devi->devi_nodeid, "monochrome-size", 1152*900/8);
    softc->fb_mono_mmap_size = ptob(btopr(softc->fb_mono_mmap_size));

    DEBUGF(5, ("monochrome   sbus delta %x mmap offset     %x size  %x\n", 
	softc->fb_mono_sbus_delta, 
	0,
	softc->fb_mono_mmap_size));

    /*  Selection memory parameters:
     */
    softc->fb_sel_sbus_delta = 
	getprop(devi->devi_nodeid, "selection-offset", 0x00D00000);
    softc->fb_sel_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_sel_mmap_size = 
	getprop(devi->devi_nodeid, "selection-size", 1152*900/8);
    softc->fb_sel_mmap_size = ptob(btopr(softc->fb_sel_mmap_size));

    DEBUGF(5, ("selection    sbus delta %x mmap offset  %x size  %x\n", 
	softc->fb_sel_sbus_delta, 
	softc->fb_mono_mmap_size,
	softc->fb_sel_mmap_size));

    /*  Eight bit frame buffer parameters (set them up even if don't use them):
     */
    softc->fb_8bit_sbus_delta = 0x00700000 - softc->dev_reg_sbus_base;
    softc->fb_8bit_mmap_size = ptob(btopr(1152*900)); 

    DEBUGF(5, ("eight_bit    sbus delta %x mmap offset  %x size  %x\n", 
	softc->fb_8bit_sbus_delta, 
	softc->fb_mono_mmap_size+softc->fb_sel_mmap_size,
	softc->fb_8bit_mmap_size));

    /*  True color frame buffer parameters:
     */
    softc->fb_tc_sbus_delta = 
	getprop(devi->devi_nodeid, "true-color-offset", 0x00800000);
    softc->fb_tc_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_tc_mmap_size = 
	getprop(devi->devi_nodeid, "true-color-size", 1152*900*4);
    softc->fb_tc_mmap_size = ptob(btopr(softc->fb_tc_mmap_size));

    DEBUGF(5, ("true_color   sbus delta %x size %x\n", 
	softc->fb_tc_sbus_delta, softc->fb_tc_mmap_size));

    /*  Video enable memory parameters (set them up even if we don't use them):
     */
    softc->fb_video_sbus_delta = 0x00E00000 - softc->dev_reg_sbus_base;
    softc->fb_video_mmap_size = ptob(btopr(1152*900/8)); 

    DEBUGF(5, ("video_enable sbus delta %x size  %x\n", 
	softc->fb_video_sbus_delta, softc->fb_video_mmap_size));



    /*  ALLOCATE SPACE IN KERNEL VIRTUAL MEMORY FOR THE DEVICE REGISTERS AND 
     *  MONOCHROME OPERATIONS, BUILD THE TABLE USED FOR USER SPACE MAPPING
     *
     *  Allocate space in virtual memory for the device register area (and
     *  possibly the frame buffers.) Remember the virtual address of the device 
     *  register area, and its physical address, which will be used by 
     *  cgeightmmap as the base mapping for all physical pages. 
     *
     *  #### The |= 18000000 kludge is because the 4.1.1 code on a SS2 does
     *       not return the correct address for the Sbus slot! (Bug 1046449)
     *       This can be removed when that is fixed.
     */
#   if NWIN > 0
	fb_map_bytes = softc->fb_mono_mmap_size + softc->fb_sel_mmap_size;
	if ( softc->pg_possible[PIXPG_8BIT_COLOR] ) 
	{
	    fb_map_bytes += softc->fb_8bit_mmap_size;
	}
#   else NWIN > 0
	fb_map_bytes = 0;
#   endif NWIN > 0

#ifdef sun4c
    (u_int)devi->devi_reg[0].reg_addr |= 0x18000000;    /* SEE NOTE ABOVE */
#endif sun4c
    if ( !(reg = (addr_t)map_regs(
	(addr_t)(devi->devi_reg[0].reg_addr + softc->dev_reg_sbus_base),
	(u_int)(fb_map_bytes + softc->dev_reg_mmap_size),
	devi->devi_reg[0].reg_bustype)))
    {
	return 0;
    }
    softc->device_reg = (Tc1_Device_Map *) reg;
    dmap = softc->device_reg;
    softc->basepage = fbgetpage(reg);

    DEBUGF(5, ("tcp_attach reg=0x%x basepage=0x%x (0x%x)\n", 
	(u_int)reg, ptob(softc->basepage), softc->basepage));
    
    /*  Default operations to emulate a Sun CG8 card (this may be modified later 
     *  via the PIPIO_S_EMULATION_MODE ioctl.)
     *  Determine if there is a pip and/or eight bit memory present. If this is 
     *  an old style frame buffer set the register to enable use of selection 
     *  memory.
     */
    softc->pg_enabled[PIXPG_8BIT_COLOR] = 0;

#ifndef NO_PIP
    if ( softc->pg_possible[PIXPG_VIDEO_ENABLE] )
    {
	if ( (dmap->control_status1 & TCP_CSR1_NO_PIP) == 0 )
	{
	    softc->pip_on_off_semaphore = 0;
	    pip_off(softc, 0);
	}
	else
	{
	    softc->pg_possible[PIXPG_VIDEO_ENABLE] = 0;
	    softc->pg_enabled[PIXPG_VIDEO_ENABLE] = 0;
	}
    }
#endif !NO_PIP
    if ( softc->fb_model == RASTEROPS_TC )
    {
	dmap->control_status |= TC1_SELECTION_ENABLE;
    }

    /*  For the kernel's use map in the monochrome frame buffer, selection 
     *  memory, and if present the eight bit memory after the device registers.
     *  For each buffer calculate its virtual address, and place this
     *  information into the device-dependent structure. Note in the kernel the
     *  the device register appears before the frame buffer so we are constantly
     *  adding in NBPG to the virtual offsets. In user mode the opposite is 
     *  true, because of the way cg8_make maps things.
     */        
#if NWIN > 0
    if (fb_map_bytes)
    {
	fbmapin(reg + NBPG,
	    (int) (softc->basepage + btop(softc->fb_mono_sbus_delta)),
	    softc->fb_mono_mmap_size);
	softc->fb_mono = (u_char *)reg + NBPG;

	DEBUGF(5, ("tcp_attach fb_mono=   0x%x/0x%x (0x%x)\n",
	    (u_int) softc->fb_mono, 
	    fbgetpage((addr_t) softc->fb_mono) << PGSHIFT,
	    fbgetpage((addr_t) softc->fb_mono)));


	fbmapin(reg + NBPG + softc->fb_mono_mmap_size,
	    (int) (softc->basepage + btop(softc->fb_sel_sbus_delta)),
	    softc->fb_sel_mmap_size);
	softc->fb_sel = (u_char *)reg + NBPG + softc->fb_mono_mmap_size;

	DEBUGF(5, ("tcp_attach fb_sel=    0x%x/0x%x (0x%x)\n",
	    (u_int) softc->fb_sel, 
	    fbgetpage((addr_t) softc->fb_sel) << PGSHIFT,
	    fbgetpage((addr_t) softc->fb_sel)));

	if ( softc->pg_possible[PIXPG_8BIT_COLOR] )
	{
	    fbmapin(reg+NBPG+softc->fb_mono_mmap_size+softc->fb_sel_mmap_size,
		(int) (softc->basepage + btop(softc->fb_8bit_sbus_delta)),
		softc->fb_8bit_mmap_size);
	    softc->fb_8bit = (u_char *)reg + NBPG + softc->fb_mono_mmap_size +
		softc->fb_sel_mmap_size;
 
	    DEBUGF(5, ("tcp_attach fb_8bit=    0x%x/0x%x (0x%x)\n",
		(u_int) softc->fb_8bit,
		fbgetpage((addr_t) softc->fb_8bit) << PGSHIFT,
		fbgetpage((addr_t) softc->fb_8bit)));
	}
    }

#endif NWIN > 0

    /*  Build the memory mapping information used to map in the various frame
     *  buffers in user (process) space.
     */
    x_build_mmap_info(softc);

    
    
    /*  INITIALIZE RAMDACs AND SELECTION MEMORY IN PREPARATION FOR WINDOW USAGE
     *
     *  Before initializing the Brooktree's make sure the selection memory 
     *  indicates that the monochrome plane is visible. This is necessary 
     *  because we are about to enable the selection memory as an input into the 
     *  overlay port on the RAMDACs.
     */
#if NWIN > 0
    sel_max = 900*softc->linebytes1 / sizeof(u_long);
    for(sel_addr=(u_long *)softc->fb_sel, sel_cnt=0; sel_cnt<sel_max; sel_cnt++)
    {
	*sel_addr++ = 0xffffffff;
    }
#endif NWIN > 0

    /*  Initialize the 3 Brooktree 457s for:
     *      (1) 4:1 multiplexing (since we are not 1280x1024)
     *      (2) Transparent overlay for overlay color 0.
     *      (3) Display the overlay plane
     *  Enable read and write to all planes, turn off blinking, and set
     *  control register.
     */
    DEBUGF(5, ("TCONE: Initializing Brooktrees\n") );

    dmap->dacs.bt457.all_address = BT457_COMMAND;    
    dmap->dacs.bt457.all_control = 0x43;
    dmap->dacs.bt457.all_address = BT457_READ_MASK;  
    dmap->dacs.bt457.all_control = 0xff;
    dmap->dacs.bt457.all_address = BT457_BLINK_MASK; 
    dmap->dacs.bt457.all_control = 0;
    dmap->dacs.bt457.all_address = BT457_CONTROL;    
    dmap->dacs.bt457.all_control = 0;
    
    /*  Initialize overlay plane color map and load Brooktree 457's as a group 
     *  with that map. (See tc_putcmap routine below for description of this 
     *  funny mapping.)
     */
    dmap->dacs.bt457.all_address = 0;
    dmap->dacs.bt457.all_overlay = /* Unused, this is transparency value! */
	softc->omap_red[0] = 
	softc->omap_green[0] = 
	softc->omap_blue[0] = 
	0x00; 
    dmap->dacs.bt457.all_address = 2;
    dmap->dacs.bt457.all_overlay = /* Window system background: white guardian*/
	softc->omap_red[1] = 
	softc->omap_green[1] = 
	softc->omap_blue[1] = 
	0xff; 
    dmap->dacs.bt457.all_address = 1;
    dmap->dacs.bt457.all_overlay = /* Current window foreground. */
	softc->omap_red[2] = 
	softc->omap_green[2] = 
	softc->omap_blue[2] = 
	0xff; 
    dmap->dacs.bt457.all_address = 3;
    dmap->dacs.bt457.all_overlay = /* Window system foreground: black guardian*/
	softc->omap_red[3] = 
	softc->omap_green[3] = 
	softc->omap_blue[3] =
	0x00; 
    softc->sw_cursor_color_frozen = 0; /*Allow cursor color to be set normally*/

    /*  Load Brooktree 457's as a group with a linear ramp for the 24-bit true
     *  color frame buffer.
     */
    for (color=0, dmap->dacs.bt457.all_address=0; color < TC_CMAP_SIZE; color++)
    {
	dmap->dacs.bt457.all_color = color; 
    }



    return 1;
}

/*"tcs_attach"
 *
 *  Perform device specific initializations for TCS frame buffer.
 *  (Called from cgeightattach().) This routine initializes parts of the
 *  per-unit data structure.  In addition it maps the device's memory regions 
 *  into virtual memory and performs hardware specific initializations.
 *  A number of per-unit data items have already been initialized from PROMs
 *  in cgeightattach().
 *
 *      = 1 if success
 *      = 0 if failure
 */
static int tcs_attach(devi, softc)
    struct dev_info *devi;  /* -> device info struct w/node id...(rest filled here) */
    SOFTC           *softc; /* -> per-unit data structure to be filled in. */
{
    int             color;        /* Index: next color to set in lut ramp. */
    Tcs_Device_Map  *dmap;        /* Fast pointer to device register area. */
    int             fb_map_bytes; /* # of bytes needed to map frame buffers. */
    addr_t          reg;          /* Virt addr of device map(&optionally fbs) */
    register u_long *sel_addr;    /* Cursor: selection memory word to init. */
    register int    sel_cnt;      /* Index: # of sel. mem. words initialized */
    int             sel_max;      /* # of words in selection memory to init. */

    /*  PROCESS REMAINING PROM PARAMETERS:
     *
     *  Pick up the device specific parameters exported by the on-board PROMs.
     *  Use this information to set up memory mapping characteristics of the 
     *  board.
     */
    DEBUG_LEVEL(5);
    DEBUGF(5, ("tcs_attach softc at %x\n", softc));

    /*  Device register area parameters:
     */
    softc->dev_reg_sbus_base = 
	getprop(devi->devi_nodeid, "device-map-offset", 0x00040000);
    softc->dev_reg_mmap_offset = CG8_VADDR_DAC;
    softc->dev_reg_sbus_delta = 0;
    softc->dev_reg_mmap_size = ptob(btopr(NBPG));
    softc->memory_map_end = softc->dev_reg_mmap_offset+softc->dev_reg_mmap_size;


    DEBUGF(5, (" dev_reg_sbus_base is %x\n", softc->dev_reg_sbus_base));

    /*  Monochrome frame buffer parameters:
     */
    softc->fb_mono_sbus_delta = 
	getprop(devi->devi_nodeid, "monochrome-offset", 0x00100000);
    softc->fb_mono_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_mono_mmap_size = 
	getprop(devi->devi_nodeid, "monochrome-size", 0x40000);
    softc->fb_mono_mmap_size = ptob(btopr(softc->fb_mono_mmap_size));

    DEBUGF(5, ("monochrome   sbus delta %x mmap offset     %x size  %x\n", 
	softc->fb_mono_sbus_delta, 
	0,
	softc->fb_mono_mmap_size));

    /*  Selection memory parameters:
     */
    softc->fb_sel_sbus_delta = 
	getprop(devi->devi_nodeid, "selection-offset", 0x00140000);
    softc->fb_sel_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_sel_mmap_size = 
	getprop(devi->devi_nodeid, "selection-size", 0x40000);
    softc->fb_sel_mmap_size = ptob(btopr(softc->fb_sel_mmap_size));

    DEBUGF(5, ("selection    sbus delta %x mmap offset  %x size  %x\n", 
	softc->fb_sel_sbus_delta, 
	softc->fb_mono_mmap_size, 
	softc->fb_sel_mmap_size));

    /*  Eight bit frame buffer parameters (set them up even if don't use them):
     */
    softc->fb_8bit_sbus_delta = 0x00180000 - softc->dev_reg_sbus_base;
    softc->fb_8bit_mmap_size = ptob(btopr(0x80000)); 

    softc->pg_possible[PIXPG_8BIT_COLOR] = 0;

    DEBUGF(5, ("eight_bit    sbus delta %x mmap offset  %x size  %x\n", 
	softc->fb_8bit_sbus_delta, 
	softc->fb_mono_mmap_size + softc->fb_sel_mmap_size, 
	softc->fb_8bit_mmap_size));

    /*  True color frame buffer parameters:
     */
    softc->fb_tc_sbus_delta = 
	getprop(devi->devi_nodeid, "true-color-offset", 0x00200000);
    softc->fb_tc_sbus_delta -= softc->dev_reg_sbus_base;
    softc->fb_tc_mmap_size = 
	getprop(devi->devi_nodeid, "true-color-size", 0x200000);
    softc->fb_tc_mmap_size = ptob(btopr(softc->fb_tc_mmap_size));

    DEBUGF(5, ("true_color   sbus delta %x size %x\n", 
	softc->fb_tc_sbus_delta, 
	softc->fb_tc_mmap_size));


    /*  ALLOCATE SPACE IN KERNEL VIRTUAL MEMORY FOR THE DEVICE REGISTERS AND 
     *  MONOCHROME OPERATIONS
     *
     *  Allocate space in virtual memory for the device register area (and
     *  possibly the frame buffers.) Remember the virtual address of the device 
     *  register area, and its physical address, which will be used by 
     *  cgeightmmap as the base mapping for all physical pages. 
     *
     *  #### The |= 18000000 kludge is because the 4.1.1 code on a SS2 does
     *       not return the correct address for the Sbus slot! (Bug 1046449)
     *       This can be removed when that is fixed.
     */
#   if NWIN > 0
	fb_map_bytes = softc->fb_mono_mmap_size + softc->fb_sel_mmap_size;
#   else NWIN > 0
	fb_map_bytes = 0;
#   endif NWIN > 0
#ifdef sun4c
    (u_int)devi->devi_reg[0].reg_addr |= 0x18000000;    /* SEE NOTE ABOVE */
#endif sun4c
    if ( !(reg = (addr_t)map_regs(
    (addr_t)(devi->devi_reg[0].reg_addr + softc->dev_reg_sbus_base),
	(u_int)(fb_map_bytes + softc->dev_reg_mmap_size),
	devi->devi_reg[0].reg_bustype)))
    {
	return 0;
    }
    softc->device_tcs = (Tcs_Device_Map *) reg;
    dmap = softc->device_tcs;
    softc->basepage = fbgetpage(reg);

    DEBUGF(5, ("tcs_attach reg=0x%x basepage=0x%x (0x%x)\n", 
	(u_int)reg, ptob(softc->basepage), softc->basepage));
    
    /*  Default operations to emulate a Sun CG8 card (this may be modified later 
     *  via the PIPIO_S_EMULATION_MODE ioctl.)
     */
    softc->pg_enabled[PIXPG_8BIT_COLOR] = 0;

    /*  For the kernel's use map in the monochrome frame buffer, and selection 
     *  memory after the device registers. For each buffer calculate its virtual 
     *  address, and place this information into the device-dependent structure. 
     *  Note in the kernel the the device register appears before the frame 
     *  buffer so we are constantly adding in NBPG to the virtual offsets. In 
     *  user mode the opposite is true, because of the way cg8_make maps things.
     */        
#if NWIN > 0
    if (fb_map_bytes)
    {
	fbmapin(reg + NBPG,
	    (int) (softc->basepage + btop(softc->fb_mono_sbus_delta)),
	    softc->fb_mono_mmap_size);
	softc->fb_mono = (u_char *)reg + NBPG;

	DEBUGF(5, ("tcs_attach fb_mono=   0x%x/0x%x (0x%x)\n",
	    (u_int) softc->fb_mono, 
	    fbgetpage((addr_t) softc->fb_mono) << PGSHIFT,
	    fbgetpage((addr_t) softc->fb_mono)));


	fbmapin(reg + NBPG + softc->fb_mono_mmap_size,
	    (int) (softc->basepage + btop(softc->fb_sel_sbus_delta)),
	    softc->fb_sel_mmap_size);
	softc->fb_sel = (u_char *)reg + NBPG + softc->fb_mono_mmap_size;

	DEBUGF(5, ("tcs_attach fb_sel=    0x%x/0x%x (0x%x)\n",
	    (u_int) softc->fb_sel, 
	    fbgetpage((addr_t) softc->fb_sel) << PGSHIFT,
	    fbgetpage((addr_t) softc->fb_sel)));

    }
    
#endif NWIN > 0
    
    /*  Build the memory mapping information used to map in the various frame
     *  buffers in user (process) space.
     */
    x_build_mmap_info(softc);


    /*  INITIALIZE VENUS CHIPS, RAMDAC AND SELECTION MEMORY IN PREPARATION FOR
     *  WINDOW USAGE
     *
     *  Initialize venus chips to turn on operation of Brooktree, memory timing, 
     *  etc. Use the switch settings if this is not a model a board. 
     *
     *  Before initializing the Brooktree make sure the selection memory 
     *  indicates that the monochrome plane is visible. This is necessary 
     *  because we are about to enable the selection memory as an input into the 
     *  overlay port on the RAMDACs.
     */
    if ( softc->device_tcs->tc_venus.io_general & VENUS_TCS_MODEL_A )
    {
	tcs_init_native(softc);
    }
    else
    {
	switch( softc->device_tcs->tc_venus.io_general & VENUS_TIMING_MASK ) 
	{
	  default:
	    printf("Unknown switch setting: defaulting to native timing.\n");
	  case VENUS_TIMING_NATIVE: tcs_init_native(softc); break;
	  case VENUS_TIMING_NTSC:   tcs_init_ntsc(softc);   break;
	  case VENUS_TIMING_PAL:    tcs_init_pal(softc);    break;
	}
	softc->linebytes1 = softc->width / 8;
	softc->linebytes32 = softc->width * 4;
    }

    sel_max = softc->width*softc->linebytes1 / sizeof(u_long);
    for(sel_addr=(u_long *)softc->fb_sel, sel_cnt=0; sel_cnt<sel_max; sel_cnt++)
    {
	*sel_addr++ = 0xffffffff;
    }

    /*  Initialize the Brooktree 473:
     */
    DEBUGF(5, ("tcs_attach: Initializing Brooktree 473\n") );

    dmap->dac.control = 0x30;       /* 8-bit pixels, NTSC timing (IRE = 7.5) */
    dmap->dac.read_mask = 0xff;

    /*  Initialize overlay plane color map.
     */
    dmap->dac.overlay_write_addr = 0; /* Overlay colors: */
    dmap->dac.overlay = 0x00;         /* 00: transparent pass-through. */
    dmap->dac.overlay = 0x00;         /* ... */
    dmap->dac.overlay = 0x00;         /* ... */
    dmap->dac.overlay = 0xff;         /* 01: true color cursor value: white. */
    dmap->dac.overlay = 0xff;         /* ... */
    dmap->dac.overlay = 0xff;         /* ... */
    dmap->dac.overlay = 0xff;         /* 10: window background color: white. */
    dmap->dac.overlay = 0xff;         /* ... */
    dmap->dac.overlay = 0xff;         /* ... */
    dmap->dac.overlay = 0x00;         /* 11: window foreground color: black. */
    dmap->dac.overlay = 0x00;         /* ... */
    dmap->dac.overlay = 0x00;         /* ... */
    
    softc->omap_red[0] = softc->omap_green[0] = softc->omap_blue[0] = 0x00; 
    softc->omap_red[1] = softc->omap_green[1] = softc->omap_blue[1] = 0xff; 
    softc->omap_red[2] = softc->omap_green[2] = softc->omap_blue[2] = 0xff; 
    softc->omap_red[3] = softc->omap_green[3] = softc->omap_blue[3] = 0x00; 

    /*  Load Brooktree 473 as a group with a linear ramp for the 24-bit true
     *  color frame buffer. Use cmap_xxx as a shadow table since we load the
     *  entire lut in tcs_putcmap(). (See note 1 in tcs_putcmap.)
     */
    for (color = 0, dmap->dac.ram_write_addr = 0; color < TC_CMAP_SIZE; color++)
    {
	softc->cmap_red[color] = dmap->dac.color = color; 
	softc->cmap_green[color] = dmap->dac.color = color; 
	softc->cmap_blue[color] = dmap->dac.color = color; 
    }

    DEBUG_LEVEL(0);

    return 1;
}

/*"tcs_init_native"
 *
 *  Initialize venus chips on SBus TCS Card to operate at 
 *  Apple 13" monitor scan rates (60Hz non-interlaced, 640 x 480.)
 */
tcs_init_native(softc)
    SOFTC           *softc; /* characteristics for our unit. */
{
    Tcs_Device_Map  *dmap;      /* -> device register area on TCS card. */
    u_int           io_conf;    /* Value to load into io config register. */
    u_int           io_gen;     /* Value to load into io general register. */

    dmap = softc->device_tcs;
    softc->timing_regimen = NATIVE_TIMING;
    softc->width = 640;
    softc->height = 480;
    softc->linebytes1 = softc->width / 8;
    softc->linebytes8 = softc->width;
    softc->linebytes32 = softc->width * 4;

    io_gen = (dmap->tc_venus.io_general & VENUS_TCS_MODEL_A) ? 0x001c : 0x781c;
    io_conf = (dmap->tc_venus.io_general & VENUS_TCS_MODEL_A) ? 0x00df : 0x1fdf;

    /*
     *  Initialize venus chip 0 which affects Brooktree operations and color 
     *   buffer operations. Configure the system as a 24-bit true color card.
     */
    dmap->tc_venus.control1             = 0x00;     
    dmap->tc_venus.control2             = 0x00;     
    dmap->tc_venus.control3             = 0x00;     
    dmap->tc_venus.control4             = 0x00;     
    dmap->tc_venus.refresh_interval     = 0x70; 
    dmap->tc_venus.control1             = 0x11;

    dmap->tc_venus.io_general           = io_gen;
    dmap->tc_venus.io_config            = io_conf;

    dmap->tc_venus.display_start        = 0x00000000;
    dmap->tc_venus.half_row_incr        = 0x00000100;
    dmap->tc_venus.display_pitch        = 0x00000140;
    dmap->tc_venus.cas_mask             = 0x7f;
    dmap->tc_venus.horiz_latency        = 0x00;

    dmap->tc_venus.horiz_end_sync       = 0x000c;
    dmap->tc_venus.horiz_end_blank      = 0x0024;
    dmap->tc_venus.horiz_start_blank    = 0x00c4;
    dmap->tc_venus.horiz_total          = 0x00d4;
    dmap->tc_venus.horiz_half_line      = 0x0000;
    dmap->tc_venus.horiz_count_load     = 0x0000;

    dmap->tc_venus.vert_end_sync        = 0x02;
    dmap->tc_venus.vert_end_blank       = 0x29;
    dmap->tc_venus.vert_start_blank     = 0x0209;
    dmap->tc_venus.vert_total           = 0x020c;
    dmap->tc_venus.vert_count_load      = 0x0000;
    dmap->tc_venus.vert_interrupt       = 0x020a;

    dmap->tc_venus.y_zoom               = 0x01;
    dmap->tc_venus.soft_register        = 0x00;

    dmap->tc_venus.io_general           = io_gen & ~VENUS_SOFT_RESET;
    dmap->tc_venus.io_general           = io_gen;

    dmap->tc_venus.control3             = 0x73;
    dmap->tc_venus.control1             = 0xf1;     /* Enable video. */
    
    /*
     *  Initialize venus chip 1 which affects 1-bit monochrome and selection 
     *  memory operations.
     */
    dmap->mono_venus.control1           = 0x00;
    dmap->mono_venus.control2           = 0x00;
    dmap->mono_venus.control3           = 0x00;
    dmap->mono_venus.control4           = 0x00;
    dmap->mono_venus.refresh_interval   = 0x70;     /* Set refresh interval. */
    dmap->mono_venus.control1           = 0x13;

    dmap->mono_venus.io_config          = 0x00;
    dmap->mono_venus.display_start      = 0x00000000;
    dmap->mono_venus.half_row_incr      = 0x00000080;
    dmap->mono_venus.display_pitch      = 0x00000050;
    dmap->mono_venus.cas_mask           = 0x3f;
    dmap->mono_venus.horiz_latency      = 0x00;
 
    dmap->mono_venus.horiz_end_sync     = 0x000c;
    dmap->mono_venus.horiz_end_blank    = 0x0024;
    dmap->mono_venus.horiz_start_blank  = 0x00c4;
    dmap->mono_venus.horiz_total        = 0x00d4;
    dmap->mono_venus.horiz_half_line    = 0x0000;
    dmap->mono_venus.horiz_count_load   = 0x0000;
 
    dmap->mono_venus.vert_end_sync      = 0x02;
    dmap->mono_venus.vert_end_blank     = 0x29;
    dmap->mono_venus.vert_start_blank   = 0x0209;
    dmap->mono_venus.vert_total         = 0x020c;
    dmap->mono_venus.vert_count_load    = 0x0000;
    dmap->mono_venus.vert_interrupt     = 0x0000;
 
    dmap->mono_venus.y_zoom             = 0x01;
    dmap->mono_venus.soft_register      = 0xf0;
 
    dmap->mono_venus.control3           = 0xf3;
    dmap->mono_venus.control1           = 0xf3;
}   

/*"tcs_init_ntsc"
 *
 *  Initialize venus chips on SBus TCS Card to operate at NTSC scan rates. 
 *  (30Hz interlaced 640 x 480.)
 *  If a genlock device is connected external (genlock) sync will be used.
 */
tcs_init_ntsc(softc)
    SOFTC           *softc; /* characteristics for our unit. */
{
    Tcs_Device_Map  *dmap;      /* -> device register area on TCS card. */
    u_int           io_conf;    /* Value to load into io config register. */
    u_int           io_gen;     /* Value to load into io general register. */

    dmap = softc->device_tcs;
    softc->timing_regimen = NTSC_TIMING;
    softc->width = 640;
    softc->height = 480;
    softc->linebytes1 = softc->width / 8;
    softc->linebytes8 = softc->width;
    softc->linebytes32 = softc->width * 4;

    if ( dmap->tc_venus.io_general & VENUS_TCS_MODEL_A )
    {
	io_gen = ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) ? 
		 0x007c : 0x0074;
	io_conf = 0x00df;
    }
    else
    {
	io_gen = ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) ? 
		 0x78bc : 0x78f4;
	io_conf = 0x1fdf;
    }

    /*
     *  Initialize venus chip 0 which affects Brooktree operations and color 
     *  buffer operations. Configure the system as a 24-bit true color card.
     */
    dmap->tc_venus.control1             = 0x00;
    dmap->tc_venus.control2             = 0x00;
    dmap->tc_venus.control3             = 0x00;
    dmap->tc_venus.control4             = 0x00;
    dmap->tc_venus.refresh_interval     = 0x70;     /* Set refresh interval. */
    dmap->tc_venus.control1             = 0x11;
    dmap->tc_venus.control2             = 0x00; 
    dmap->tc_venus.control3             = 0x7b; 
    dmap->tc_venus.control4             = 0x00;

    dmap->tc_venus.io_general           = io_gen;   
    dmap->tc_venus.io_config            = io_conf;

    dmap->tc_venus.display_start        = 0x00000000;
    dmap->tc_venus.half_row_incr        = 0x00000100;
    dmap->tc_venus.display_pitch        = 0x00000140;
    dmap->tc_venus.cas_mask             = 0x7f;
    dmap->tc_venus.horiz_latency        = 0x00;

    dmap->tc_venus.horiz_end_sync       = 0x000d;
    dmap->tc_venus.horiz_half_line      = 0x0060;
    dmap->tc_venus.horiz_count_load     = 0x0000;
    dmap->tc_venus.vert_end_sync        = 0x02;
    dmap->tc_venus.vert_end_blank       = 0x12;
    dmap->tc_venus.vert_start_blank     = 0x0102;
    dmap->tc_venus.vert_count_load      = 0x0000;
    dmap->tc_venus.vert_interrupt       = 0x0103;
    if ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK )
    {
	dmap->tc_venus.horiz_end_blank      = 0x001c;
	dmap->tc_venus.horiz_start_blank    = 0x00bc;
	dmap->tc_venus.horiz_total          = 0x00c1;
	dmap->tc_venus.vert_total           = 0x0105;
    }
    else
    {
	dmap->tc_venus.horiz_end_blank      = 0x001b;
	dmap->tc_venus.horiz_start_blank    = 0x00bb;
	dmap->tc_venus.horiz_total          = 0x00ff;
	dmap->tc_venus.vert_total           = 0x01ff;
    }

    dmap->tc_venus.y_zoom               = 0x01;
    dmap->tc_venus.soft_register        = 0xf0;

    dmap->tc_venus.io_general           = io_gen & ~VENUS_SOFT_RESET;
    dmap->tc_venus.io_general           = io_gen;

    if ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) 
    {
	dmap->tc_venus.control3         = 0x7b;
	dmap->tc_venus.control1         = 0xf9;     
    }
    else
    {
	dmap->tc_venus.control3         = 0xfb;
	dmap->tc_venus.control1         = 0xf1;     
    }
    
    /*
     *  Initialize venus chip 1 which affects 1-bit monochrome and selection 
     *  memory operations.
     */
    dmap->mono_venus.control1           = 0x00;
    dmap->mono_venus.control2           = 0x00;
    dmap->mono_venus.control3           = 0x00;
    dmap->mono_venus.control4           = 0x00;
    dmap->mono_venus.refresh_interval   = 0x70;     /* Set refresh interval. */
    dmap->mono_venus.control1           = 0x13;
    dmap->mono_venus.io_config          = 0x0000;
 
    dmap->mono_venus.display_start      = 0x00000000;
    dmap->mono_venus.half_row_incr      = 0x00000080;
    dmap->mono_venus.display_pitch      = 0x00000050;
    dmap->mono_venus.cas_mask           = 0x3f;
    dmap->mono_venus.horiz_latency      = 0x00;
 
    dmap->mono_venus.horiz_end_sync     = 0x000d;
    dmap->mono_venus.horiz_end_blank    = 0x001b;
    dmap->mono_venus.horiz_start_blank  = 0x00bb;
    dmap->mono_venus.horiz_total        = 0x00ff;
    dmap->mono_venus.horiz_half_line    = 0x0000;
    dmap->mono_venus.horiz_count_load   = 0x0000;
 
    dmap->mono_venus.vert_end_sync      = 0x02;
    dmap->mono_venus.vert_end_blank     = 0x12;
    dmap->mono_venus.vert_start_blank   = 0x0102;
    dmap->mono_venus.vert_total         = 0x01ff;
    dmap->mono_venus.vert_count_load    = 0x0000;
    dmap->mono_venus.vert_interrupt     = 0x0000;
 
    dmap->mono_venus.y_zoom             = 0x01;
    dmap->mono_venus.soft_register      = 0xf0;
 
    dmap->mono_venus.control3           = 0xfb;
    if ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) 
    {
	dmap->mono_venus.control1           = 0xfb;
    }
    else
    {
	dmap->mono_venus.control1           = 0xf3;
    }
}   

/*"tcs_init_pal"
 *
 *  Initialize venus chips on SBus TCS Card to operate at PAL scan rates. 
 *  (25Hz interlaced 768 x 592.)
 *  If a genlock device is connected external (genlock) sync will be used.
 */
tcs_init_pal(softc)
    SOFTC           *softc; /* characteristics for our unit. */
{
    Tcs_Device_Map  *dmap;      /* -> device register area on TCS card. */
    u_int           io_conf;    /* Value to load into io config register. */
    u_int           io_gen;     /* Value to load into io general register. */

    dmap = softc->device_tcs;
    softc->timing_regimen = PAL_TIMING;
    softc->width = 768;
    softc->height = 592;
    softc->linebytes1 = softc->width / 8;
    softc->linebytes8 = softc->width;
    softc->linebytes32 = softc->width * 4;

    if ( dmap->tc_venus.io_general & VENUS_TCS_MODEL_A )
    {
	io_gen = ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) ? 
		 0x00bc : 0x00b4;
	io_conf = 0x00df;
    }
    else
    {
	io_gen = ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) ? 
		 0x787c : 0x78f4;
	io_conf = 0x1fdf;
    }

    /*
     *  Initialize venus chip 0 which affects Brooktree operations and color 
     *  buffer operations. Configure the system as a 24-bit true color card.
     */
    dmap->tc_venus.control1             = 0x00;
    dmap->tc_venus.control2             = 0x00;
    dmap->tc_venus.control3             = 0x00;
    dmap->tc_venus.control4             = 0x00;
    dmap->tc_venus.refresh_interval     = 0x70;     
    dmap->tc_venus.control1             = 0x11;
    dmap->tc_venus.control2             = 0x00; 
    dmap->tc_venus.control3             = 0x00; 
    dmap->tc_venus.control4             = 0x00;

    dmap->tc_venus.io_general           = io_gen;   
    dmap->tc_venus.io_config            = io_conf;

    dmap->tc_venus.display_start        = 0x00000000;
    dmap->tc_venus.half_row_incr        = 0x00000100;
    dmap->tc_venus.display_pitch        = 0x00000180;
    dmap->tc_venus.cas_mask             = 0x7f;
    dmap->tc_venus.horiz_latency        = 0x00;

    dmap->tc_venus.horiz_end_sync       = 0x0010;
    dmap->tc_venus.horiz_end_blank      = 0x0024;
    dmap->tc_venus.horiz_start_blank    = 0x00e4;
    dmap->tc_venus.horiz_total          = 0x00ea;
    dmap->tc_venus.horiz_half_line      = 0x0075;
    dmap->tc_venus.horiz_count_load     = 0x0000;

    dmap->tc_venus.vert_end_sync        = 0x02;
    dmap->tc_venus.vert_end_blank       = 0x16;
    dmap->tc_venus.vert_start_blank     = 0x0136;
    dmap->tc_venus.vert_total           = 0x0138;
    dmap->tc_venus.vert_count_load      = 0x0000;
    dmap->tc_venus.vert_interrupt       = 0x0137;

    dmap->tc_venus.y_zoom               = 0x01;
    dmap->tc_venus.soft_register        = 0xf0;

    dmap->tc_venus.io_general           = io_gen & ~VENUS_SOFT_RESET;
    dmap->tc_venus.io_general           = io_gen;

    if ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) 
    {
	dmap->tc_venus.control3         = 0x7b;
	dmap->tc_venus.control1         = 0xf9;     
    }
    else
    {
	dmap->tc_venus.control3         = 0xfb;
	dmap->tc_venus.control1         = 0xf1; 
    }
    
    /*
     *  Initialize venus chip 1 which affects 1-bit monochrome and selection 
     *  memory operations.
     */
    dmap->mono_venus.control1           = 0x00;
    dmap->mono_venus.control2           = 0x00;
    dmap->mono_venus.control3           = 0x00;
    dmap->mono_venus.control4           = 0x00;
    dmap->mono_venus.refresh_interval   = 0x70;     /* Set refresh interval. */
    dmap->mono_venus.control1           = 0x13;

    dmap->mono_venus.io_config          = 0x0000;
    dmap->mono_venus.display_start      = 0x00000000;
    dmap->mono_venus.half_row_incr      = 0x00000080;
    dmap->mono_venus.display_pitch      = 0x00000060;
    dmap->mono_venus.cas_mask           = 0x3f;
    dmap->mono_venus.horiz_latency      = 0x00;
 
    dmap->mono_venus.horiz_end_sync     = 0x0010;
    dmap->mono_venus.horiz_end_blank    = 0x0024;
    dmap->mono_venus.horiz_start_blank  = 0x00e4;
    dmap->mono_venus.horiz_total        = 0x00ff;
    dmap->mono_venus.horiz_half_line    = 0x0075;
    dmap->mono_venus.horiz_count_load   = 0x0000;
 
    dmap->mono_venus.vert_end_sync      = 0x02;
    dmap->mono_venus.vert_end_blank     = 0x16;
    dmap->mono_venus.vert_start_blank   = 0x0136;
    dmap->mono_venus.vert_total         = 0x01ff;
    dmap->mono_venus.vert_count_load    = 0x0000;
    dmap->mono_venus.vert_interrupt     = 0x0000;
 
    dmap->mono_venus.y_zoom             = 0x01;
    dmap->mono_venus.soft_register      = 0xf0;
 
    dmap->mono_venus.control3           = 0xfb;
    if ( dmap->tc_venus.io_general & VENUS_NO_GENLOCK ) 
    {
	dmap->mono_venus.control1           = 0xfb;
    }
    else
    {
	dmap->mono_venus.control1           = 0xf3;
    }
}   

/*"tcs_putcmap"
 *
 *  Local routine to set the color map values for either the 24-bit or the 
 *  monochrome frame buffer for RASTEROPS_TCS card. In addition to real 
 *  lookup table manipulation, emulation of 8-bit indexed color maps is 
 *  done for 24-bit mode. 
 *
 *  Note that for this card the lut load must occur during vertical retrace, 
 *  so the vertical retrace interrupt is used. Since the SPARCstation is really 
 *  quite fast relative to the loading of the lut we always load the entire lut. 
 *  This is possible because we maintain the whole lut in cmap_xxx within softc.
 *
 *      = 0 if success
 *      =   error indication otherwise
 */
 static int tcs_putcmap( softc, cmap )
    SOFTC          *softc; /* -> characteristics for our unit. */
    struct fbcmap  *cmap;  /* -> where to get color map (ioctl data area). */
{
    u_int          count;           /* # of color map entries to set. */
    int            force_update;    /* Indicates if access real luts. */
    u_int          index;           /* Index: entry in cmap_xxx to access. */
    int            n_entry;         /* Index: ordinal # of current map entry. */
    int            plane_group;     /* Top 7 bits of index = plane group. */

    /*
     *  Verify we need to actually move some data. Initialize variables from 
     *  caller's parameters, etc. If the plane group is zero, default an 
     *  appropriate plane group. 
     */
    if ( (cmap->count == 0) || !cmap->red || !cmap->green || !cmap->blue )
    {
	return 0;
    }
    count = cmap->count;
    index = cmap->index & PIX_ALL_PLANES;
    if (index+count > TC_CMAP_SIZE)
    {
	return EINVAL;
    }
    force_update = cmap->index & PR_FORCE_UPDATE;
    if ( !(plane_group = PIX_ATTRGROUP(cmap->index)) )
    {
	plane_group = softc->pg_enabled[PIXPG_24BIT_COLOR] ? 
	    PIXPG_24BIT_COLOR : PIXPG_8BIT_COLOR;
    }
    
    /*  If interruts are enabled disable so we can update things in peace.
     */
#ifdef NOTDEF
    if ( softc->device_tcs->tc_venus.control4 & VENUS_VERT_INT_ENA )
    {
	softc->device_tcs->tc_venus.control4 = 0;
	softc->device_tcs->tc_venus.status = 0;
    }
#endif NOTDEF

    /*  Move caller's data, there are 4 cases: kernel space and user space
     *  in overlay or colormap luts. 
     *  Note we move the data into the proper index location RASTEROPS_TCS 
     *  (see note 1 above.)
     */
    if ( plane_group == PIXPG_OVERLAY )
    {
	if (softc->tc1d.flags & CG8_KERNEL_UPDATE)
	{
	   softc->tc1d.flags &= ~CG8_KERNEL_UPDATE;
	   BCOPY( cmap->red, &softc->omap_red[index], count );
	   BCOPY(cmap->green,&softc->omap_green[index],count);
	   BCOPY(cmap->blue,&softc->omap_blue[index], count );
	}
	else
	{   
	    if ( COPYIN(cmap->red,&softc->omap_red[index], count ) ||
		 COPYIN(cmap->green,&softc->omap_green[index], count) ||
		 COPYIN( cmap->blue, &softc->omap_blue[index], count ) )
	    {
		return EFAULT;
	    }
	}    
    }
    else
    {
	if (softc->tc1d.flags & CG8_KERNEL_UPDATE)
	{
	    softc->tc1d.flags &= ~CG8_KERNEL_UPDATE;
	    BCOPY( cmap->red, &softc->cmap_red[index], count );
	    BCOPY( cmap->green, &softc->cmap_green[index], count);
	    BCOPY( cmap->blue, &softc->cmap_blue[index], count );
	}
	else
	{   
	    if ( COPYIN( cmap->red, &softc->cmap_red[index], count ) ||
		 COPYIN(cmap->green,&softc->cmap_green[index], count) ||
		 COPYIN( cmap->blue, &softc->cmap_blue[index], count ) )
	    {
		return EFAULT;
	    }
	}    
    }

    /*  If we are doing cg8 emulation, and hardware update is not forced,
     *  set 8-bit index emulation color table entries for 24-bit buffer. 
     *  (The check for the 8-bit plane group is here in case the kernel
     *   knows 8-bit hardware exists, but the driver did not export it to 
     *   Sunview.)
     */
    if ( !force_update && (softc->pg_enabled[PIXPG_8BIT_COLOR] == 0) && 
	((plane_group==PIXPG_24BIT_COLOR) || (plane_group==PIXPG_8BIT_COLOR)) )
    {
	for (n_entry = 0; n_entry < count; n_entry++, index++)
	{
	    softc->em_red[index] = softc->cmap_red[index];
	    softc->em_green[index] = softc->cmap_green[index];
	    softc->em_blue[index] = softc->cmap_blue[index];
	}
    }    

    /*  
     *  Set entries in actual Brooktree lookup tables for 8/24-bit buffer.
     */
    else if ( plane_group==PIXPG_24BIT_COLOR || plane_group==PIXPG_8BIT_COLOR )
    {
	softc->tc1d.flags |= CG8_24BIT_CMAP;
	softc->device_tcs->tc_venus.control4 = 5;
    }    

    /*  
     *  Set entries in actual Brooktree lookup tables for monochrome overlay.
     */
    else if ( plane_group == PIXPG_OVERLAY )
    {
	if ( softc->pg_enabled[PIXPG_24BIT_COLOR] != 0 )
	{
	    softc->tc1d.flags |= CG8_OVERLAY_CMAP;
	    softc->device_tcs->tc_venus.control4 = 5;
	}
    }
    
    else
    {
	printf("Illegal plane group %d encountered\n", plane_group);
	return EINVAL;
    }

    return 0;
}

/*"tcs_update_cmap"
 *
 *  For a TCS card update the color maps (color and overlay) if 
 *  required. This routine is called from cgeightpoll() when 
 *  a vertical interrupt is detected on a tcs board. It does 
 *  not clear the interrupt, as this is done in cgeightpoll().
 */
static tcs_update_cmap(softc)
    SOFTC           *softc;      /* -> to characteristics for our unit. */
{
    Tcs_Device_Map  *device_tcs; /* Pointer to tcs device register area. */
    int             n_entry;     /* Index: entry in cmap_xxx now accessing. */
    
    device_tcs = softc->device_tcs;
    if ( softc->tc1d.flags & CG8_24BIT_CMAP )
    {
	device_tcs->dac.ram_write_addr = 0;
	for (n_entry=0; n_entry < TC_CMAP_SIZE; n_entry++)
	{
	    device_tcs->dac.color = softc->cmap_red[n_entry];
	    device_tcs->dac.color = softc->cmap_green[n_entry];
	    device_tcs->dac.color = softc->cmap_blue[n_entry];
	}
	softc->tc1d.flags &= ~CG8_24BIT_CMAP;
    }       
    if ( softc->tc1d.flags & CG8_OVERLAY_CMAP )
    {
	for (n_entry=0; n_entry < TC_OMAP_SIZE; n_entry++)
	{
	    if ( !softc->sw_cursor_color_frozen || n_entry != 2 )
	    {
		device_tcs->dac.overlay_write_addr = 
		    translate_omap[n_entry];
		device_tcs->dac.overlay = softc->omap_red[n_entry];
		device_tcs->dac.overlay = softc->omap_green[n_entry];
		device_tcs->dac.overlay = softc->omap_blue[n_entry];
	    }
	}
	softc->tc1d.flags &= ~CG8_OVERLAY_CMAP;
    }       
}

/*"x_build_mmap_info"
 *
 *  Local routine to build table used to memory map frame buffers into
 *  user space (as opposed to kernel space which is done using fb_mapin).
 *  Note the order of mapping will properly map a traditional cg8 card
 *  (if other extraneous plane groups are not present.)
 */
x_build_mmap_info( softc )
    SOFTC           *softc; /* -> characteristics for this instance. */
{
    int         mmi;    /* Index: number of entries in mmap_info set up. */
    Mmap_Info   *mmp;   /* Cursor: entry in mmap_info to initialize. */
    u_int       offset; /* Offset into virtual memory of next fb. */

    mmi = 0;
    mmp = softc->mmap_info;
    offset = 0;

    if ( softc->pg_enabled[PIXPG_OVERLAY] )
    {
	mmp->group = PIXPG_OVERLAY;
	mmp->bgn_offset = offset;
	mmp->end_offset = offset + softc->fb_mono_mmap_size;
	mmp->sbus_delta = softc->fb_mono_sbus_delta;
	offset = mmp->end_offset;
	mmp++;
	mmi++;
    }
    if ( softc->pg_enabled[PIXPG_OVERLAY_ENABLE] )
    {
	mmp->group = PIXPG_OVERLAY_ENABLE;
	mmp->bgn_offset = offset;
	mmp->end_offset = offset + softc->fb_sel_mmap_size;
	mmp->sbus_delta = softc->fb_sel_sbus_delta;
	offset = mmp->end_offset;
	mmp++;
	mmi++;
    }
    if ( softc->pg_enabled[PIXPG_24BIT_COLOR] )
    {
	mmp->group = PIXPG_24BIT_COLOR;
	mmp->bgn_offset = offset;
	mmp->end_offset = offset + softc->fb_tc_mmap_size;
	mmp->sbus_delta = softc->fb_tc_sbus_delta;
	offset = mmp->end_offset;
	mmp++;
	mmi++;
    }
    if ( softc->pg_enabled[PIXPG_8BIT_COLOR] )
    {
	mmp->group = PIXPG_8BIT_COLOR;
	mmp->bgn_offset = offset;
	mmp->end_offset = offset + softc->fb_8bit_mmap_size;
	mmp->sbus_delta = softc->fb_8bit_sbus_delta;
	offset = mmp->end_offset;
	mmp++;
	mmi++;
    }
    if ( softc->pg_enabled[PIXPG_VIDEO_ENABLE] )
    {
	mmp->group = PIXPG_VIDEO_ENABLE;
	mmp->bgn_offset = offset;
	mmp->end_offset = offset + softc->fb_video_mmap_size;
	mmp->sbus_delta = softc->fb_video_sbus_delta;
	offset = mmp->end_offset;
	mmp++;
	mmi++;
    }

    softc->mmap_count = mmi;
}

/*"x_g_attr"
 *
 *  Local routine to return the characteristics (attributes) associated
 *  with the current frame buffer. (Called by tcone_ioctl).
 *
 *      = 0 if success
 *      = error indication otherwise
 */
 static int x_g_attr( softc, gattr )
    SOFTC           *softc; /* -> characteristics for our unit. */
    struct  fbgattr *gattr; /* -> where to put attributes(data area from ioctl) */
 {
    /*  Return the characteristics of the frame buffer being emulated. Fill in
     *  the characteristics which were received from the PROMs.
     */
    if ( softc->pg_enabled[PIXPG_24BIT_COLOR] )
    {
	*gattr = tc1_attrdefault;
	gattr->fbtype.fb_height = softc->height;
	gattr->fbtype.fb_width = softc->width;
	gattr->fbtype.fb_size = 
	    softc->fb_mono_mmap_size + softc->fb_sel_mmap_size +
	    softc->fb_tc_mmap_size;
    }
    else
    {
	*gattr = cg4_attrdefault;
	gattr->fbtype.fb_height = softc->height;
	gattr->fbtype.fb_width = softc->width;
	gattr->fbtype.fb_size = 
	    softc->fb_mono_mmap_size + softc->fb_sel_mmap_size +
				softc->fb_8bit_mmap_size;
    }

    return 0;
}

/*"x_g_emulation_mode"
 *
 *  Local routine to get emulation mode of driver operations.
 *  
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_g_emulation_mode( softc, mode )
    SOFTC           *softc; /* -> characteristics for our unit. */
    Pipio_Emulation *mode;  /* -> where to put mode of emulation. */
{
    int             pgi;    /* Index: next entry in pg_enabled to access. */

    /*
     *  Set up plane group information based on softc entries.
     *  Get timing regimen.
     */
    for ( pgi = 0; pgi < FB_NPGS; pgi++ )
    {
	if ( softc->pg_enabled[pgi] ) 
	{
	    mode->plane_groups[pgi] = 1;
	}
	else if ( softc->pg_possible[pgi] )
	{
	    mode->plane_groups[pgi] = 2;
	}
	else
	{
	    mode->plane_groups[pgi] = 0;
	}
    }

    mode->timing = softc->timing_regimen;

    return 0;
}

/*"x_getcmap"
 *
 *  Local routine to return color map values for either the 24-bit or the 
 *  monochrome frame buffer. In addition to real lookup table manipulation, 
 *  emulation of 8-bit indexed color maps is done for 24-bit mode. 
 *  (Called by tcone_ioctl).
 *
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_getcmap( softc, cmap )
    SOFTC          *softc; /* -> characteristics for our unit. */
    struct fbcmap  *cmap;  /* -> where to return the color map values. */
{
    u_int          count;           /* # of color map entries to return. */
    Tc1_Device_Map *device_reg;     /* Pointer to the Brooktrees. */
    Tcs_Device_Map *device_tcs;     /* Pointer to the TCS card device regs. */
    int            force_update;    /* Indicates if should access real luts. */
    u_int          index;           /* Index of first entry to access. */
    int            n_entry;         /* Loop index: current color map entry. */
    int            plane_group;     /* Top 7 bits of index = plane group. */
    u_char         tmp_blue[TC_CMAP_SIZE];   /* Temporary for blue values. */
    u_char         tmp_green[TC_CMAP_SIZE];  /* Temporary for green values. */
    u_char         tmp_red[TC_CMAP_SIZE];    /* Temporary for red values. */

    /*
     *  Initialize variables from caller's parameters, etc. Verify we need to
     *  actually move some data. A zero plane group defaults based on emulation 
     *  type.
     */
    count = cmap->count;
    if ( (count == 0) || !cmap->red || !cmap->green || !cmap->blue )
    {
	return 0;
    }

    index = cmap->index & PIX_ALL_PLANES;
    if (index+count > TC_CMAP_SIZE)
    {
	return EINVAL;
    }
    force_update = cmap->index & PR_FORCE_UPDATE;
    if ( !(plane_group = PIX_ATTRGROUP(cmap->index)) )
    {
	plane_group = softc->pg_enabled[PIXPG_24BIT_COLOR] ? 
		PIXPG_24BIT_COLOR : PIXPG_8BIT_COLOR;
    }

    /*  If there is no 8-bit frame buffer, and an update is not forced,
     *  get 8-bit index emulation color table entries for 24-bit buffer. 
     */
    if ( !force_update && (softc->pg_enabled[PIXPG_8BIT_COLOR] == 0)
	 && (plane_group == PIXPG_24BIT_COLOR) )
    {
	if ( COPYOUT( &softc->em_red[index], cmap->red, count) ||
	     COPYOUT( &softc->em_green[index], cmap->green, count) ||
	     COPYOUT( &softc->em_blue[index], cmap->blue, count) )
	{
	    return EFAULT;
	}
    }    

    /*  Get entries for the actual Brooktree lookup tables for 8/24-bit buffer. 
     */
    else if ( plane_group == PIXPG_24BIT_COLOR || 
	      plane_group == PIXPG_8BIT_COLOR )
    {
	switch(softc->fb_model)
	{
	  default:
	  case RASTEROPS_TC:
	  case RASTEROPS_TCP:
	    device_reg = softc->device_reg;
	    device_reg->dacs.bt457.all_address = index;
	    for (n_entry = 0; n_entry < count; n_entry++)
	    {
		tmp_red[n_entry] = device_reg->dacs.bt457.red_color;
		tmp_green[n_entry] = device_reg->dacs.bt457.green_color;
		tmp_blue[n_entry] = device_reg->dacs.bt457.blue_color;
	    }
	    break;
	  case RASTEROPS_TCS:
	    device_tcs = softc->device_tcs;
	    device_tcs->dac.ram_read_addr = index;
	    for (n_entry = 0; n_entry < count; n_entry++)
	    {
		tmp_red[n_entry] = device_tcs->dac.color;
		tmp_green[n_entry] = device_tcs->dac.color;
		tmp_blue[n_entry] = device_tcs->dac.color;
	    }
	    break;
	  case RASTEROPS_TCL:
	    break;
	}
	if ( COPYOUT( tmp_red, cmap->red, count) ||
	     COPYOUT( tmp_green, cmap->green, count) ||
	     COPYOUT( tmp_blue, cmap->blue, count) )
	{
	    return EFAULT;
	}
    }

    /*
     *  Get entries for the actual Brooktree lookup tables for monochrome 
     *  overlay.  (A shadow color map is accessed rather than hardware.)
     */
    else if (plane_group == PIXPG_OVERLAY)
    {   
	if ( COPYOUT( &softc->omap_red[index], cmap->red, count) ||
	     COPYOUT( &softc->omap_green[index], cmap->green, count) ||
	     COPYOUT( &softc->omap_blue[index], cmap->blue, count) )
	{
	    return EFAULT;
	}
    }    
    
    else
    {
	printf("Illegal plane group %d encountered\n", plane_group);
	return EINVAL;
    }

    return 0;
}

/*"x_g_fb_info"
 *
 *  Local routine to return a description of the frame buffers supported
 *  by this device instance. The plane groups are processed in order of
 *  their virtual address so that total_mmap_bytes can be properly calculated.
 *
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_g_fb_info( softc, info )
    SOFTC           *softc;     /* -> characteristics for our unit. */
    Pipio_Fb_Info   *info;      /* -> where to place frame buffer info. */
{
    int         fbi;    /* Index: next frame buffer description to move. */
    int         mmi;    /* Index: # of mmap_info entry looking at. */
    Mmap_Info   *mmp;   /* Cursor: mmap_info entry looking at. */

    fbi = 0;
    
    for ( mmi=0, mmp=softc->mmap_info; mmi < softc->mmap_count; mmi++, mmp++ )
    {
	info->fb_descriptions[fbi].group = mmp->group;
	info->fb_descriptions[fbi].width = softc->width;
	info->fb_descriptions[fbi].height = softc->height;
	info->fb_descriptions[fbi].mmap_size = mmp->end_offset-mmp->bgn_offset;
	info->fb_descriptions[fbi].mmap_offset = mmp->bgn_offset;
	switch(mmp->group)
	{
	  case PIXPG_24BIT_COLOR:
	    info->fb_descriptions[fbi].depth = 32;
	    info->fb_descriptions[fbi].linebytes = softc->linebytes32;
	    break;
	  case PIXPG_8BIT_COLOR:
	    info->fb_descriptions[fbi].depth = 8;
	    info->fb_descriptions[fbi].linebytes = softc->linebytes8;
	    break;
	  default:
	    info->fb_descriptions[fbi].depth = 1;
	    info->fb_descriptions[fbi].linebytes = softc->linebytes1;
	    break;
	}
	fbi++;
    }
	    
    info->frame_buffer_count = softc->mmap_count;
    info->total_mmap_size = softc->mmap_info[softc->mmap_count-1].end_offset;
    return 0;
}    

#if NWIN > 0
/*"x_g_pixrect"
 *
 *  Local routine to return the pixrect structure for use by the kernel to 
 *  access this device instance.
 *
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_g_pixrect( softc, dev, fbpr )
    SOFTC            *softc; /* -> characteristics for our unit. */
    dev_t            dev;    /* =  device number. */
    struct fbpixrect *fbpr;  /* -> where to return pixrect info (ioctl data)*/
 {
    struct cg8_data *cg8d;      /* Pointer to pixrect private data area. */ 
    Pipio_Fb_Info   fb_info;    /* Frame buffer info about this device instance. */
    struct cg4fb    *fbp;       /* Index: fb in pixrect private area now init'ing */ 
    int             i;          /* Index: # of frame buffer now initializing. */
    u_int           initplanes; /* Plane group/mask for current plane group. */
    Pixrect         *tmppr;     /* Pointer to pixrect (to call cg8_putattributes). */

    tmppr = &softc->pr;
    
    /* "Allocate" pixrect by pointing into our instance's private storage (the 
     *  kernel will use this pixrect, so access is certainly possible!). Set the 
     *  private data pointer. 
     */
    fbpr->fbpr_pixrect = &softc->pr;
    softc->pr.pr_data = (caddr_t) & softc->tc1d;

    /*  Initialize pixrect area within our software characteristics area
     *  now that we are allocated. Also default driver's private data.
     */
    softc->pr.pr_ops = &tcp_ops;
    softc->pr.pr_size.x = softc->width;
    softc->pr.pr_size.y = softc->height;
    
    softc->tc1d.flags = 0;
    softc->tc1d.planes = 0;
    softc->tc1d.fd = minor (dev);

    /*  Get the information about the frame buffers and memories this instance 
     *  supports. Then for each frame buffer provided by this device, initialize
     *  its characteristics (other than image pointers) in the pixrect private
     *  data area.
     */
    x_g_fb_info(softc, &fb_info);

    softc->tc1d.num_fbs = fb_info.frame_buffer_count;
    DEBUGF(4, ("ioctl GPIXRECT num fbs = %d\n", softc->tc1d.num_fbs));

    for (fbp=softc->tc1d.fb, i=0; i < fb_info.frame_buffer_count; fbp++, i++) 
    {
	fbp->group = fb_info.fb_descriptions[i].group;
	fbp->depth = fb_info.fb_descriptions[i].depth;
	switch(fb_info.fb_descriptions[i].depth)
	{
	    case 1:     fbp->mprp.mpr.md_linebytes = softc->linebytes1; break;
	    case 8:     fbp->mprp.mpr.md_linebytes = softc->linebytes8; break;
	    case 32:    fbp->mprp.mpr.md_linebytes = softc->linebytes32; break;
	    default:
		printf("Error unsupported depth %d encountered for plane group %i in GPIXRECT\n", 
			fb_info.fb_descriptions[i].depth, 
			fb_info.fb_descriptions[i].group);
		break;
	}
	fbp->mprp.mpr.md_offset.x = 0;
	fbp->mprp.mpr.md_offset.y = 0;
	fbp->mprp.mpr.md_primary = 0;
	fbp->mprp.mpr.md_flags = fb_info.fb_descriptions[i].depth != 1 ? 
	    MP_DISPLAY | MP_PLANEMASK : MP_DISPLAY;
	fbp->mprp.planes = 
	    fb_info.fb_descriptions[i].depth != 1 ? PIX_ALL_PLANES : 0;
	DEBUGF(5, ("ioctl GPIXRECT group %d depth %d\n", 
	    fbp->group, fbp->depth));
    }

    /*  The majority of the pixrect structure is now set up, so initialize the 
     *  image pointers and perform any memory initializations which are 
     *  necessary. We loop backwards to guarantee that the last frame buffer 
     *  accessed is the monochrome.
     */
    for (cg8d = cg8_d(tmppr), i = fb_info.frame_buffer_count-1; i >= 0; i--) 
    {
	switch ( cg8d->fb[i].group )
	{
	    case PIXPG_OVERLAY:
		initplanes = PIX_GROUP(PIXPG_OVERLAY);
		cg8_putattributes (tmppr, &initplanes);
		DEBUGF(5, ("ioctl GPIXRECT monochrome pr_depth = %d\n", 
		    tmppr->pr_depth));
		cg8d->fb[i].mprp.mpr.md_image = 
			    cg8d->mprp.mpr.md_image = (short *) softc->fb_mono;
		pr_rop (tmppr, 0, 0, softc->pr.pr_size.x, softc->pr.pr_size.y, 
		    PIX_SRC, (Pixrect *) 0, 0, 0);
		break;

	    case PIXPG_OVERLAY_ENABLE:
		initplanes = PIX_GROUP(PIXPG_OVERLAY_ENABLE);
		cg8_putattributes (tmppr, &initplanes);
		DEBUGF(5, ("ioctl GPIXRECT selection pr_depth = %d\n", 
		    tmppr->pr_depth));
		cg8d->fb[i].mprp.mpr.md_image = cg8d->mprp.mpr.md_image 
					      = (short *) softc->fb_sel;
		pr_rop(tmppr, 0, 0, softc->pr.pr_size.x, softc->pr.pr_size.y, 
		    PIX_SRC | PIX_COLOR (1), 
			(Pixrect *) 0, 0, 0); 
		break;

	    case PIXPG_8BIT_COLOR:
		cg8d->flags |= CG8_EIGHT_BIT_PRESENT;
		cg8d->fb[i].mprp.mpr.md_image = cg8d->mprp.mpr.md_image 
					      = (short *) softc->fb_8bit;
		cg8d->mprp.mpr.md_linebytes = 0;
		break;

	    case PIXPG_24BIT_COLOR:
		cg8d->fb[i].mprp.mpr.md_image = 
		    cg8d->mprp.mpr.md_image 
		    = (short *) -1;
		cg8d->mprp.mpr.md_linebytes = 0;
		break;

	    case PIXPG_VIDEO_ENABLE:
		cg8d->flags |= CG8_PIP_PRESENT;
		cg8d->fb[i].mprp.mpr.md_image = 
		    cg8_d (tmppr)->mprp.mpr.md_image = (short *) -1;
		cg8d->mprp.mpr.md_linebytes = 0;
		break;
	}
    }
		

    /*  Enable video display. 
     */
    switch(softc->fb_model)
    {
      default:
	mfb_set_video(&softc->device_reg->sun_mfb_reg, FBVIDEO_ON);        
	break;
      case RASTEROPS_TCS:
	break;
    }

    return 0;
}

#endif NWIN > 0

/*"x_g_type"
 *
 *  Local routine to return the type of the active frame buffer and its
 *  resolution, etc.
 *
 *      = 0 if success
 *      = error indication otherwise
 */
 static int x_g_type( softc, fb )
    SOFTC          *softc; /* -> characteristics for our unit. */
    struct fbtype  *fb;    /* -> where to put type info (ioctl data area). */
 {
    /*  Return information based on the type of frame buffer being emulated.
     */
    if ( softc->pg_enabled[PIXPG_24BIT_COLOR] )
    {
	/*  If we are emulating a Sun monochrome BW2 return that data, otherwise 
	 *  return the actual information about the current frame buffer.
	 */
	switch (tc1_attrdefault.sattr.emu_type) 
	{
	    case FBTYPE_SUN2BW:
		*fb = tc1typedefault;
		fb->fb_height = softc->height;
		fb->fb_width = softc->width;
		fb->fb_size = softc->fb_mono_mmap_size;
		break;

	    default:
		*fb = tc1_attrdefault.fbtype;
		fb->fb_height = softc->height;
		fb->fb_width = softc->width;
		fb->fb_size = 
		    softc->fb_mono_mmap_size + softc->fb_sel_mmap_size + 
		    softc->fb_8bit_mmap_size + softc->fb_tc_mmap_size;
		break;
	}
    }
    else
    {
	*fb = cg4_attrdefault.fbtype;
	fb->fb_height = softc->height;
	fb->fb_width = softc->width;
	fb->fb_size = softc->fb_mono_mmap_size + softc->fb_sel_mmap_size + 
	    softc->fb_8bit_mmap_size;
    }

    return 0;
}

/*"x_s_emulation_mode"
 *
 *  Local routine to set emulation mode for driver operations.
 *  Use the mode specification to determine how to configure the
 *  system.
 *  
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_s_emulation_mode( softc, mode )
    SOFTC           *softc; /* characteristics for our unit. */
    Pipio_Emulation *mode;  /* -> mode of emulation to set. */
{
    int             color;       /* Index: color to put into look-up table. */
    Tc1_Device_Map  *device_reg; /* Quick pointer to device registers. */
    Tcs_Device_Map  *device_tcs; /* Quick pointer to tcs device registers. */
    int             pgi;         /* Index: entry in pg_enabled to access. */

    /*
     *  Process the emulation timing and size specified by the caller.
     *  If our device is not capable of providing the specified timing
     *  or size return an error.
     */
    switch ( mode->timing )
    {
      case NATIVE_TIMING:
	if ( softc->fb_model == RASTEROPS_TCS )
	{
	    tcs_init_native(softc);
	}
	break;
      case NTSC_TIMING:
	if ( softc->fb_model != RASTEROPS_TCS ) return EINVAL;
	tcs_init_ntsc(softc);
	break;
      case PAL_TIMING:
	if ( softc->fb_model != RASTEROPS_TCS ) return EINVAL;
	tcs_init_pal(softc);
	break;
      default:
	return EINVAL;
    }


    /*
     *  Set only the plane groups specified by the caller.
     */
    for ( pgi = 0; pgi < FB_NPGS; pgi++ )
    {
	if ( mode->plane_groups[pgi] == 1 )
	{
	    if ( softc->pg_possible[pgi] == 0 ) return EINVAL;
	    softc->pg_enabled[pgi] = 1;
	}
	else
	{
	    softc->pg_enabled[pgi] = 0;
	}
    }

    /*  
     *  If 24-bit memory is enabled, reset true color ramp in luts.
     */
    if ( softc->pg_enabled[PIXPG_24BIT_COLOR] )
    {
	switch(softc->fb_model)
	{
	  default:
	    device_reg = softc->device_reg;
	    device_reg->dacs.bt457.all_address = 0;
	    for (color = 0; color < TC_CMAP_SIZE; color++)
	    {
		device_reg->dacs.bt457.red_color = color;
		device_reg->dacs.bt457.green_color = color;
		device_reg->dacs.bt457.blue_color = color;
	    }
	    break;
	  case RASTEROPS_TCS:
	    device_tcs = softc->device_tcs;
	    device_tcs->dac.ram_write_addr = 0;
	    for (color = 0; color < TC_CMAP_SIZE; color++)
	    {
		device_tcs->dac.color = color;
		device_tcs->dac.color = color;
		device_tcs->dac.color = color;
	    }
	    break;
	  case RASTEROPS_TCL:
	    break;
	}
    }

    /*
     *  Recompute the memory mapping information, so new configuration
     *  will be mapped properly.
     */
    x_build_mmap_info(softc);

    return 0;
}

/*"x_s_map_slot"
 *
 *  Local routine to return information using test ioctl entry
 *  point.
 *
 *      = 0 if success
 *      = error indication otherwise
 */
static int x_s_map_slot( softc, phys_addr )
   SOFTC    *softc;     /* -> characteristics for our unit. */
   u_int    *phys_addr; /* -> where to put attributes(data area from ioctl) */
{
    addr_t  reg;        /* Mapping point for region. */

    if ( softc->test_basepage ) return 0;
    if ( !(reg = (addr_t)map_regs( (addr_t)*phys_addr, (u_int)0x1000, 0x01)) )
    {
	return -1;
    }
    softc->test_basepage = fbgetpage(reg);
    return 0;
}

/*
 *  OVERVIEW OF TC BOARD HARDWARE
 *
 *      (1) The SBus TC Card product
 *          contains two frame buffers. One frame buffer is 24 bits deep, 
 *          and its pixel values are treated as 3 8-bit indices into 3 separate 
 *          8-in-8-out lookup tables to yield 8 bits for each of the red, green,
 *          and blue guns of the monitor. Each of the 8-in-8-out lookup tables 
 *          is loaded with a linear ramp (or possibly a gamma corrected ramp) 
 *          yielding a true color display of pixel values.
 *
 *                  +------------+
 *                  |            |        +---+
 *                  |            |---/--->|vlt|---/---> red
 *                  |     24     |   8    +---+   8 
 *                  |            +
 *                  |    bit     |        +---+
 *          ---/--->|            |---/--->|vlt|---/---> green 
 *            24    |    frame   |   8    +---+   8   
 *                  |            +
 *                  |   buffer   |        +---+
 *                  |            |---/--->|vlt|---/---> blue
 *                  |            |   8    +---+   8 
 *                  +------------+
 *
 *      (2) The other frame buffer is monochrome and is 1 bit deep.
 *          Its pixel values are treated as inputs into the overlay
 *          of the 8-in-8-out lookup tables associated with the 
 *          24-bit frame buffer. Thus, the 1-bit frame buffer appears
 *          as an overlay over the 24-bit frame buffer on the screen.
 *          For each pixel, if the overlay is visible the 24-bit value
 *          is not and vice-versa. Whether the overlay is visible or not
 *          is controlled by a separate 1-bit deep selection memory. The
 *          output of this memory is the input for the overlay transparency
 *          bit on each of the 8-in-8-out lookup tables. Thus, the value in 
 *          the selection memory associated with a pixel specifies which 
 *          value is displayed for a given pixel, the one from the 24-bit 
 *          memory or the monochrome overlay. 
 *
 *                  +--------+                         +--------+
 *                  |        |      +-----------+      |        | 
 *                  |   1    |--/-->|  red vlt  |<--/--|   1    | 
 *                  |        |  1   +-----------+   1  |        | 
 *                  |  bit   |      +-----------+      |  bit   |
 *          ---/--->|        |--/-->| green vlt |<--/--|        |
 *             1    |  frame |  1   +-----------+   1  |select. |
 *                  |        |      +-----------+      |        |
 *                  | buffer |--/-->| blue vlt  |<--/--| memory |
 *                  |        |  1   +-----------+   1  |        |
 *                  +--------+                         +--------+
 *                               
 * 
 */

/*
 *  INDEXED COLOR MAP EMULATION
 *
 *  The SBus TCP support provides emulation of indexed color maps for both
 *  the 24-bit true color frame buffer and the monochrome overlay frame buffer.
 *  This support is done jointly by this kernel pixrect driver (tcone.c) and
 *  the normal pixrect driver colormap support (tc1_colormap.c).
 *
 *  For indexed color map emulation in the 24-bit buffer the kernel level 
 *  driver for the tc1 maintains a main memory color map with 256 entries. When 
 *  pixels are written or read in emulation mode the entries in this color map 
 *  are accessed to perform a mapping between the 8-bit value used by the user, 
 *  and the 24-bit true color value actually written to frame buffer memory. 
 *  This mapping is performed by routines in this module, which are called from 
 *  other modules (e.g., cg8_getput.c). In emulation mode the actual lookup 
 *  table entries in the 3 Brooktree 457 RAMDACs are not touched, only the 
 *  separate main memory color map. However, it is also possible to actually 
 *  access the look up table entries in the RAMDACs. This is done by specifying 
 *  the PR_FORCE_UPDATE flag in the plane parameter passed to cg8_getcolormap 
 *  or cg8_putcolormap.
 *
 *  The monochrome overlay emulation is handled in tc1_colormap.c. It is also
 *  possible to by-pass the emulation and actually write the four overlay color
 *  values. 
 */

/*""
 *  PHYSICAL TO VIRTUAL MEMORY MAPPING FOR DEVICES ON THE SBus TC Card
 *
 *  This driver performs mapping for the devices on the SBus TC Card. The
 *  mapping makes the physical devices appear to be contiguous with each other
 *  within the virtual space
 *
 *          Virtual Space                       Physical Space
 *
 *                                  Physical                    Offset from
 *                                  address:                    device registers
 *                                  xx000000+------------------+
 *                                          | On-board PROMs   |
 *                                          +------------------+
 *
 *                                  xx400000+------------------+00000000
 *                                          | Device registers |
 *  Offset from device                      +------------------+
 *  registers
 *  00000000+------------------+    xx800000+------------------+00400000
 *          | Monochrome buffer|            | True color buffer|
 *  00020000+------------------+            +------------------+
 *          | Selection memory |                        
 *  00040000+------------------+    xxC00000+------------------+00800000
 *          | True color buffer|            | Monochrome buffer|
 *  00435000+------------------+            +------------------+
 *          | Device registers |                        
 *          +------------------+    xxD00000+------------------+00900000
 *                                          | Selection memory |
 *                                          +------------------+
 *
 *  The offset from the device registers in virtual space is defined near the
 *  beginning of this file in constants of the form TCP_xxx_VADDR_OFFSET, where
 *  xxx is MONOCHROME, TRUE_COLOR, etc. 
 *
 *  The offset from the device registers in physical space is defined near the
 *  beginning of this file in constants of the form TCP_xxx_PADDR_OFFSET.
 *
 *  The offset from the beginning of the SBus physical device space (and thus
 *  the on-board PROMs) is defined in constants of the form 
 *  TCP_xxx_sbus_delta_offset.
 *
 *  The mapping from virtual to physical space is accomplished in cgeightmmap, 
 *  and is based on an offset relative to the beginning of the device registers
 *  in virtual address space. The reason the offset is based on the device 
 *  registers is that the device registers where passed into map_regs to set up 
 *  the virtual space.
 *
 *  ADDITIONAL DEVICES ON THE SBus TCP Card (w/pip option)
 *
 *  The SBus TCP Card provides two additional buffers which must be mapped 
 *  these are the 8-bit view of the 24-bit true color memory and the video 
 *  enable memory. This results in a virtual space which looks like:
 *
 *          Virtual Space                       Physical Space
 *
 *                                  Physical                    Offset from
 *                                  address:                    device registers
 *                                  xx000000+------------------+
 *                                          | On-board PROMs   |
 *                                          +------------------+
 *
 *                                  xx400000+------------------+00000000
 *                                          | Device registers |
 *                                          +------------------+
 *
 *                                  xx700000+------------------+00000000
 *                                          | 8-bit buffer     |
 *  Offset presented to cgeightmmap for        +------------------+
 *  mapping
 *  00000000+------------------+    xx800000+------------------+00400000
 *          | Monochrome buffer|            | True color buffer|
 *  00020000+------------------+            +------------------+
 *          | Selection memory |                        
 *  00040000+------------------+    xxC00000+------------------+00800000
 *          | True color memory|            | Monochrome buffer|
 *  00435000+------------------+            +------------------+
 *          | 8-bit buffer     |                        
 *  00533000+------------------+    xxD00000+------------------+00900000
 *          | Video enable mem.|            | Selection memory |
 *  00553000+------------------+            +------------------+
 *          | Device Registers |
 *          +------------------+    xxE00000+------------------+00A00000
 *                                          | Video enable mem.|
 *                                          +------------------+
 */
