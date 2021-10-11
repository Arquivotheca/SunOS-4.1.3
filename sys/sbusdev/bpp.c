
/*
 * 	Copyright (c) 1990 by Sun Microsystems, Inc.
 *      @(#)bpp.c  1.1     92/07/30     SMI
 *
 *	Source code for the bidirectional parallel port
 *	driver for the Zebra SBus card.
 *
 */

/*
 * These switches are needed to provide full source code compatibility
 * for both the modload (loadable) case and the config (linked in) case.
 * Also, some header files do not include some kernel information unless
 * KERNEL is defined.
 */
#ifndef		KERNEL			/* predefined in the config case*/
#	define	KERNEL
#else		KERNEL
#	define	LINKED_IN		/* prevent errors in make depend*/
#endif		KERNEL

/*		#includes below			*/
/*
#include <stdio.h>
*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <machine/psl.h>
#include <sun/openprom.h>
#include <sun/vddrv.h>
#include <sys/map.h>
#include <sys/debug.h>
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include "bpp_io.h"
#include "bpp_reg.h"
#include "bpp_var.h"


/*		structure definitions below			*/

static struct	bpp_transfer_parms	bpp_default_transfer_parms = {
	BPP_ACK_BUSY_HS,		/* read_handshake */
	1000,				/* read_setup_time - 1 us */
	1000,				/* read_strobe_width - 1 us */
	60,				/* read_timeout - 1 minute */
	BPP_ACK_HS,			/* write_handshake */
	1000,				/* write_setup_time - 1 us */
	1000,				/* write_strobe_width - 1 us */
	60,				/* write_timeout - 1 minute */
};

static struct bpp_pins		bpp_default_pins = {
	0,				/* output pins	*/
	0,				/* input pins	*/
};

static struct bpp_error_status	bpp_default_error_stat = {
	0,				/* no timeout		*/
	0,				/* no bus error		*/
	0,				/* no pin status set	*/
};

#ifdef	NO_BPP_HW
/* buffer to simulate hardware */
static	u_char			bpp_hard_buf[FAKE_BUF_SIZE];
/* hardware mimicing registers */
static	struct bpp_regs		bpp_fake_regs[MAX_NBPP];
/* default register values */
static	struct bpp_regs		bpp_def_regs;
#endif	NO_BPP_HW


/*		static variable declarations below		*/

					/* array of pointers to unit structs */
static	struct 	bpp_unit 		*bpp_units[MAX_NBPP];
static	u_char	nbpp		= 0;	/* number of units in system */
static	int	sys_clock	= 0;	/* system clock freq in MHz */
static	int	clock_cycle	= 0;	/* system clock period in nsec */
static	int	sbus_clock	= 0;	/* sbus clock freq prop in MHz */
static	int	sbus_cycle	= 0;	/* sbus clock prop period in nsec */
static	int	sbus_clock_cycle = 0;	/* actual sbus clock period in nsec */
extern	int	hz;			/* clock ticks per second	*/
extern	struct map *dvmamap;		/* pointer to DVMA resource map */

/*	Declarations for bpp_fakeout hardware simulation support	*/
static	u_int	sim_byte_count = 0;	/* bytes desired to be transferred */
					/* - this will be set by bpp_ioctl() */
					/*    for testing error conditions */
static	int	err_code = 0;		/* error code for simulating */
					/* hardware errors */
static	enum	trans_type {		/* state variable for last transfer */
	read_trans,
	write_trans
}		last_trans;

/*
 * bpp_printlevel controls what level of messages seen
 * on the console. LOG_DEBUG is the most verbose.
 * Do not set bpp_printlevel below LOG_CRIT or you
 * will not even see error messages from the bpp driver.
 * bpp_print depends on these levels being legal values to pass
 * to the kernel log() function.
 * These are level defs from <sys/syslog.h>, and are included
 * here for reference.
 *
 * int	bpp_printlevel = LOG_ERR;	 error conditions
 * int	bpp_printlevel = LOG_WARNING;	 warning conditions
 * int	bpp_printlevel = LOG_NOTICE;	 normal but significant condition
 * int	bpp_printlevel = LOG_INFO;	 informational
 * int	bpp_printlevel = LOG_DEBUG;	 debug-level messages
 */

#ifdef	BPP_DEBUG
int	bpp_printlevel = LOG_DEBUG;	/* debug-level messages */
#else	BPP_DEBUG
int	bpp_printlevel = LOG_ERR;	/* error conditions only */
#endif	BPP_DEBUG

/*		private procedure declarations below		*/
/* Autoconfig Declarations */
static	int	bpp_identify();
static	int	bpp_attach();
#ifdef notdef
int	bpp_init(); /* bpp_init MUST be global or modload will fail */
#endif notdef
/* Driver function Declarations */
/*
 * bpp_open, bpp_close, bpp_read, bpp_write and
 * bpp_ioctl MUST be global to build this driver into a kernel
 */
int	bpp_open();
int	bpp_close();
int	bpp_read();
int	bpp_write();
int	bpp_ioctl();
static	int	bpp_poll();
static	void	bpp_intr();
static	bpp_strategy();
static	void	bpp_minphys();

/* Utility Function Declarations */
static  bpp_transfer_timeout(); 
static	void	see_devinfo();
static	char *	get_property();
static	int 	get_num_property();
static	int	check_bpp_registers();
static	void	set_dss_dsw();
static	u_short	check_write_params();
static	u_short	check_read_params();
static	u_short check_read_pins();
static	u_short check_write_pins();
static	void	read_outpins();
#ifdef notdef
static	void	bpp_decommission();
#endif notdef
static	void	start_critical();
static	void	end_critical();
static  int 	bpp_transfer_failed();
#ifdef	NO_BPP_HW
static	void	bpp_fakeout();
static	void	bpp_stuff_fregs();
#endif	NO_BPP_HW
static	void	bpp_print_regs();
static	void	bpp_print();

caddr_t kmem_zalloc();

/*
 * The bpp_ops struct enables the kernel to find the
 * bpp_identify and bpp_attach routines. Note
 * that the name of this structure is REQUIRED to be "<drivername>_ops"
 * if the driver is to be configured into the kernel.
 */
struct	dev_ops bpp_ops =
{
	1,				/* revision number		*/
	bpp_identify,			/* confirm device ID		*/
	bpp_attach,			/* attach routine of driver	*/
};

#ifdef notdef
/*
 * Character device entry points for this driver.
 * These are in cdevsw[] in sun/conf.c for preconfigured drivers.
 */
struct cdevsw bpp_cdevsw =
{
	bpp_open,	bpp_close,	bpp_read,	bpp_write,
	bpp_ioctl,	nulldev,	seltrue,	0,		0
};

/*
 * The bpp_drv structure provides the linkage between the vd driver
 * (for loadable drivers) and the dev_ops structure for this driver
 * (bpp_ops).
 */
struct vdldrv bpp_drv = {
	VDMAGIC_DRV,				/* Drv_magic		*/
#ifndef	BPP_DEBUG
	"bpp 1.1 @(#) 1.1 92/07/30",			/* *Drv_name		*/
#else	BPP_DEBUG
	"bpp 1.1 Debug @(#) 1.1 92/07/30",		/* *Drv_name		*/
#endif	BPP_DEBUG
	&bpp_ops,				/* *Drv_dev_ops		*/
	NULL,					/* *Drv_bdevsw		*/
	&bpp_cdevsw,				/* *Drv_cdevsw		*/
	0,					/* Drv_blockmajor	*/
	0					/* Drv_charmajor	*/
};
#endif notdef

/* Autoconfig Support Functions */

/*
 *	bpp_identify()
 * Determine whether this is the right driver to work with the named device.
 * Count the number of times a match occurs - this is the number of
 * devices actually in the system.
 */
static int
bpp_identify(name)
char *name;
{
	bpp_print(LOG_INFO, "Entering bpp_identify, name: %s\n",
								name);
	if (strcmp(name, BPP_PROM_NAME) == 0)
	{
		/*
		 * Sanity check, perhaps paranoid. Limit the number of
		 * units that we accept. Any not accepted cannot be
		 * attached, and therefore cannot be serviced.
		 */
		if (nbpp < MAX_NBPP)
		{
			nbpp++;
			bpp_print(LOG_INFO,
				"Leaving bpp_identify, ACCEPTED (#%d)\n",
							nbpp);
			return (1);	/* Good match; device accepted. */
		} else {
			bpp_print(LOG_ERR,
				"bpp_identify: too many bpps (%d).\n",
							nbpp);
		}
	}
	bpp_print(LOG_INFO, "Leaving bpp_identify, (REJECTED)\n");
	return (0);	/* No match or over limit; the device rejected. */
}

/*
 *	bpp_attach()
 *
 * Map the bpp device registers into kernel virtual memory.
 * Add the bpp driver to the level 2 interrupt chain.
 * Initialize the bpp portion of the zebra card.
 * Turn on the interrupts.
 * Allocate unit structures based on the number of units (nbpp).
 */
static int
bpp_attach(devinfo_p)
register struct dev_info *devinfo_p;
{
	struct dev_reg	*dev_reg_p;
	struct dev_intr	*dev_intr_p;
	u_int		counter;
	int		myslot;
	u_int		unit_no;	/* attaching unit's number */
	int	cpu_frequency;		/* system clock frequency (in cycles) */
	int	sbus_frequency;		/* sbus clock frequency (in cycles) */
	int	interrupt_level;	/* real interrupt level used */
	int	i;

	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	struct bpp_regs	*bpp_regs_p;

	static int unit_count = 0; 	/*
					 * The static variable unit_count
					 * is the source of unit numbers to
					 * assign to the devinfo struct.
					 * It is the responsibility of the
					 * attach routine to fill in the value
					 * of devinfo_p->devi_unit.
					 */

	bpp_print(LOG_INFO, "Entering bpp_attach: devinfo_p: 0x%x\n",
								devinfo_p);
	
	/* Initialize bpp_units struct before allocating any unit structs */
	bpp_print(LOG_INFO, "Initializing units array.\n");
	if (unit_count == 0) {	/* first time attach is called */
		for (i = 0; i < MAX_NBPP; i++)
			bpp_units[i] = NULL;
	}

	if (devinfo_p == NULL) {
		/*
		 * Make the attach fail. A physical device should
		 * have been found by the prom, and therefore have a
		 * devinfo pointer. The code below requires it.
		 * This is a "can't happen".
		 */
		bpp_print(LOG_ERR,
				"bpp_attach: devinfo_p is NULL!\n");
		goto attach_failed;
	}


#ifdef	notdef
	OLD METHOD
	/*
	 * Allocate the bpp_units array based on the number of
	 * units (nbpp) counted by bpp_identify.
	 * The unit structs are allocated as zeroed memory.
	 */
	if (bpp_units == NULL) {	/* not allocated yet - 1st time */
		bpp_units =
		(struct bpp_unit *)kmem_zalloc(nbpp * sizeof (struct bpp_unit));
	}
#endif	notdef

	if (bpp_units[unit_count] != NULL) {
		/* Massive error - unit_no is out of sync? */
		bpp_print(LOG_ERR,
			"bpp_attch: unit %d struct pointer not NULL!\n",
			unit_count);
		goto attach_failed;
	}

	/*
	 * Allocate a unit structure for this unit.
	 * Each bpp_unit struct is allocated as zeroed memory.
	 * Store away its address for future use.
	 */
	bpp_print(LOG_INFO, "Allocating unit struct for unit %d.\n", 
		unit_count);
	bpp_units[unit_count] = 
		(struct bpp_unit *)kmem_zalloc(sizeof (struct bpp_unit));
	/*
	 * Initialize the unit structures. The unit structure for
	 * each unit is initialized when bpp_attach is called for that unit.
	 */

	/*
	 * Assign a unit structure and fill in the value of
	 * devinfo_p->devi_unit.
	 */
	unit_no = unit_count++;
	devinfo_p->devi_unit = unit_no;

	/* Make sure we're not in a slave-only slot */
	if ((myslot = slaveslot(devinfo_p->devi_reg->reg_addr)) >= 0) {
		bpp_print(LOG_ERR,
		    "bpp unit %d: NOT used - SBus slot %d is slave only.\n",
		    unit_no, myslot);
		goto attach_failed;
	}

	/* assign a pointer to this unit's state struct */
	bpp_p = bpp_units[unit_no];
	/*
	 * Store the devinfo pointer in the unit structure.  We will need
	 * it for decommissioning. Initialize the saved spl in the
	 * unit structure for assertion checking for critical sections.
	 * Initialize the pointer to the transfer parameters structure
	 * for this unit.
	 */
	bpp_p->devinfo_p = devinfo_p;
	bpp_p->saved_spl = (-1);

	/*
	 * Initialize the transfer parameters structure for this unit.
	 */
	bpp_p->transfer_parms = bpp_default_transfer_parms;

	/*
	 * Initialize the control pins structure for this unit.
	 */
	bpp_p->pins = bpp_default_pins;

	/*
	 * Initialize the error status structure for this unit.
	 * Initialize the timeout status byte for this unit.
	 */
	bpp_p->error_stat = bpp_default_error_stat;
	bpp_p->timeouts = NO_TIMEOUTS;
	bpp_print(LOG_INFO, "Timeout block is 0x%x.\n", bpp_p->timeouts);
	/* clear the buf struct before using it */
	bzero ((char*)&(bpp_p->buf), sizeof (struct buf));
	/*
	 * Look at the properties in the devinfo struct.  Demonstrate
	 * how to get other properties by getting the value of the
	 * "name" property the hard way.
	 */
#ifdef	DEBUG
	(void) see_devinfo(devinfo_p);
#endif	DEBUG
	(void) get_property(devinfo_p, "name", "NO NAME PROPERTY?");
	/*
	 * Check that the clock frequency in the boot prom is in
	 * a sensible range. If it isn't, the math used when setting
	 * the transfer parameters strobe and width times will
	 * fail. Flag the future problem here rather than in the ioctl.
	 */
	/*
	 * Sparcstation and its clones only have a clock-frequency
	 * property associated with the top_devinfo node. This
	 * means that the system clock and sbus clock are the
	 * same frequency. Calvin runs the sbus at a slower
	 * speed than the cpu. Try to find a clock-frequency
	 * property in my parent (sbus) node. If none there,
	 * then try the cpu node (top_devinfo).
	 */
	sbus_frequency =  get_num_property(devinfo_p->devi_parent,
				"clock-frequency", 1000000);
	sbus_clock = sbus_frequency/1000000;
	if (sbus_clock >= 10 && sbus_clock <= 25) {
		bpp_print(LOG_INFO, "Sbus clock is %d MHz.\n", sbus_clock);
		/* calculate clock period (in nsec) */
		sbus_cycle = (1000 / sbus_clock);
		sbus_clock_cycle = sbus_cycle;
	} else {
		bpp_print(LOG_WARNING, "Sbus frequency out of range.\n");
		/* try for cpu clock frequency instead */
		cpu_frequency =  get_num_property(top_devinfo,
					"clock-frequency", 1000000);
		sys_clock = cpu_frequency/1000000;
		if (sys_clock >= 10 && sys_clock <= 25) {
			bpp_print(LOG_WARNING,
				"Using CPU clock property for SBus.\n");
			bpp_print(LOG_INFO,
				"Sbus clock is %d MHz.\n", sys_clock);
			/* calculate clock period (in nsec) */
			clock_cycle = (1000 / sys_clock);
			sbus_clock_cycle = clock_cycle;
		} else {
			bpp_print(LOG_ERR,
				"Warning! System Clock freq out of range!\n");
			goto attach_failed;
		}
	}
	bpp_print(LOG_INFO, "Sbus Clock period is %d nsec.\n",
		sbus_clock_cycle);

	/*
	 * Map in any device registers. The zebra parallel section
	 * has only one register area. I'm assuming devinfo_p->nreg = 1.
	 */
	dev_reg_p = devinfo_p->devi_reg;
	/*
	 * dev_reg_p may be NULL, but only if devi_nreg is zero.
	 */
	if (dev_reg_p == NULL)
	{
		/*
		 * A "can't happen".
		 */
		bpp_print(LOG_ERR,
			"bpp_attach: dev_reg_p is NULL!\n");
		goto attach_failed;

		/*NOTREACHED*/
	}

#ifndef	NO_BPP_HW
	/* The "real" hardware case */
	/*
	 * Map the structure into kernel virtual space.
	 */
	bpp_p->bpp_regs_p =
		(struct bpp_regs *) map_regs(dev_reg_p->reg_addr,
					    sizeof (struct bpp_regs),
					    dev_reg_p->reg_bustype);
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_print(LOG_INFO, "After mapping registers, int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);
	/*
	 * Check that the size of the register set is the same as the
	 * size of the structure that overlays it.
	 */
	if (sizeof (struct bpp_regs) != dev_reg_p->reg_size)
	{
		bpp_print(LOG_ERR,
		"bpp_attach, unit %d, structure size mismatch!\n", unit_no);
		bpp_print(LOG_ERR,
		"Struct (%d bytes) != reg (%d bytes)!\n",
			sizeof (struct bpp_regs), dev_reg_p->reg_size);
		goto attach_failed;
	}


#else	NO_BPP_HW
	/* use allocated memory instead of reg_addr if no real hardware */
	/* The "faked" hardware case */
	/* bustype SPO_VIRTUAL */
	/*
	 * Map the (fake) structure into kernel virtual space.
	 */
	bpp_p->bpp_regs_p =
		(struct bpp_regs *) map_regs(&bpp_fake_regs[unit_no],
						sizeof (struct bpp_regs),
						SPO_VIRTUAL);
	bpp_print(LOG_INFO,
			"Mapped fake registers: %x mapped to %x\n",
			&bpp_fake_regs[unit_no], bpp_p->bpp_regs_p);
	/* stuff initial vals into fake registers */
	bpp_stuff_fregs(unit_no);
#endif	NO_BPP_HW

	if (bpp_p->bpp_regs_p == NULL) {
		bpp_print(LOG_ERR,
		"bpp_attach unit %d: map_regs failed!\n", unit_no);
		goto attach_failed;
	}
	if (check_bpp_registers(unit_no)) {	/* registers don't seem right */
		bpp_print(LOG_ERR,
		"bpp_attach unit %d: register check failed!\n", unit_no);
		goto attach_failed;
	}

	/*
	 * For devices that issue interrupts, the driver must install
	 * itself on the interrupt chain for each level that the hardware
	 * can interrupt at.
	 * This must be done before expecting to receive any interrupts.
	 */
	dev_intr_p = devinfo_p->devi_intr;
	/*
	 * dev_intr_p may be NULL, but only if devi_nintr is zero.
	 */
	for (counter = devinfo_p->devi_nintr; counter > 0;
						--counter, ++dev_intr_p)
	{
		if (dev_intr_p == NULL)
		{
			/*
			 * A "can't happen".
			 */
			bpp_print(LOG_ERR,
				"bpp_attach: dev_intr_p is NULL!\n");
			goto attach_failed;

			/*NOTREACHED*/
			break;
		}

		/*
		 * Convert the hardware interrupt priority level (0-15)
		 * into a psr for critical sections of code. If there
		 * is more than 1 interrupt level, use the highest one.
		 * Note that the units may differ.
		 */
		interrupt_level = dev_intr_p->int_pri;
		if (ipltospl(interrupt_level) >
				bpp_p->interrupt_pri)
		{
			bpp_p->interrupt_pri =
					ipltospl(interrupt_level);
			bpp_print(LOG_INFO,
				"Set interrupt_pri to 0x%x for unit %d.\n",
				bpp_p->interrupt_pri, unit_no);
		}


		/*
		 * If the device can do dma, then the dma routines need
		 * to know the highest priority that the device may
		 * interrupt at. The routine adddma is used to gather
		 * this information.
		 */
		adddma(interrupt_level);

#ifndef	NO_BPP_HW

		start_critical(unit_no);

		/*
		 * Addintr() does the registration. Duplicate
		 * registrations (such as from several units) are
		 * ignored except for data collection for vmstat.
		 * Addintr() panics if this registration would make
		 * too many devices on the interrupt chain. Sorry.
		 */
		(void) addintr(interrupt_level, bpp_poll,
			devinfo_p->devi_name, devinfo_p->devi_unit);

		end_critical(unit_no);

		bpp_print(LOG_INFO,
			"Installed bpp_poll: unit %d at SBus priority 0x%x\n",
						unit_no, interrupt_level);
#endif	NO_BPP_HW
	}

	/*
	 * The routine report_dev() writes a line on the console and system
	 * log file to announce the attachment of the driver. This will
	 * happen either at boot or modload time, one line for each unit.
	 * It is nice to call report_dev before contacting the device
	 * if possible so that if something horrible happens there is
	 * a record of what was being attempted.
	 */
	report_dev(devinfo_p);

	/*
	 * Perform device initialization.
	 */

	bpp_regs_p->dma_csr  |= BPP_RESET_BPP;
	bpp_regs_p->dma_csr  &= ~BPP_RESET_BPP;
	bpp_regs_p->dma_csr  |= BPP_TC_INTR_DISABLE;

	/*
	 * Set up the polarities for the ERR, SLCT PE, and BUSY interrupts.
	 * Changing the polarities could cause a stray interrupt,
	 * so clear them here.
	 * These polarities are handshake dependent.
	 * This setup corresponds to the default handshakes.
	 */
	bpp_print(LOG_INFO, "Before setting polarities, int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);
	bpp_regs_p->int_cntl |= BPP_ERR_IRP;	/* ERR rising edge */
	bpp_regs_p->int_cntl |= BPP_SLCT_IRP;	/* SLCT rising edge */
						/* SLCT+ means off-line */
	bpp_regs_p->int_cntl &= ~BPP_PE_IRP;	/* PE falling edge */
	bpp_regs_p->int_cntl |= BPP_BUSY_IRP;	/* BUSY rising edge */
	/* clear any stray interrupts */
	bpp_regs_p->int_cntl |= BPP_ALL_IRQS;
	bpp_print(LOG_INFO, "After setting polarities, int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);

	/* Last of all, turn on interrupts */
	bpp_p->bpp_regs_p->dma_csr |= BPP_INT_EN;

	if (bpp_p->bpp_regs_p->op_config
	    & BPP_VERSATEC_INTERLOCK) {	/* versatec connector absent */
		/* block versatec handshake modes */
		bpp_p->versatec_allowed = 0;
		bpp_print(LOG_INFO, "Versatec connector absent.\n");
	} else {
		/* allow versatec handshake modes */
		bpp_p->versatec_allowed = 1;
		bpp_print(LOG_INFO, "Versatec connector present.\n");
	}
	bpp_print_regs (unit_no, "At end of attach:\n");
	bpp_print(LOG_INFO, "Leaving bpp_attach: unit %d\n", unit_no);

	bpp_print(LOG_INFO, "ATTACH SUCCEEDED.\n");
	return (0);

attach_failed:
	bpp_print(LOG_INFO, "ATTACH FAILED.\n");
	bpp_p->open_inhibit = 1;	/* prevent open */
	return (-1);
}


/*
 * bpp_init -- Support for a loadable device driver.
 *		   Required by the utilities modload modstat & modunload.
 *	The VDLOAD case is called (indirectly) from modload.
 *
 *	The VDUNLOAD case is called (indirectly) from modunload.
 *
 *	The VDSTAT case is called (indirectly) from modstat.
 *
 */

#ifdef notdef
int
bpp_init(command, vdp, vdi, vds)
int		command;	/* Identify why module called	*/
struct vddrv	*vdp;		/* Pointer to kernel's structure*/
struct vdioctl_load	*vdi;	/* pointer to struct vdioctl_*	*/
struct vdstat	*vds;		/* ID and status information	*/
{
	struct vdconf		*vdconf_p;
	char			*string_p;
	u_int			bpp_unit_no;
	int			status = 0;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	bpp_print(LOG_INFO,
		"-----------------(%d)--------------- \n", vdp->vdd_id);
	bpp_print(LOG_INFO,
		"Entering bpp_init, vdp 0x%x, vdi 0x%x, vds 0x%x.\n",
							vdp, vdi, vds);
	switch (command)
	{
	case VDLOAD:
		bpp_print(LOG_DEBUG, "Command is VDLOAD.\n");
		/*
		 * For the driver of an SBus device there are generally
		 * only two things to do in the VDLOAD step. The first is
		 * to get any available configuration information that may
		 * have been passed in from a modload configuration file.
		 * The second is to fill in the linkage pointer to the
		 * vdldrv structure. Most initialization of the driver or
		 * it's devices must wait until the driver's attach()
		 * routine is called because this is when (1) the count
		 * of physical devices is known as all calls to the
		 * driver's identify() routine have taken place, and
		 * (2) the dev_info pointer and therefore the devices
		 * themselves become available.
		 */

		/*
		 * Get the configuration info from the vd driver if any
		 * was passed in from modload. Some of this code cannot
		 * be useful to the bpp driver. It is included for
		 * illustration.
		 */
		if (vdi != NULL)
		{
			bpp_print(LOG_DEBUG, "vdi_id: %d\n",
						vdi->vdi_id);
			bpp_print(LOG_DEBUG, "vdi_status: %d\n",
						vdi->vdi_status);
			bpp_print(LOG_DEBUG, "vdi_mmapaddr: 0x%x\n",
						vdi->vdi_mmapaddr);
			bpp_print(LOG_DEBUG, "vdi_symtabsize: %d\n",
						vdi->vdi_symtabsize);
			bpp_print(LOG_DEBUG, "vdi_userconf: 0x%x\n",
						vdi->vdi_userconf);
			if (vdi->vdi_userconf != NULL)
			{
				vdconf_p = vdi->vdi_userconf;
				if (vdconf_p->vdc_type == VDCEND)
				{
					bpp_print(LOG_DEBUG,
					    "No config info available.\n");
				}

				while (vdconf_p->vdc_type != VDCEND)
				{
					switch (vdconf_p->vdc_type)
					{
					case VDCCONTROLLER:

						string_p =
							"(VDCCONTROLLER)";
						break;

					case VDCDEVICE:
						string_p =
							"(VDCDEVICE)";
						break;

					case VDCBLOCKMAJOR:
						string_p =
							"(VDCBLOCKMAJOR)";
						break;

					case VDCCHARMAJOR:
						string_p =
							"(VDCCHARMAJOR)";
						break;

					case VDCSYSCALLNUM:
						string_p =
							"(VDCSYSCALLNUM)";
						break;

					default:
						bpp_print(LOG_INFO,
						"vdconf switch error!\n");
						string_p =
							"(SWITCH ERROR!)";
						break;
					}

					bpp_print(LOG_DEBUG,
					    "vdc_type: %d %s %s 0x%x\n",
						vdconf_p->vdc_type,
						string_p, "vdc_data:",
						vdconf_p->vdc_data);

					/* Move to next element of array */
					vdconf_p++;
				}
			} else {
				bpp_print(LOG_INFO,
				"No config info, vdi_userconf is NULL\n");
			}

		} else {
			bpp_print(LOG_INFO,
					"No config info, vdi is NULL\n");
		}

		/*
		 * Configure the bpp driver from the available
		 * information. If no information was passed in from
		 * modload, then use the default info from this file.
		 */

		/*
		 * Fill in pointer to the vdldrv structure,
		 * which must be already initialized at this point.
		 */
		vdp->vdd_vdtab = (struct vdlinkage *)&bpp_drv;

		bpp_print(LOG_DEBUG, "Done VDLOAD.\n");
		break;

	case VDUNLOAD:
		bpp_print(LOG_DEBUG, "Command is VDUNLOAD.\n");
		/*
		 * Check if the driver is in a condition to allow safe
		 * unloading. If it is, then cleanly shut the driver down,
		 * clean up, and release all dynamically allocated space.
		 * BE SURE to remove the driver from any interrupt chains
		 * that it is on.
		 */
		for (bpp_unit_no = 0; bpp_unit_no < nbpp;
							bpp_unit_no++)
		{
			bpp_p = bpp_units[bpp_unit_no];
			/*
			 * Inhibit further opens on the unit.
			 */
			bpp_p->open_inhibit = 1;

			/*
			 * If any any unit is open, cannot unload.
			 */
			if (bpp_p->unit_open != 0)
			{
				status = EBUSY;
			}
			/*
			 * should check that the DMA engine
			 * is idle before allowing unloading.
			 */
		}

		/*
		 * If it looks good so far, turn off interrupts, remove
		 * registration of interrupt vector, release all memory,
		 * and permit unload.
		 */
		if (status == 0)
		{
			(void) bpp_decommission();
		} else {
			/*
			 * Refuse unload, return to business as usual.
			 */
			for (bpp_unit_no = 0; bpp_unit_no < nbpp;
							bpp_unit_no++)
			{
				bpp_p = bpp_units[bpp_unit_no];
				/*
				 * Release the open inhibits.
				 */
				bpp_p->open_inhibit = 0;
			}
		}

		bpp_print(LOG_DEBUG, "Done VDUNLOAD.\n");
		break;

	case VDSTAT:
		bpp_print(LOG_DEBUG, "Command is VDSTAT.\n");

		/* DEVICE SPECIFIC */

		bpp_print(LOG_DEBUG, "Done VDSTAT.\n");
		break;

	default:
		bpp_print(LOG_ERR,
			"bpp_init: unknown command 0x%x\n", command);
		status = EINVAL;
		break;
	}

	bpp_print(LOG_INFO,
		"Leaving bpp_init, status (errno): %d.\n", status);
	return (status);
}
/*
 * Turn off interrupts, remove registration of all interrupt vectors,
 * and release all memory. Note that the info for the interrupt levels
 * came from the Fcode prom on each device, and therefore may vary from
 * unit to unit. Be sure to remove all interrupt vectors, or the kernel
 * will bus error if the device interrupts while the driver is unloaded.
 *
 */
static void
bpp_decommission()
{
	u_int			bpp_unit_no;
	int	i;
	int			counter;
	struct dev_intr		*dev_intr_p;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	struct bpp_regs	*bpp_regs_p;

	bpp_print(LOG_INFO, "Entering bpp_decommission.\n");

	/*
	 * Turn off interrupts for each unit.
	 */
	for (bpp_unit_no = 0; bpp_unit_no < nbpp; bpp_unit_no++) {
		bpp_p = bpp_units[bpp_unit_no];
		bpp_regs_p = bpp_p->bpp_regs_p;
		if (bpp_regs_p->dma_csr & BPP_ENABLE_DMA) {
			/* was transferring */
			bpp_print(LOG_ERR,
			"ERROR: bpp unload of unit %d while DMA active!\n",
				bpp_unit_no);
			/* turn off DMA  and byte count */
			bpp_regs_p->dma_csr &= ~BPP_ENABLE_DMA;
			bpp_regs_p->dma_csr &= ~BPP_ENABLE_BCNT;
			/* Reset PP state machine */
			bpp_regs_p->op_config |= BPP_SRST;
			bpp_regs_p->op_config &= ~BPP_SRST;
			/* flush the cache */
			bpp_regs_p->dma_csr |= BPP_FLUSH;
		}
		/*
		 * Disable the TC interrupts.
		 * Mask the error interrupts too.
		 * These shouldn't be on if we weren't transferring
		 * at the time, but it's safest to just turn
		 * them off anyway.
		 */
		bpp_regs_p->dma_csr |= BPP_TC_INTR_DISABLE;
		bpp_regs_p->int_cntl &=
		    ~(BPP_ERR_IRQ_EN | BPP_SLCT_IRQ_EN | BPP_PE_IRQ_EN);
		/* ALL FIELDS CLEARED, idle state */
		/*
		 * To be safer, I really should free the buf which
		 * was being used to do the transfer, and wait on
		 * a semaphore that tells me that bpp_read or
		 * bpp_write have returned the partial error.
		 */
	}

	/*
	 * Everything that we need to know is in the unit structures.
	 * From there we access the devinfo struct to find the interrupt
	 * levels that we are (might be) installed on. Since duplicate
	 * registrations with addintr() are ignored, duplicate
	 * unregistrations can not be considered an error.
	 * March through and do everything meticulously.
	 * Do not take short cuts or you die.
	 */
	for (bpp_unit_no = 0; bpp_unit_no < nbpp; bpp_unit_no++)
	{
		bpp_p = bpp_units[bpp_unit_no];
		start_critical(bpp_unit_no);

		dev_intr_p = bpp_p->devinfo_p->devi_intr;
		/*
		 * dev_intr_p may be NULL, but only if devi_nintr is zero.
		 */
		for (counter = bpp_p->devinfo_p->devi_nintr; counter > 0;
						--counter, ++dev_intr_p)
		{
			if (dev_intr_p == NULL)
			{
				/*
				 * A "can't happen".
				 */
				bpp_print(LOG_ERR,
				    "bpp_decommission ASSERT FAILED.\n");
				break;
			}
			/*
			 * In the multiple-unit case,
			 * on the first remintr really removes the
			 * interrupt routine from the poll chain.
			 * All subsequent attempts will generate
			 * a message. This is normal.
			 */
#ifdef VDDRV
			(void) remintr(dev_intr_p->int_pri, bpp_poll);
#endif VDDRV
		}

		end_critical(bpp_unit_no);
	}

#ifdef	notdef
	OLD METHOD
	/*
	 * Release the memory allocated for the unit structures.
	 */
	if ((bpp_units != NULL) && (nbpp > 0))
	{
		(void) kmem_free ((caddr_t)bpp_units,
			(u_long)(nbpp * sizeof (struct bpp_unit)));
		bpp_units = NULL;
	}
#endif	notdef
	/*
	 * Release the memory allocated for the unit structures.
	 */
	if ((bpp_units[0] != NULL) && (nbpp > 0)) {
		for (i = 0; i < nbpp; i++) {
			(void) kmem_free ((caddr_t)bpp_units[i],
				(u_int)(sizeof (struct bpp_unit)));
			bpp_units[i] = NULL;
		}
	}

	bpp_print(LOG_INFO, "Leaving bpp_decommission.\n");

	return;
}
#endif notdef
/* Normal Device Driver routines	*/

/*
 * Open the device.
 */
int
bpp_open(dev, openflags)
dev_t	dev;
int	openflags;
{
	u_char	unit_no;
	u_short	retval = 0;	/* return value (errno) for system call */
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	unit_no = BPP_UNIT(dev);
	bpp_p = bpp_units[unit_no];
	bpp_print(LOG_INFO,
		"bpp%d: Entering bpp_open, flags %d.\n", unit_no, openflags);
	/*
	 * Check for how many are allocated. At this point
	 * nbpp must be set.
	 */
	if (unit_no >= nbpp)
	{
		retval = ENXIO;
		goto out;
	}

	/*
	 * Check for allocation of unit structures.
	 */
	if (bpp_units[unit_no] == NULL)
	{
		retval = ENXIO;			/* attach failed ?? */
		goto out;
	}

	start_critical(unit_no);
	/*
	 * Only allow a single open. If this device has
	 * already been opened, return an error.
	 */
	if (bpp_p->unit_open != 0) {
		bpp_print(LOG_NOTICE, "bpp%d already opened.\n", unit_no);
		retval = EBUSY;
		goto out;
	}

	/*
	 * Check if the open mode requested is permissible.
	 */
	if (bpp_p->open_inhibit != 0) {
		bpp_print(LOG_NOTICE,
				"bpp%d open inhibit.\n", unit_no);
		retval = ENXIO;
		goto out;
	}

	/*
	 * Mark the bpp as opened.
	 */
	bpp_p->unit_open = 1;
	end_critical(unit_no);

	/*
	 * Initialize the transfer parameters structure
	 * and initialize the control pins structure
	 * for this unit.
	 */
	bpp_p->transfer_parms = bpp_default_transfer_parms;
	bpp_p->pins = bpp_default_pins;
	bpp_p->openflags = openflags;	/* record the open mode */


out:
	bpp_print(LOG_INFO, "Leaving bpp_open, unit %d: errno %d.\n",
							unit_no, retval);
	return (retval);

}

/*
 * Close the device.
 */
/*ARGSUSED*/
int
bpp_close(dev,	openflags)
dev_t	dev;
int	openflags;
{
	u_char	unit_no;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	unit_no = BPP_UNIT(dev);
	bpp_p = bpp_units[unit_no];
	bpp_print(LOG_INFO,
		"Entering bpp_close, unit number %d.\n", unit_no);

	/*
	 * Implement the close.
	 */
	bpp_print(LOG_INFO, "In bpp_close, Timeout block is 0x%x.\n",
		bpp_p->timeouts);
	if (bpp_p->timeouts) {			 /* any timeouts pending? */
		bpp_print(LOG_INFO, "Some timeouts still pending.\n");
		if (bpp_p->timeouts & TRANSFER_TIMEOUT) {
			bpp_print(LOG_INFO, "Clearing transfer timeout.\n");
			untimeout(bpp_transfer_timeout, (caddr_t)unit_no);
		}
		if (bpp_p->timeouts & FAKEOUT_TIMEOUT) {
#ifdef	NO_BPP_HW
			bpp_print(LOG_INFO, "Clearing fakeout timeout.\n");
			untimeout(bpp_fakeout, (caddr_t) 0);
#else	NO_BPP_HW
			bpp_print(LOG_WARNING, "BOGUS fakeout timeout.\n");
#endif	NO_BPP_HW
		}
	}
	bpp_p->timeouts = NO_TIMEOUTS;
	bpp_print(LOG_INFO, "At end of  bpp_close, Timeout block is 0x%x.\n",
		bpp_p->timeouts);

	/*
	 * Mark unit closed.
	 */
	bpp_p->unit_open = 0;

	bpp_print(LOG_INFO, "Leaving bpp_close, unit %d:\n", unit_no);
	return (0);
}

/*
 * Read system call.
 */
/*ARGSUSED*/
int
bpp_read(dev, uio)
dev_t		dev;
struct uio	*uio;
{
	u_char	unit_no;
	u_short	retval = 0;	/* return value (errno) for system call */
	struct bpp_regs	*bpp_regs_p;
	struct	bpp_transfer_parms	*bpp_transfer_parms_p;
	/* time to allow the scanner to change from data sink to source */
	static	int	scan_turnaround = 1000;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	unit_no = BPP_UNIT(dev);
	bpp_p = bpp_units[unit_no];
	bpp_print(LOG_INFO, "Entering bpp_read, unit number %d.\n",
								unit_no);

	bpp_print_regs(unit_no, "in bpp_read");

	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_transfer_parms_p = &bpp_p->transfer_parms;

	/*
	 * delay to allow for scanning write/read turnaround
	 */

	if (last_trans == write_trans) {
		DELAY(scan_turnaround);
	}

	/*
	 * Set the handshake bits
	 */

	start_critical(unit_no);
	/*
	 * make sure the memory clear operation is turned off
	 */
	bpp_regs_p->op_config &= ~BPP_EN_MEM_CLR;

	switch (bpp_transfer_parms_p->read_handshake) {
	case BPP_NO_HS:
		bpp_print(LOG_INFO, "BPP_NO_HS case\n");
		bpp_regs_p->op_config &= ~(BPP_ACK_OP | BPP_BUSY_OP);
		bpp_regs_p->op_config |= (BPP_DS_BIDIR | BPP_BUSY_BIDIR);
		break;
	case BPP_ACK_HS:
		bpp_print(LOG_INFO, "BPP_ACK_HS case\n");
		bpp_regs_p->op_config &= ~BPP_BUSY_OP;
		bpp_regs_p->op_config |= BPP_ACK_OP;
		bpp_regs_p->op_config |=
			(BPP_DS_BIDIR | BPP_ACK_BIDIR | BPP_BUSY_BIDIR);
		break;
	case BPP_BUSY_HS:
	case BPP_HSCAN_HS:
		bpp_print(LOG_INFO, "BPP_BUSY_HS case\n");
		bpp_regs_p->op_config |= BPP_BUSY_OP;
		bpp_regs_p->op_config &= ~BPP_ACK_OP;
		bpp_regs_p->op_config |= (BPP_DS_BIDIR | BPP_BUSY_BIDIR);
		break;
	case BPP_ACK_BUSY_HS:
		bpp_print(LOG_INFO, "BPP_ACK_BUSY_HS case\n");
		bpp_regs_p->op_config |= (BPP_BUSY_OP | BPP_ACK_OP);
		bpp_regs_p->op_config |=
			(BPP_DS_BIDIR | BPP_ACK_BIDIR | BPP_BUSY_BIDIR);
		break;
	case BPP_XSCAN_HS:
		/*
		 *reads with the Xerox use ACK handshake
		 * and unidirectional operation
		 */
		bpp_print(LOG_INFO, "BPP_XSCAN_HS case\n");
		bpp_regs_p->op_config &= ~BPP_BUSY_OP;
		bpp_regs_p->op_config |= BPP_ACK_OP;
		bpp_regs_p->op_config &=
			~(BPP_DS_BIDIR | BPP_BUSY_BIDIR | BPP_ACK_BIDIR);
		break;
	case BPP_CLEAR_MEM:
		bpp_print(LOG_INFO, "BPP_CLEAR_MEM case\n");
		bpp_regs_p->op_config &= ~BPP_DMA_DATA;
		bpp_regs_p->op_config |= BPP_EN_MEM_CLR;
		break;
	case BPP_SET_MEM:
		bpp_print(LOG_INFO, "BPP_SET_MEM case\n");
		bpp_regs_p->op_config |= BPP_DMA_DATA;
		bpp_regs_p->op_config |= BPP_EN_MEM_CLR;
		break;
	}
	/*
	 * The direction should not be marked until after the handshake
	 * bits have been set.
	 */
	bpp_regs_p->trans_cntl |= BPP_DIRECTION;
	end_critical(unit_no);

	/* set the dss and dsw values */
	set_dss_dsw(unit_no, 1);

	/*
	 * If we're opened for read/write,
	 * toggle the scan/print line for scanning
	 */
	if ((bpp_p->openflags & FREAD) &&
	    (bpp_p->openflags & FWRITE)) {
		start_critical(unit_no);
		if (bpp_transfer_parms_p->read_handshake == BPP_HSCAN_HS) {
			/* The HP Scanjet uses AFX */
			bpp_regs_p->out_pins |= BPP_AFX_PIN;
		} else {
			bpp_regs_p->out_pins |= BPP_SLCTIN_PIN;
		}
		end_critical(unit_no);
	}
	bpp_print_regs(unit_no, "after switch, before physio:");

	retval = physio(bpp_strategy, &bpp_p->buf, dev, 
			B_READ, bpp_minphys, uio);

	bpp_print(LOG_INFO,
		"Leaving bpp_read, unit %d: errno %d.\n",
							unit_no, retval);
	last_trans = read_trans;	/* set last transfer type */
	return (retval);
}

/*
 * Write data to the bpp. Block until the write is finished.
 */
int
bpp_write(dev, uio)
dev_t		dev;
struct uio	*uio;
{
	u_char	unit_no;
	u_short	retval = 0;	/* return value (errno) for system call */
	register struct bpp_regs	*bpp_regs_p;
	register struct	bpp_transfer_parms	*bpp_transfer_parms_p;

	register struct bpp_error_status *bpp_errorstat_p; /* error stat */
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	unit_no = BPP_UNIT(dev);
	bpp_print(LOG_INFO,
		"Entering bpp_write, unit number %d.\n", unit_no);

	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_transfer_parms_p = &bpp_p->transfer_parms;
	bpp_errorstat_p = &bpp_p->error_stat;

	/* clear any old error status */
	*bpp_errorstat_p = bpp_default_error_stat;

	/*
	 * Set up the polarities for the ERR, SLCT PE, and BUSY interrupts.
	 * Changing the polarities could cause a stray interrupt,
	 * so clear them here.
	 * These polarities are handshake dependent.
	 * This setup corresponds to the default handshakes.
	 */
	bpp_print(LOG_INFO, "Before setting polarities, int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);
	start_critical(unit_no);
	bpp_regs_p->int_cntl |= BPP_ERR_IRP;	/* ERR rising edge */
	bpp_regs_p->int_cntl |= BPP_SLCT_IRP;	/* SLCT rising edge */
						/* SLCT+ is off-line */
	bpp_regs_p->int_cntl &= ~BPP_PE_IRP;	/* PE falling edge */

	bpp_regs_p->int_cntl |= (BPP_ERR_IRQ | BPP_SLCT_IRQ | BPP_PE_IRQ);
	end_critical(unit_no);

	bpp_print(LOG_INFO, "After setting polarities, int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);

	start_critical(unit_no);
	check_for_active_pins(unit_no);
	end_critical(unit_no);

	/*
	 * if any active pins were found, don't attempt the transfer,
	 * unless we're in scanner mode (read-write), scanners use the PE line
	 * to get the host's attention.
	 */
	if ((bpp_p->openflags & (FREAD | FWRITE)) == (FREAD | FWRITE)) {
		/*
		 * Toggle the scan/print line for scanning
		 */
		start_critical(unit_no);
		if (bpp_transfer_parms_p->read_handshake == BPP_HSCAN_HS) {
			/* The HP Scanjet uses AFX */
			bpp_regs_p->out_pins &= ~BPP_AFX_PIN;
		} else {
			bpp_regs_p->out_pins &= ~BPP_SLCTIN_PIN;
		}
		end_critical(unit_no);
	} else {
		if ((bpp_errorstat_p->pin_status &
		    (BPP_ERR_ERR | BPP_SLCT_ERR | BPP_PE_ERR))) {
			/* printer error - no transfer allowed */
			bpp_print(LOG_INFO,
				"In bpp_write, pending error pin condition\n");
			retval = ENXIO;
			goto out;
		}
	}

	start_critical(unit_no);
	/* mark the transfer direction in the hardware */
	bpp_regs_p->trans_cntl &= ~BPP_DIRECTION;

	/*
	 * make sure the memory clear operation is turned off
	 */
	bpp_regs_p->op_config &= ~BPP_EN_MEM_CLR;

	/*
	 * Set the handshake bits
	 */
	if (bpp_transfer_parms_p->write_handshake == BPP_NO_HS) {
		bpp_print(LOG_INFO, "BPP_NO_HS case\n");
		bpp_regs_p->op_config &= ~(BPP_ACK_OP | BPP_BUSY_OP);
	} else if (bpp_transfer_parms_p->write_handshake == BPP_ACK_HS) {
		bpp_print(LOG_INFO, "BPP_ACK_HS case\n");
		bpp_regs_p->op_config &= ~BPP_BUSY_OP;
		bpp_regs_p->op_config |= BPP_ACK_OP;
	} else if (bpp_transfer_parms_p->write_handshake == BPP_BUSY_HS) {
		bpp_print(LOG_INFO, "BPP_BUSY_HS case\n");
		bpp_regs_p->op_config |= BPP_BUSY_OP;
		bpp_regs_p->op_config &= ~BPP_ACK_OP;
	}

	/* Make sure that ACK and BUSY are unidirectional */
	bpp_regs_p->op_config &= ~(BPP_ACK_BIDIR | BPP_BUSY_BIDIR);
	end_critical(unit_no);

	/* set the dss and dsw values */
	set_dss_dsw(unit_no, 0);

	retval = physio(bpp_strategy, &bpp_p->buf, dev, 
			B_WRITE, bpp_minphys, uio);

out:
	bpp_print(LOG_INFO,
		"Leaving bpp_write, unit %d: errno %d.\n", unit_no, retval);
	last_trans = write_trans;	/* set last transfer type */
	return (retval);
}

/*
 * Limit transfer size to the smaller of
 *	- system minphys size
 *	- 16 MB limit in HIOD address register
 */
static void
bpp_minphys(bp)
struct buf *bp;
{
	minphys(bp);
	if (bp->b_bcount > BPP_MAX_DMA)
		bp->b_bcount = BPP_MAX_DMA;
}

/*
 * Check to see if any of the control pins are active.
 * NOTE: if this routine is called from the top half,
 * it should be bracketed by start_critical and end_critical.
 */
int
check_for_active_pins(unit_no)
u_char	unit_no;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register struct bpp_regs	*bpp_regs_p;
	register struct bpp_error_status *bpp_errorstat_p; /* error stat */

	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_errorstat_p = &bpp_p->error_stat;

	bpp_print(LOG_INFO,
		"Entering check_for_active_pins, unit number %d.\n", unit_no);
	/*
	 * Check that there are no pending ERR, SLCT or PE error
	 * conditions. If there are, do not attempt the transfer.
	 */
	bpp_print(LOG_INFO, "check_active_pins: in_pins = 0x%x\n",
			bpp_regs_p->in_pins);
	bpp_print(LOG_INFO, "check_active_pins: int_cntl = 0x%x\n",
			bpp_regs_p->int_cntl);

	if (((bpp_regs_p->in_pins & BPP_ERR_PIN) &&
		(bpp_regs_p->int_cntl & BPP_ERR_IRP)) ||
		((~bpp_regs_p->in_pins & BPP_ERR_PIN)&&
		(~bpp_regs_p->int_cntl & BPP_ERR_IRP))) { /* ERR active */
			bpp_print(LOG_INFO,
				"In ck_active_pins, pending ERR condition\n");
			bpp_errorstat_p->pin_status |= BPP_ERR_ERR;
	}

	if (((bpp_regs_p->in_pins & BPP_SLCT_PIN) &&
		(bpp_regs_p->int_cntl & BPP_SLCT_IRP)) ||
		((~bpp_regs_p->in_pins & BPP_SLCT_PIN)&&
		(~bpp_regs_p->int_cntl & BPP_SLCT_IRP))) { /* SLCT active */
			bpp_print(LOG_INFO,
				"In ck_active_pins, pending SLCT condition\n");
			bpp_errorstat_p->pin_status |= BPP_SLCT_ERR;
	}

	if (((bpp_regs_p->in_pins & BPP_PE_PIN) &&
		(bpp_regs_p->int_cntl & BPP_PE_IRP)) ||
		((~bpp_regs_p->in_pins & BPP_PE_PIN)&&
		(~bpp_regs_p->int_cntl & BPP_PE_IRP))) { /* PE active */
			bpp_print(LOG_INFO,
				"In check_active_pins, pending PE condition\n");
			bpp_errorstat_p->pin_status |= BPP_PE_ERR;
	}
	bpp_print(LOG_INFO,
		"Leaving check_for_active_pins, unit number %d.\n", unit_no);
}

/*
 * Setup and start a transfer on the device.
 */
/*ARGSUSED*/
static 
bpp_strategy(bp)
register struct buf	*bp;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	u_int		unit_no;
	int	timeout_value;		/* read or write timeout in secs */
	u_long	start_address;		/* kernel virt. DVMA start address */

#ifdef notdef 
	u_long	end_address;		/* kernel virt. DVMA end address */
#endif  notdef

	u_long	size;			/* size of DVMA transfer */
	register struct	bpp_transfer_parms	*bpp_transfer_parms_p;
	register struct bpp_regs	*bpp_regs_p;


	unit_no = BPP_UNIT(bp->b_dev);
	bpp_print(LOG_INFO,
		"bpp%d:Entering bpp_strategy: length 0x%x.\n",
		unit_no, bp->b_bcount);
	
	/*
	 * Use the unit number to locate our data structures.
	 */
	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_transfer_parms_p = &bpp_p->transfer_parms;

	/* Clear the unit error status struct */
	bpp_p->error_stat = bpp_default_error_stat;
	bpp_print_regs(unit_no, "Start strategy, before checking for bpp_idle");



	start_critical(unit_no);
	/*
	 * Get dvma bus resource, sleeping if necessary.
	 */
	bp->b_mbinfo =
		mb_mapalloc(dvmamap, bp, 0, ((int (*)())0), (caddr_t) 0);
#ifdef	DEBUG
	bpp_print(LOG_INFO,
		"In bpp_strategy, *(&bp->b_mbinfo) = %x\n", *(&bp->b_mbinfo));
#endif	DEBUG

#ifdef	notdef
	/* FIXME: This code has never been tested properly */

	/*
	 * The bpp hardware will not properly transfer across
	 * a 16 MB boundary. If the transfer will cross the boundary,
	 * I trim the size I pass to the DMA engine, so
	 * that the resulting end address is just short of
	 * the boundary.
	 */

	size = bp->b_bcount;
	start_address = (u_long) MBI_ADDR(bp->b_mbinfo + DVMA);
	end_address = start_address + size;
	
	if ((start_address % BPP_MAX_DMA) > 
		(end_address % BPP_MAX_DMA)) {
	/* transfer would cross a boundary */
		bpp_print(LOG_DEBUG, 
			"bpp_strategy, unit %d, transfer crosses boundary.\n",
			unit_no);
		size = BPP_MAX_DMA - (start_address % BPP_MAX_DMA);
		bpp_p->transfer_remainder = (bp->b_bcount - size); 
	} else {
		size = bp->b_bcount;
		bpp_p->transfer_remainder = 0; 
	}
#else	notdef

	size = bp->b_bcount;
	start_address = (u_long) MBI_ADDR(bp->b_mbinfo + DVMA);

#ifdef notdef 
	end_address = start_address + size;
#endif  notdef

	bpp_p->transfer_remainder = 0; 

#endif	notdef

	/*
	 * Write the dma start address to the hardware.
	 * Write the transfer byte count to the hardware.
	 */
	bpp_regs_p->dma_addr = start_address;

	bpp_regs_p->dma_bcnt = size;
	end_critical(unit_no);

	bpp_print(LOG_INFO,
			"bpp_strategy: Transfer %d bytes starting at 0x%x.\n",
				bpp_regs_p->dma_bcnt, bpp_regs_p->dma_addr);
	bpp_print(LOG_INFO,
		"before enabling interrupts, dma csr=0x%x, int_cntl=0x%x.\n",
		bpp_regs_p->dma_csr, bpp_regs_p->int_cntl);

	/*
	 * Enable byte-counter during DVMA.
	 * Enable TC interrupts so we will know when the DVMA is done.
	 * Start the DVMA.
	 * Enable the peripheral error interrupts.
	 */
	start_critical(unit_no);
	/*
	 * Do not close critical section until timeouts have been
	 * enabled, otherwise we might get an untimeout before
	 * the timeout has been set!
	 */
	bpp_regs_p->dma_csr |= BPP_ENABLE_BCNT;
	bpp_regs_p->dma_csr &= ~BPP_TC_INTR_DISABLE;
	bpp_regs_p->dma_csr |= BPP_ENABLE_DMA;

	if (bp->b_flags & B_READ) {
		bpp_print(LOG_INFO,
			"bp->b_flags indicates READ mode\n");
		timeout_value = bpp_transfer_parms_p->read_timeout;
	} else {
		bpp_print(LOG_INFO,
			"bp->b_flags indicates WRITE mode\n");
		if (!(bpp_p->openflags & FREAD &&
		    bpp_p->openflags & FWRITE)) {
			bpp_regs_p->int_cntl |= (BPP_ERR_IRQ_EN |
				BPP_SLCT_IRQ_EN | BPP_PE_IRQ_EN);
		}
		bpp_print(LOG_INFO,
			"after enable error int. int cntl contains 0x%x.\n",
			bpp_regs_p->int_cntl);
		timeout_value = bpp_transfer_parms_p->write_timeout;
	}

	bpp_print(LOG_INFO,
	"after enabling interrupts, dma csr = 0x%x, int_cntl = 0x%x.\n",
		bpp_regs_p->dma_csr, bpp_regs_p->int_cntl);
	bpp_print(LOG_INFO,
		"Setting timeout to call bpp_transfer_timeout in %d sec\n",
			timeout_value);
	timeout(bpp_transfer_timeout, (caddr_t)unit_no, (timeout_value*hz));
	bpp_p->timeouts |= TRANSFER_TIMEOUT;
	bpp_print(LOG_INFO, "In bpp_strategy, Timeout block is 0x%x.\n",
		bpp_p->timeouts);
	end_critical(unit_no);


#ifdef	NO_BPP_HW
	bpp_print(LOG_INFO,
		"Setting timeout to call bpp_fakeout in 2 sec\n");
	timeout(bpp_fakeout, unit_no, 2*hz); /* 2 seconds */
	bpp_p->timeouts |= FAKEOUT_TIMEOUT;
	bpp_print(LOG_INFO, "In bpp_strategy, Timeout block is 0x%x.\n",
		bpp_p->timeouts);
#endif	NO_BPP_HW

	bpp_print_regs(unit_no, "end of strategy");
	bpp_print(LOG_INFO, "Leaving bpp_strategy.\n");
	return;
}

/*
 * Handle special control requests
 */
/*ARGSUSED*/
int
bpp_ioctl(dev, cmd, arg, flag)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	flag;
{
	u_char	unit_no;
	u_short	retval = 0;	/* return value (errno) for system call */
	u_short	read_retval = 0;
	u_short	write_retval = 0;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register struct bpp_regs	*bpp_regs_p;
	register struct	bpp_transfer_parms	*bpp_transfer_parms_p;
	register struct bpp_error_status *bpp_errorstat_p; /* error stat */
	register struct bpp_pins 	*bpp_pins_p; /* error stat */

	register enum	handshake_t	write_handshake;
	register enum	handshake_t	read_handshake;

	unit_no = BPP_UNIT(dev);
	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_transfer_parms_p = &bpp_p->transfer_parms;
	bpp_errorstat_p = &bpp_p->error_stat;
	bpp_pins_p = &bpp_p->pins;

	write_handshake = bpp_transfer_parms_p->write_handshake;
	read_handshake = bpp_transfer_parms_p->read_handshake;

	bpp_print(LOG_INFO,
		"Entering bpp_ioctl, unit number %d.\n", unit_no);

	switch (cmd) {
		case BPPIOC_SETPARMS:	/* set transfer parameters */
			bpp_print(LOG_INFO, "BPPIOC_SETPARMS case.\n");
			bpp_transfer_parms_p = (struct bpp_transfer_parms *)arg;
			if  (flag & FREAD) {
				bpp_print(LOG_INFO,
					"Checking read parameters.\n");
				read_retval =
					check_read_params(bpp_transfer_parms_p,
								flag);
			}
			if  (flag & FWRITE) {
				bpp_print(LOG_INFO,
					"Checking write parameters.\n");
				write_retval =
					check_write_params(bpp_transfer_parms_p,
								flag);
			}
			if (read_retval || write_retval) {
				retval = EINVAL;
			} else {	/* valid parameters */
				bpp_p->transfer_parms =
				*(struct bpp_transfer_parms *)arg;
			}
			break;
		case BPPIOC_GETPARMS:	/* get transfer parameters */
			bpp_print(LOG_INFO, "BPPIOC_GETPARMS case.\n");
			*(struct bpp_transfer_parms *)arg =
				bpp_p->transfer_parms;
			retval = 0;
			break;
		case BPPIOC_SETOUTPINS:	/* set output pins */
			bpp_print(LOG_INFO, "BPPIOC_SETOUTPINS case.\n");
			bpp_pins_p = (struct bpp_pins *)arg;
			if  (flag & FREAD) {
				bpp_print(LOG_INFO,
					"Checking read pins.\n");
				read_retval = check_read_pins(bpp_pins_p,
						flag, read_handshake);
				if (read_retval == 0) {	/* valid pins */
					bpp_p->pins =
					*(struct bpp_pins *)arg;

				}
			}
			if  (flag & FWRITE) {
				bpp_print(LOG_INFO,
					"Checking write pins.\n");
				write_retval = check_write_pins(bpp_pins_p,
						flag, write_handshake);
				if (write_retval == 0) { /* valid pins */
					bpp_p->pins =
					*(struct bpp_pins *)arg;

				}
			}
			if (read_retval || write_retval) {
				retval = EINVAL;
			} else {	/* All is well, write the registers */
				start_critical(unit_no);
				bpp_regs_p->out_pins =
					bpp_p->pins.output_reg_pins;
				/* the previous line will not cstyle */
				bpp_regs_p->in_pins =
					bpp_p->pins.input_reg_pins;
				end_critical(unit_no);
			}
			break;
		case BPPIOC_GETOUTPINS:	/* get output pins */
			bpp_print(LOG_INFO, "BPPIOC_GETOUTPINS case.\n");
			/* read the current pin state into the struct */
			read_outpins(unit_no, flag, write_handshake);
			*(struct bpp_pins *)arg =
				bpp_p->pins;
			retval = 0;
			break;
		case BPPIOC_GETERR:	/* get error block status */
			bpp_print(LOG_INFO, "BPPIOC_GETERR case.\n");
			*(struct bpp_error_status *)arg =
				bpp_p->error_stat;
			retval = 0;
			break;
		case BPPIOC_TESTIO:	/* test transfer readiness */
			bpp_print(LOG_INFO, "BPPIOC_TESTIO case.\n");
			bpp_errorstat_p = &bpp_p->error_stat;
			retval = 0;
			/* clear any old error status */
			*bpp_errorstat_p = bpp_default_error_stat;
			start_critical(unit_no);
			check_for_active_pins(unit_no);
			end_critical(unit_no);
			/* if any active pins were found, return -1 */
			if (bpp_errorstat_p->pin_status &
				(BPP_ERR_ERR | BPP_SLCT_ERR | BPP_PE_ERR)) {
				bpp_print(LOG_INFO,
				"In TESTIO, found error pin condition\n");
				retval = EIO;
			} else
				retval = 0;
			break;
		/* TEST - request partial fake transfer */
		case BPPIOC_SETBC:
			bpp_print(LOG_INFO, "BPPIOC_SETBC case.\n");
			sim_byte_count = *(u_int *)arg;
			retval = 0;
			break;
		/* TEST - get DMA_BCNT from last data transfer */
		case BPPIOC_GETBC:
			bpp_print(LOG_INFO, "BPPIOC_GETBC case.\n");
			retval = 0;
			break;
		/* TEST - get contents of device registers */
		case BPPIOC_GETREGS:
			bpp_print(LOG_INFO, "BPPIOC_GETREGS case.\n");
			*(struct bpp_regs *)arg =
				*(bpp_p->bpp_regs_p);
			retval = 0;
			break;
		/* TEST - set special fakeout error code to simulate errs */
		case BPPIOC_SETERRCODE:
			bpp_print(LOG_INFO, "BPPIOC_SETERRCODE case.\n");
			err_code = *(int *)arg;
			retval = 0;
			break;
		/* TEST - get pointer to fakeout transferred data */
		case BPPIOC_GETFAKEBUF:
			bpp_print(LOG_INFO, "BPPIOC_GETFAKEBUF case.\n");
#ifdef	NO_BPP_HW
			bpp_print(LOG_INFO, "bpp_hard_buf = 0x%x\n",
					bpp_hard_buf);
			bpp_print(LOG_INFO, "arg = 0x%x\n", arg);
			bpp_print(LOG_INFO, "*arg = 0x%x\n", *arg);
			{
				u_char **argpp;

				argpp = (u_char **)arg;
			retval = (copyout(&bpp_hard_buf[0],
					*argpp, FAKE_BUF_SIZE));
			bpp_print(LOG_INFO, "argpp = 0x%x\n", argpp);
			bpp_print(LOG_INFO, "*argpp = 0x%x\n", *argpp);
			}
#else	NO_BPP_HW
			bpp_print(LOG_WARNING, "Bad ioctl call!\n");
#endif	NO_BPP_HW
			retval = ENOTTY;
			break;
		default:
			bpp_print(LOG_NOTICE, "Error in bpp_ioctl switch!\n");
			retval = ENOTTY;
			break;
	}

	bpp_print(LOG_INFO,
		"Leaving bpp_ioctl, unit %d: errno %d.\n",
							unit_no, retval);
	return (retval);
}


/*
 * Handle an interrupt or interrupts that may or may not be from
 * one or more of the bpp units. This routine is safely declared
 * static because it is explicitly registered by address.
 */
static int
bpp_poll()
{
	u_int	bpp_unit_no;
	u_int	int_serviced = 0;
	register struct bpp_regs	*bpp_regs_p;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	bpp_print(LOG_INFO, "Entering bpp_poll.\n");

	/*
	 * March through the units, checking for an interrupt pending.
	 */
	for (bpp_unit_no = 0; bpp_unit_no < nbpp; ++bpp_unit_no) {
		bpp_p = bpp_units[bpp_unit_no];
		bpp_regs_p = bpp_p->bpp_regs_p;
		if ((bpp_regs_p->dma_csr & BPP_INT_PEND) ||
		    (bpp_regs_p->dma_csr & BPP_ERR_PEND)) {
			bpp_print(LOG_INFO, "Interrupt found, unit #%d.\n",
						bpp_unit_no);
			/*
			 * Mark that we found an interrupting device.
			 * Call the interrupt service routine.
			 */
			int_serviced = 1;
			(void) bpp_intr(bpp_unit_no);
		}
	}
	bpp_print(LOG_INFO, "Leaving bpp_poll, int_serviced = 0x%x.\n",
		int_serviced);
	return (int_serviced);
}
/*
 * Interrupt routine for the bpp hardware. This routine is always called
 * from bpp_poll with the unit number. When this routine is called,
 * we know that this unit issued an interrupt that needs service.
 */
static void
bpp_intr(unit_no)
u_int	unit_no;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register struct bpp_regs	*bpp_regs_p;
	register struct bpp_error_status *bpp_errorstat_p; /* error stat */
	register struct buf		*bp;

	bpp_print(LOG_INFO, "Entering bpp_intr, unit %d.\n",
				unit_no);
	/*
	 * Set pointer to dma registers.
	 * Set pointer to error stat struct.
	 * Set pointer to buf structure.
	 */
	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_errorstat_p = &bpp_p->error_stat;
	bp = &bpp_p->buf;


	bpp_print(LOG_INFO, "dma csr contains 0x%x.\n",
				bpp_regs_p->dma_csr);
	bpp_print(LOG_INFO, "int cntl contains 0x%x.\n",
				bpp_regs_p->int_cntl);

	/*
	 * The bpp hardware can interrupt for errors, or
	 * for several transfer conditions. These can happen
	 * at the same time.
	 * I process any errors first, checking to see
	 * if a transfer was in process.
	 * In the future, I may want to count how many times I've
	 * tried to flush the cache, and eventually give up and
	 * reset the hardware.
	 * Then I process "normal" conditions.
	 */

	/*
	 * Check for an error, recover if possible.
	 */
	if (bpp_regs_p->dma_csr & BPP_ERR_PEND) {
		bpp_print(LOG_INFO, "Error interrupt detected\n");
		if (bpp_regs_p->dma_csr & BPP_SLAVE_ERR) {
			bpp_print(LOG_INFO, "Slave error detected\n");
		}
		if (bpp_regs_p->dma_csr & BPP_ENABLE_DMA) {
			/* was transferring */
			/*
			 * The transfer has failed. Notify the application
			 * how many bytes got out, and that there was
			 * an IO error, and turn off the transfer.
			 */
			bpp_transfer_failed(unit_no);
			return;
		} else {
			/* will make return value -1 */
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
		}
		/*
		 * capture the error status in the error_stat struct
		 * so the application can get it with the GETERR
		 * ioctl later.
		 */
		bpp_errorstat_p->bus_error = 1;

		/* Mark the error.  */
		bp->b_error = EIO;

		/* clear the error interrupt */
		while (bp->b_flags & B_READ)
			/* cannot assert FLUSH till cache drains */
			/* spin on draining bit */
		bpp_regs_p->dma_csr |= BPP_FLUSH;
	}
	if (bpp_regs_p->dma_csr & BPP_INT_PEND) {
		bpp_print(LOG_INFO, "Interrupt pending found.\n");
		bpp_print(LOG_INFO, "dma csr contains 0x%x.\n",
					bpp_regs_p->dma_csr);
		bpp_print(LOG_INFO, "int cntl contains 0x%x.\n",
					bpp_regs_p->int_cntl);
		/* TC case - terminal count */
		if (bpp_regs_p->dma_csr & BPP_TERMINAL_CNT &&
		    ((bpp_regs_p->dma_csr & BPP_TC_INTR_DISABLE) == 0)) {
			bpp_print(LOG_INFO,
			"Terminal count interrupt found.\n");
			/* mask this interrupt */
			bpp_regs_p->dma_csr |= BPP_TC_INTR_DISABLE;
			bpp_regs_p->dma_csr &= ~BPP_ENABLE_BCNT;
			/* and clear the interrupting condition */
			bpp_regs_p->dma_csr |= BPP_TERMINAL_CNT;
			bpp_regs_p->dma_csr &= ~BPP_ENABLE_DMA;
			bp->b_resid = bpp_p->transfer_remainder;
			/* Mask the error interrupt conditions */
			bpp_regs_p->int_cntl &=
				~(BPP_ERR_IRQ_EN | BPP_SLCT_IRQ_EN |
				BPP_PE_IRQ_EN);
			bpp_print(LOG_INFO, "dma csr contains 0x%x.\n",
					bpp_regs_p->dma_csr);
			bpp_print(LOG_INFO, "int cntl contains 0x%x.\n",
					bpp_regs_p->int_cntl);
		}
		/* ERR_IRQ case - error pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_ERR_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_ERR_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Error pin interrupt found.\n");
			bpp_errorstat_p->pin_status |= BPP_ERR_ERR;
			if (bpp_regs_p->dma_csr & BPP_ENABLE_DMA) {
				/* was transferring */
				bpp_transfer_failed(unit_no);
			}
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_ERR_IRQ;
		}
		/* SLCT_IRQ case - select pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_SLCT_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_SLCT_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Select pin interrupt found.\n");
			bpp_errorstat_p->pin_status |= BPP_SLCT_ERR;
			if (bpp_regs_p->dma_csr & BPP_ENABLE_DMA) {
				/* was transferring */
				bpp_transfer_failed(unit_no);
			}
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_SLCT_IRQ;
		}
		/* PE_IRQ case - paper error pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_PE_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_PE_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Paper error pin interrupt found.\n");
			bpp_errorstat_p->pin_status |= BPP_PE_ERR;
			if (bpp_regs_p->dma_csr & BPP_ENABLE_DMA) {
				/* was transferring */
				bpp_transfer_failed(unit_no);
			}
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_PE_IRQ;
		}
	/*
	 * The interrupts below (BUSY, ACK, and DS)
	 * are available in the hardware, but are not
	 * being used for anything now.
	 */
		/* BUSY_IRQ case - busy pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_BUSY_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_BUSY_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Busy pin interrupt found.\n");
			/* for pio only */
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_BUSY_IRQ;
		}
		/* ACK_IRQ case - acknowledge pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_ACK_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_ACK_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Acknowledge pin interrupt found.\n");
			/* for pio only */
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_ACK_IRQ;
		}
		/* DS_IRQ case - data strobe pin interrupt */
		if ((bpp_regs_p->int_cntl & BPP_DS_IRQ) &&
		    (bpp_regs_p->int_cntl & BPP_DS_IRQ_EN)) {
			bpp_print(LOG_INFO,
			"Data strobe pin interrupt found.\n");
			/*  for pio only */
			/* clear interrupting condition */
			bpp_regs_p->int_cntl |= BPP_DS_IRQ;
		}
	}		/* end of INT_PEND check */

	bpp_print(LOG_INFO, "dma csr contains 0x%x.\n", bpp_regs_p->dma_csr);
	bpp_print(LOG_INFO, "int cntl contains 0x%x.\n", bpp_regs_p->int_cntl);

	/* Clear the transfer timeout */
	bpp_print(LOG_INFO, "In bpp_intr, Clearing transfer timeout.\n");
	untimeout(bpp_transfer_timeout, (caddr_t)unit_no);
	bpp_p->timeouts &= ~TRANSFER_TIMEOUT;
	bpp_print(LOG_INFO, "In bpp_intr, Timeout block is 0x%x.\n",
		bpp_p->timeouts);
	/*
	 * Release the dvma bus resource.
	 */
#ifdef	DEBUG
	bpp_print(LOG_INFO,
		"In bpp_intr, *(&bp->b_mbinfo) = %x\n", *(&bp->b_mbinfo));
#endif	DEBUG
	(void) mb_mapfree(dvmamap, &bp->b_mbinfo);

	bpp_print(LOG_DEBUG, "bpp_intr, unit %d, Calling iodone.\n", unit_no);
	/*
	 * Mark the io on the buf as finished, with the side effect
	 * of waking up others who want to use the buf.
	 */
	(void) iodone(bp);

	bpp_print(LOG_INFO, "Leaving bpp_intr.\n");
	return;
}

/*
 * A transfer has failed for some reason.
 * Mark the bp struct to indicate how much happened,
 * and turn off the transfer and its interrupts.
 * NOTE: if this routine is called from the top half,
 * it should be bracketed by start_critical and end_critical.
 */
static  int
bpp_transfer_failed(unit_no)
u_int	unit_no;		/* unit which had transfer failure */
{
	struct buf		*bp;
	register struct bpp_regs	*bpp_regs_p;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	bpp_p = bpp_units[unit_no];
	bp = (struct buf *)(&bpp_p->buf),
	bpp_regs_p = bpp_p->bpp_regs_p;

	bpp_print(LOG_INFO, "Entering bpp_transfer_failed.\n");
	bpp_print(LOG_WARNING, "ERROR: bpp%d transfer failed!\n", unit_no);

	/*
	 * The transfer has failed. Notify the application
	 * how many bytes got out, and turn off the transfer.
	 * NOTE: don't set B_ERROR in b_flags else the return from
	 * write() will be -1. See syscall().
	 * NOTE: the kernel ignores the the b_error field
	 * in the short-write case - errno is always set to zero.
	 */
	/* Disable the DMA first for safety */
	bpp_regs_p->dma_csr &= ~BPP_ENABLE_DMA;

	bpp_print_regs(unit_no, "Just after disable DMA in transfer_failed");

	/* If the DMA state machines are not idle, reset them. */
	if (!(bpp_regs_p->op_config & BPP_IDLE)) {
		bpp_print(LOG_INFO, "Warning: DMA is not IDLE!\n");
		bpp_regs_p->dma_csr &= ~BPP_ENABLE_BCNT;
		bpp_print(LOG_INFO,
			"In bpp_strategy, resetting PP state machine\n");
		bpp_regs_p->op_config |= BPP_SRST;
		bpp_regs_p->op_config &= ~BPP_SRST;

		/*
		 * we have not received the acknowledge for the last
		 * byte transferred, so the byte counter was never
		 * decremented for it.
		 */
		bp->b_resid = ((bpp_regs_p->dma_bcnt - 1) +
			bpp_p->transfer_remainder);
	} else {
		bpp_regs_p->dma_csr &= ~BPP_ENABLE_BCNT;
		bp->b_resid = (bpp_regs_p->dma_bcnt +
			bpp_p->transfer_remainder);
	}
	/* flush the cache */
	bpp_regs_p->dma_csr |= BPP_FLUSH;

	bpp_print(LOG_INFO,
		"In bpp_transfer_failed, Residual is %d.\n", bp->b_resid);

	/* make sure the DMA doesn't start again. */
	bpp_regs_p->dma_bcnt = 0;
	/*
	 * Disable the TC interrupts.
	 * Mask the error interrupts too.
	 */
	bpp_regs_p->dma_csr |= BPP_TC_INTR_DISABLE;
	bpp_regs_p->int_cntl &=
		~(BPP_ERR_IRQ_EN | BPP_SLCT_IRQ_EN | BPP_PE_IRQ_EN);

	/* Check for any of the input pins active */
	/*
	 * Already inside a critical section here,
	 * thus this routine is not bracketed with
	 * start_critical and end_critical calls.
	 */
	check_for_active_pins(unit_no);
	bpp_print_regs(unit_no, "At end of bpp_transfer_failed");
	bpp_print(LOG_INFO, "Leaving bpp_transfer_failed.\n");
	return;
}


/*
 * This routine is called when the DVMA does not complete
 * and generate a TC interrupt.
 * I mark the bp struct to indicate that the transfer failed,
 * and turn off the transfer. I then call iodone to wake up strategy.
 */
static
bpp_transfer_timeout(unit_no)
u_int	unit_no;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register struct buf		*bp;
	register struct bpp_regs	*bpp_regs_p;

	bpp_print(LOG_INFO, "Entering bpp_transfer_timeout, unit #%d.\n",
			unit_no);

	bpp_p = bpp_units[unit_no];
	bpp_print(LOG_INFO, "In bpp_transfer_timeout, Timeout block is 0x%x.\n",
		bpp_p->timeouts);
	ASSERT(bpp_p->timeouts != 0);
	/*
	 * Use the unit number to locate our data structures.
	 */
	bp = (struct buf *)(&bpp_p->buf),
	bpp_regs_p = bpp_p->bpp_regs_p;

	/*
	 * If we're talking to the
	 * Ricoh scanner, handle it's special "hold busy"
	 * protocol -- Toggle the print/scan (PR/SC) bit and
	 * reset the parallel port
	 */
	bpp_print(LOG_INFO, "byte count is %d.\n", bpp_regs_p->dma_bcnt);
	bpp_print(LOG_INFO, "write_timeout is %d.\n",
			bpp_p->transfer_parms.write_timeout);
	if (bpp_regs_p->trans_cntl & BPP_DIRECTION) {
		/* read mode - partial reads will time out */
		bpp_print(LOG_DEBUG, "read timeout, clearing registers.\n");
		start_critical(unit_no);
		bp->b_resid = (bpp_regs_p->dma_bcnt +
			bpp_p->transfer_remainder);
		/* make sure the DMA doesn't start again. */
		bpp_regs_p->dma_bcnt = 0;
		/*
		 * Disable the byte counting, and the TC interrupts.
		 * Mask the error interrupts too.
		 */
		bpp_regs_p->dma_csr |= BPP_TC_INTR_DISABLE;
		bpp_regs_p->dma_csr &= ~BPP_ENABLE_BCNT;
		bpp_regs_p->dma_csr &= ~BPP_ENABLE_DMA;
		bpp_regs_p->int_cntl &= ~(BPP_ERR_IRQ_EN | BPP_SLCT_IRQ_EN
						| BPP_PE_IRQ_EN);
		end_critical(unit_no);
	} else if ((bpp_regs_p->trans_cntl & BPP_DIRECTION) == 0) {
		/* other write cases (read can time out w/no error) */
		start_critical(unit_no);
		bpp_transfer_failed(unit_no);
		/* Mark the error.  */
		/*
		 * bp->b_resid will be set to indicate the number
		 * of bytes actually transferred by bpp_transfer_failed().
		 * If no bytes were transferred, set B_ERROR
		 * so that -1 is returned, and set the b_error value.
		 */
		if (!(bp->b_resid)) {	/* not a partial transfer */
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		end_critical(unit_no);
	}

	/* mark the timeout as no longer pending */
	bpp_p->timeouts &= ~TRANSFER_TIMEOUT;
	/* mark the error status structure */
	bpp_p->error_stat.timeout_occurred = 1;
	bpp_print(LOG_INFO, "In bpp_transfer_timeout, Timeout blk is 0x%x.\n",
		bpp_p->timeouts);

	/*
	 * Release the dvma bus resource.
	 */
	start_critical(unit_no);
	(void) mb_mapfree(dvmamap, &bp->b_mbinfo);
	end_critical(unit_no);
#ifdef	DEBUG
	bpp_print(LOG_INFO,
		"In bpp_transfer_timeout, *(&bp->b_mbinfo) = %x\n",
					*(&bp->b_mbinfo));
#endif	DEBUG

	bpp_print(LOG_DEBUG, "bpp_transfer_timeout, unit %d, Calling iodone.\n",
				unit_no);
	(void) iodone(bp);
	bpp_print(LOG_INFO, "Leaving bpp_transfer_timeout, unit #%d.\n",
				unit_no);
	return;
}



/*<<<<<<<<<<<<<<<<<<<<<  PRIVATE SUPPORT ROUTINES  >>>>>>>>>>>>>>>>>>>>>*/

/*
 * Protect a critical section of code from interruption by the unit whose
 * data structures are currently being serviced. In BPP_DEBUG mode,
 * assertion checking is done to assure the proper pairing of
 * start_critical() and end_critical() calls.
 *
 * ARGS:		unit_no		The current unit number, an index
 *					into the bpp_units array.
 *
 * RETURN VALUE:			None.
 */
static void
start_critical(unit_no)
register u_int	unit_no;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register int	temp_saved_spl;

	bpp_print(LOG_INFO, "Entering start_critical: unit %d\n", unit_no);

	bpp_p = bpp_units[unit_no];
	temp_saved_spl = splr((int)(bpp_p->interrupt_pri));
	bpp_print(LOG_DEBUG, "in start_critical, tmp_svd_spl = %x\n",
				temp_saved_spl);
	bpp_print(LOG_DEBUG, "in start_critical, svd_spl = %x\n",
				bpp_p->saved_spl);
#ifdef	BPP_DEBUG
	/*
	 * Do assertion checking. This is a critical section in itself.
	 * If saved_spl is something other than -1, you are calling
	 * start_critical for the second time in a row.
	 * No nested critical sections are allowed!
	 */
	if (bpp_p->saved_spl != (-1))
	{
		bpp_print(LOG_INFO,
		    "start_critical: ASSERT failed, saved_spl: %d\n",
				bpp_p->saved_spl);
	}
#endif	BPP_DEBUG

	bpp_p->saved_spl = temp_saved_spl;

	bpp_print(LOG_INFO, "Leaving start_critical: unit %d\n",
								unit_no);
	return;
}

/*
 * End the protection of code from interruption by the unit whose
 * data structures are currently being serviced. In BPP_DEBUG mode,
 * assertion checking is done to assure the proper pairing of
 * start_critical() and end_critical() calls.
 *
 * ARGS:		unit_no		The current unit number, an index
 *					into the bpp_units array.
 *
 * RETURN VALUE:			None.
 */
static void
end_critical(unit_no)
register u_int	unit_no;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register int	temp_saved_spl;

	bpp_print(LOG_INFO, "Entering end_critical: unit %d\n", unit_no);
	bpp_p = bpp_units[unit_no];
	temp_saved_spl = bpp_p->saved_spl;
	bpp_print(LOG_DEBUG, "in end_critical, tmp_svd_spl = %x\n",
				temp_saved_spl);
	bpp_print(LOG_DEBUG, "in end_critical, svd_spl = %x\n",
				bpp_p->saved_spl);

#ifdef	BPP_DEBUG
	/*
	 * Do assertion checking.
	 * If saved_spl is -1, you called end_critical without
	 * calling start_critical first.
	 */
	if (bpp_p->saved_spl == (-1))
	{
		bpp_print(LOG_INFO,
		    "end_critical: ASSERT failed, saved_spl: %d\n",
				bpp_p->saved_spl);
	}

	bpp_p->saved_spl = (-1);
#endif	BPP_DEBUG

	/*
	 * Release the critical section last.
	 * The above is a critical section in itself.
	 */
	(void) splx(temp_saved_spl);

	bpp_print(LOG_INFO, "Leaving end_critical: unit %d\n",
								unit_no);
	return;
}

/*	Utility Functions				*/

/*
 *	see_devinfo:
 * Show interesting information stored in this node of the devinfo tree
 * (for development and debugging).
 */
static void
see_devinfo(devinfo_p)
register struct dev_info *devinfo_p;
{
	struct dev_reg	*dev_reg_p;
	struct dev_intr	*dev_intr_p;
	u_int		counter;

	bpp_print(LOG_INFO, "Entering see_devinfo: devinfo_p 0x%x\n",
								devinfo_p);
	bpp_print(LOG_DEBUG, "devinfo_p->devi_name:    %s\n",
						devinfo_p->devi_name);
	bpp_print(LOG_DEBUG, "devinfo_p->devi_unit:    %d\n",
						devinfo_p->devi_unit);

	bpp_print(LOG_DEBUG, "devinfo_p->devi_nreg:    %d\n",
						devinfo_p->devi_nreg);
	bpp_print(LOG_DEBUG, "devinfo_p->devi_reg:     0x%x\n",
						devinfo_p->devi_reg);
	/*
	 * Show all the device registers.
	 */
	dev_reg_p = devinfo_p->devi_reg;
	/*
	 * dev_reg_p may be NULL, but only if devi_nreg is zero.
	 */
	for (counter = 1; counter <= devinfo_p->devi_nreg;
						++counter, ++dev_reg_p)
	{
		if (dev_reg_p == NULL)
		{
			break;
		}

		bpp_print(LOG_DEBUG,
			"REGISTER #%d: devi_reg->reg_bustype: 0x%x\n",
					counter, dev_reg_p->reg_bustype);
		bpp_print(LOG_DEBUG,
			"REGISTER #%d: devi_reg->reg_addr:    0x%x\n",
					counter, dev_reg_p->reg_addr);
		bpp_print(LOG_DEBUG,
			"REGISTER #%d: devi_reg->reg_size:    0x%x\n",
					counter, dev_reg_p->reg_size);
	}

	bpp_print(LOG_DEBUG, "devinfo_p->devi_nintr:   %d\n",
						devinfo_p->devi_nintr);
	bpp_print(LOG_DEBUG, "devinfo_p->devi_intr:    0x%x\n",
						devinfo_p->devi_intr);

	/*
	 * Show all the interrupt data.
	 */
	dev_intr_p = devinfo_p->devi_intr;
	/*
	 * dev_intr_p may be NULL, but only if devi_nintr is zero.
	 */
	for (counter = 1; counter <= devinfo_p->devi_nintr;
						++counter, ++dev_intr_p)
	{
		if (dev_intr_p == NULL)
		{
			break;
		}

		bpp_print(LOG_DEBUG,
			"INTERRUPT #%d: devi_intr->int_pri:    0x%x\n",
					counter, dev_intr_p->int_pri);
		bpp_print(LOG_DEBUG,
			"INTERRUPT #%d: devi_intr->int_vec:    0x%x\n",
					counter, dev_intr_p->int_vec);
	}

	bpp_print(LOG_DEBUG, "devinfo_p->devi_data:    0x%x\n",
						devinfo_p->devi_data);
	bpp_print(LOG_DEBUG, "devinfo_p->devi_nodeid:  0x%x\n",
						devinfo_p->devi_nodeid);

	bpp_print(LOG_INFO, "Leaving see_devinfo: devinfo_p 0x%x\n",
								devinfo_p);
	return;
}

/*
 * Get a property from the fcode prom on the SBus card, or if the property
 * is not available, substitute the assigned default. Only string valued
 * properties are handled here. You have to know the name of a property
 * that you want to get. There is no way to get all defined properties.
 *
 * ARGS:	propname	A pointer to string which is the assigned
 *				name of the desired property.
 *
 *		defaultvalue	A pointer to string which is the default
 *				value to use if the named property is not
 * 				defined in the devinfo structure.
 *
 * RETURN VALUE:		A pointer to the value string selected.
 *				This pointer may have any value that was
 *				passed in for the default, including
 * 				a NULL string or a NULL pointer.
 */
static char *
get_property(devinfo_p, propname, defaultvalue)
struct dev_info	*devinfo_p;
char		*propname;
char		*defaultvalue;
{
	char	*ret_getlongprop;
	char	*propval;
	char	*msg_string;

	bpp_print(LOG_INFO, "Entering get_property: devinfo_p: 0x%x\n",
								devinfo_p);

	ret_getlongprop = (char *) getlongprop(devinfo_p->devi_nodeid,
								propname);
	if (ret_getlongprop == NULL)
	{
		propval = defaultvalue;
		msg_string = "default";
	} else {
		propval = ret_getlongprop;
		msg_string = "from prom";
	}

	bpp_print(LOG_INFO, "Leaving get_property: %s=%s (%s)\n",
					propname, propval, msg_string);
}

/*
 * Get a numeric property from the fcode prom on the SBus card,
 * or if the property
 * is not available, substitute the assigned default. Only numeric valued
 * properties are handled here. You have to know the name of a property
 * that you want to get. There is no way to get all defined properties.
 *
 * ARGS:	propname	A pointer to string which is the assigned
 *				name of the desired property.
 *
 *		defaultvalue	A pointer to string which is the default
 *				value to use if the named property is not
 * 				defined in the devinfo structure.
 *
 * RETURN VALUE:		The numeric property from the prom,
 *				or the default value.
 */
static int
get_num_property(devinfo_p, propname, defaultvalue)
struct dev_info	*devinfo_p;
char		*propname;
int		defaultvalue;
{
	addr_t	ret_getprop;
	int	propval;
	char	*msg_string;

	bpp_print(LOG_INFO, "Entering get_num_property: devinfo_p: 0x%x\n",
								devinfo_p);

	ret_getprop = (addr_t)getprop(devinfo_p->devi_nodeid,
					propname, defaultvalue);
	propval = (int)ret_getprop;
	if (propval == defaultvalue)
	{
		msg_string = "default";
	} else {
		msg_string = "from prom";
	}

	bpp_print(LOG_INFO, "Leaving get_num_property: %s=%d (%s)\n",
					propname, propval, msg_string);
	return (propval);
}

/*
 * The values of read_setup_time and read_strobe_width
 * have already been bounds-checked. Convert the requested times
 * (in nanoseconds) to SBus clock cycles for the dss and dsw registers.
 * Always round the requested setup time up to the next clock
 * cycle boundary.
 */
static	void
set_dss_dsw(unit_no, read_mode)
u_char	unit_no;
int	read_mode;		/* 1 if called from read */
{
	int	dss_temp;		/* tentative dss value */
	int	dsw_temp;		/* tentative dsw value */
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	register struct	bpp_transfer_parms	*bpp_transfer_parms_p;

	bpp_print(LOG_INFO,
		"Entering set_dss_dsw, unit:%d, read_mode = %x.\n",
			unit_no, read_mode);

	bpp_p = bpp_units[unit_no];
	bpp_transfer_parms_p = &bpp_p->transfer_parms;

	if (read_mode) {
		dss_temp =
		    bpp_transfer_parms_p->read_setup_time / sbus_clock_cycle;
		if (bpp_transfer_parms_p->read_setup_time % sbus_clock_cycle)
			dss_temp ++;	/* round up */
		dsw_temp =
		    bpp_transfer_parms_p->read_strobe_width / sbus_clock_cycle;
		if (bpp_transfer_parms_p->read_strobe_width % sbus_clock_cycle)
			dsw_temp ++;	/* round up */
	} else {
		dss_temp =
		    bpp_transfer_parms_p->write_setup_time / sbus_clock_cycle;
		if (bpp_transfer_parms_p->write_setup_time % sbus_clock_cycle)
			dss_temp ++;	/* round up */
		dsw_temp =
		    bpp_transfer_parms_p->write_strobe_width / sbus_clock_cycle;
		if (bpp_transfer_parms_p->write_strobe_width % sbus_clock_cycle)
			dsw_temp ++;	/* round up */
	}

	bpp_print(LOG_INFO, "dss = 0x%x, dsw = 0x%x\n", dss_temp, dsw_temp);
	start_critical(unit_no);
	bpp_p->bpp_regs_p->hw_config =
			((((u_char)dsw_temp) << 8) | ((u_char)dss_temp));
	end_critical(unit_no);
	bpp_print(LOG_INFO,
		"Leaving set_dss_dsw, unit:%d.\n", unit_no);
}
/*
 * Check the values of the write parameters in the passed bpp_transfer_parms
 * structure. If all the parameters are in range, 0 is returned.
 * If there is an out-of-range parameter, EINVAL is returned.
 */
/*ARGSUSED*/
static	u_short
check_write_params(parms_p, flags)
struct	bpp_transfer_parms	*parms_p;
int	flags;		/* this unit's (read-write) open flags */
{
	u_short	retval = 0;	/* return value (errno) for system call */
	static int max_setup = 0;	/* maximum setup time allowed (ns) */
	static int max_width = 0;	/* maximum width time allowed (ns) */

	retval = 0;
	bpp_print(LOG_INFO,
		"Entering check_write_params, parms_p = %x.\n", parms_p);
	bpp_print(LOG_DEBUG, "write_hs %d, write_time %d, ",
			parms_p->write_handshake, parms_p->write_setup_time);
	bpp_print(LOG_DEBUG, "write_width %d, timeout %d.\n",
			parms_p->write_strobe_width, parms_p->write_timeout);
#ifndef	lint
	/* better rangechecking will be added later */
	/* check for legal range */
	if ((parms_p->write_handshake < BPP_NO_HS) ||
	    (parms_p->write_handshake > BPP_VPLOT_HS)) {
		bpp_print(LOG_WARNING, "Handshake out of legal range!\n");
		retval = EINVAL;
		goto out;
	}
	/* the handshake values overlap. Check for read handshakes */
	if ((parms_p->write_handshake > BPP_BUSY_HS) &&
	    (parms_p->write_handshake < BPP_VPRINT_HS)) {
		bpp_print(LOG_WARNING, "Handshake out of legal write range!\n");
		retval = EINVAL;
		goto out;
	}
	/* versatec handshakes illegal in read-write mode */
	if  ((flags & FREAD) && (parms_p->write_handshake > BPP_BUSY_HS)) {
		bpp_print(LOG_WARNING, "No versatec handshakes in read md!\n");
		retval = EINVAL;
		goto out;
	}
	/*
	 * Originally there was a plan to support a versatec mode.
	 * The decision was made not to support it in software.
	 * However, the hooks are still there in the hardware.
	 * I leave the versatec fragments in case the decision is ever
	 * reversed.
	 */
	/* versatec handshakes not implemented in current code */
	if  ((parms_p->write_handshake > BPP_BUSY_HS)) {
		bpp_print(LOG_WARNING, "No versatec handshakes allowed yet!\n");
		retval = EINVAL;
		goto out;
	}
#endif	lint
	/* check range of setup time and strobe width here */
	max_setup = BPP_DSS_SIZE * sbus_clock_cycle;
	max_width = BPP_DSW_SIZE * sbus_clock_cycle;

	if ((parms_p->write_setup_time < 0) ||
	    (parms_p->write_setup_time > max_setup)) {
		bpp_print(LOG_WARNING,
			"Write setup time out of legal range!\n");
		retval = EINVAL;
		goto out;
	}
	if ((parms_p->write_strobe_width < 0) ||
	    (parms_p->write_strobe_width > max_width)) {
		bpp_print(LOG_WARNING,
			"Write strobe width out of legal range!\n");
		retval = EINVAL;
		goto out;
	}

	/* check range of write timeout */
	if ((parms_p->write_timeout < 0) ||
	    (parms_p->write_timeout > MAX_TIMEOUT)) {
		bpp_print(LOG_WARNING,
			"Write timeout out of legal range!\n");
		retval = EINVAL;
		goto out;
	}

out:
	bpp_print(LOG_INFO,
		"Leaving check_write_params, retval = %d.\n", retval);
	return (retval);
}

/*
 * Check the values of the read parameters in the passed bpp_transfer_parms
 * structure. If all the parameters are in range, 0 is returned.
 * If there is an out-of-range parameter, EINVAL is returned.
 */
/*ARGSUSED*/
static	u_short
check_read_params(parms_p, flags)
struct	bpp_transfer_parms	*parms_p;
int	flags;		/* this unit's (read-write) open flags */
{
	u_short	retval = 0;	/* return value (errno) for system call */
	static int max_setup = 0;	/* maximum setup time allowed (ns) */
	static int max_width = 0;	/* maximum width time allowed (ns) */

	retval = 0;
	bpp_print(LOG_INFO,
		"Entering check_read_params, parms_p = %x.\n", parms_p);
	bpp_print(LOG_DEBUG,
		"read_hs %d, read_time %d, read_width %d, timeout %d.\n",
		parms_p->read_handshake, parms_p->read_setup_time,
		parms_p->read_strobe_width, parms_p->read_timeout);
#ifndef	lint
	/* check for legal range */
	if ((parms_p->read_handshake < BPP_NO_HS) ||
	    (parms_p->read_handshake > BPP_SET_MEM)) {
		bpp_print(LOG_WARNING, "Handshake out of legal range!\n");
		retval = EINVAL;
		goto out;
	}
#endif	lint
	/* check range of setup time and strobe width here */
	max_setup = BPP_DSS_SIZE * sbus_clock_cycle;
	max_width = BPP_DSW_SIZE * sbus_clock_cycle;

	if ((parms_p->read_setup_time < 0) ||
	    (parms_p->read_setup_time > max_setup)) {
		bpp_print(LOG_WARNING,
			"Read setup time out of legal range!\n");
		retval = EINVAL;
		goto out;
	}
	if ((parms_p->read_strobe_width < 0) ||
	    (parms_p->read_strobe_width > max_width)) {
		bpp_print(LOG_WARNING,
			"Read strobe width out of legal range!\n");
		retval = EINVAL;
		goto out;
	}

	/* check range of read timeout */
	if ((parms_p->read_timeout < 0) ||
	    (parms_p->read_timeout > MAX_TIMEOUT)) {
		bpp_print(LOG_WARNING,
			"Read timeout out of legal range!\n");
		retval = EINVAL;
		goto out;
	}

out:
	bpp_print(LOG_INFO,
		"Leaving check_read_params, retval = %d.\n", retval);
	return (retval);
}

/*ARGSUSED*/
static	u_short
check_read_pins(pins_p, flags, handshake)
struct	bpp_pins	*pins_p;
int	flags;		/* this unit's (read-write) open flags */
	/* this unit's read handshake	*/
register enum	handshake_t	handshake;
{
	u_short	retval = 0;	/* return value (errno) for system call */
	bpp_print(LOG_INFO,
		"Entering check_read_pins, pins_p = 0x%x ", pins_p);
	bpp_print(LOG_DEBUG,
		"outpins = 0x%x, inpins = 0x%x.\n", pins_p->output_reg_pins,
		pins_p->input_reg_pins);
	/* check for bogus bits turned on */
	if ((pins_p->output_reg_pins & ~BPP_ALL_OUT_PINS) ||
	    (pins_p->input_reg_pins  & ~BPP_ALL_IN_PINS)) {
		bpp_print(LOG_WARNING,
			"Check pins : Bogus bit in bpp pins structure!\n");
		retval = EINVAL;
		goto out;
	}
out:
	bpp_print(LOG_INFO,
		"Leaving check_read_pins, retval = %d.\n", retval);
	return (retval);
}

/*ARGSUSED*/
static	u_short
check_write_pins(pins_p, flags, handshake)
	struct	bpp_pins	*pins_p;
	int	flags;		/* this unit's (read-write) open flags */
	/* this unit's write handshake	*/
	register enum	handshake_t	handshake;
{
	u_short	retval = 0;	/* return value (errno) for system call */
	bpp_print(LOG_INFO,
		"Entering check_write_pins, pins_p = 0x%x, ", pins_p);
	bpp_print(LOG_DEBUG, "outpins = 0x%x, inpins = 0x%x.\n",
		pins_p->output_reg_pins, pins_p->input_reg_pins);
	/* check for bogus bits turned on */
	if ((pins_p->output_reg_pins & ~BPP_ALL_OUT_PINS) ||
	    (pins_p->input_reg_pins  & ~BPP_ALL_IN_PINS)) {
		bpp_print(LOG_WARNING,
			"Check pins : Bogus bit in bpp pins structure!\n");
		retval = EINVAL;
		goto out;
	}
	/*
	 * Originally there was a plan to support a versatec mode.
	 * The decision was made not to support it in software.
	 * However, the hooks are still there in the hardware.
	 * I leave the versatec fragments in case the decision is ever
	 * reversed.
	 */
#ifndef	lint
	/* versatec handshakes not implemented in current code */
	if  ((handshake > BPP_BUSY_HS)) {
		bpp_print(LOG_WARNING, "No versatec handshakes allowed yet!\n");
		/*
		 * really, need to check for one bit only of remote
		 * pins set.
		 */
		retval = EINVAL;
		goto out;
	}
#endif	lint
out:
	bpp_print(LOG_INFO,
		"Leaving check_write_pins, retval = %d.\n", retval);
	return (retval);
}

static	void
read_outpins(unit_no, flags, handshake)
u_char	unit_no;
int	flags;		/* this unit's (read-write) open flags */
	/* this unit's write handshake	*/
register enum	handshake_t	handshake;
{
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */
	u_char	temppins;
	bpp_print(LOG_INFO,
		"Entering read_outpins, unit = %d, flags = 0x%x, ",
					unit_no, flags);
	bpp_p = bpp_units[unit_no];

	bpp_print(LOG_DEBUG, "handshake = %d.\n", handshake);
	if  (flags & FWRITE) {

#ifndef	lint
		if  ((handshake > BPP_BUSY_HS)) {
			bpp_print(LOG_WARNING,
				"No versatec handshakes allowed yet!\n");
		}
#endif	lint
		temppins = bpp_p->bpp_regs_p->out_pins &
			(BPP_SLCTIN_PIN | BPP_AFX_PIN | BPP_INIT_PIN);
		bpp_p->pins.output_reg_pins |= temppins;
	}

	bpp_print(LOG_INFO, "Leaving read_outpins.\n");
}
/*
 * bpp_stuff_fregs()
 *
 * This routine will simulate the device registers for the bpp DMA
 * registers of the SBus-HIOD
 *
 */

/*ARGSUSED*/
static	void
bpp_stuff_fregs(unit_no)
u_int		unit_no;	/* attaching unit's number */
{

#ifdef	NO_BPP_HW
	/*
	 * set up default registers to the expected (inital) values
	 */

	bpp_def_regs.dma_csr = 0x40000100;	/* device id = 0100 */
	bpp_def_regs.dma_addr = 0;
	bpp_def_regs.dma_bcnt = 0;
	bpp_def_regs.unused = 0;
	bpp_def_regs.hw_config = 0;
	bpp_def_regs.op_config = BPP_DS_BIDIR;
	bpp_def_regs.data = 0;
	bpp_def_regs.trans_cntl = BPP_DIRECTION | 0x40;
	bpp_def_regs.out_pins = 0;
	bpp_def_regs.in_pins = 0;
	bpp_def_regs.int_cntl = 0;

	/*
	 * now copy default registers into mimic struct
	 */

	bpp_fake_regs[unit_no] = bpp_def_regs;
#endif	NO_BPP_HW
}

/*
 * Peek at the bpp registers to make sure that they really
 * exist. Also check initial conditions. If any of this
 * fails, return a non-zero value.
 */
static	int
check_bpp_registers(unit_no)
u_int		unit_no;	/* attaching unit's number */
{
	u_long		*l_reg_addr;	/* address of a 32-bit register */
	u_long		l_reg_contents;	/* contents of a 32-bit register */
	u_short		*s_reg_addr;	/* address of a 16-bit register */
	u_short		s_reg_contents;	/* contents of a 16-bit register */
	u_char		*c_reg_addr;	/* address of a 8-bit register */
	u_short		c_reg_contents;	/* contents of a 8-bit register */
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	bpp_print(LOG_INFO, "Entering check_bpp_registers, unit %d.\n",
				unit_no);
	bpp_p = bpp_units[unit_no];
/* check the long dma registers */
	/* dma csr */
	l_reg_addr = &(bpp_p->bpp_regs_p->dma_csr);
	if (peekl((long *)l_reg_addr, (long *)&l_reg_contents) == -1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed\n",
			"dma csr, address %x, contents %x\n",
			l_reg_addr, l_reg_contents);
		return (1);
	}


	else
		bpp_print(LOG_INFO, "dma_csr contains %x\n", l_reg_contents);

	/* dma addr */
	l_reg_addr = &(bpp_p->bpp_regs_p->dma_addr);
	if (peekl((long *)l_reg_addr, (long *)&l_reg_contents) == -1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed dma addr, address %x\n",
		l_reg_addr, l_reg_contents);
		return (1);
	} else
		bpp_print(LOG_INFO, "dma_addr contains %x\n", l_reg_contents);

	/* dma bcnt */
	l_reg_addr = &(bpp_p->bpp_regs_p->dma_bcnt);
	if (peekl((long *)l_reg_addr, (long *)&l_reg_contents) == -1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed dma bcnt, address %x\n",
			l_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "dma_bcnt contains %x\n", l_reg_contents);

/* short hardware registers */
	/* hw_config */
	s_reg_addr = &(bpp_p->bpp_regs_p->hw_config);
	if ((s_reg_contents = peek((short *)s_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed hw_config, address %x\n",
			s_reg_addr);
		return (1);
	}
	if (poke((short *)s_reg_addr, (short)s_reg_contents) != 0) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: poke failed hw_config, address %x\n",
			s_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "hw_config contains %x\n", s_reg_contents);

	/* op_config */
	s_reg_addr = &(bpp_p->bpp_regs_p->op_config);
	if ((s_reg_contents = peek((short *)s_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed op_config, address %x\n",
			s_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "op_config contains %x\n", s_reg_contents);

	/* int_cntl */
	s_reg_addr = &(bpp_p->bpp_regs_p->int_cntl);
	if ((s_reg_contents = peek((short *)s_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed int_cntl, address %x\n",
			s_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "int_cntl contains %x\n", s_reg_contents);

/* char hardware registers */
	/* data */
	c_reg_addr = &(bpp_p->bpp_regs_p->data);
	if ((c_reg_contents = peekc((char *)c_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed data, address %x\n",
			c_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "data contains %x\n", c_reg_contents);

	/* trans_cntl */
	c_reg_addr = &(bpp_p->bpp_regs_p->trans_cntl);
	if ((c_reg_contents = peekc((char *)c_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers:peek failed trans_cntl, address %x\n",
			c_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "trans_cntl contains %x\n", c_reg_contents);

	/* out_pins */
	c_reg_addr = &(bpp_p->bpp_regs_p->out_pins);
	if ((c_reg_contents = peekc((char *)c_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed out_pins, address %x\n",
			c_reg_addr);
		return (1);
	} else
		bpp_print(LOG_INFO, "out_pins contains %x\n", c_reg_contents);

	/* in_pins */
	c_reg_addr = &(bpp_p->bpp_regs_p->in_pins);
	if ((c_reg_contents = peekc((char *)c_reg_addr)) == (u_short)-1) {
		bpp_print(LOG_WARNING,
			"ck_bpp_registers: peek failed in_pins, address %x\n",
			c_reg_addr);
		return (1);
	} else
		bpp_print(LOG_DEBUG, "in_pins contains %x\n", c_reg_contents);
	bpp_print(LOG_INFO, "Leaving check_bpp_registers, unit %d.\n", unit_no);
	return (0);
}

static	void
bpp_print_regs (unit_no, msg)
u_int	unit_no;
char	*msg;
{
	register struct bpp_regs	*bpp_regs_p;
	register struct	bpp_unit	*bpp_p;	/* will point to this */
						/* unit's state struct */

	bpp_p = bpp_units[unit_no];
	bpp_regs_p = bpp_p->bpp_regs_p;
	bpp_print(LOG_INFO, "%s, dma_csr = 0x%x, dma_addr = 0x%x.\n",
		msg, bpp_regs_p->dma_csr, bpp_regs_p->dma_addr);
	bpp_print(LOG_INFO, "dma_bcnt = %d, hw_config = 0x%x.\n",
		bpp_regs_p->dma_bcnt, bpp_regs_p->hw_config);
	bpp_print(LOG_INFO, "op_config = 0x%x, data = 0x%x.\n",
		bpp_regs_p->op_config, bpp_regs_p->data);
	bpp_print(LOG_INFO, "trans_cntl = 0x%x, out_pins = 0x%x.\n",
		bpp_regs_p->trans_cntl, bpp_regs_p->out_pins);
	bpp_print(LOG_INFO, "in_pins = 0x%x, int_cntl = 0x%x.\n",
		bpp_regs_p->in_pins, bpp_regs_p->int_cntl);
}


/* VARARGS */
static	void
bpp_print(level, fmt, a, b, c, d, e, f, g, h)
	int level;
	char *fmt;
	int a, b, c, d, e, f, g, h;
{
	if (level <= bpp_printlevel) {
		log(level, fmt, a, b, c, d, e, f, g, h);
	}
}


#ifdef	NO_BPP_HW
#include "bpp_test_code.c"
#endif	NO_BPP_HW
