#ident "@(#) dbri_driver.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

/*
 * AUDIO/ISDN Chip driver - for ATT T5900FC (DBRI) and MMCODEC
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <machine/intreg.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <sundev/mbvar.h>

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>
#if !defined(STANDALONE)
#include <sbusdev/aclone.h>
#endif
#include <sun/isdnio.h>
#include <sun/dbriio.h>
#include <sbusdev/dbrivar.h>

#include <sys/callout.h>	/* XXX - to debug timers not being cancelled */
#include "audiodebug.h"


/* Local declarations */
dbri_dev_t	*Dbri_devices = NULL;	/* device ctrlr array */
int		Ndbri = 0;		/* number of devices found up to now */
int		Curdbri = 0;		/* current dbri attach unit */

int	dbri_identify();
int	dbri_attach();
addr_t	map_regs();
void	report_dev();


/* Declare audio ops vector for DBRI support routines */
struct aud_ops	Dbri_audops = {
	dbri_close,
	dbri_ioctl,
	dbri_start,
	dbri_stop,
	dbri_setflag,
	dbri_setinfo,
	dbri_queuecmd,
	dbri_flushcmd,
};


/*
 * Declare ops vector for auto configuration.
 * It must be named dbri_ops, since the name is constructed from the
 * device name when config writes ioconf.c.
 */
struct dev_ops	dbri_ops = {
	1,			/* revision */
	dbri_identify,		/* identify routine */
	dbri_attach,		/* attach routine */
};


/*
 * Streams declarations
 */

static struct module_info dbri_modinfo = {
	0x6175,			/* module ID number */
	"dbri",			/* module name */
	0,			/* min packet size accepted */
	DBRI_MAXPACKET,		/* max packet size accepted */
	DBRI_HIWATER,		/* hi-water mark */
	DBRI_LOWATER,		/* lo-water mark */
};

/*
 * Queue information structure for read queue
 */
static struct qinit dbri_rinit = {
	(int(*)())audio_rput,	/* put procedure */
	(int(*)())audio_rsrv,	/* service procedure */
	dbri_open,		/* called on startup */
	(int(*)())audio_close,	/* called on finish */
	NULL,			/* for 3bnet only */
	&dbri_modinfo,		/* module information structure */
	NULL,			/* module statistics structure */
};

/*
 * Queue information structure for write queue
 */
static struct qinit dbri_winit = {
	(int(*)())audio_wput,	/* put procedure */
	(int(*)())audio_wsrv,	/* service procedure */
	NULL,			/* called on startup */
	NULL,			/* called on finish */
	NULL,			/* for 3bnet only */
	&dbri_modinfo,		/* module information structure */
	NULL,			/* module statistics structure */
};

char *dbri_modules[] = {
	0,
};

struct streamtab dbri_info = {
	&dbri_rinit,		/* qinit for read side */
	&dbri_winit,		/* qinit for write side */
	NULL,			/* mux qinit for read */
	NULL,			/* mux qinit for write */
	dbri_modules,		/* list of modules to be pushed */
};


/*
 * Called by match_driver() and add_drv_layer() in autoconf.c.  Returns
 * TRUE if the given string refers to this driver, else FALSE.
 */
static int
dbri_identify(name)
	char	*name;
{
	if (strncmp(name, "SUNW,DBRI", 9) != 0)
		return (FALSE);

	switch (name[9]) {
	case 'a':
	case 'b':
	case 'c':
		(void) printf("DBRI: Driver does not support DBRI version %c\n",
		    name[9]);
		return (FALSE);

	case 'd':
		(void) printf("DBRI: driver will work but upgrade your hardware\n");
		break;		/* okay, found one */

	case 'e':
	case 'f':
		break;

	default:
		(void) printf("DBRI: Warning Unknown DBRI version '%c'.\n",
			name[9]);
		break;
	}

	Ndbri++;
	return (TRUE);
} /* dbri_identify */


/*ARGSUSED*/
void
dbri_dont_cache(start, size)
	caddr_t	start;
	int	size;
{
#ifdef sun4m
#ifdef	SUN4M_35

	/* XXX - hack attack - Sunergy 4.1.3 support - never used */
	/* Round up size to full pages */
	size = mmu_ptob(mmu_btopr(size));

	/* Round start down to nearest page start */
	start = (caddr_t)((int)start & PAGEMASK);

	{
		caddr_t	end;

		for (end = start+size; start <= end; start += MMU_PAGESIZE) {
			(void) hat_dontcache((addr_t)start);
		}
	}
#endif	SUN4M_35
#else sun4m

	/* Round up size to full pages */
	size = mmu_ptob(mmu_btopr(size));

	/* Round start down to nearest page start */
	start = (caddr_t)((int)start & PAGEMASK);

	{
		caddr_t	end;

		for (end = start+size; start <= end; start += MMU_PAGESIZE) {
			(void) vac_dontcache((addr_t)start);
		}
	}
#endif /* sun4m */

	return;
}


char *
dbri_mode_name(mode)
	dbri_mode_t	mode;
{
	int	i;
	static struct {
		char		*name;
		dbri_mode_t	value;
	} mode_names[] = {
		{"DBRI_MODE_UNKNOWN",	DBRI_MODE_UNKNOWN},
		{"DBRI_MODE_XPRNT",	DBRI_MODE_XPRNT},
		{"DBRI_MODE_HDLC",	DBRI_MODE_HDLC},
		{"DBRI_MODE_HDLC_D",	DBRI_MODE_HDLC_D},
		{"DBRI_MODE_SER",	DBRI_MODE_SER},
		{"DBRI_MODE_FIXED",	DBRI_MODE_FIXED},
		{0, (dbri_mode_t)0}
	};
	static char	buf[64];

	for (i = 0; mode_names[i].name != NULL; ++i) {
		if (mode_names[i].value == mode)
			return (mode_names[i].name);
	}

	(void) sprintf(buf, "unknown mode(0x%x)", (unsigned int)mode);
	return (buf);
}


char *
dbri_channel_name(channel)
	int	channel;
{
	int	i;

	for (i = 0; i < (int)DBRI_LAST_CHNL; ++i) {
		if (Chan_tab[i].chan == (dbri_chnl_t)channel)
			return (Chan_tab[i].name);
	}

	if ((dbri_chnl_t)i == DBRI_NO_CHNL)
		return ("DBRI_NO_CHNL");
	if ((dbri_chnl_t)i == DBRI_HOST_CHNL)
		return ("DBRI_HOST_CHNL");

	return ("unknown channel");
}


static int
dbri_setup_mem_map(driver, size)
	dbri_dev_t	*driver;
	int		size;
{
	caddr_t		tvaddr;
	int		i, nb;

	dprintf("dbri_setup_mem_map: allocating %d bytes\n", size);

	size += DBRI_MD_ALIGN;
	driver->mem_base = new_kmem_zalloc((u_int) size, KMEM_SLEEP);
	if (driver->mem_base == NULL) {
		(void) printf("dbri_setup_mem_map: out of memory\n");
		return (0);
	}

	dprintf("dbri_setup_mem_map: mem base is 0x%x\n",
	    (unsigned int)driver->mem_base);

	driver->mem_size = size;
	dbri_dont_cache(driver->mem_base, driver->mem_size);

#ifdef sun4m
#ifndef lint	/* XXX lint barfs on arg 1 */
	driver->sb_addr = (caddr_t)mb_nbmapalloc(dvmamap,
			driver->mem_base, driver->mem_size,
			MDR_BIGSBUS|MB_CANTWAIT, (func_t)0, (caddr_t)0);
#endif /* lint */
	if (driver->sb_addr == (caddr_t)0) {
		(void) printf("dbri_setup_mem_map: out of dvma memory\n");
		kmem_free(driver->mem_base, (u_int) driver->mem_size);
		return (0);
	}
	dprintf("dbri_setup_mem_map: dvma base is 0x%x\n", driver->sb_addr);
	driver->addr_offset = (int)(driver->sb_addr - driver->mem_base);
#else /* sun4m */
	driver->addr_offset = 0;
#endif /* sun4m */
	dprintf("dbri_setup_mem_map: addr_offset is 0x%x\n",
		(unsigned int)driver->addr_offset);
	/*
	 * Map is as follows:
	 * First is the command queue, then the interrupt queues, then the
	 * command descriptors for each minor device
	 */
	tvaddr = driver->mem_base;
	driver->cmdqp = (dbri_chip_cmdq_t *)tvaddr;
	tvaddr += sizeof (dbri_chip_cmdq_t);
	driver->intq.intq_bp = (dbri_intq_t *)tvaddr;
	tvaddr += DBRI_NUM_INTQS * sizeof (dbri_intq_t);

	ASSERT ((sizeof(dbri_cmd_t) % DBRI_MD_ALIGN) == 0);
	dprintf("old tvaddr = 0x%x    ",tvaddr);
	tvaddr = (caddr_t)roundup((u_int)tvaddr, DBRI_MD_ALIGN);
	dprintf("new tvaddr = 0x%x  \n",tvaddr);
	dprintf("dbri_setup_mem_map: starting cmd's at 0x%x\n",
		(unsigned int)tvaddr);
	for (i = 0; i < Nstreams; ++i) {
		aud_stream_t		*asp;
		struct aud_cmdlist	*list;

		asp = &driver->ioc[i].as;
		nb = Chan_tab[i].numbufs;
		if (nb > 0) {
			dbri_cmd_t	*pool;
			int		j;

			j = nb * sizeof (dbri_cmd_t);
			pool = (dbri_cmd_t *)tvaddr;
			tvaddr += j;
			bzero((caddr_t)pool, (unsigned)j);

			list = &asp->cmdlist;
			list->memory = (caddr_t)pool;
			list->size = j;

			for (j = 0; j < nb; ++j, ++pool) {
				pool->cmd.next = list->free;
				list->free = &pool->cmd;
			}

		} else {
			struct aud_cmdlist	*bchan_list;
			aud_stream_t		*bchan_asp;
			int			redundant_channel = TRUE;

			/*
			 * kludge for 56kbps and transparent channels
			 * so that they point back to the already allocated
			 * cmdlists of the base B channels.
			 */
			switch (i) {
			case DBRI_TE_B1_56_OUT:
			case DBRI_TE_B1_TR_OUT:
				bchan_asp =
					&driver->ioc[(int)DBRI_TE_B1_OUT].as;
				break;

			case DBRI_TE_B2_56_OUT:
			case DBRI_TE_B2_TR_OUT:
				bchan_asp =
					&driver->ioc[(int)DBRI_TE_B2_OUT].as;
				break;

			case DBRI_TE_B1_56_IN:
			case DBRI_TE_B1_TR_IN:
				bchan_asp = &driver->ioc[(int)DBRI_TE_B1_IN].as;
				break;

			case DBRI_TE_B2_56_IN:
			case DBRI_TE_B2_TR_IN:
				bchan_asp = &driver->ioc[(int)DBRI_TE_B2_IN].as;
				break;

			case DBRI_NT_B1_56_OUT:
			case DBRI_NT_B1_TR_OUT:
				bchan_asp =
					&driver->ioc[(int)DBRI_NT_B1_OUT].as;
				break;

			case DBRI_NT_B2_56_OUT:
			case DBRI_NT_B2_TR_OUT:
				bchan_asp =
					&driver->ioc[(int)DBRI_NT_B2_OUT].as;
				break;

			case DBRI_NT_B1_56_IN:
			case DBRI_NT_B1_TR_IN:
				bchan_asp = &driver->ioc[(int)DBRI_NT_B1_IN].as;
				break;

			case DBRI_NT_B2_56_IN:
			case DBRI_NT_B2_TR_IN:
				bchan_asp = &driver->ioc[(int)DBRI_NT_B2_IN].as;
				break;

			default:
				/* Do nothing for all other channels */
				redundant_channel = FALSE;
				break;
			}

			/*
			 * If this is a transparent or 56kbps channel then
			 * point back to the command lists of the base
			 * B channels. Otherwise just zero out the cmdlist
			 * pointers.
			 */
			list = &asp->cmdlist;
			if (redundant_channel) {
				bchan_list = &bchan_asp->cmdlist;
				list->memory = bchan_list->memory;
				list->size = bchan_list->size;
				list->free = bchan_list->free;
				continue;
			} else {
				list->memory = NULL;
				list->size = 0;
			}
		}
	} /* for */

	dprintf("dbri_setup_mem_map: ending tvaddr is 0x%x\n",
		(unsigned int)tvaddr);

	return (1);
} /* dbri_setup_mem_map */


/*
 * Attach to the device.
 */
static int
dbri_attach(dev)
	struct dev_info		*dev;
{
	int			nb, total_nb;
	dbri_dev_t		*driver;
	int			i, tbytes;
	extern struct aud_ops	Dbri_audops;
	extern dbri_chan_tab_t	Chan_tab[];
	extern void		mmcodec_startup_audio();

	ATRACEINIT();
	ATRACEPRINT("dbri0: debugging version of audio driver\n");

	/*
	 * Each unit has a 'aud_state_t' that contains generic audio
	 * device state information.  Also, each unit has a
	 * 'dbri_dev_t' that contains device-specific data.
	 */
	ASSERT(dev != NULL);
	ASSERT(Ndbri > 0);

	/*
	 * Sanity check the configuration tables.
	 */
	for (i = 0; i < (int)DBRI_LAST_CHNL; ++i) {
		if (Chan_tab[i].chan != (dbri_chnl_t)i) {
			printf("dbri: %s: Chan_tab[%d].chan = %d\n",
				((Chan_tab[i].chan)
					? "Invalid entry"
					: "FYI"),
				i, Chan_tab[i].chan);
			if (Chan_tab[i].chan)
				return (-1);
		}
	}

	for (i = 0; i < (int)ISDN_LAST_PORT; ++i) {
		if (Port_tab[i].port != (isdn_port_t)i) {
			if ((isdn_port_t)i == ISDN_PRI_MGT)
				continue;
			printf("dbri: %s: Port_tab[%d].port = %d\n",
				((Port_tab[i].port == ISDN_PORT_NONE)
					? "Invalid entry"
					: "FYI"),
				i, Port_tab[i].port);
			if (Port_tab[i].port)
				return (-1);
		}
	}

#ifdef	sun4c
	if (slaveslot(dev->devi_reg->reg_addr) >= 0) {
		printf("%s in slave slot will not work.\n",
			dev->devi_name);
		return (-1);
	}
#endif	sun4c

	dev->devi_unit = Curdbri;
	dprintf("dbri_attach: attaching device %d\n", Curdbri);

	if (Dbri_devices == NULL) {
		Dbri_devices = ((dbri_dev_t *)
				new_kmem_zalloc((u_int)
						(Ndbri * sizeof (dbri_dev_t)),
						KMEM_SLEEP));
	}

	/*
	 * Identify the audio device and assign a unit number.
	 * Get the address of this unit's audio device state structure.
	 */
	driver = &Dbri_devices[Curdbri];
	ASSERT(driver != NULL);
	driver->hwi_state.devdata = (void *)driver;
	driver->hwi_state.monitor_gain = 0;
	driver->hwi_state.output_muted = FALSE;
	driver->dbri_dev = dev;
	driver->dbri_version = dev->devi_name[9];	/* XXX DIC */

#if !defined(STANDALONE)
	{
		int status;

		status = aclone_register((caddr_t)&dbri_info,
			1 + Curdbri, /* Base for dbri is 1 */
			(int)DBRI_MINOR_AUDIO_RW + Nstreams,
			(int)DBRI_MINOR_AUDIO_RO + Nstreams,
			(int)DBRI_MINOR_AUDIOCTL + Nstreams);
		if (status < 0)
			printf("dbri: Could not register with aclone!\n");
	}
#endif /* STANDALONE */

	total_nb = 0;
	/* Initialize the play info struct */
	for (i = 0; i < Nstreams; ++i) {
		aud_stream_t	*asp;

		asp = &driver->ioc[i].as;
		ASSERT(asp != NULL);
		asp->v = &driver->hwi_state;
		asp->info.minordev = (int)Chan_tab[i].minordev;

		asp->record_as = &driver->ioc[(int)Chan_tab[i].in_as].as;
		asp->play_as = &driver->ioc[(int)Chan_tab[i].out_as].as;
		asp->control_as = &driver->ioc[(int)Chan_tab[i].control_as].as;
		asp->info.sample_rate = Chan_tab[i].samplerate;
		asp->info.channels = Chan_tab[i].channels;
		asp->info.precision = Chan_tab[i].len;	/* precision */
		asp->info.encoding = AUDIO_ENCODING_ULAW;   /* XXX work this */
		asp->info.gain = AUDIO_MAX_GAIN;
		asp->info.balance = AUDIO_MID_BALANCE;
		asp->type = Chan_tab[i].audtype;
		AsToDs(asp)->pipe = DBRI_BAD_PIPE;
		AsToDs(asp)->recv_eol = FALSE;
		AsToDs(asp)->i_info.type = Chan_tab[i].isdntype;

		switch (i) {			/* adjust special channels */
		case DBRI_AUDIO_OUT:
			asp->info.port = AUDIO_SPEAKER;
			/* XXX - avail_ports needs to be adjusted for Sunergy*/
			asp->info.avail_ports =
				AUDIO_SPEAKER|AUDIO_HEADPHONE|AUDIO_LINE_OUT;
			asp->info.gain = DBRI_DEFAULT_GAIN;
			break;

		case DBRI_AUDIO_IN:
			asp->info.port = AUDIO_MICROPHONE;
			/* XXX - avail_ports needs to be adjusted for Sunergy*/
			asp->info.avail_ports = AUDIO_MICROPHONE|AUDIO_LINE_IN;
			asp->info.gain = DBRI_DEFAULT_GAIN;
			break;

		case DBRI_AUDIOCTL:
			break;

		case DBRI_TE_D_IN:
		case DBRI_TE_D_OUT:
		case DBRI_NT_D_IN:
		case DBRI_NT_D_OUT:
			asp->info.encoding = AUDIO_ENCODING_NONE;
			break;

		default:
			/*
			 * Nothing here currently.
			 * Would like to have additional channels added
			 * in dbri_conf.c to be sufficiently defined.
			 */
			break;
		} /* switch */

		/*
		 * Initialize virtual chained DMA command block free
		 * lists.  Record channels receive a different number of
		 * buffers than play chanels.
		 */
		asp->input_size = Chan_tab[i].input_size;
		AsToDs(asp)->i_var.t101 = Default_T101_timer;
		AsToDs(asp)->i_var.t102 = Default_T102_timer;
		AsToDs(asp)->i_var.t103 = Default_T103_timer;
		AsToDs(asp)->i_var.t104 = Default_T104_timer;
		AsToDs(asp)->i_var.asmb = Default_asmb;
		AsToDs(asp)->i_var.power = Default_power;
		nb = Chan_tab[i].numbufs;
		total_nb += nb;
	} /* for */

	tbytes = sizeof (dbri_chip_cmdq_t);
	tbytes += DBRI_NUM_INTQS * sizeof (dbri_intq_t);
	tbytes += total_nb * sizeof (dbri_cmd_t);
	(void) dbri_setup_mem_map(driver, tbytes);

	dprintf("dbri_attach: intq = 0x%x   cmdq = 0x%x\n",
	    (unsigned int)driver->intq.intq_bp,
	    (unsigned int)driver->cmdqp);
	dprintf("dbri_attach: serialp = 0x%x\n",
	    (unsigned int)&driver->ser_sts);

	/*
	 * Init the ops vector and back-pointers to the audio struct
	 */
	driver->hwi_state.ops = &Dbri_audops;

	/*
	 * Map in the registers for this device.
	 */
	driver->chip = (dbri_reg_t *)map_regs(dev->devi_reg->reg_addr,
	    dev->devi_reg->reg_size, dev->devi_reg->reg_bustype);

#if 1	/* XXX */
	{
		int oldlevel = dev->devi_intr->int_pri;

		switch (dev->devi_name[9]) {
		case 'a':
			panic("dbri: Found version 'a' where it couldn't be.");
			break;

		case 'c':
#ifdef sun4m
			dev->devi_intr->int_pri = 57; /* XXX - C2 Specific */
#endif
			break;
		default:
			break;
		}

		if (oldlevel != dev->devi_intr->int_pri) {
			printf("dbri: Changed int_pri from %d to %d!!!\n",
				oldlevel, dev->devi_intr->int_pri);
		}
	}
#endif	/* XXX3 */

	/*
	 * Add the interrupt for this device.
	 */
	driver->intrlevel = ipltospl(dev->devi_intr->int_pri);

	if (Curdbri == 0) {
		dprintf("dbri_attach: before addintr dev->pri=0x%x ilev=0x%x\n",
		    (unsigned int)dev->devi_intr->int_pri,
		    (unsigned int)driver->intrlevel);
		addintr(dev->devi_intr->int_pri, dbri_intr, dev->devi_name,
		    dev->devi_unit);
		adddma(dev->devi_intr->int_pri);
	}
	report_dev(dev);
	dbri_initchip(driver);			/* Initialize the chip */

	/* Allow users to use this unit */
	driver->openinhibit = FALSE;

	/* Start up audio functionality */
	mmcodec_startup_audio(driver);

	Curdbri++; 	/* Attach succeeded, increment unit number counter. */
	return (0);
} /* dbri_attach */


#if defined(DBRI_LOADABLE)

#include <sys/conf.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>

struct cdevsw	dbri_cdevsw = {
	/* open,	close,		read,		write, */
	0,		0,		0,		0,
	/* ioctl,	reset,		select,		mmap, */
	0,		0,		0,		0,
	/* str,		segmap */
	&dbri_info,	0,
};

extern char dbri_version[];

/*
 * struct vdldrv is different in sun4m and sun4c, -Dsun4m or -Dsun4c is
 * required for proper compilation. The new sun4m fields are at the end of
 * the structure and are therefore initialized to zeros here.
 */
struct vdldrv	Dbri_driver = {
	VDMAGIC_DRV,		/* Drv_magic */
	dbri_version,		/* Drv_name */
	&dbri_ops,		/* Drv_dev_ops */
	NULL,			/* Drv_bdevsw */
	&dbri_cdevsw,		/* Drv_cdevsw */
	0,			/* Drv_blockmajor, 0 means let system choose */
	0,			/* Drv_charmajor, 0 means let system choose */
};

/*
 * dbri_init - Entry point routine, modload/unload support.
 */
int
dbri_init(function_code, vdp, vdi, vds)
	unsigned 	function_code;
	struct vddrv	*vdp;
	addr_t		vdi;
	struct vdstat	*vds;
{
	int		status;
	int		dbri_unattach();

#ifdef lint
	(void) dbri_init(0, vdp, vdi, vds);
#endif

	switch (function_code) {
	case VDLOAD:
		vdp->vdd_vdtab = (struct vdlinkage*)&Dbri_driver;

		dprintf("dbri_init: loading driver\n");
		Ndbri = 0;
		Curdbri = 0;
		Dbri_devices = NULL;

		return (0);

	case VDUNLOAD:
		if (Dbri_devices == NULL)
			return (0);

#if !defined(STANDALONE)
		/*
		 * Removing audioclone mappings
		 */
		aclone_unregister((caddr_t)&dbri_info);
#endif /* STANDALONE */

		status = dbri_unattach(vdp);

		return (status);

	case VDSTAT:
		return (0);
	}

	return (EINVAL);
} /* dbri_init */


/*ARGSUSED*/
static int
dbri_unattach(vdp)
	struct vddrv	*vdp;
{
	dbri_dev_t	*driver;
	int		busy = 0;
	int		i, dev;
	int		s;

	/*
	 * Cycle through devices and if any minor device is open then the
	 * driver cannot be unloaded.
	 */

	for (dev = 0; dev < Ndbri; ++dev) {
		driver = &Dbri_devices[dev];

		s = splr(driver->intrlevel);

		for (i = 0; i < Nstreams; ++i) {
			aud_stream_t	*asp;

			asp = &driver->ioc[i].as;
			if (asp->info.open != 0)
				++busy;
		}

		if (busy) {
			(void) splx(s);
			return (EBUSY);
		}

		/* Set DBRI back to initial state */
		dbri_initchip(driver);

		remintr(driver->dbri_dev->devi_intr->int_pri, dbri_intr);

		(void) splx(s);
	} /* for */


#if 1 /* XXX */
	{
		struct callout	*p1,
		*p2;
		extern void	mmcodec_watchdog();
		char		*fn = 0;

		/* already spl'ed above clock? */
		for (p1 = &calltodo; (p2 = p1->c_next) != 0; p1 = p2) {
			if (p2->c_func == (int(*)())dbri_keep_alive) {
				fn = "dbri_keep_alive";
			} else if (p2->c_func == (int(*)())dbri_nt_timer) {
				fn = "dbri_nt_timer";
			} else if (p2->c_func == (int(*)())dbri_te_timer) {
				fn = "dbri_te_timer";
			} else if (p2->c_func == (int(*)())mmcodec_watchdog) {
				fn = "mmcodec_watchdog";
			} else {
				continue;
			}
			dprintf("dbri: callout queue = %s(0x%x) time=%d.\n",
			    fn, (unsigned int)p2->c_arg, p2->c_time);
			untimeout(p2->c_func, p2->c_arg); /* XXX */
		}
	}
#endif

	/* cycle through devices releasing memory */
	for (dev = 0; dev < Ndbri; ++dev) {
		driver = &Dbri_devices[dev];
		/*
		 * Deallocate per minor device resources.
		 */
		dprintf("dbri_init: deallocating memory\n");
#ifdef	sun4m
		mb_mapfree(dvmamap, &driver->sb_addr);
#endif	/* sun4m */
		kmem_free(driver->mem_base, (u_int) driver->mem_size);
	}

	/* Now release main driver structure */
	kmem_free((caddr_t)Dbri_devices, (u_int) (Ndbri * sizeof (dbri_dev_t)));
	Ndbri = 0;
	Dbri_devices = NULL;

	return (0);
}
#endif /* DBRI_LOADABLE */
