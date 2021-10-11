#ifndef lint
static char sccsid[] = "@(#)fd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

/*
 * Floppy Disk Driver
 *
 * Supports the Intel 82072 fdc (in non dma mode)
 * contains support for vpc software package.
 *
 * ASSUMES:
 *	only one controller chip
 *	(for now) only one drive - although multiple drives should be "easy"
 *	only sun4c autoconf
 *	controller is *always* present on the board
 * DESIGN NOTES: since (1) no one in their right mind would have more than
 * two floppy drives, and (2) ditto for more than one controller (especially
 * without dma support!!), and
 * (3) 3.5" drives don`t have RDY so you can't do overlapped seeks so
 * *all* operations are single threaded.
 *   therefore --- this device driver is designed to be as simple as possible.
 *
 * data structures:
 *	controller - "fdctlr" -
 *	per unit (drive) - "fdunit" - one preallocated per drive configured,
 *		hangs off fdctlr.
 *	per partition (minor device) - "fdpart" - one preallocated (for
 *		now 'cause they're short) for each possible minor device
 *	command/status block - "fdcsb" - one preallocated, hangs off of fdctlr
 *	a dk_label structure for each unit - hangs off fdunit
 *	one buf struct for all physio/ioctls that need it.
 *
 * control flow:
 *   for data xfr:
 *	read/write - call physio, which calls strategy
 *	strategy - checks operation, hangs buf struct off fdctlr, calls start
 *		if not already busy.
 *	start - if a bp is on queue, grab it.  grab/wait-for the fdctlr and
 *		its fdcsb.  fill in args, call fdexec().  due to need to handle
 *		cylinder crossings, we sleep for done in here.
 *	fdexec - check DISKCHG, check command busy bit - TBD if these errors
 *		else stuff commands into chip.  if command has no interrupts
 *		then immediately grab result.
 *		To allow transparent retrys, all sleeps on operations done
 *		are in here - since error checking for diskchanged has to
 *		occur at process level (for recalibrate).
 *   for ioctls/raw/open-label_read/attach - grab/wait-for the fdctlr and
 *		its fdcsb.  fill in args, call fdexec.  ioctl may sleep for
 *		done on fdcsb.
 * note:  sleep for fdctlr (and its fdcsb) on address of fdctlr.c_flags,
 *	sleep for operation on the fdcsb on csb pointed to by fdctlr.c_csb.
 *	(this allows nested csb sleeping).
 * interrupt:
 * @level 11
 *  hardware: Operates like a DMA engine, see fd_asm.s for it.
 *		The two parts of the driver (fd_asm.s and fd.c) communicate
 *		via a set of shared variables, defined in fd_asm.s - so that
 *		fd.c can be deconfigured from the system.  see the comments
 *		in fd_asm.s for a more complete description of the protocol.
 * @level4
 *  software: Invoked by hardware after op is done and results are all in.
 *		It snarfs results back into fdcsb; if there was an error it
 *		invokes fdrecover() to retry and returns.  If no errors,
 *		If someone is waiting on fdcsb done, then wake them.
 *		Note that fdstart() will get activated if it was waiting on
 *		the fdctlr/csb.
 *
 * ADDITION: 3rd density support was added
	if (fdctlr.c_units[fdctlr.c_csb->csb_unit]->un_chars->medium)
 *
 *
 */


#include "fd.h"

#if NFD > 0
#if NFD > 3
DON'T MAKE NFD > 3, OK? - the driver doesn't support that
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>

#include <sun/autoconf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sun/vddrv.h>

#include <sbusdev/fdreg.h>
#include <sbusdev/fdvar.h>

#include <machine/psl.h>
#if defined(sun4c) || defined(sun4m)
#include <machine/auxio.h>
#include <machine/intreg.h>
#endif sun4c || sun4m

#include <sys/vm.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>

#include <machine/cpu.h>

#include <sundev/mbvar.h>
#include <sys/fcntlcom.h>


/* Sony MP-F17W-50D Drive Parameters
 *				High Capacity
 *	Capacity unformatted	2Mb
 *	Capacity formatted	1.47Mb
 *	Encoding method	 MFM
 *	Recording density	17434 bpi
 *	Track density		135 tpi
 *	Cylinders		80
 *	Heads			2
 *	Tracks			160
 *	Rotational speed	300 rpm
 *	Transfer rate		250/500 kbps
 *	Latency (average)	100 ms
 *	Access time
 *		Average		95 ms
 *		Track to track	3 ms
 *	Head settling time	15 ms
 *	Motor start time	500 ms
 *	Head load time		? ms
 */


/*
 * DEFINES ********************************************************************
 */
/* the auxiliary register */
#define	Auxio (*(u_char *)AUXIO_REG)
#define MEDIUM_DENSITY	0x40
#define SEC_SIZE_CODE	(fdctlr.c_csb->csb_unit]->un_chars->medium ? 3 : 2)
#define	CMD_READ	(MT + SK + FRAW_RDCMD + MFM)
#define	CMD_WRITE	(MT + FRAW_WRCMD + MFM)
/* for main status and fifo registers on chip */
#ifdef I82077
#define Msr *fd_msr
#define Dsr *fd_dsr
#define Dor *fd_dor
#define Dir *fd_dir
#define Fifo *fd_fifo
#else
#define	Msr (fdctlr_reg->fdc_control)	/* main status reg */
#define	Dsr (fdctlr_reg->fdc_control)	/* data rate select reg */
#define	Fifo (fdctlr_reg->fdc_fifo)
#endif

#define	FDREAD	1			/* for fdrw() flag */
#define	FDWRITE	2			/* for fdrw() flag */

#define	FDPRI	PRIBIO			/* at what priority to sleep */

#define	FD_CRETRY 1000000		/* retry while sending comand */
#define	FD_RRETRY 1000000		/* retry while getting results */

#define	FDXC_SLEEP	0x1		/* tell fdexec to sleep 'till done */
#define	FDXC_CHECKCHG	0x2		/* tell fdexec to check disk chnged */

#define	IOCTL_DEBUG	0xaa		/* For debugging only */

/*
 * EXTERNS ********************************************************************
 */
/*
 * these are in fd_asm.s
 */
#ifndef lint
#ifdef I82077
extern union fdcreg *fdctlr_reg;	/* ptr to floppy controller registers */
extern u_char	*fd_msr,	 	/* ptr to msr */
		*fd_dsr,		/* ptr to dsr */
		*fd_dor,		/* ptr to dor - 82077 only */
		*fd_dir,		/* ptr to dir - 82077 only */
		*fd_fifo;
#else
extern struct fdcreg *fdctlr_reg; /* ptr to floppy controller registers */
#endif I82077
	/* for interface with low level interrupt handler */
	/* I = input - set before operation, O = output - available after */
extern u_int fdintr_opmode;	/* mode for interrupt handler to run in */
				/* I: 1 = r/w/fmt, 2 = seek/recal; */
				/* O: hw intr sets it to 4 to notify us */
extern u_char *fdintr_addr;	/* I/O: (current) data xfer address */
extern u_int fdintr_len;	/* I: data xfer length; O: remainder */
extern u_int fdintr_status;	/* I: n/a; O: 0 = ok, TBD XXX NYD errors */
extern u_int fdintr_timeout;	/* timeout value for fdc_hardintr ??? */
extern u_char fdintr_statbuf[10]; /* I: n/a; O: 10 bytes of status buffer */
extern u_char *fdintr_statp;	/* I: pointer for fdintr_statbuf; O: n/a */

#else lint
/* keep lint happy - pretend to declare these here */
#ifdef I82077
union fdcreg *fdctlr_reg;
u_char	*fd_msr, *fd_dsr, *fd_dor, *fd_dir, *fd_fifo;
#else
struct fdcreg *fdctlr_reg;
#endif I82077
u_int fdintr_opmode;
u_char *fdintr_addr;
u_int fdintr_len;
u_int fdintr_status;
u_int fdintr_timeout;
u_char fdintr_statbuf[10];
u_char *fdintr_statp;
#endif lint


/*
 * STATIC and other forward reference FUNCTIONS
 */
static void fdselect();
static fdeject();
static fdsense_chng();
fdwatch();

#if defined(sun4c) || defined(sun4m)	/* campus sends TC in fdc_hardintr */
extern int	fdc_hardintr();		/* defined in  ../sun4c/fd_asm.s */
extern int	fdc_intr();		/* defined in  ../sun4c/fd_asm.s */
#else
static fdterm_count();
static fdsense_dnsty();
#endif


char *strcpy();

int fdopen();
int fdclose();
int fdstrategy();
int fddump();
int fdsize();
int fdread();
int fdwrite();
int fdioctl();

int fdtype = -1;	/* Holds type of controller (i.e FD_82072 or FD_82077).
			 *
			 * XXX - This should be initialized in fdattach by
			 *	 checking the OBP.
			 */

extern int nulldev();
extern int seltrue();

/*
 * BSS UN-INITIALIZED DATA ****************************************************
 */
struct fdunit fdunits[NFD];	/* array of floppy unit state structs */
int	fdpri;			/* mask this level in critical sections */
struct buf fdkbuf;		/* a buf structure for raw io and ioctls */

char    fdkl_alabel[NFD][128];	/* space for packed label's ascii strings */

/*
 * INITIALIZED DATA ***********************************************************
 */
struct fdctlr fdctlr = {		/* floppy ctlr state struct */
	&fdunits[0],			/* init pts to unit data structs */
#if NFD > 1
	&fdunits[1],
#endif
#if NFD > 2
	&fdunits[2],
#endif
#if NFD > 3
	&fdunits[3],
#endif
	0 };		/* rest is uninitialized */

short rwretry = 25;
short skretry = 25;

/*
 * ERROR HANDLING *************************************************************
 */

/*
 * flags/masks for error printing.
 * the levels are for severity
 */
#define	FDEP_L0		0	/* chatty as can be - for debug! */
#define	FDEP_L1		1	/* best for debug */
#define	FDEP_L2		2	/* minor errors - retries, etc. */
#define	FDEP_L3		3	/* major errors */
#define	FDEP_L4		4	/* catastophic errors, don't mask! */
#define	FDEP_LMAX	4	/* catastophic errors, don't mask! */
#define	FDERRPRINT(l, m, args)	\
	{ if (((l) >= fderrlevel) && ((m) & fderrmask)) printf args; }

/*
 * for each function, we can mask off its printing by clearing its bit in
 * the fderrmask.  Some functions (attach, ident) share a mask bit
 */
#define	FDEM_IDEN 0x00000001	/* fdidentify */
#define	FDEM_ATTA 0x00000001	/* fdattach */
#define	FDEM_SIZE 0x00000002	/* fdsize */
#define	FDEM_OPEN 0x00000004	/* fdopen */
#define	FDEM_GETL 0x00000008	/* fdgetlabel */
#define	FDEM_CLOS 0x00000010	/* fdclose */
#define	FDEM_STRA 0x00000020	/* fdstrategy */
#define	FDEM_STRT 0x00000040	/* fdstart */
#define	FDEM_RDWR 0x00000080	/* fdrdwr */
#define	FDEM_CMD  0x00000100	/* fdcmd */
#define	FDEM_EXEC 0x00000200	/* fdexec */
#define	FDEM_RECO 0x00000400	/* fdrecover */
#define	FDEM_INTR 0x00000800	/* fdintr */
#define	FDEM_WATC 0x00001000	/* fdwatch */
#define	FDEM_IOCT 0x00002000	/* fdioctl */
#define	FDEM_RAWI 0x00004000	/* fdrawioctl */
#define	FDEM_DUMP 0x00008000	/* fddump */
#define	FDEM_GETC 0x00010000	/* fdgetcsb */
#define	FDEM_RETC 0x00020000	/* fdretcsb */
#define	FDEM_RESE 0x00040000	/* fdreset */
#define	FDEM_RECA 0x00080000	/* fdrecalseek */
#define	FDEM_FORM 0x00100000	/* fdformat */
#define	FDEM_RW   0x00200000	/* fdrw */
#define	FDEM_CHEK 0x00400000	/* fdcheckdisk */
#define	FDEM_DSEL 0x00800000	/* fdselect */
#define	FDEM_EJEC 0x01000000	/* fdeject */
#define	FDEM_SCHG 0x02000000	/* fdsense_chng */
#define	FDEM_PACK 0x04000000	/* fdpacklabel */
#ifdef I82077
#define	FDEM_MOFF 0x08000000	/* fdmotoff */
#endif I82077

int	fderrmask = 0xFFFFFFFF;

static short	fderrlevel = 3;

int	fdhardtimeout	= 100000;

int	tosec = 16;	/* long timeouts for sundiag for now */


/*
 * SPECIFY/CONFIGURE CMD PARAMETERS *******************************************
 */
/* so they can be easily changed */
u_char fdspec[2] = {0xc2, 0x33};	/* the "specify" parameters */

u_char fdconf[3] = {0x64, 0x58, 0x00};	/* the "configure" parameters */

/*
 * DEFAULT CHARACTERISTICS ****************************************************
 */
struct fdk_char fdtypes[] = {
/* struct fdk_char fdchar_highdens = */
{	0,		/* medium */
	500,		/* transfer rate */
	80,		/* number of cylinders */
	2,		/* number of heads */
	512,		/* sector size */
	18,		/* sectors per track */
	-1,		/* (NA) # steps per data track */
},
/* struct fd_char fdchar_meddens */
{
	1,              /* medium */
	500,            /* transfer rate */
	77,             /* number of cylinders */
	2,              /* number of heads */
	1024,           /* sector size */
	8,              /* sectors per track */
	-1,             /* (NA) # steps per data track */
},
/* struct fdk_char fdchar_lowdens = */
{	0,		/* medium */
	250,		/* transfer rate */
	80,		/* number of cylinders */
	2,		/* number of heads */
	512,		/* sector size */
	9,		/* sectors per track */
	-1,		/* (NA) # steps per data track */
},
};

int nfdtypes = sizeof (fdtypes)/sizeof (fdtypes[0]);
int curfdtype;

/*
 * DEFAULT LABEL & PARTITION MAPS *********************************************
 */
/*
 * since a label occupies 512 bytes, and we need at least 2, no make that 4
 * for 40 and 80 in both high and low density. = 2K of mostly 0's.
 * we stash them in a packed array, then call fdunpacklabel() to
 * expand them when needed.
 */
struct packed_label fdlbl_high_80 = {
	"3.5\" floppy cyl 80 alt 0 hd 2 sec 18", /* for compatibility */
	300,				/* rotations per minute */
	80,				/* # physical cylinders */
	0,				/* alternates per cylinder */
	1,				/* interleave factor */
	80,				/* # of data cylinders */
	0,				/* # of alternate cylinders */
	2,				/* # of heads in this partition */
	18,				/* # of 512 byte sectors per track */
	{ 0, 79 * 2 * 18 },	/* part 0 - all but last cyl */
	{ 79, 1 * 2 * 18 },	/* part 1 - just the last cyl */
	{ 0, 80 * 2 * 18 },	/* part 2 - "the whole thing" */
	{ 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};

struct packed_label fdlbl_medium_80 = {
	"3.5\" floppy cyl 77 alt 0 hd 2 sec 8",
	360,				/* rotations per minute */
	77,				/* # physical cylinders */
	0,				/* alternates per cylinder */
	1,				/* interleave factor */
	77,				/* # of data cylinders */
	0,				/* # of alternate cylinders */
	2,				/* # of heads in this partition */
	8,				/* # of 512 byte sectors per track */
	{ 0, 76 * 2 * 8 * 2 },	/* part 0 - all but last cyl */
	{ 76, 1 * 2 * 8 * 2 },	/* part 1 - just the last cyl */
	{ 0, 77 * 2 * 8 * 2 },	/* part 2 - "the whole thing" */
	{ 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};

struct packed_label fdlbl_low_80 = {
	"3.5\" floppy cyl 80 alt 0 hd 2 sec 9",
	300,				/* rotations per minute */
	80,				/* # physical cylinders */
	0,				/* alternates per cylinder */
	1,				/* interleave factor */
	80,				/* # of data cylinders */
	0,				/* # of alternate cylinders */
	2,				/* # of heads in this partition */
	9,				/* # of 512 byte sectors per track */
	{ 0, 79 * 2 * 9 },	/* part 0 - all but last cyl */
	{ 79, 1 * 2 * 9 },	/* part 1 - just the last cyl */
	{ 0, 80 * 2 * 9 },	/* part 2 - "the whole thing" */
	{ 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};

/*
 * SUN4C AUTOCONF SUPPORT ******************************************************
 */

/* for sun4c autoconf stuff, referred to in ioconf.c */
int fdidentify(), fdintr(), fdattach();
struct dev_ops fd_ops = {
	1,
	fdidentify,
	fdattach,
};

/*
 * LOADABLE MODULE SUPPORT ****************************************************
 */

#ifndef lint
struct bdevsw fd_bdevsw = {
	fdopen, fdclose, fdstrategy, fddump, fdsize, 0
};

struct cdevsw fd_cdevsw = {
	fdopen, fdclose, fdread, fdwrite, fdioctl, nulldev, seltrue, 0, 0
};

struct vdldrv fddrv = {
	VDMAGIC_DRV,			/* Drv_magic */
#ifdef I82077
	"Onboard 82072/82077 Floppy Driver",	/* *Drv_name */
#else
	"Onboard 82072 Floppy Driver",	/* *Drv_name */
#endif
	&fd_ops,			/* *Drv_dev_ops */
	&fd_bdevsw,			/* *Drv_bdevsw */
	&fd_cdevsw,			/* *Drv_cdevsw */
	16,				/* Drv_blockmajor */
	54				/* Drv_charmajor */
};
#endif !lint

/*
 * FUNCTIONS ******************************************************************
 */

#if defined(sun4c) || defined(sun4m)	/* for new style autoconf stuff */

/*
 * fdidentify
 *	identify ctlr
 */
int
fdidentify(name)
	char *name;
{
	FDERRPRINT(FDEP_L0, FDEM_IDEN, ("fdidentify: name %s\n", name));
	if (strcmp(name, "fd") == 0 || strcmp(name, "SUNW,fdtwo") == 0) {
		return (1);
	} else
		return (0);
}

/*
 * fdattach
 *	attach controller
 */
fdattach(dev)
	register struct dev_info *dev;
{
	struct fdunit *un;
#ifdef I82077
	u_char *fdtestaddr; 
#endif I82077

	FDERRPRINT(FDEP_L0, FDEM_ATTA, ("fdattach\n"));

	/* ASSUME we only have one controller, ever! */
	dev->devi_unit = 0;

	/*
	 * map in controller regs; ASSUME it's ALWAYS there
	 */
#ifdef I82077
	fdctlr_reg = (union fdcreg *) map_regs(dev->devi_reg->reg_addr,
	    dev->devi_reg->reg_size,
	    dev->devi_reg->reg_bustype);

	/* See if we have an 072 or 077.  If we have an 072 the
	 * dsr/msr will be at location 0x2 since 0x2 maps to 0x0.
	 * If we have an 077, the dor will be at 0x2.
	 */
	fdtype = FD_82077;
	fdtestaddr = (u_char *) fdctlr_reg + 2;
	if (*fdtestaddr == 0x80) {

		/* We expect the msr to 0x80, but we'll do some
		 * more checking just in case.
		 */
		*fdtestaddr = 0x2; 	/* default value of dsr */
		DELAY(100);
		if (*fdtestaddr == 0x80)
			fdtype = FD_82072;
	}
		
	switch (fdtype) {
	case FD_82072:

		FDERRPRINT(FDEP_L0, FDEM_ATTA, ("fdattach: type is 82072\n"));

		/* Initialize addresses of key registers.
		 */
		fd_msr = &fdctlr_reg->fdc_82072_reg.fdc_control;
		fd_dsr = &fdctlr_reg->fdc_82072_reg.fdc_control;
		fd_fifo = &fdctlr_reg->fdc_82072_reg.fdc_fifo;

		FDERRPRINT(FDEP_L0, FDEM_ATTA, ("fdattach: msr/dsr at 0x%x\n", fd_msr));
		/* Insure that the eject line is reset and TC is clear.
		 */
		set_auxioreg(AUX_EJECT, 1);
		set_auxioreg(AUX_TC, 0);
		break;

	case FD_82077:

		FDERRPRINT(FDEP_L0, FDEM_ATTA, ("fdattach: type is 82077\n"));

		/* Initialize addresses of key registers.
		 */
		fd_msr = &fdctlr_reg->fdc_82077_reg.fdc_control;
		fd_dsr = &fdctlr_reg->fdc_82077_reg.fdc_control;
		fd_dor = &fdctlr_reg->fdc_82077_reg.fdc_dor;
		fd_dir = &fdctlr_reg->fdc_82077_reg.fdc_dir;
		fd_fifo = &fdctlr_reg->fdc_82077_reg.fdc_fifo;

		FDERRPRINT(FDEP_L0, FDEM_ATTA, ("fdattach: msr/dsr at 0x%x\n", fd_msr));

		/* The 82077 doesnt use the first configuration parameter
		 * so let's adjust that while we know we're an 82077.
		 */
		fdconf[0] = 0;

		/* The reset bit must be cleared to take the 82077 out of
		 * reset state. (Dor = 0x0f after Reset).
		 */
		fd_set_dor_reg(0xe8, 0);
		DELAY(5);
		fd_set_dor_reg(RESET | DRVSEL, 1);
		DELAY(5);

		/* Insure that the eject line is reset and TC is clear.
		 */
		set_auxioreg(AUX_TC, 0);
		break;
	}
#else
	fdctlr_reg = (struct fdcreg *) map_regs(dev->devi_reg->reg_addr,
	    dev->devi_reg->reg_size,
	    dev->devi_reg->reg_bustype);

	set_auxioreg(AUX_EJECT, 1); /* insure that the eject line is reset */
#endif I82077

	fdctlr.c_csb = (struct fdcsb *)kmem_zalloc(sizeof (struct fdcsb));
	/* setup for commands will initialize the csb according to need */

	un = fdctlr.c_units[0]; /* XXX unit 0 is hardcoded for now */

	/*
	 * set initial controller/drive/disk "characteristics/geometry"
	 */
	un->un_chars =
	    (struct fdk_char *)kmem_zalloc(sizeof (struct fdk_char));
	curfdtype = 0; /* init current fdtype pointer to beginning of tbl */
	*(un->un_chars) = fdtypes[curfdtype]; /* structure copy */
	un->un_label =
	    (struct packed_label *)kmem_zalloc(sizeof (struct packed_label));
	*(un->un_label) = fdlbl_high_80; /* structure copy */

	if (fdctlr.c_csb == NULL || un->un_chars == NULL || un->un_label == NULL) {
		printf("fdattach%d:  kmem_zalloc failed!\n", dev->devi_unit);
		return (-1);
	}


	/* Add a direct trap to the assembly language interrupt handler. */
	fdctlr.c_intr = dev->devi_intr->int_pri;

	switch (fdtype) {
	case FD_82072:
		/* reset, specify and configure the controller */
		fdgetcsb();
		fdreset();
		fdretcsb();
		fdselect(0, 0);	/* DE-select drive zero (used in fdreset) */
				/* XXX unit 0 is hardcoded for now */
		if (!settrap(fdctlr.c_intr, fdc_hardintr))
			panic("fdattach: Cannot set fd trap vector");
		break;
	case FD_82077:
		addintr(11, fdc_intr, dev->devi_name, 0);
		/* reset, specify and configure the controller 
		 * for Campus2s do the reset after adding ourselves to the
		 * level 11 handler list. The reset causes an interrupt when
		 * the driver is modloaded. Therefore I rearranged the 
		 * addintr and reset sequences.  Since nobody seems
	  	 * to why so, I have no choice but to  rearrange the code.
		 */
		fdgetcsb();
		fdreset();
		fdretcsb();
		fdselect(0, 0);	/* DE-select drive zero (used in fdreset) */
				/* XXX unit 0 is hardcoded for now */
		break;
	default:
		panic("fdattach: cannot identify controller");
		break;
	}

	/* Set up level 4 soft interrupt service routine. */
	fdpri = ipltospl(4);
	addintr(4, fdintr, dev->devi_name, 0);

	report_dev(dev);

	return (0);
}

#ifndef lint
/*
 * fdinit
 *	for a loadable driver
 *	modload modstat & modunload
 */
/*ARGSUSED*/
fdinit(fcn, vdp, vdi, vds)
	int			fcn;
	register struct vddrv	*vdp;
	caddr_t			vdi;
	struct vdstat		*vds;
{
	struct fdunit		*un;
	int status = 0;

	switch (fcn) {

	case VDLOAD:
		vdp->vdd_vdtab = (struct vdlinkage *)&fddrv;
		break;

	case VDUNLOAD:
		un = fdctlr.c_units[0]; /* XXX unit0 hardcoded */
		/*
		 * is anybody still open?
		 */
		if (un->un_openmask || un->un_exclmask) {
			status = EBUSY;
		} else {
			/* Remove the direct trap vector */
			switch (fdtype) {
			case FD_82072:
				if (!settrap(fdctlr.c_intr, NULL))

					panic("fdinit: Cannot reset fd trap vector");
				break;
			default:
				break;
			}
#ifdef VDDRV
			(void) remintr(4, fdintr);
#endif

			/* free space allocated in fdattach */
			kmem_free((caddr_t)fdctlr.c_csb, sizeof (struct fdcsb));
			kmem_free((caddr_t)un->un_chars,
			    sizeof (struct fdk_char));
			kmem_free((caddr_t)un->un_label,
			    sizeof (struct packed_label));
		}
		break;

	case VDSTAT:
		break;

	default:
		printf("fdinit: unknown function 0x%x\n", fcn);
		status = EINVAL;
	}

	return (status);
}

#endif !lint

#endif sun4c sun4m	/* for new style autoconf stuff */

/*
 * fdsize
 *	return the size of a logical partition
 */
fdsize(dev)
	dev_t dev;
{
	struct fdunit *un;
	struct dk_map *lp;
	int	unit;

	if ((unit = UNIT(dev)) >= NFD || fdtype == -1)
		return (-1);

	un = fdctlr.c_units[ unit ];
	lp = &un->un_label->dkl_map[ PARTITION(dev) ];

	FDERRPRINT(FDEP_L0, FDEM_SIZE,
		("fdsize: unit %d,  part %d,  size 0x%x\n",
		unit, PARTITION(dev), lp->dkl_nblk));
	/*
	 * for now simply return what's there (from defaults set in fdattach)
	 * XXX someday maybe attempt to read a label if it hasn't been done yet
	 */
	return ((int)lp->dkl_nblk);
}

/*
 * fdopen
 *
 */
fdopen(dev, flag)
	dev_t dev;
	int flag;
{
	int unit, part;
	struct fdunit *un;
	u_char	pmask;
	struct dk_map *dkm;
	int	err;
	static int fdopening=0;
	int fderr = 0;
	int fderr2 = 0;

	/* check unit */
	unit = UNIT(dev);
	if (unit >= NFD || fdtype == -1) {
		fderr = ENXIO;
		goto fdout;
	}
	un = fdctlr.c_units[ unit ];

	/* check partition */
	part = PARTITION(dev);
	pmask = 1 << part;
	dkm = &(un->un_label->dkl_map[part]);
	if (dkm->dkl_nblk == 0) {
		fderr = ENXIO;
		goto fdout;
	}

	FDERRPRINT(FDEP_L1, FDEM_OPEN, ("**********FDOPEN --- A --->: unit %d, part %d\n",
		unit, part));

	/* go to sleep if there is a previous fdopen in process */
	while (fdopening)
		(void) sleep((caddr_t)&fdopening, FDPRI);
	fdopening=1;

	/*
	 * insure that drive is present with a recalibrate on first open.
	 */
	if (un->un_openmask == 0) {
		fdgetcsb();
		err = fdrecalseek(unit, -1, 0); /* no check changed! */
		fdretcsb();
		if (err) {
			FDERRPRINT(FDEP_L3, FDEM_OPEN,
				("fd%d: drive not ready\n", unit));
			fdselect(unit, 0); /* deselect drv on last close */
			fderr = ENXIO;
			goto fdout;
		}
	}
	fdctlr.c_csb->csb_unit = (u_char) UNIT(dev);

	/*
	 * check for previous exclusive open, or trying to exclusive open
	 */
	if ((un->un_exclmask & pmask) ||
	    ((flag & FEXCL) && (un->un_openmask & pmask))) {
		fderr = EBUSY;
		goto fdout;
	}
	if (flag & FEXCL) {
		un->un_exclmask |= pmask;
	}

	/* don't attempt access, just return successfully */
	if (flag & (FNDELAY | FNBIO)) {
		goto out;
	}

	if ((un->un_openmask == 0) && (fderr2 = fdgetlabel(unit))) {
		/* didn't find label (couldn't read anything) */
		FDERRPRINT(FDEP_L3, FDEM_OPEN,
			("fd%d: drive not ready\n", unit));
		un->un_exclmask &= ~(pmask);
		if (un->un_openmask == 0) {
			fdselect(unit, 0); /* deselect drv on last close */
		}
		fderr = fderr2;
		if (fderr == 0)
			fderr = EIO;
		goto fdout;
	}

	/*
	 * if opening for writing, check write protect on diskette
	 */
	if (flag & FWRITE) {
		fdgetcsb();
		err = fdsensedrv(unit) & WP_SR3;
		fdretcsb();
		if (err) {
			fdselect (unit, 0);
			fderr = EROFS;
			goto fdout;
		}
	}

	/* mark open */
out:
	un->un_openmask |= pmask;

fdout:
	fdopening = 0;
	wakeup((caddr_t)&fdopening);
	FDERRPRINT(FDEP_L1, FDEM_OPEN, ("___________ END FDOPEN\n"));
	return (fderr);
}

/*
 * fdgetlabel - read the SunOS lable off the diskette
 *	if it can read a valid label it does so, else it will use a
 *	default.  If it can`t read the diskette - that is an error.
 *
 * RETURNS: 0 for ok - meaning that it could at least read the device,
 *	!0 for error XXX TBD NYD error codes
 */
fdgetlabel(unit)
	int unit;
{
	register struct dk_label *label;
	register struct fdunit *un;
	short *sp;
	short count;
	short xsum;
	int	i, tries;
	int	err = 0;
#if NO
	short	oldlvl;
#endif

	FDERRPRINT(FDEP_L1, FDEM_GETL, ("fdgetlabel: unit %d\n", unit));
	un = fdctlr.c_units[unit];

	un->un_flags &= ~(FDUNIT_LABELOK|FDUNIT_UNLABELED);

	/*
	 * get some space to play with the label
	 */
	label = (struct dk_label *)kmem_zalloc(sizeof (struct dk_label));
	FDERRPRINT(FDEP_L0, FDEM_GETL,
		("fdgetlabel, kmem_zalloc: ptr = 0x%x, size = 0x%x\n",
		label, sizeof (struct dk_label)));

	if (label == NULL)
		return (ENOMEM);

	/*
	 * read block 0 (0/0/1) to find the label
	 * (disk is potentially not present or unformatted)
	 */
	/* noerrprint since this is a private cmd */
#if NO
	oldlvl = fderrlevel;
	fderrlevel = FDEP_L4;
#endif
	/* try different characteristics (ie densities) */
	/*
	 * if curfdtype is -1 then the current characteristics were set by
	 * ioctl and need to try it as well as everything in the table
	 *
	 * Note: with new medium density, we need to check the content
	 *       of the label as well (medium/high could be mistaken
	 *	 since the rotation speed is different (360 ->300 )
	 * 	 (360rpm +medium could be confused with 300rpm + high)
	 */
	if (curfdtype == -1)
		tries = nfdtypes+1;
	else
		tries = nfdtypes;
	for (i=0; i < tries; i++) {
		FDERRPRINT(FDEP_L0, FDEM_GETL,
		    ("TRYING READ LABEL  curfdtype = %d\n", curfdtype));
		err = fdrw(FDREAD, unit, 0, 0, 1, (caddr_t)label,
		    sizeof (struct dk_label));
		FDERRPRINT(FDEP_L0, FDEM_GETL,
			("fdgetlabel: fdrw returned %d\n", err));
		FDERRPRINT(FDEP_L0, FDEM_GETL,
			("label: rpm = %d\n", label->dkl_rpm));
		FDERRPRINT(FDEP_L0, FDEM_GETL,
			("       pcyl = %d\n", label->dkl_pcyl));
		FDERRPRINT(FDEP_L0, FDEM_GETL,
			("       nsect = %d\n", label->dkl_nsect));

		if (!err) break;
		/* try the next entry in the characteristics tbl */
		/* if curfdtype is -1, the nxt entry in tbl is 0 (the first) */
		curfdtype = (curfdtype + 1) % nfdtypes;
		*(un->un_chars) = fdtypes[curfdtype]; /* structure copy */
		/* data transfer rate given to the controller in fdexec */
	}
#if NO
	fderrlevel = oldlvl;	/* print errors again */
#endif

	if (err)
		goto out;			/* couldn't read anything */

	FDERRPRINT(FDEP_L0, FDEM_GETL, ("LABEL SUCCESSFULLY READ\n"));
	/*
	 * _something_ was read  -  look for unixtype label
	 */
	if (label->dkl_magic != DKL_MAGIC) {
		/* not a label - no magic number */
		goto nolabel;	/* no errors, tho` no label */
	}

	count = sizeof (struct dk_label)/sizeof (short);
	sp = (short *)label;
	xsum = 0;
	while (count--)
		xsum ^= *sp++;	/* should add up to 0 */
	if (xsum) {
		/* not a label - checksum didn't compute */
		goto nolabel;	/* no errors, tho` no label */
	}

	/*
	 * if label found pack it away
	 */
	fdpacklabel(label, un->un_label, unit);
	un->un_flags |= FDUNIT_LABELOK;
	goto out;

nolabel:
	/*
	 * if not found fill in label info from default (mark default used)
	 */
	un->un_flags |= FDUNIT_UNLABELED;
	switch (un->un_chars->secptrack) {
	case 9:
		*(un->un_label) = fdlbl_low_80; /* structure copy */
		break;
	case 8:
		*(un->un_label) = fdlbl_medium_80; /* structure copy */
		break;
	case 18:
	default:
		*(un->un_label) = fdlbl_high_80; /* structure copy */
		break;
	}

out:
	kmem_free((caddr_t)label, sizeof (struct dk_label));
	return (err);

}

/*
 * fdclose
 *
 */
fdclose(dev)
dev_t	dev;
{
	int unit;
	register struct fdunit *un;
	u_char	npmask;		/* "NOT" partition mask */

	unit = UNIT(dev);
	FDERRPRINT(FDEP_L1, FDEM_CLOS, ("fdclose: dev=0x%x\n", dev));
	un = fdctlr.c_units[ unit ];

	/*
	 * invalidate stuff
	 * XXX - clean busy operations still waiting??? - HOW CAN THAT HAPPEN?
	 */

	npmask = ~(1 << PARTITION(dev));
	un->un_exclmask &= npmask;
	un->un_openmask &= npmask;
	if (un->un_openmask == 0) {
		fdselect(unit, 0);	/* deselect drive on last close */
		un->un_flags &= ~FDUNIT_CHANGED;
	}
	return (0);
}

/*
 * fdstrategy
 *	checks operation, hangs buf struct off fdctlr, calls fdstart
 *	if not already busy.  Note that if we call start, then the operation
 *	will already be done on return (start sleeps).
 */
fdstrategy(bp)
	register struct buf *bp;
{
	register struct fdunit *un;
	int unit, part;
	int oldpri;
	int err;
	struct dk_map *dkm;

	FDERRPRINT(FDEP_L1, FDEM_STRA, ("fdstrategy: bp = 0x%x, dev = 0x%x\n",
		bp, bp->b_dev));

	unit = UNIT(bp->b_dev);
	un = fdctlr.c_units[ unit ];	/* at least unit # valid, get un */
	part = PARTITION(bp->b_dev);

	dkm = &(un->un_label->dkl_map[ part ]);

	/* if this request is already off the end (existence checked in open) */
	if ((bp->b_blkno > dkm->dkl_nblk)) {
		FDERRPRINT(FDEP_L3, FDEM_STRA,
			("fd%d: block %d is past the end! (nblk=%d)\n",
			unit, bp->b_blkno, dkm->dkl_nblk));
		bp->b_error = ENOSPC;
		goto bad;
	}

	/* if at end of file, skip out now */
	if (bp->b_blkno == dkm->dkl_nblk) {
		if ((bp->b_flags & B_READ) == 0) {
			/* a write needs to get an error! */
			bp->b_error = ENOSPC;
			goto bad;
		}
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}

	/* if operation not a multiple of sector size, is error! */
	if (bp->b_bcount % 512)  {
		FDERRPRINT(FDEP_L3, FDEM_STRA,
			("fd%d: count %d must be a multiple of 512\n",
			unit, bp->b_bcount));
		bp->b_error = EINVAL;
		goto bad;
	}

	/*
	 * if the operation will go off the end of disk...
	 *	handled in fdstart()
	 */

	/*
	 ******** START CRITICAL SECTION ********
	 */
	oldpri = splr(fdpri);	/* raise priority */

	/*
	 * queue the buf request on controller's queue - first in, first out
	 * basis - nothing fancy here!
	 * NOTE: list from head.b_forw is singly linked using av_forw,
	 * also a pointer to the end of the queue is also kept in head.b_back
	 */
	bp->av_forw = (struct buf *)0;	/* last guy points to nobody */
	if (fdctlr.c_head.b_forw) {	/* if someone already on queue... */
		fdctlr.c_head.b_back->av_forw = bp;	/* ...put on end... */
	} else {
		fdctlr.c_head.b_forw = bp;		/* ...else onto head */
	}
	fdctlr.c_head.b_back = bp;	/* and do back link also */
	bp->av_back = (struct buf *)0;	/* just to be clean */

	/* if no operation in progress, call start */
	if (!(fdctlr.c_flags & FDCFLG_BUSY)) {
		err = fdstart();
	}
	if (err)
		goto bad;

	(void) splx(oldpri);
	/*
	 ******** END CRITICAL SECTION *******
	 */

	return;

bad:	/* bad operation - mark buffer with err and vamanos */
	bp->b_resid = bp->b_bcount;
	bp->b_flags |= B_ERROR;
	if ((bp->b_flags & B_DONE) == 0)
		iodone(bp);
	return;
}

/*
 * fdstart
 *	called from fdstrategy() or from fdintr() to setup and
 *	start operations of read or write only (using buf structs).
 *	Because the chip doesn't handle crossing cylinder boundaries on
 *	the fly, this takes care of those boundary conditions.  Note that
 *	it sleeps until the operation is done *within fdstart* - so that
 *	when fdstart returns, the operation is already done.
 *
 */
fdstart()
{
	register struct buf *bp;
	register struct fdcsb *csb;
	register struct fdunit *un;
	register struct fdk_char *ch;
	struct dk_map *dkm;
	int 	err;
	u_int	sec_size;
	u_int	unit, part;
	u_int	len;
	u_int	logical_blk;
	u_int	fd_action;
	u_int	phys_blk, phys_blk_begin;
	u_int	plus1;		/* real data starts middle of 1st sector */
	u_int	minus1;		/* real data ends  middle of last sector */
	u_int	sector_count;	/* Number of physical sectors to xfer */
	u_int	plus2;		/* xfer not multiple of sec_size */
	u_int	phys_blk_begin_part, phys_blk_end_part;
	caddr_t	addr_buf, temp_buf;

	err = 0;

loop:
	/*
	 * if no buf on queue, just bailout
	 */
	bp = fdctlr.c_head.b_forw;
	if (bp == (struct buf *)0)
		return (err);

	bp->b_flags &= ~B_ERROR;
	bp->b_error = 0;
	bp->b_resid = bp->b_bcount;	/* init resid */

	FDERRPRINT(FDEP_L1, FDEM_STRT,
		("fdstart: bp=0x%x blkno=0x%x bcount=0x%x\n",
		bp, bp->b_blkno, bp->b_bcount));

	/* we're here only if there are queued bufs */
	fdgetcsb();	/* get csb (maybe wait for it) */

	csb = fdctlr.c_csb;
	csb->c_current = bp; 
	unit = UNIT(bp->b_dev);	/* floppy unit number */
	un = fdctlr.c_units[ unit ];
	ch = un->un_chars;
	part = PARTITION(bp->b_dev);
	dkm = &(un->un_label->dkl_map[ part ]);
	sec_size = ch->sec_size;
	FDERRPRINT(FDEP_L1, FDEM_STRT,
	("sec_size = %d  secptrack = %d\n",ch->sec_size, ch->secptrack));

	bp_mapin(bp);		/* map in buffers */

	/*
	 * now fill in the fdcsb to send to controller
	 */
	csb->csb_un = un;		/* ptr to fdunit struct */
	csb->csb_unit = unit;		/* floppy unit number */
	csb->csb_part = part;		/* floppy partition number */

	/* fill in cmd parameters that won't change */

	if (bp->b_flags & B_READ)
		fd_action = CMD_READ;
	else
		fd_action = CMD_WRITE;


	csb->csb_cmds[5] = ch->medium ? 3 : 2;
	csb->csb_cmds[6] = ch->secptrack;	/* EOT - # of sectors/trk */
	csb->csb_cmds[7] = GPLN;		/* GPL - gap 3 size code */
	csb->csb_cmds[8] = SSSDTL;		/* DTL - be 0xFF if N != 0 */

	csb->csb_ncmds = NCBRW;		/* how many command bytes */
	csb->csb_nrslts = NRBRW;		/* number of result bytes */

	/* opflags for hwintr handler et.al.*/
	csb->csb_opflags = CSB_OFXFEROPS | CSB_OFTIMEIT;

	/* ================================================================ */

	logical_blk = bp->b_blkno;		/* start logicl block in part */
	phys_blk = logical_blk >> ch->medium;	/* physical start block */

	plus1 = 0;
	plus2 = 0;
	minus1 = 0;
	sector_count = bp->b_bcount / sec_size;

	if (ch->medium) {
		plus1 = logical_blk % 2;	/* Set if start block odd */
		plus2 = (bp->b_bcount % ch->sec_size ? 1 : 0);
		minus1 = plus1 ^ plus2;
		if (plus1 | plus2)
			sector_count++;
	}

	phys_blk_begin_part = (dkm->dkl_cylno * ch->secptrack * ch->nhead);
	phys_blk_begin = phys_blk_begin_part + phys_blk;
	phys_blk_end_part = phys_blk_begin_part + (dkm->dkl_nblk >> ch->medium);

	/*
	 * Truncate len if extend beyond partition
	 */
	if (phys_blk_begin + sector_count > phys_blk_end_part)
		len = (phys_blk_end_part - phys_blk_begin) * ch->sec_size;
	else
		len = sector_count * ch->sec_size;

	addr_buf = bp->b_un.b_addr;

	FDERRPRINT(FDEP_L1, FDEM_STRT,
	("log %d  phys %d  secount %d len %d ppm %d%d%d\n", logical_blk,
	    phys_blk, sector_count, len, plus1, plus2, minus1));

	/*
	 * Call transfer routine.
	 * If we have medium density, we need to use temp_buf
	 * and each write will be preceded by a read (unless plus1=minus1=0)
	 */

	if (plus1 || minus1) {

		/*
		 * allocate len bytes in temp_buf,  multiple of real sector
		 */
		temp_buf = (caddr_t)kmem_zalloc(len);

		/*
		 * Transfer involving temp_buf
		 * if READ: we read more in temp_buf, then throw away 512 bytes
		 * if WRITE: we need to do a READ/MODIFY/WRITE
		 */
		if (fd_action == CMD_READ) {
			err = temp_disk(fd_action, phys_blk_begin, len, temp_buf);
			buf_temp(fd_action, plus1, bp, temp_buf);
		} else {
			err = temp_disk(CMD_READ, phys_blk_begin, len, temp_buf);
			buf_temp(fd_action, plus1, bp, temp_buf);
			if (err == 0)
				err = temp_disk(fd_action, phys_blk_begin, len, temp_buf);
		}
		kmem_free((caddr_t)temp_buf, len);

	} else {

		/*
		 * Normal transfer directly from or to struct buf (addr_buf)
		 */
		FDERRPRINT(FDEP_L0, FDEM_STRT,("Normal transfer\n"));
		err = temp_disk(fd_action, phys_blk_begin, len, addr_buf);
	}

	/* finish off the buffer operation just done */
	FDERRPRINT(FDEP_L0, FDEM_STRT,
		("fdstart done: b_resid %d, b_count %d, csb_rlen %d\n",
		bp->b_resid, bp->b_bcount, csb->csb_rlen));

	/* dequeue off of header */
	if (bp->av_forw != (struct buf *)0) {
		fdctlr.c_head.b_forw = bp->av_forw;
		bp->av_forw = (struct buf *)0;
		/* c_head.b_back doesn't matter (we hope) */
	} else {
		/* nobody else there, clear em all */
		fdctlr.c_head.b_forw = (struct buf *)0;
		fdctlr.c_head.b_back = (struct buf *)0;
	}
	/* if (the special buffer) just wants woke, just wake it only */
	/* raw io also comes here */
	if (bp == &fdkbuf) {
		bp->b_flags |= B_DONE;
		wakeup((caddr_t)bp);
	} else {
		/* block io comes here */
		/* NOTE: the iodone() does a bp_mapout() */
		iodone(bp);
	}
	/* release csb */
	fdctlr.c_flags &= ~FDCFLG_BUSY;
	fdretcsb();

	goto loop;
}




/*
 * TRANSFER data from  Disk to temp_buf or vice-versa
 * --------------------------------------------------
 *
 * fd_action		Type of transfer (Read from or Write to disk)
 * phys_block_begin_i	phys block where to start on the disk
 * len_i		Number of Bytes to transfer.
 * addr_i		temp_buf address.
 *
 * plus1		real data starts middle of 1st sector
 * minus1		real data ends middle of last sector.
 * sector_count		Number of physical sectors to xfer
 *
 */



temp_disk(fd_action, phys_blk_begin_i, len_i, addr_i)

	u_int fd_action;
	u_int phys_blk_begin_i;
	u_int len_i; 
	caddr_t addr_i;

{
        u_int   secpcyl;        /* number of sectors per cylinder */
        u_int   begin_sec;      /* block begin xfer within cyl. */
	u_int	sec_size;
        u_int   unit;
        u_int   cyl, head, sect;
        u_int   tlen;

	register struct fdcsb *csb = fdctlr.c_csb;
	register struct buf *bp = csb->c_current;
	register struct fdunit *un;
	register struct fdk_char *ch;

        unit = UNIT(bp->b_dev); /* floppy unit number */
        un = fdctlr.c_units[ unit ];
        ch = un->un_chars;
	sec_size = ch->sec_size;
	
	secpcyl = ch->nhead * ch->secptrack;    /* sectors per cylinder */


	while (len_i != 0) {

		cyl = phys_blk_begin_i / secpcyl;
		begin_sec = phys_blk_begin_i % secpcyl;
		head = begin_sec / ch->secptrack;
		sect = (begin_sec % ch->secptrack) + 1;

		/*
		 * Truncate tlen if len extends beyond cylinder
		 */
		if (len_i > ((secpcyl - begin_sec) * sec_size))
			tlen = (secpcyl - begin_sec) * sec_size; 
		else
			tlen = len_i;

		FDERRPRINT(FDEP_L1, FDEM_STRT,
		    ("    phys_blk_begin_i 0x%x, addr_i 0x%x, len_i 0x%x\n",
		    phys_blk_begin_i , addr_i, len_i));
		FDERRPRINT(FDEP_L1, FDEM_STRT,
		    ("    cyl 0x%x, head 0x%x, sec 0x%x\n", cyl, head, sect));
		FDERRPRINT(FDEP_L1, FDEM_STRT,
		    ("    resid 0x%x, tlen %d\n", bp->b_resid, tlen));

		csb->csb_cmds[0] = fd_action;
		csb->csb_cmds[1] = (head << 2) | (unit & 0x3);
		csb->csb_cmds[2] = cyl;		/* C - cylinder address */
		csb->csb_cmds[3] = head;	/* H - head number */
		csb->csb_cmds[4] = sect;	/* R - sector number */

		csb->csb_len = tlen;
		csb->csb_addr = addr_i;

		csb->csb_maxretry = rwretry;	/* retry this many times max */
		csb->csb_retrys = 0;

		/* Execute Operation - failure returns an errno */
		if ((bp->b_error = fdexec(FDXC_SLEEP | FDXC_CHECKCHG)) != 0) {
			FDERRPRINT(FDEP_L1, FDEM_STRT,
			    ("fdstart: bad exec of bp: 0x%x, err %d\n",
			    bp, bp->b_error));
			bp->b_flags |= B_ERROR;
			break;
		}

		phys_blk_begin_i += tlen / sec_size;
		len_i -= tlen;
		addr_i += tlen;
		bp->b_resid -= tlen;

	}

	if (bp->b_resid < 0)
		bp->b_resid = 0;

	return (bp->b_error);

}	/* end temp_disk */


/*
 * TRANSFER data from  buf  to temp_buf or vice-versa
 * --------------------------------------------------
 *
 * fd_action		Type of transfer (Read from or Write to disk)
 * plus1		real data starts middle of 1st sector
 * bp			os buf pointer.
 * addr_i		temp_buf address.
 *
 * (minus1		real data ends middle of last sector.)
 */



buf_temp(fd_action, plus1, bp, addr_i)

	u_int fd_action;
	u_int plus1;
	struct buf *bp;
	caddr_t addr_i;
{
	int i;
	caddr_t pt_temp, pt_buf;
	
	pt_buf = bp->b_un.b_addr;
	pt_temp = addr_i;
	FDERRPRINT(FDEP_L1, FDEM_STRT,
	    ("buf_temp: b_bcount=%x  plus1 %x\n",bp->b_bcount, plus1));

	if (plus1)
		pt_temp += 512;
	if (fd_action == CMD_READ) {
		for (i=0; i<bp->b_bcount; i++)
			*pt_buf++ = *pt_temp++;
	} else {
		for (i=0; i<bp->b_bcount; i++) {
			*pt_temp++ = *pt_buf++;
		}
	}
} 	/* end buf_temp */

/*
 * raw read and write operations share a common function to do their "work" :-)
 */
fdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (fdrdwr(dev, uio, B_READ));
}

fdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (fdrdwr(dev, uio, B_WRITE));
}

static
fdrdwr(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	int	rw;
{

	FDERRPRINT(FDEP_L0, FDEM_RDWR, ("fdrdwr, rw = %d\n", rw));
	FDERRPRINT(FDEP_L0, FDEM_RDWR, ("uio->uio_iov->iov_base = 0x%X\n",
		uio->uio_iov->iov_base));
	FDERRPRINT(FDEP_L0, FDEM_RDWR, ("uio->uio_iov->iov_len = %d\n",
		uio->uio_iov->iov_len));
	/* checks for UNIT(dev)) >= NFD, etc.  done in fdstrategy */
	return (physio(fdstrategy, &fdkbuf, dev, rw, minphys, uio));

}

#ifdef I82077
static
fdmotoff()
{
	FDERRPRINT(FDEP_L1, FDEM_MOFF, ("fdmotoff\n"));
	
	if (!(Msr & CB) && (Dor & MOTEN))
		fd_set_dor_reg(MOTEN, 0);
}
#endif I82077

/*
 * fdexec
 *	all commands go thru here.  Assumes the command block
 * 	fdctlr.c_csb is filled in.  The bytes are sent to the
 *	controller and then we do whatever else the csb says -
 *	like wait for immediate results, etc.
 *
 *	All waiting for operations done is in here - to allow retrys
 *	and checking for disk changed - so we don't have to worry
 *	about sleeping at interrupt level.
 *
 * RETURNS: 0 if all ok,
 *	ENXIO - diskette not in drive
 *	ETIMEDOUT - for immediate operations that timed out
 *	EBUSY - if stupid chip is locked busy???
 *	ENOEXEC - for timeout during sending cmds to chip
 */
fdexec(flags)
int	flags;		/*
			 * to sleep: set FDXC_SLEEP, to check for disk
			 * changed: set FDXC_CHECKCHG
			 */
{
	register struct fdcsb *csb;
	register int	i;
	int	to;
	u_char	mstat, tmp;
	int oldpri;
	int medium;

	medium = fdctlr.c_units[fdctlr.c_csb->csb_unit]->un_chars->medium;
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("\n******************** FDEXEC: flags 0x%x\n", flags));
	csb = fdctlr.c_csb;

retry:
#ifdef I82077
	switch (fdtype) {
	case FD_82077:

		FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: cmd is %s\n",
				fdcmds[csb->csb_cmds[0] & 0x1f].cmdname));

		untimeout(fdmotoff, (caddr_t) 0);

		/* fd_set_dor(MEDIUM_DENSITY, 0); */
		/* fd_set_dor_reg(MOTEN, 1);  HR XXX NEW CHANGE */
				
		if (!(Dor & MOTEN) || ((Dor & MEDIUM_DENSITY) >> 6) ^ medium ) {

			/* Turn on the motor.
			 */
			FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: turning on motor\n",
				fdcmds[csb->csb_cmds[0] & 0x1f].cmdname));

			fd_set_dor_reg(MOTEN, 1);

			if (flags & FDXC_SLEEP) {
				timeout(wakeup, (caddr_t) fdctlr_reg, MOTON_DELAY);
                        	(void) sleep((caddr_t) fdctlr_reg, PZERO - 1);
			} else {
				DELAY(1000000);
			}
		}
		break;
	}
#endif I82077

	fdselect((int)csb->csb_unit, 1);	/* select drive */
	/* select data rate for this unit/command */
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: medium        = %d\n",
		csb->csb_un->un_chars->medium   ));
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: transfer rate = %d\n",
		csb->csb_un->un_chars->transfer_rate));
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: ncyl          = %d\n",
		csb->csb_un->un_chars->ncyl));
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: sec_size      = %d\n",
		csb->csb_un->un_chars->sec_size ));
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("fdexec: secptrack     = %d\n",
		csb->csb_un->un_chars->secptrack));



	switch (csb->csb_un->un_chars->transfer_rate) {
	case 500:
		Dsr = 0;
		break;
	case 300:
		Dsr = 1;
		break;
	case 250:
		Dsr = 2;
		break;
	}
	DELAY(2);

	/*
	 * if checking for changed is enabled (ie not seeking in checkdisk)
	 * we sample the DSKCHG line to see if the diskette has wandered away
	 */
	if ((flags & FDXC_CHECKCHG) && fdsense_chng()) {
		FDERRPRINT(FDEP_L1, FDEM_EXEC, ("diskette changed!!!\n"));
		csb->csb_un->un_flags |= FDUNIT_CHANGED;
		/* if the diskette is still gone... so are we, adios! */
		if (fdcheckdisk((int)csb->csb_unit))
			return (ENXIO);
	}

	/*
	 * gather some statistics
	 */
	switch (csb->csb_cmds[0] & 0x1f) {
	case FRAW_RDCMD:
		fdctlr.fdstats.rd++;
		break;
	case FRAW_WRCMD:
		fdctlr.fdstats.wr++;
		break;
	case FRAW_REZERO:
		fdctlr.fdstats.recal++;
		break;
	case FRAW_FORMAT:
		fdctlr.fdstats.form++;
		break;
	default:
		fdctlr.fdstats.other++;
		break;
	}

	/*
	 * set info for level11 hardware interrupt handler
	 */
	/* if (operation is seek/relative-seek/recalibrate) ... */
	if (csb->csb_opflags & CSB_OFSEEKOPS) {
		fdintr_opmode = 2;
		fdintr_addr = fdintr_statbuf;	/* safe place to point */
		fdintr_len = 0;
	} else
	/* if immediate mode ... */
	if (csb->csb_opflags & CSB_OFIMMEDIATE) {
		fdintr_opmode = 0;
		fdintr_addr = fdintr_statbuf;	/* safe place to point */
		fdintr_len = 0;
	} else {
		fdintr_opmode = 1;	/* normal data xfer commands */
		fdintr_addr = (u_char *)csb->csb_addr;
		fdintr_len = csb->csb_len;
	}
	fdintr_status = 0;
	fdintr_statp = fdintr_statbuf;	/* set pointer to statbuf !!!!! */
	fdintr_timeout = fdhardtimeout;	/* XXX load timeout value */
	csb->csb_cmdstat = 0;

	/*
	 * give cmd to controller
	 */
	/* check for a command in progress? - avoid catastrophic errs */
	/*
	 * XXX ***
	 * i saw this (chip unexpectedly busy) happen when i shoved the
	 * floppy into the drive while
	 * running a dd if=/dev/rfd0c.  so it *is* possible for this to happen.
	 * we need to do a ctlr reset ...
	 */
	if ((mstat = Msr) & CB) {
		/* tried to give command to chip when it is busy! ***/
		FDERRPRINT(FDEP_L3, FDEM_EXEC,
		    ("fdc: unexpectedly busy - stat 0x%x\n", mstat));
#ifdef XXX
XXX arf! needs to avoid recusion
		fdreset();
		/* fdrecal already has csb */
		fdrecalseek(unit, -1, 0);	/* no check changed! */
#endif XXX
		csb->csb_cmdstat = -1;	/* XXX TBD ERRS NYD for now */
		return (EBUSY);
	}

	/* if sleeping, raise the priority */
	/*
	 ******** START CRITICAL SECTION ********
	 */
	if (flags & FDXC_SLEEP) {
		oldpri = splr(fdpri);	/* raise priority */
	}

	FDERRPRINT(FDEP_L0, FDEM_EXEC, ("fdexec: C M D = "));

	for (i = 0; i < csb->csb_ncmds; i++) {
#ifdef I82077
		DELAY(10);
#endif I82077
		/* is this loop really necessary? */
		for (to = FD_CRETRY; to; to--) {
			/* insure that rqm of main status reg is ok */
			if ((Msr & (DIO|RQM)) == RQM)
				break;
		}
		if (to == 0) {
			/* was a timeout - XXX TBD NYD */
			FDERRPRINT(FDEP_L3, FDEM_EXEC,
				("fdc: no RQM - stat 0x%x\n", Msr));
			csb->csb_cmdstat = -1;	/* XXX TBD ERRS NYD for now */
			if (flags & FDXC_SLEEP)
				(void) splx(oldpri);
			return (ENOEXEC);
		}
		Fifo = csb->csb_cmds[i];	/* send command to controller */
		FDERRPRINT(FDEP_L0, FDEM_EXEC, ("%x%c ",
			csb->csb_cmds[i],( Msr == 0x10 ? ' ':'?')));
	}

	/*
	 * start watchdog timer on data transfer type commands - required
	 * in case a diskette is not present or is unformatted
	 */
	if (csb->csb_opflags & CSB_OFTIMEIT) {
		timeout(fdwatch, (caddr_t)0, tosec*hz);
	}

	FDERRPRINT(FDEP_L0, FDEM_EXEC,
		("\nfdexec: cmd sent, Msr 0x%x\n", Msr));
	/*
	 * if the operation has no results - then just return
	 */
	if (csb->csb_opflags & CSB_OFNORESULTS) {
		if (flags & FDXC_SLEEP) /* not likely, but... */
			(void) splx(oldpri);
#ifdef I82077
		if (fdtype == FD_82077)
			timeout(fdmotoff, (caddr_t) 0, MOTOFF_DELAY);
#endif I82077
		return (0);
	}

	/*
	 * if this operation has no interrupt AND an immediate result
	 * then we just busy wait for the results and stuff them into
	 * the csb
	 */
	if (csb->csb_opflags & CSB_OFIMMEDIATE) {
		DELAY(10*hz); /* 10 sec delay */
		to = FD_RRETRY;
		csb->csb_nrslts = 0;
		/* while this command is still going on */
		while ((tmp = Msr) & CB) {
			/* if RQM + DIO, then a result byte is at hand */
			if ((tmp & (RQM|DIO|CB)) == (RQM|DIO|CB)) {
				csb->csb_rslt[ csb->csb_nrslts++ ] = Fifo;
	/* XXX limit number of status bytes read? ***/
			} else
			if (--to == 0) {
				FDERRPRINT(FDEP_L4, FDEM_EXEC,
				    ("fdexec: rslt timeout, Msr 0x%x, nr %d\n",
				    Msr, csb->csb_nrslts));
	/* XXX what to do now? ***/
				csb->csb_status = 2;
				/* should never be set, but... */
				if (flags & FDXC_SLEEP)
					(void) splx(oldpri);
#ifdef I82077
				if (fdtype == FD_82077)
					timeout(fdmotoff, (caddr_t) 0, MOTOFF_DELAY);
#endif I82077
				return (ETIMEDOUT);
			}
		}
	}

	/* if told to  sleep here, well then sleep! */
	if (flags & FDXC_SLEEP) {
		/* we're splr'd, so we can set this without worry (we hope) */
		fdctlr.c_flags |= FDCFLG_WAITING;

		/* Sleep till cmd complete */
		while (fdctlr.c_flags & FDCFLG_WAITING) {
			(void)sleep((caddr_t)(fdctlr.c_csb), FDPRI);
		}

		(void) splx(oldpri);
		/*
		 ******** END CRITICAL SECTION
		 *****************************/
	}

	/*
	 * see if there was an error detected, if so,
	 * fdrecover() will check it out and say what to do.
	 */
	if (((csb->csb_rslt[0] & IC_SR0) || (csb->csb_status)) &&
	    ((csb->csb_cmds[0] != FRAW_SENSE_DRV) && /* not sense drv status */
	    (csb->csb_cmds[0] != DUMPREG))) {	/* not dumpreg */
		/* if it can restarted OK, then do so, else return error */
		if (fdrecover() != 0) {
#ifdef I82077
			if (fdtype == FD_82077)
				timeout(fdmotoff, (caddr_t) 0, MOTOFF_DELAY);
#endif I82077
			return (EIO);
		} else {
			/* ASSUMES that cmd is still intact in csb */
			goto retry;
		}
	}
	/* things went ok */
#ifdef I82077
	if (fdtype == FD_82077)
		timeout(fdmotoff, (caddr_t) 0, MOTOFF_DELAY);
#endif I82077
	FDERRPRINT(FDEP_L1, FDEM_EXEC, ("____________________END_FDEXEC\n"));
	return (0);
}

/*
 * fdrecover
 *	see if possible to retry an operation.
 * 	All we can do is restart the operation.  If we are out of allowed
 * 	retries - return non-zero so that the higher levels will be notified.
 *
 * RETURNS: 0 if ok to restart, !0 if can't or out of retries
 */
fdrecover()
{
	struct fdcsb *ccsb;	/* private csb for resetting */
	struct fdcsb *csb;
	int unit;

	FDERRPRINT(FDEP_L1, FDEM_RECO, ("fdrecover\n"));
	csb = fdctlr.c_csb;

	if (fdctlr.c_flags & FDCFLG_TIMEDOUT) {
		fdctlr.c_flags &= ~FDCFLG_TIMEDOUT;
		csb->csb_rslt[1] |= 0x08;
		FDERRPRINT(FDEP_L1, FDEM_RECO,
			("fd%d: %s timed out\n", csb->csb_unit,
			fdcmds[csb->csb_cmds[0] & 0x1f].cmdname));
		unit = csb->csb_unit;

		/* use private csb */
		ccsb = (struct fdcsb *)kmem_zalloc(sizeof (struct fdcsb));
		fdctlr.c_csb = ccsb;

		FDERRPRINT(FDEP_L1, FDEM_RECO, ("fdc: resetting\n"));
		fdreset();
		/* check change first?? */
		/* don't ckchg in fdexec, too convoluted */
		(void)fdrecalseek(unit, -1, 0);

		fdctlr.c_csb = csb; /* restore original csb */
		kmem_free((caddr_t)ccsb, sizeof (struct fdcsb));
	}

	/*
	 * gather statistics on errors
	 */
	if (csb->csb_rslt[1] & DE_SR1) {
		fdctlr.fdstats.de++;
	}
	if (csb->csb_rslt[1] & OR_SR1) {
		fdctlr.fdstats.run++;
	}
	if (csb->csb_rslt[1] & (ND_SR1+MA_SR1)) {
		fdctlr.fdstats.bfmt++;
	}
	if (csb->csb_rslt[1] & 0x08) {
		fdctlr.fdstats.to++;
	}

	/* if raw ioctl don't examine results just pass status back via fdraw */
	/* raw commands are timed too, so put this after the above check */
	if (csb->csb_opflags & CSB_OFRAWIOCTL) {
		return (1);
	}

	/*
	 * if we have run out of retries, return an error
	 * XXX need better status interp
	 */
	csb->csb_retrys++;
	if (csb->csb_retrys > csb->csb_maxretry) {
		FDERRPRINT(FDEP_L3, FDEM_RECO,
			("fd%d: %s failed (%x %x %x)\n",
			csb->csb_unit,
			fdcmds[csb->csb_cmds[0] & 0x1f].cmdname,
			csb->csb_rslt[0], csb->csb_rslt[1], csb->csb_rslt[2]));
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.1\n"));
		if (csb->csb_rslt[1] & NW_SR1) {
			FDERRPRINT(FDEP_L3, FDEM_RECO,
				("fd%d: not writable\n", csb->csb_unit));
		}
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.2\n"));
		if (csb->csb_rslt[1] & DE_SR1) {
			if (fdctlr.c_head.b_forw) {
				FDERRPRINT(FDEP_L3, FDEM_RECO,
					("fd%d: crc error  blk %d\n",
					csb->csb_unit,
					fdctlr.c_head.b_forw->b_blkno));
			} else {
				FDERRPRINT(FDEP_L3, FDEM_RECO,
					("fd%d: crc error fdrw\n",
					csb->csb_unit));
			}
		}
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.3\n"));
		if (csb->csb_rslt[1] & OR_SR1) {
			FDERRPRINT(FDEP_L3, FDEM_RECO,
				("fd%d: overrun/underrun\n", csb->csb_unit));
		}
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.4\n"));
		if (csb->csb_rslt[1] & (ND_SR1+MA_SR1)) {
			FDERRPRINT(FDEP_L3, FDEM_RECO,
				("fd%d: bad format\n", csb->csb_unit));
		}
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.5\n"));
		if (csb->csb_rslt[1] & 0x08) {
			FDERRPRINT(FDEP_L3, FDEM_RECO,
				("fd%d: timeout\n", csb->csb_unit));
		}
		FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 1.6\n"));

		csb->csb_cmdstat = -1; /* failed - give up */
		return (1);
	}
	FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 2\n"));

	if (csb->csb_opflags & CSB_OFSEEKOPS) {
		/* seek, recal type commands - just look at st0 */
		FDERRPRINT(FDEP_L2, FDEM_RECO,
			("fd%d: %s error : st0 0x%x\n",
				csb->csb_unit,
				fdcmds[csb->csb_cmds[0] & 0x1f].cmdname,
				csb->csb_rslt[0]));
	}
	FDERRPRINT(FDEP_L0, FDEM_RECO, ("fdrecover 3\n"));
	if (csb->csb_opflags & CSB_OFXFEROPS) {
		/* rd, wr, fmt type commands - look at st0, st1, st2 */
		FDERRPRINT(FDEP_L2, FDEM_RECO,
			("fd%d: %s error : st0=0x%x st1=0x%x st2=0x%x\n",
			csb->csb_unit, fdcmds[csb->csb_cmds[0] & 0x1f].cmdname,
			csb->csb_rslt[0], csb->csb_rslt[1], csb->csb_rslt[2]));
	}

	return (0);	/* tell fdexec to retry */
}


/*
 * fdintr
 *	this is the level4 sw interrupt handler triggered by the level11
 *	trap handler when the command is complete.  we also get here from
 *	fdwatch.
 * NOTE: this software interrupt level is shared.
 * NOTE: we must return 0 if not ours, 1 if it was.
 */
fdintr()
{
	register struct fdcsb *csb;
	register int	i;

	FDERRPRINT(FDEP_L1, FDEM_INTR, ("fdintr: opmode %d\n", fdintr_opmode));
	/* check that lowlevel interrupt really meant to trigger us */
	if (fdintr_opmode != 4)
		return (0);
	fdintr_opmode = 0;

	csb = fdctlr.c_csb;

	/*  reset watchdog timer if armed and not already triggered */
	if ((csb->csb_opflags & CSB_OFTIMEIT) &&
	    ! (fdctlr.c_flags & FDCFLG_TIMEDOUT))
		untimeout(fdwatch, (caddr_t)0);

	/*
	 * examine results of command just completed (it places status
	 * bytes in buffer) - retry if there was a recoverable error.
	 */
	csb->csb_raddr = (caddr_t)fdintr_addr; /* (current) data xfer address */
	csb->csb_rlen = fdintr_len;	/* I: data xfer length; O: remainder */
	csb->csb_status = fdintr_status; /* I: n/a; O: 0 = ok, TBD errors */
	/* XXX - who to believe, chip or csb? */
	for (i = 0; i < csb->csb_nrslts; i++) {
		csb->csb_rslt[i] = fdintr_statbuf[i];
	}

/*XXX what to do on a level11 timeout??? */
	if (fdintr_timeout == 0) {
		FDERRPRINT(FDEP_L4, FDEM_INTR,
			("fdintr: fdintr_timeout!, opmode %d, MSR 0x%x\n",
			fdintr_opmode, Msr));
	}

	if (fdctlr.c_flags & FDCFLG_WAITING) {
		/* somebody's waiting on finish of fdctlr/csb, wake them */
		fdctlr.c_flags &= ~FDCFLG_WAITING;
		wakeup((caddr_t)(fdctlr.c_csb));
		/*
		 * FDCFLG_BUSY is NOT cleared, NOR is the csb given back; so
		 * the operation just finished can look at the csb
		 */
		return (1);
	}
	FDERRPRINT(FDEP_L3, FDEM_INTR,
		("fdintr: nobody sleeping\n"));
	return (1);
}

/*
 * fdwatch
 *	is called from timein() when a floppy operation has expired.
 */
fdwatch()
{
	struct fdcsb *csb;

	csb = fdctlr.c_csb;
	FDERRPRINT(FDEP_L1, FDEM_WATC, ("fd%d: timeout, opmode = %d\n",
	    csb->csb_unit, fdintr_opmode));

	fdintr_opmode = 4; /* for fdintr @level 4 */
	fdctlr.c_flags |= FDCFLG_TIMEDOUT;
	fdintr_status = CSB_CMDTO;

	/* trigger the level4 int @fdintr */
	set_intreg(IR_SOFT_INT4, 1);
}

/*
 * fdioctl
 *	implements more ioctls than a floppy disk ought to have
 *
 * RETURNS: an errno for user >>> at end of each case <<<
 */
/*ARGSUSED*/
fdioctl(dev, cmd, arg, flag)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	flag;
{
	int	unit;
	struct fdunit *un;
#ifdef XXX
	u_int sec_size;
#endif XXX

	FDERRPRINT(FDEP_L1, FDEM_IOCT, ("fdioctl: cmd 0x%x, arg 0x%x\n",
		cmd, arg));
	/* check for valid unit - failure should *never happen*  */
	unit = UNIT(dev);
	if (unit >= NFD || fdtype == -1) {
		return (ENXIO);
	}
	un = fdctlr.c_units[ unit ];
#ifdef XXX
	sec_size = ch->sec_size;
#endif XXX

	switch (cmd) {
	case DKIOCINFO:	/* return struct dk_info describing controller/unit */
		{
		struct dk_info *dki = (struct dk_info *)arg;
		dki->dki_ctlr = OBIO_FDC_ADDR;	/* addres of controller */
		dki->dki_unit = unit;
		if (fdtype == FD_82072)
			dki->dki_ctype = DKC_INTEL82072;
		if (fdtype == FD_82077)
			dki->dki_ctype = DKC_INTEL82077;
		dki->dki_flags = DKI_FMTTRK;	/* format track at a time */
		}
		return (0);

		/* get disk geometry */
	case DKIOCGGEOM: /* return struct dk_geom describing disk geometry */
		{
		struct dk_geom *dkg = (struct dk_geom *)arg;
		dkg->dkg_ncyl = un->un_chars->ncyl;
		dkg->dkg_nhead = un->un_chars->nhead;
		dkg->dkg_nsect = un->un_chars->secptrack;
		dkg->dkg_intrlv = un->un_label->dkl_intrlv;
		dkg->dkg_rpm = un->un_label->dkl_rpm;
		dkg->dkg_pcyl = un->un_chars->ncyl;
		}
		return (0);

	case DKIOCSGEOM:
		FDERRPRINT(FDEP_L3, FDEM_IOCT,
			("fdioctl: DKIOCSGEOM not supported\n"));
		return (EINVAL);

	/*
	 * Return the map for the specified logical partition.
	 * This has been made obsolete by the get all partitions command.
	 */
	case DKIOCGPART:
		/* structure copy! */
		*(struct dk_map *)arg =
		    un->un_label->dkl_map[ PARTITION(dev) ];
		return (0);

	case DKIOCSPART:
		{
		struct dk_map *dkm_src = (struct dk_map *)arg;
		struct dk_map *dkm_dst =
			&(un->un_label->dkl_map[ PARTITION(dev) ]);

		*dkm_dst = *dkm_src;
		}
		return (0);

	/*
	 * return the map of all logical partitions
	 */
	case DKIOCGAPART:
		/* structure copy! */
		*(struct dk_allmap *)arg =
		    *(struct dk_allmap *)un->un_label->dkl_map;
		return (0);

	/*
	 * Set the map of all logical partitions
	 */
	case DKIOCSAPART:
		{
		struct dk_allmap *dkam_src = (struct dk_allmap *)arg;
		struct dk_allmap *dkam_dst =
			(struct dk_allmap *)un->un_label->dkl_map;

		*dkam_dst = *dkam_src;
		}
		return (0);

	case FDKIOGCHAR:
		/* structure copy! */
		*(struct fdk_char *)arg = *(un->un_chars);
		return (0);

	case FDKIOSCHAR:
		{
		struct fdk_char *fdchar = (struct fdk_char *)arg;
		switch (fdchar->transfer_rate) {
		case 500:
		case 300:
		case 250:
			break;
		default:
			FDERRPRINT(FDEP_L3, FDEM_IOCT,
			("fdioctl: FDKIOSCHAR unsupported transfer rate %d\n",
				fdchar->transfer_rate));
			return (EINVAL);
		}
		/* structure copy! */
		*(un->un_chars) = *fdchar;
		curfdtype = -1;
		FDERRPRINT(FDEP_L0, FDEM_IOCT,
		    ("fdioctl: S E T T I N G     C H A R A C T\n"));
		}
		return (0);

	case FDKEJECT:  /* eject disk */
		fdselect(UNIT(dev), 1);
		fdeject();
		return (0);

	case FDKGETCHANGE: /* disk changed */
		if (un->un_flags & FDUNIT_CHANGED)
			*(int *)arg |= FDKGC_HISTORY;
		else
			*(int *)arg &= ~FDKGC_HISTORY;
		un->un_flags &= ~FDUNIT_CHANGED;

		if (fdsense_chng()) { /* check disk only if changed */
			if (fdcheckdisk(unit)) {
				*(int *)arg |= FDKGC_CURRENT;
			} else {
				*(int *)arg &= ~FDKGC_CURRENT;
			}
		} else {
			*(int *)arg &= ~FDKGC_CURRENT;
		}

		return (0);

	case FDKGETDRIVECHAR:
		{
		struct fdk_drive *drvchar = (struct fdk_drive *)arg;

		drvchar->fdd_ejectable = -1;	/* we do support autoeject */
		drvchar->fdd_maxsearch = nfdtypes; /* density:hi, m, lo */
		/* the rest of the fdk_drive struct is meaningless to us */
		}
		return (0);

	case FDKSETDRIVECHAR:
		FDERRPRINT(FDEP_L3, FDEM_IOCT,
			("fdioctl: FDKSETDRIVECHAR not supported\n"));
		return (EINVAL);

	case DKIOCSCMD:
	case FDKIOCSCMD:
		{
		struct dk_cmd *dkc = (struct dk_cmd *)arg;
		int cyl, hd, spc, spt;

		spt = un->un_chars->secptrack;		/* sec/trk */
		spc = un->un_chars->nhead * spt;	/* sec/cyl */

		cyl = dkc->dkc_blkno / spc;
		hd = (dkc->dkc_blkno % spc) / spt;

		switch (dkc->dkc_cmd) {
#ifdef XXX
/* XXX need to mapin and as_fault lock user address! XXX */
		case FKREAD:
			sec = ((dkc->dkc_blkno % spc) % spt) + 1;
			if (fdrw(FDREAD, unit, cyl, hd, sec,
			    dkc->dkc_bufaddr,
			    (u_int)dkc->dkc_secnt * sec_size))
				return (EIO);
			return (0);
		case FKWRITE:
			sec = ((dkc->dkc_blkno % spc) % spt) + 1;
			if (fdrw(FDWRITE, unit, cyl, hd, sec,
			    dkc->dkc_bufaddr,
			    (u_int)dkc->dkc_secnt * sec_size))
				return (EIO);
			return (0);
#endif XXX
		case FKFORMAT_TRACK:
			if (fdformat(unit, cyl, hd))
				return (EIO);
			return (0);
		default:
			FDERRPRINT(FDEP_L3, FDEM_IOCT,
				("fdioctl: FDKIOCSCMD not yet complete\n"));
			return (EINVAL);
		}
		}

	/*
	 *
	 */
	case F_RAW:
		return (fdrawioctl(dev, arg));

	case IOCTL_DEBUG:
		if (fderrlevel == 0)
			fderrlevel = 3;
		if (fderrlevel == 3)
			fderrlevel = 0;
		printf("fdioctl: TOGGLING trace\n");
		return(0);
	default:
		FDERRPRINT(FDEP_L2, FDEM_IOCT,
			("fdioctl: invalid ioctl 0x%x\n", cmd));
		return (ENOTTY);
	}
#ifdef NEVER
	/* hopefuly nobody gets here! */
	/*NOTREACHED*/
	return (ENOTTY);
#endif NEVER
}	/* end fdioctl */
/*
 * fdrawioctl
 *
 */
fdrawioctl(dev, arg)
dev_t	dev;
caddr_t	arg;
{
	struct fdunit *un = fdctlr.c_units[ UNIT(dev) ];
	struct fdraw *fdr = (struct fdraw *)arg;
	struct fdcsb *csb;
	struct buf *bp;
	int i;
	int flag = B_READ;
	int	oldpri;
	caddr_t	addr, fa;
	u_int	fc;
	long	count;

	FDERRPRINT(FDEP_L1, FDEM_RAWI,
		("fdrawioctl: dev=0x%x cmd[0]=0x%x\n", dev, fdr->fr_cmd[0]));

	/*
	 * check if medium density is asked but not supported
	 */
	if (un->un_chars->medium && fdtype == FD_82072) {
		printf("fdrawioctl: Medium density NOT supported\n");
		return(ENXIO);
	}

	fdgetcsb();

	csb = fdctlr.c_csb;
	csb->csb_un = un;
	csb->csb_unit = UNIT(dev);
	csb->csb_part = 0;

	/* copy cmd bytes into csb */
	for (i=0; i<=fdr->fr_cnum; i++) {
		csb->csb_cmds[i] = fdr->fr_cmd[i];
	}
	csb->csb_ncmds = fdr->fr_cnum;

	csb->csb_maxretry = 0;	/* let the application deal with errors */
	csb->csb_retrys = 0;

	switch (fdr->fr_cmd[0] & 0x0f) {

	case FRAW_SPECIFY:
		csb->csb_opflags = CSB_OFNORESULTS;
		csb->csb_nrslts = 0;
		break;

	case FRAW_SENSE_DRV:
		csb->csb_opflags = CSB_OFIMMEDIATE;
		csb->csb_nrslts = 1;
		break;

	case FRAW_REZERO:
	case FRAW_SEEK:
		csb->csb_opflags = CSB_OFSEEKOPS + CSB_OFTIMEIT;
		csb->csb_nrslts = 2;
		break;

	case FRAW_FORMAT:
		csb->csb_opflags = CSB_OFXFEROPS + CSB_OFTIMEIT;
		csb->csb_nrslts = NRBRW;
		flag = B_WRITE;

		fc = (u_int)(fdr->fr_nbytes + 16);
		fa = (caddr_t)kmem_zalloc(fc);
		(void) copyin(fdr->fr_addr, fa, (u_int)fdr->fr_nbytes);
/*XXX handle copyin errors */
		break;

	case FRAW_WRCMD:
	case FRAW_WRITEDEL:
		flag = B_WRITE;
	case FRAW_RDCMD:
	case FRAW_READDEL:
	case FRAW_READTRACK:
		csb->csb_opflags = CSB_OFXFEROPS + CSB_OFTIMEIT;
		csb->csb_nrslts = NRBRW;
		/* flag was set to B_READ above */
		break;

#ifdef XXX
cmds not supported for now
	case FRAW_READID:
		break;
	case FRAW_SENSE_INT:
		break;
#endif XXX

	default:
		fdretcsb();
		return (EINVAL);
	}

	if ((csb->csb_opflags & CSB_OFXFEROPS) && (fdr->fr_nbytes == 0)) {
		fdretcsb();
		return (EINVAL);
	}

	csb->csb_opflags |= CSB_OFRAWIOCTL;

	if ((fdr->fr_cmd[0] & 0x0f) != FRAW_FORMAT) {
	/* if data transfer involved need to use the special bp */
	if (fdr->fr_nbytes > 0) {
		bp = &fdkbuf;
		oldpri = splr(fdpri);
		while (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			(void) sleep((caddr_t)bp, FDPRI);
		}
		bp->b_flags = B_BUSY;
		(void) splx(oldpri);

		bp->b_flags |= B_PHYS;
		addr = bp->b_un.b_addr = fdr->fr_addr;
		count = bp->b_bcount = (long)fdr->fr_nbytes;
		bp->b_proc = u.u_procp;
		u.u_procp->p_flag |= SPHYSIO;

		(void) as_fault(u.u_procp->p_as, addr,
			(u_int)count, F_SOFTLOCK,
			(flag == B_WRITE) ? S_WRITE : S_READ);
/*XXX handle errors from as_fault ****/
		bp_mapin(bp);
		csb->csb_addr = bp->b_un.b_addr;
		csb->csb_len = bp->b_bcount;
	} else {
/*XXX or maybe point someplace safer?? */
		csb->csb_addr = 0;
		csb->csb_len = 0;
	}
	} else {
		csb->csb_addr = fa;
		csb->csb_len = fc;
	}

	FDERRPRINT(FDEP_L1, FDEM_RAWI,
		("cmd: %x %x %x %x %x %x %x %x %x %x\n", csb->csb_cmds[0],
		csb->csb_cmds[1], csb->csb_cmds[2], csb->csb_cmds[3],
		csb->csb_cmds[4], csb->csb_cmds[5], csb->csb_cmds[6],
		csb->csb_cmds[7], csb->csb_cmds[8], csb->csb_cmds[9]));
	FDERRPRINT(FDEP_L1, FDEM_RAWI,
		("nbytes: %x, opflags: %x, addr: %x, len: %x\n",
		csb->csb_ncmds, csb->csb_opflags, csb->csb_addr, csb->csb_len));

	if ((csb->csb_opflags & CSB_OFNORESULTS) ||
	    (csb->csb_opflags & CSB_OFIMMEDIATE)) {
		i = fdexec(0); /* don't sleep, don't check change */
	} else {
		i = fdexec(FDXC_SLEEP | FDXC_CHECKCHG);  /* sleep till done */
	}
	DELAY(1*hz); /* 1 sec delay */

	FDERRPRINT(FDEP_L1, FDEM_RAWI,
		("rslt: %x %x %x %x %x %x %x %x %x %x\n", csb->csb_rslt[0],
		csb->csb_rslt[1], csb->csb_rslt[2], csb->csb_rslt[3],
		csb->csb_rslt[4], csb->csb_rslt[5], csb->csb_rslt[6],
		csb->csb_rslt[7], csb->csb_rslt[8], csb->csb_rslt[9]));

	if ((fdr->fr_cmd[0] & 0x0f) != FRAW_FORMAT) {
	if (fdr->fr_nbytes > 0) {
		bp_mapout(bp);
		(void) as_fault(u.u_procp->p_as, addr,
			(u_int)count, F_SOFTUNLOCK,
			(flag == B_WRITE)? S_WRITE: S_READ);
/*XXX handle errors from as_fault ****/
		oldpri = splr(fdpri);
		u.u_procp->p_flag &= ~SPHYSIO;
		if (bp->b_flags & B_WANTED)
			wakeup((caddr_t)bp);
		(void) splx(oldpri);
		bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	}
	} else {
		kmem_free(fa, fc);
	}

	/* copy cmd results into fdr */
	for (i=0; i<=csb->csb_nrslts; i++)
		fdr->fr_result[i] = csb->csb_rslt[i];
	fdr->fr_nbytes = csb->csb_rlen; /* return resid */

	fdretcsb();

	return (0);
}

/*
 * fddump
 *	'cause the config stuff wants it
 *	it doesn't make sense to dump system on a floppy, so don't even
 *	try, just return an error.
 *	(dump occurs to swap area, and there isn't enough room on a floppy
 *	to swap on)
 */
/*ARGSUSED*/
fddump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	return (ENOSPC);
}


/*
 * fdgetcsb
 *	wait until the csb is free
 */
fdgetcsb()
{
	int	oldpri;

	FDERRPRINT(FDEP_L1, FDEM_GETC, ("fdgetcsb\n"));
	oldpri = splr(fdpri);	/* raise priority */

	while (fdctlr.c_flags & FDCFLG_BUSY) {
		fdctlr.c_flags |= FDCFLG_WANT;
		(void)sleep((caddr_t)&(fdctlr.c_flags), FDPRI);
	}
	fdctlr.c_flags |= FDCFLG_BUSY; /* got it! */

	(void) splx(oldpri);
}

/*
 * fdretcsb
 *	return csb
 */
fdretcsb()
{
	FDERRPRINT(FDEP_L1, FDEM_RETC, ("fdretcsb\n"));
	fdctlr.c_flags &= ~FDCFLG_BUSY; /* let go */
	if (fdctlr.c_flags & FDCFLG_WANT) {
		fdctlr.c_flags &= ~FDCFLG_WANT;
		wakeup((caddr_t)&(fdctlr.c_flags));
	}
}


/*
 * fdreset
 *	reset THE controller, and configure it to be
 *	the way it ought to be
 * ASSUMES: that it already owns the csb/fdctlr!
 */
fdreset()
{
	register struct fdcsb *csb = fdctlr.c_csb;

	FDERRPRINT(FDEP_L1, FDEM_RESE, ("fdreset\n"));

	/* count resets */
	fdctlr.fdstats.reset++;

	/* toggle software reset */
	Dsr = SWR;
	DELAY(5);
	/*
	 * select data rate for this unit/command
	 */
	switch (fdctlr.c_units[csb->csb_unit]->un_chars->transfer_rate) {
        case 500:
                Dsr = 0;
                break;
        case 300:
                Dsr = 1;
                break;
        case 250:
                Dsr = 2;
                break;
        }

	DELAY(5);

#ifdef I82077
	switch (fdtype) {
	case FD_82077:

		untimeout(fdmotoff, (caddr_t) 0);

		/* The reset bit must be cleared to take the 077 out of
		 * reset state and the DMAGATE bit must be high to enable
		 * interrupts.
		 */
		fd_set_dor_reg(ZEROBITS, 0);
		fd_set_dor_reg(DMAGATE|RESET, 1);
		break;
	}
#endif I82077


	/* setup common things in csb */
	csb->csb_un = fdctlr.c_units[0];
	csb->csb_unit = 0;
	csb->csb_part = 0;
	csb->csb_nrslts = 0;
	csb->csb_opflags = CSB_OFNORESULTS;
	csb->csb_maxretry = 0;
	csb->csb_retrys = 0;

	/* send SPECIFY command to fdc */
	/* csb->unit is don't care */
	csb->csb_cmds[0] = FRAW_SPECIFY;
	csb->csb_cmds[1] = fdspec[0]; /* step rate, head unload time */
	csb->csb_cmds[2] = fdspec[1]; /* head load time, DMA mode */
	csb->csb_ncmds = 3;

	/* XXX for now ignore errors, they "CAN'T HAPPEN" */
	(void) fdexec(0);	/* no FDXC_CHECKCHG, ... */
	/* no results */

	/* send CONFIGURE command to fdc */
	/* csb->unit is don't care */
	csb->csb_cmds[0] = CONFIGURE;
	csb->csb_cmds[1] = fdconf[0]; /* motor info, motor delays */
	csb->csb_cmds[2] = fdconf[1]; /* enaimplsk, disapoll, fifothru */
	csb->csb_cmds[3] = fdconf[2]; /* track precomp */
	csb->csb_ncmds = 4;

	csb->csb_retrys = 0;

	/* XXX for now ignore errors, they "CAN'T HAPPEN" */
	(void) fdexec(0);	/* no FDXC_CHECKCHG, ... */
	/* no results */

}

/*
 * fdrecalseek
 *	performs recalibrates or seeks if the "arg" is -1 does a
 *	recalibrate on a drive, else it seeks to the cylinder of
 *	the drive.  The recalibrate is also used to find a drive,
 *	ie if the drive is not there, the controller says "error"
 *	on the operation
 * NOTE: that there is special handling of this operation in the hardware
 * interrupt routine - it causes the operation to appear to have results;
 * ie the results of the SENSE INTERRUPT STATUS that the hardware interrupt
 * function did for us.
 * NOTE: because it uses sleep/wakeup it must be protected in a critical
 * section so create one before calling it!
 *
 * RETURNS: 0 for ok,
 *	else	errno from fdexec,
 *	or	ENODEV if error (infers hardware type error)
 */
fdrecalseek(unit, arg, execflg)
	int unit;
	int arg;	/* -1 recalibrate, 0-? seek to here */
	int execflg;	/* bits to OR into flag sent to fdexec */
{
	register struct fdcsb *csb;
	int result;

	FDERRPRINT(FDEP_L1, FDEM_RECA, ("fdrecalseek to %d\n", arg));

/*XXX TODO: check see argument for <= num cyls OR < 256 ****/

	csb = fdctlr.c_csb;
	csb->csb_un = fdctlr.c_units[unit];
	csb->csb_unit = unit;
	csb->csb_part = 0;
	csb->csb_cmds[1] = unit & 0x03;
	if (arg == -1) {			/* is recal... */
		csb->csb_cmds[0] = FRAW_REZERO;
		csb->csb_ncmds = 2;
	} else {
		csb->csb_cmds[0] = FRAW_SEEK;
		csb->csb_cmds[2] = (u_char)arg;
		csb->csb_ncmds = 3;
	}
	csb->csb_nrslts = 2;	/* 2 for SENSE INTERRUPTS */
	csb->csb_opflags = CSB_OFSEEKOPS | CSB_OFTIMEIT;
/* MAYBE NYD need to set retries to different values? - depending on
 * drive characteristics - if we get to high capacity drives */
	csb->csb_maxretry = skretry;
	csb->csb_retrys = 0;

	/* send cmd off to fdexec */
	if (result = fdexec(FDXC_SLEEP | execflg)) {
		goto out;
	}

	/*
	 * if recal, test for equipment check error
	 * ASSUMES result = 0 from above call
	 */
	if (arg == -1) {
#ifdef I82077
		result = 0;
#else
		if (csb->csb_rslt[0] & EC_SR0)
			result = ENODEV;
#endif I82077
	} else {
		/* for seeks, any old error will do */
		if ((csb->csb_rslt[0] & IC_SR0) || csb->csb_cmdstat)
			result = ENODEV;
	}

out:
	return (result);
}

/*
 * fdformat
 *	format a track - builds a table of sector data values with 16 bytes
 * (sizeof fdc's fifo) of dummy on end.  This is so than when fdintr_len
 * goes to 0 and fdc_hardintr sends TC that all the real formatting will
 * have already been done.
 */
fdformat(unit, cyl, hd)
	int unit, cyl, hd;
{
	register struct fdcsb *csb;
	register struct fdunit *un;
	register struct fdk_char *ch;
	int	cmdresult;
	u_char	*fmthdrs;
	register u_char	*fd;
	int	i;

	FDERRPRINT(FDEP_L1, FDEM_FORM, ("fdformat cyl %d, hd %d\n", cyl, hd));
	fdgetcsb();

	csb = fdctlr.c_csb;
	un = fdctlr.c_units[ unit ];
	ch = un->un_chars;

	/* setup common things in csb */
	csb->csb_un = un;
	csb->csb_unit = unit;
	csb->csb_part = 0;

	/*
	 * stupid controller needs to do a seek before each format to get
	 * to right cylinder.
	 */
	if (fdrecalseek(unit, cyl, FDXC_CHECKCHG)) {
		fdretcsb();
		return (EIO);
	}

	/*
	 * now do the format itself
	 */
	csb->csb_nrslts = NRBRW;
	csb->csb_opflags = CSB_OFXFEROPS | CSB_OFTIMEIT;

	csb->csb_cmds[0] = FRAW_FORMAT;
	/* always or in MFM bit */
	csb->csb_cmds[0] |= MFM;
	csb->csb_cmds[1] = (hd << 2) | (unit & 0x03);
	printf("  medium = %d\n",ch->medium);
	csb->csb_cmds[2] = ch->medium ? 3 : 2;
	printf("  cmds[2] = %d\n",csb->csb_cmds[2]);
	csb->csb_cmds[3] = ch->secptrack;
	csb->csb_cmds[4] = GPLF;
	csb->csb_cmds[5] = FDATA;
	csb->csb_ncmds = 6;
	csb->csb_maxretry = rwretry;
	csb->csb_retrys = 0;

	/* just kmem zalloc space for formattrk cmd */
	/* NOTE: have to add size of fifo also - for dummy format action */
	fd = fmthdrs = (u_char *)kmem_zalloc(((u_int)4 * ch->secptrack) + 16);

	csb->csb_addr = (caddr_t)fd;
	csb->csb_len = (4 * ch->secptrack) + 16;

	for (i = 1; i <= ch->secptrack; i++) {
		*fd++ = cyl;		/* cylinder */
		*fd++ = hd;		/* head */
		*fd++ = (u_char)i;	/* sector number */
		*fd++ = ch->medium ? 3 : 2;
	}

	if ((cmdresult = fdexec(FDXC_SLEEP | FDXC_CHECKCHG)) == 0) {
		if (csb->csb_cmdstat)
			cmdresult = EIO;	/* XXX TBD NYD for now */
	}

	fdretcsb();

	kmem_free((caddr_t)fmthdrs, ((u_int)4 * ch->secptrack) + 16);
	return (cmdresult);
}

/*
 * fdrw
 *	used for internal sorts of reads/writes, ie reading labels,
 *	implementing raw commands thru ioctls, ...
 */
fdrw(flag, unit, cyl, head, sector, bufp, len)
int flag, unit, cyl, head, sector;
caddr_t	bufp;
u_int len;
{
	register struct fdcsb *csb;
	int	cmdresult;

	FDERRPRINT(FDEP_L1, FDEM_RW, ("fdrw\n"));
	fdgetcsb();

	csb = fdctlr.c_csb;
	if (flag == FDREAD)
		csb->csb_cmds[0] = MT + SK + FRAW_RDCMD;
	else /* write */
		csb->csb_cmds[0] = MT + FRAW_WRCMD;
	/* always or in MFM bit */
	csb->csb_cmds[0] |= MFM;
	csb->csb_cmds[1] = (unit & 0x3) | ((head & 0x1) << 2);
	csb->csb_cmds[2] = cyl;
	csb->csb_cmds[3] = head;
	csb->csb_cmds[4] = sector;
	csb->csb_cmds[5] = ((fdctlr.c_units[unit])->un_chars->sec_size == 512) ? 2 : 3;
	csb->csb_cmds[6] = (fdctlr.c_units[unit])->un_chars->secptrack;
	csb->csb_cmds[7] = GPLN;
	csb->csb_cmds[8] = SSSDTL;
	csb->csb_ncmds = NCBRW;
	csb->csb_addr = bufp;
	csb->csb_len = len;
	csb->csb_maxretry = rwretry;
	csb->csb_retrys = 0;

	/* init result area ?? */
	csb->csb_nrslts = NRBRW;
	csb->csb_opflags = CSB_OFXFEROPS | CSB_OFTIMEIT;

	cmdresult = 0;

	if (fdexec(FDXC_SLEEP | FDXC_CHECKCHG) != 0) {
		/* things was really hosed! */
		cmdresult = EIO;
		goto out;
	}

	if (csb->csb_cmdstat)
		cmdresult = EIO;	/* XXX TBD NYD for now */

out:
	fdretcsb();
	return (cmdresult);
}

/*
 * fdsensedrv
 *	do a sense_drive command.  used by fdopen and fdcheckdisk.
 */
fdsensedrv(unit)
int	unit;
{
	struct fdcsb *csb;

	csb = fdctlr.c_csb;

	/* setup common things in csb */
	csb->csb_un = fdctlr.c_units[0];
	csb->csb_unit = unit;
	csb->csb_part = 0;

	csb->csb_opflags = CSB_OFIMMEDIATE;
	csb->csb_cmds[0] = FRAW_SENSE_DRV;
	/* MOT bit set means don't delay */
	csb->csb_cmds[1] = MOT | (unit & 0x03);
	csb->csb_ncmds = 2;
	csb->csb_nrslts = 1;
	csb->csb_maxretry = skretry;
	csb->csb_retrys = 0;

	/* XXX for now ignore errors, they "CAN'T HAPPEN" */
	(void) fdexec(0);	/* DON't check changed!, no sleep */

	return (csb->csb_rslt[0]); /* return status byte 3 */
}

/*
 * fdcheckdisk
 *	check to see if the disk is still there - do a recalibrate,
 *	then see if DSKCHG line went away, if so, diskette is in; else
 *	it's (still) out.
 * INPUTS/ASSUMES: unit #, assumes it owns the csb/fdctlr.
 *
 * RETURNS: 0 if diskette is in drive, -1 if otherwise
 */
fdcheckdisk(unit)
int	unit;			/* drive number to check */
{
	struct fdcsb *ccsb;		/* private csb for recal op */
	struct fdcsb *csb;		/* ptr to "old" csb */
	int	err, st3;
	int	seekto;			/* where to seek for reset of DSKCHG */

	FDERRPRINT(FDEP_L1, FDEM_CHEK, ("fdcheckdisk, unit %d\n", unit));
	/* save old csb */
	csb = fdctlr.c_csb;

	ccsb = (struct fdcsb *)kmem_zalloc(sizeof (struct fdcsb));
	fdctlr.c_csb = ccsb;

	/*
	 * read drive status to see if at TRK0, if so, seek to cyl 1,
	 * else seek to cyl 0.  We do this because the controller is
	 * "smart" enough to not send any step pulses (which are how
	 * the DSKCHG line gets reset) if it sees TRK0 'cause it
	 * knows the drive is already recalibrated.
	 */
	st3 = fdsensedrv(unit);

	/* check TRK0 bit in status */
	if (st3 & T0_SR3)
		seekto = 1;	/* at TRK0, seek out */
	else
		seekto = 0;

	err = fdrecalseek(unit, seekto, 0); /* DON'T recurse check changed */

	/* "restore" old csb, check change state */
	fdctlr.c_csb = csb;
	kmem_free((caddr_t)ccsb, sizeof (struct fdcsb));

	/* any recal/seek errors are too serious to attend to */
	if (err) {
		FDERRPRINT(FDEP_L2, FDEM_CHEK, ("fdcheckdisk err %d\n", err));
		return (err);
	}

	/* if disk change still asserted, no diskette in drive! */
	if (fdsense_chng()) {
		FDERRPRINT(FDEP_L2, FDEM_CHEK, ("fdcheckdisk no disk\n"));
		return (1);
	}

	return (0);
}

/*
 * the following functions are hw dependent
 *	fdselect() - select drive, needed for external to chip select logic
 *	fdeject() - ejects drive, must be previously selected
 *	fdterm_count() - sends terminal count to previously selected drive
 *	fdsense_chng() - sense disk changed line from previously selected drive
 *		returns 1 is signal asserted, else 0
 *	fdsense_dnsty() - sense density line from previously selected drive
 *		returns 1 is signal asserted, else 0
 */
static void
fdselect(unit, on)
int	unit;
int	on;	/* non-zero = select, 0 = de-select */
{
	FDERRPRINT(FDEP_L1, FDEM_DSEL,
		("fdselect, unit %d, on = %d\n", unit, on));
#if defined(sun4c) || defined(sun4m)
#ifdef I82077
	switch (fdtype) {
	case FD_82077:
		fd_set_dor_reg(DRVSEL, !on);
		break;

	case FD_82072:
		set_auxioreg(AUX_DRVSELECT, on);
		break;
	}
#else
	set_auxioreg(AUX_DRVSELECT, on);
#endif I82077
#endif sun4c sun4m
}

static
fdeject()
{
	FDERRPRINT(FDEP_L1, FDEM_EJEC, ("fdeject\n"));
#if defined(sun4c) || defined(sun4m)
	/* assume delay of function calling sufficient settling time */
	/* eject line is NOT driven by inverter so it is true low */
#ifdef I82077
	switch (fdtype) {
	case FD_82077:
		DELAY(4);
		fd_set_dor_reg(EJECT, 1);
		DELAY(4);
		fd_set_dor_reg(EJECT, 0);
		break;

	case FD_82072:
		DELAY(4);
		set_auxioreg(AUX_EJECT, 0);
		DELAY(4);
		set_auxioreg(AUX_EJECT, 1);
		break;
	}
#else
	DELAY(4);
	set_auxioreg(AUX_EJECT, 0);
	DELAY(4);
	set_auxioreg(AUX_EJECT, 1);
#endif I82077
#endif sun4c sun4m
/*
 * XXX set ejected state?
 */
}

static
fdsense_chng()
{
	FDERRPRINT(FDEP_L1, FDEM_SCHG, ("fdsense_chng\n"));
#if defined(sun4c) || defined(sun4m)
#ifdef I82077
	switch (fdtype) {
	case FD_82077:
		if (Dir & DISKCHG)
			return (1);
		break;

	case FD_82072:
		if (Auxio & AUX_DISKCHG)
			return (1);
		break;
	}
#else
	if (Auxio & AUX_DISKCHG)
		return (1);
#endif
#endif sun4c sun4m
	return (0);
}

#ifdef NEVER	/* NOT NEEDED for Campus */
/* term count is issued by the level11 handler on Campus (in fd_asm.s) */
static
fdterm_count()
{
	int	i;
#if defined(sun4c) || defined(sun4m)
	set_auxioreg(AUX_TC, 1);
	DELAY(5);
	set_auxioreg(AUX_TC, 0);
#endif sun4c sun4m
}

static
fdsense_dnsty()
{
#if defined(sun4c) || defined(sun4m)
	if (Auxio & AUX_DENSITY)
		return (1);
#endif sun4c sun4m
	return (0);
}
#endif NEVER

/*
 * fdpacklabel
 *	this packs an (unpacked) struct dk_label into a packed label.
 */
fdpacklabel(from, to, unit)
struct dk_label *from;
struct packed_label *to;
int	unit;
{
	FDERRPRINT(FDEP_L1, FDEM_PACK, ("fdpacklabel\n"));
	/* clear the destination */
	bzero((caddr_t)to, sizeof (struct packed_label));

	to->dkl_asciip = fdkl_alabel[unit]; /* point at something reasonable */
	(void)strcpy(to->dkl_asciip, from->dkl_asciilabel);

	to->dkl_rpm = from->dkl_rpm;	/* rotations per minute */
	to->dkl_pcyl = from->dkl_pcyl;	/* # physical cylinders */
	to->dkl_apc = from->dkl_apc;	/* alternates per cylinder */
	to->dkl_intrlv = from->dkl_intrlv;	/* interleave factor */
	to->dkl_ncyl = from->dkl_ncyl;	/* # of data cylinders */
	to->dkl_acyl = from->dkl_acyl;	/* # of alternate cylinders */
	to->dkl_nhead = from->dkl_nhead; /* # of heads in this partition */
	to->dkl_nsect = from->dkl_nsect; /* # of 512 byte sectors per track */

	/* logical partitions */
	bcopy((caddr_t)from->dkl_map, (caddr_t)to->dkl_map,
		sizeof (struct dk_map) * NDKMAP);

}

#ifdef NEVER
/*
 * fdunpacklabel
 *	this unpacks a packed default label into a struct dk_label,
 *	complete with magic number and checksum
 */
fdunpacklabel(from, to)
struct packed_label *from;
struct dk_label *to;
{
	short	xsum;
	short	*sp;
	int	i;

	/* clear the destination */
	bzero((caddr_t)to, sizeof (struct dk_label));

	strcpy(to->dkl_asciilabel, from->dkl_asciip);

	to->dkl_rpm = from->dkl_rpm;	/* rotations per minute */
	to->dkl_pcyl = from->dkl_pcyl;	/* # physical cylinders */
	to->dkl_apc = from->dkl_apc;	/* alternates per cylinder */
	to->dkl_intrlv = from->dkl_intrlv;	/* interleave factor */
	to->dkl_ncyl = from->dkl_ncyl;	/* # of data cylinders */
	to->dkl_acyl = from->dkl_acyl;	/* # of alternate cylinders */
	to->dkl_nhead = from->dkl_nhead; /* # of heads in this partition */
	to->dkl_nsect = from->dkl_nsect; /* # of 512 byte sectors per track */

	/* logical partitions */
	bcopy((caddr_t)from->dkl_map, (caddr_t)to->dkl_map,
		sizeof (struct dk_map) * NDKMAP);

	to->dkl_magic = DKL_MAGIC;

	xsum = 0;
	sp = (short *)to;
	for (i = (sizeof (struct dk_label)/sizeof (short)); i > 1; i--)
		xsum ^= *sp++;

	to->dkl_cksum = xsum;
}
#endif NEVER


fd_set_dor_reg(bit, flag)
int bit;
int flag;
{
	if (bit & MOTEN ) {
		/* deselect the drive according to Sony spec */
		fd_set_dor(3,1);
		if (fdctlr.c_units[fdctlr.c_csb->csb_unit]->un_chars->medium) {
			bit |= MEDIUM_DENSITY;
			fd_set_dor(MEDIUM_DENSITY , 1);
		} else {
			fd_set_dor(MEDIUM_DENSITY, 0);
		}
		DELAY(20);
		/* select */
		fd_set_dor(3,0);
		if (fderrlevel == 0)
			printf("----> M O T O R  %d = %x\n",flag, bit);
	}

	fd_set_dor(bit, flag);
	
	/*
	 * Give at least 500 ms MIN for the motor to spin
	 */

	if (flag)
		DELAY(650000);
}


#endif /* NFD > 0 */
