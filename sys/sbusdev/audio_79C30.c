#ident	"@(#) audio_79C30.c 1.1@(#) Copyright (c) 1989-92 Sun Microsystems, Inc."

/*
 * AUDIO Chip driver - for AMD AM79C30A
 *
 * The heart of this driver is its ability to convert sound to a
 * bit stream and back to sound.  The chip itself supports lots of
 * telephony functions, but the driver doesn't (yet).
 *
 * The basic facts:
 *	- The chip has a built in 8 bit DAC and ADC
 *	- When it is active, it interrupts every 125us,
 *	  or 8000 times a second.
 *	- The digital representation is u-law by default.
 *	  The high order bit is a sign bit, the low order seven bits
 *	  encode amplitude, and the entire 8 bits are inverted.
 *	- The driver does not currently support A-law encoding.
 *	- There are some additional registers which let you do ISDN control
 *	  and telephone style things like touch tone and ringers.
 *	- We use the Bb channel for both reading and writing.
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <machine/intreg.h>
#include <sundev/mbvar.h>
#include <machine/cpu.h>

#define	AMD_CHIP		/* include AUDIOGETREG/AUDIOSETREG defs */

/*
 * Make the C version of the interrupt handler the default except on sun4c
 * machines where the SPARC assembly version can be used
 */
#if !defined(sun4c)
#undef AMD_C_TRAP
#define	AMD_C_TRAP		/* use C interrupt routine */
#endif

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/audio_79C30.h>
#if !defined(STANDALONE)
#include <sbusdev/aclone.h>
#endif

#include "audiodebug.h"


/* External routines */
extern addr_t	map_regs();
extern void	report_dev();
extern int	splaudio();

/* Local routines */
static int		amd_identify();
static int		amd_attach();
static int		amd_open();
static void		amd_close();
static aud_return_t	amd_ioctl();
static void		amd_start();
static void		amd_stop();
static unsigned	int 	amd_setflag();
static aud_return_t	amd_setinfo();
static void		amd_queuecmd();
static void		amd_flushcmd();
static int		amd_intr4();
static void		amd_devinit();
static unsigned int 	amd_outport();
static unsigned int 	amd_play_gain();
static unsigned int 	amd_gx_gain();


/* Record and monitor gain use the same lookup table */
#define	amd_record_gain(chip, val) \
	amd_gx_gain(chip, val, AUDIO_UNPACK_REG(AUDIO_MAP_GX))
#define	amd_monitor_gain(chip, val) \
	amd_gx_gain(chip, val, AUDIO_UNPACK_REG(AUDIO_MAP_STG))

/* Level 13 interrupt handler, either in C (below) or in amd_intr.s */
extern int	amd_intr(/* */);

/* Local declarations */
amd_dev_t	*amd_devices;	/* device ctrlr array */
static unsigned int	devcnt;		/* number of devices found */

/* This is the size of the STREAMS buffers we send down the read side */
int			amd_bsize = AUD_79C30_BSIZE;


/* Declare audio ops vector for AMD79C30 support routines */
static struct aud_ops	amd_ops = {
	amd_close,
	amd_ioctl,
	amd_start,
	amd_stop,
	amd_setflag,
	amd_setinfo,
	amd_queuecmd,
	amd_flushcmd,
};

/*
 * Declare ops vector for auto configuration.
 * It must be named audioamd_ops, since the name is constructed from the
 * device name when config writes ioconf.c.
 */
struct dev_ops audioamd_ops = {
	1,			/* revision */
	amd_identify,		/* identify routine */
	amd_attach,		/* attach routine */
};

/*
 * Streams declarations
 */

static struct module_info amd_modinfo = {
	0x6175,			/* module ID number */
	"audioamd",		/* module name */
	0,			/* min packet size accepted */
	INFPSZ,			/* max packet size accepted */
	AUD_79C30_HIWATER,	/* hi-water mark */
	AUD_79C30_LOWATER,	/* lo-water mark */
};

/*
 * Queue information structure for read queue
 */
static struct qinit amd_rinit = {
	(int(*)())audio_rput,	/* put procedure */
	(int(*)())audio_rsrv,	/* service procedure */
	amd_open,		/* called on startup */
	(int(*)())audio_close,	/* called on finish */
	NULL,			/* for 3bnet only */
	&amd_modinfo,		/* module information structure */
	NULL,			/* module statistics structure */
};

/*
 * Queue information structure for write queue
 */
static struct qinit amd_winit = {
	(int(*)())audio_wput,	/* put procedure */
	(int(*)())audio_wsrv,	/* service procedure */
	NULL,			/* called on startup */
	NULL,			/* called on finish */
	NULL,			/* for 3bnet only */
	&amd_modinfo,		/* module information structure */
	NULL,			/* module statistics structure */
};

static char *amd_modules[] = {
	0,			/* no modules defined yet */
};

struct streamtab audio_79C30_info = {
	&amd_rinit,		/* qinit for read side */
	&amd_winit,		/* qinit for write side */
	NULL,			/* mux qinit for read */
	NULL,			/* mux qinit for write */
	amd_modules,		/* list of modules to be pushed */
};


/*
 * Loadable module wrapper
 */
#if defined(AMD_LOADABLE) || defined(VDDRV)
#include <sys/conf.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>

static struct cdevsw amd_cdevsw = {
	0, 0, 0, 0, 0, 0, 0, 0, &audio_79C30_info, 0
};

static struct vdldrv amd_modldrv = {
	VDMAGIC_DRV,		/* Type of module */
	"AMD79C30 audio driver", /* Descriptive name */
	&audioamd_ops,		/* Address of dev_ops */
	NULL,			/* bdevsw */
	&amd_cdevsw,		/* cdevsw */
	0,			/* Drv_blockmajor, 0 means let system choose */
	0,			/* Drv_charmajor, 0 means let system choose */
};


/*
 * Called by modload/modunload.
 */
int
amd_init(function_code, vdp, vdi, vds)
	uint		function_code;
	struct vddrv	*vdp;
	addr_t		vdi;
	struct vdstat	*vds;
{
#ifdef lint
	(void) amd_init(0, vdp, vdi, vds);
#endif

	switch (function_code) {
	case VDLOAD:		/* Load module */
		/* Set pointer to loadable driver structure */
		vdp->vdd_vdtab = (struct vdlinkage *)&amd_modldrv;

		return (0);

	case VDUNLOAD: {	/* Unload module */
		amd_dev_t	*amdp;
		unsigned int	i;
		int		s;

#if !defined(STANDALONE)
		/* Unregister our aclone mappings */
		aclone_unregister((caddr_t)&audio_79C30_info);
#endif

		/* Cannot unload driver if any minor device is open */
		s = splaudio();
		for (i = 0; i < devcnt; i++) {
			amdp = &(amd_devices[i]);
			if (amdp->play.as.openflag ||
			    amdp->rec.as.openflag ||
			    amdp->control.as.openflag) {
				(void) splx(s);
				return (EBUSY);
			}
		}
		(void) splx(s);

		/*
		 * Remove device from interrupt queues.
		 * XXX - assumes a single device interrupt priority
		 */
#if defined(AMD_C_TRAP)
		(void) remintr(amdp->amd_dev->devi_intr->int_pri,
		    amd_intr);
#else /* !AMD_C_TRAP */
		if (!settrap(amdp->amd_dev->devi_intr->int_pri, (int(*)())NULL))
			panic("audio unload: Cannot reset trap vector");
#endif /* !AMD_C_TRAP */
		(void) remintr(4, amd_intr4);

		/*
		 * Deallocate structures allocated in attach
		 * routine
		 */
		for (i = 0; i < devcnt; i++) {
			kmem_free(amd_devices[i].play.as.cmdlist.memory,
				(u_int)amd_devices[i].play.as.cmdlist.size);
			kmem_free(amd_devices[i].rec.as.cmdlist.memory,
				(u_int)amd_devices[i].rec.as.cmdlist.size);
		}
		return (0);
	}

	case VDSTAT:
		return (0);

	default:
		return (EINVAL);
	}
}
#endif /* VDDRV */

/*
 * Called by match_driver() and add_drv_layer() in autoconf.c
 * Returns TRUE if the given string refers to this driver, else FALSE.
 */

static int
amd_identify(name)
	char	*name;
{

	if (strcmp(name, "audio") == 0) {
		devcnt++;
		return (TRUE);
	}
	return (FALSE);
}


/*
 * Attach to the device.
 */
static int
amd_attach(dev)
	struct dev_info	*dev;
{
	aud_stream_t	*as, *play_as, *record_as;
	aud_state_t	*v;
	amd_dev_t	*amdv;
	int		i;
	struct aud_cmd	*pool;
#if !defined(STANDALONE)
	dev_t		amddev;
#endif

	ATRACEINIT();
	ATRACEPRINT("audio: debugging version of audio driver\n");

	ASSERT(dev != NULL);

	/*
	 * Each unit has a 'aud_state_t' that contains generic audio
	 * device state information.  Also, each unit has a
	 * 'amd_dev_t' that contains device-specific data.
	 * Allocate storage for them here.
	 */
	if (amd_devices == NULL) {
		amd_devices = ((amd_dev_t *)
			new_kmem_zalloc((u_int)
				(devcnt * sizeof (amd_dev_t)),
				KMEM_SLEEP));
	} else {
		/* XXX - We only support one audio device at this time */
		(void) printf("audioamd: cannot support multiple audio devices\n");
		return (-1);
	}

	/*
	 * Identify the audio device and assign a unit number.  Get the
	 * address of this unit's audio device state structure.
	 *
	 * XXX - With the sun4c architecture, we know that exactly one
	 * device is there; we should probe anyway, to be safe.
	 */
	dev->devi_unit = 0;

	v = (aud_state_t *)&amd_devices[dev->devi_unit];
	ASSERT(v != NULL);
	v->devdata = (void *)&amd_devices[dev->devi_unit];

	amdv = (amd_dev_t *)v;

	amdv->amd_dev = dev;
	amdv->control.as.control_as = &amdv->control.as;
	amdv->control.as.play_as = &amdv->play.as;
	amdv->control.as.record_as = &amdv->rec.as;
	amdv->play.as.control_as = &amdv->control.as;
	amdv->play.as.play_as = &amdv->play.as;
	amdv->play.as.record_as = &amdv->rec.as;
	amdv->rec.as.control_as = &amdv->control.as;
	amdv->rec.as.play_as = &amdv->play.as;
	amdv->rec.as.record_as = &amdv->rec.as;

	as = &amdv->control.as;
	play_as = as->play_as;
	record_as = as->record_as;

	ASSERT(as != NULL);
	ASSERT(play_as != NULL);
	ASSERT(record_as != NULL);

	/* Initialize the play info struct */
	play_as->info.gain = AUD_79C30_DEFAULT_GAIN;
	play_as->info.sample_rate = AUD_79C30_SAMPLERATE;
	play_as->info.channels = AUD_79C30_CHANNELS;
	play_as->info.precision = AUD_79C30_PRECISION;
	play_as->info.encoding = AUDIO_ENCODING_ULAW;
	play_as->info.minordev = AMD_MINOR_RW;
	play_as->info.balance = AUDIO_MID_BALANCE;

	/* Copy play struct to record struct and init ports */
	record_as->info = play_as->info;

#if defined(CPU_SUN4M_690)
	/*
	 * XXX - This only works when compiled on *a* sun4m machine because
	 * <machine/cpu.h> points to the correct sun4c/4m cpu.h.  Ugh.
	 */
	if (cpu == CPU_SUN4M_690) {
		/*
		 * XXX - The SPARCserver 600 series does not have a built-in
		 * speaker.  Send the audio out the headphone jack by default.
		 */
		play_as->info.port = AUDIO_HEADPHONE;
	} else {
		play_as->info.port = AUDIO_SPEAKER;
	}
#else
	play_as->info.port = AUDIO_SPEAKER;
#endif /* CPU_SUN4M_690 */

	play_as->info.avail_ports = AUDIO_SPEAKER;

	record_as->info.port = AUDIO_MICROPHONE;
	record_as->info.minordev = AMD_MINOR_RO;
	record_as->info.avail_ports = AUDIO_MICROPHONE;

	/* Control stream info */
	as->info.minordev = AMD_MINOR_CTL;

	/*
	 * Set types and modes
	 */
	play_as->type = AUDTYPE_DATA;
	record_as->type = AUDTYPE_DATA;
	as->type = AUDTYPE_CONTROL;
	play_as->mode = AUDMODE_AUDIO;
	record_as->mode = AUDMODE_AUDIO;
	as->mode = AUDMODE_NONE;

	/* Init the ops vector and back-pointers to the audio struct */
	v->ops = &amd_ops;
	as->play_as->v = v;
	as->record_as->v = v;
	as->control_as->v = v;

	/*
	 * Initialize virtual chained DMA command block free lists.
	 * Reserve a couple of command blocks for record buffers.
	 * Then allocate the rest for play buffers.
	 */
	pool = (struct aud_cmd *)
		new_kmem_zalloc(AUD_79C30_CMDPOOL * sizeof (struct aud_cmd),
				KMEM_SLEEP);
	ASSERT(pool != NULL);

	for (i = 0; i < AUD_79C30_CMDPOOL; i++) {
		struct aud_cmdlist	*list;

		list = (i < AUD_79C30_RECBUFS)	? &as->record_as->cmdlist
						: &as->play_as->cmdlist;
		pool->next = list->free;
		list->free = pool++;
	}

	/*
	 * Map in the registers for this device.  There is only one set
	 * of registers, so we quit if we see some different number.
	 */
	ASSERT(dev != NULL);
	if (dev->devi_nreg == 1) {
		CSR(as) = ((struct aud_79C30_chip *)
			    map_regs(dev->devi_reg->reg_addr,
				    dev->devi_reg->reg_size,
				    dev->devi_reg->reg_bustype));
		ASSERT(CSR(as) != NULL);
	} else {
		(void) printf("audio%d: warning: bad register specification\n",
			dev->devi_unit);
		return (-1);
	}

	/*
	 * Add the interrupt for this device, so that we get interrupts
	 * when they occur.  We only expect one hard interrupt address.
	 * Set up the level 4 soft interrupt service routine, called when
	 * there is real work to do.
	 */
	if (dev->devi_nintr == 1) {
#if defined(AMD_C_TRAP)
		addintr(dev->devi_intr->int_pri, amd_intr,
		    dev->devi_name, dev->devi_unit);
#else /* !AMD_C_TRAP */
		if (!settrap(dev->devi_intr->int_pri, amd_intr))
			panic("amd: Cannot set level 13 trap vector");
#endif /* !AMD_C_TRAP */
		addintr(4, amd_intr4, dev->devi_name, dev->devi_unit);
	} else {
		(void) printf("audio%d: warning: bad interrupt specification\n",
			dev->devi_unit);
		return (-1);
	}

	/* Initialize the audio chip */
	amd_devinit(as);

	report_dev(dev);

#if !defined(STANDALONE)
	/* Register with audioclone device */
	amddev = aclone_register((caddr_t)&audio_79C30_info,
				    0, /* slot 0 is reserved for amd */
				    10 + AMD_MINOR_RW, /* audio */
				    10 + AMD_MINOR_RO, /* audioro */
				    10 + AMD_MINOR_CTL); /* audioctl */

	if (amddev < 0)
		(void) printf("audioamd: could not register with aclone!\n");
#endif

	return (0);
}


/* Device open routine: set device structure ptr and call generic routine */
/*ARGSUSED*/
static int
amd_open(q, dev, flag, sflag)
	queue_t		*q;
	dev_t		dev;
	int		flag;
	int		sflag;
{
	aud_stream_t	*as = NULL;
	amd_dev_t	*amdp;
	int		minornum = minor(dev);

	/*
	 * Necessary for use with audioclone. XXX
	 */
	if (minornum == AMD_MINOR_CTL_OLD) {
		printf("amd_open: old control device minor number %d has changed to %d\n",
			AMD_MINOR_CTL_OLD, AMD_MINOR_CTL);
		u.u_error = ENODEV;
		return (OPENFAIL);
	}
	/*
	 * If this is an open from audioclone, normalize both dev
	 * and minornum; see call to aclone_register to find out about "10"
	 */
	if (minornum >= 10) {
		minornum -= 10;	/* XXX DIC */
		dev = makedev(major(dev), minornum);
	}

	/*
	 * Handle clone device opens
	 */
	if (sflag == CLONEOPEN) {
		switch (minornum) {
		case 0:				/* Normal clone open */
		case AMD_MINOR_RW:		/* Audio clone open */
			if (flag & FWRITE) {
				dev = makedev(major(dev), AMD_MINOR_RW);
				break;
			}
			/* fall through */
		case AMD_MINOR_RO:	/* Audio clone open */
			dev = makedev(major(dev), AMD_MINOR_RO);
			break;

		default:
			u.u_error = ENODEV;
			return (OPENFAIL);
		}
	} else if (minornum == 0) {
		/*
		 * Because the system temporarily uses the streamhead
		 * corresponding to major,0 during a clone open, and because
		 * audio_open() can sleep, audio drivers are not allowed
		 * to use major,0 as a valid device number.
		 *
		 * A sleeping clone open and a non-clone use of maj,0
		 * can mess up the reference counts on vnodes/snodes
		 */
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	minornum = minor(dev);

	amdp = &amd_devices[0];	/* XXX - one device */
	if (minornum == amdp->play.as.info.minordev)
		as = &amdp->play.as;
	else if (minornum == amdp->rec.as.info.minordev)
		as = &amdp->rec.as;
	else if (minornum == amdp->control.as.info.minordev)
		as = &amdp->control.as;

	if (as == NULL) {
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	/* Set input buffer size now, in case the value was patched */
	as->record_as->input_size = amd_bsize;

	if (ISDATASTREAM(as) && ((flag & (FREAD|FWRITE)) == FREAD))
		as = as->record_as;

	if (audio_open(as, q, flag, sflag) == OPENFAIL)
		return (OPENFAIL);

	if (ISDATASTREAM(as) && (flag & FREAD))
		audio_process_record(as);

	return (minornum);
}


static void
amd_close(as)
	aud_stream_t	*as;
{
	amd_dev_t	*devp;
	struct aud_79C30_chip	*chip;

	ASSERT(as != NULL);
	devp = DEVP(as);
	ASSERT(devp != NULL);
	chip = devp->chip;
	ASSERT(chip != NULL);

	/* Reset status bits.  The device will already have been stopped. */
	if (as == as->play_as) {
		devp->play.samples = 0;
		devp->play.error = FALSE;
	} else {
		devp->rec.samples = 0;
		devp->rec.error = FALSE;
	}

	/*
	 * Reset to u-Law on last close
	 */
	if ((as == as->play_as && as->record_as->readq == NULL) ||
	    (as == as->record_as && as->play_as->readq == NULL)) {
		as->play_as->info.encoding = AUDIO_ENCODING_ULAW;
		as->record_as->info.encoding = AUDIO_ENCODING_ULAW;
		chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
		chip->dr = (AUDIO_MMR1_BITS_u_LAW |
		    AUDIO_MMR1_BITS_LOAD_GR |
		    AUDIO_MMR1_BITS_LOAD_GER |
		    AUDIO_MMR1_BITS_LOAD_GX |
		    AUDIO_MMR1_BITS_LOAD_STG);
	}

#if defined(AMD_CHIP)
	/* If a user process mucked up the device, reset it when fully closed */
	if (devp->setreg_used &&
	    !as->play_as->info.open &&
	    !as->record_as->info.open) {
		amd_devinit(as);
		devp->setreg_used = FALSE;
	}
#endif /* AMD_CHIP */

	ATRACE(amd_close, 'clos', as);
	return;
}


/*
 * Process ioctls not already handled by the generic audio handler.
 *
 * If AMD_CHIP is defined, we support ioctls that allow user processes
 * to muck about with the device registers.
 */
static aud_return_t
amd_ioctl(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	aud_return_t		change;
	int			s;

	change = AUDRETURN_NOCHANGE;		/* detect device state change */

	switch (iocp->ioc_cmd) {

	case AUDIO_GETDEV:
		{
		int	*ip;

		ip = ((int *)mp->b_cont->b_rptr);
		mp->b_cont->b_wptr += sizeof (int);
		*ip = AUDIO_DEV_AMD;
		iocp->ioc_count = sizeof (int);
		}
		break;

#if defined(AMD_CHIP)
	/*
	 * The following chip manipulation functions are provided for
	 * backward-compatibiliity and ISDN control prototyping.
	 * They may be removed in a future release.
	 *
	 * The audio_ioctl structure contains the register load information:
	 *	register (one byte)
	 *	length (one byte)
	 *	data (0-46 bytes)
	 * However, to make the programming interface simpler, there are
	 * defines that combine the register and length into a short.
	 * See <sbusdev/amd.h> for the gory details.
	 */
	case AUDIOGETREG: {
		/* Read a register from the audio chip */
		int			i;
		struct audio_ioctl	*aioctl;

		/* Disallow these ioctls on control streams */
		if (as == as->control_as)
			goto einval;

		/* The input struct designates the register to be read */
		aioctl = (struct audio_ioctl *)mp->b_cont->b_rptr;

		s = splaudio();
		CSR(as)->cr = AUDIO_UNPACK_REG(aioctl->control);
		for (i = 0; i < AUDIO_UNPACK_LENGTH(aioctl->control); i++)
			aioctl->data[i] = CSR(as)->dr;
		(void) splx(s);

		iocp->ioc_count = (char *)&aioctl->data[i] - (char *)aioctl;
		mp->b_cont->b_wptr += iocp->ioc_count;
		break;
	    }

	case AUDIOSETREG: {
		/* Write data to an audio chip register */
		int			i;
		struct audio_ioctl	*aioctl;

		/* Disallow these ioctls on control streams */
		if (as == as->control_as)
			goto einval;

		aioctl = (struct audio_ioctl *)mp->b_cont->b_rptr;

		s = splaudio();
		CSR(as)->cr = AUDIO_UNPACK_REG(aioctl->control);
		for (i = 0; i < AUDIO_UNPACK_LENGTH(aioctl->control); i++)
			CSR(as)->dr = aioctl->data[i];
		(void) splx(s);

		/* Note a status change and set flag to reset on close() */
		DEVP(as)->setreg_used = TRUE;
		change = AUDRETURN_CHANGE;
		break;
	    }
#endif	/* AMD_CHIP */

	default:
einval:
		iocp->ioc_error = EINVAL;
		break;
	}
	return (change);
}


/*
 * The next routine is used to start reads or writes.
 * If there is a change of state, enable the chip.
 * If there was already i/o active in the desired direction,
 * or if i/o is paused, don't bother enabling the chip.
 */
static void
amd_start(as)
	aud_stream_t	*as;
{
	amd_stream_t	*cp;
	int		idle;
	int		pause;

	ASSERT(as != NULL);
	if (as == as->play_as) {
		cp = &DEVP(as)->play;
		idle = !(DEVP(as)->rec.active);
		pause = as->play_as->info.pause;
	} else {
		cp = &DEVP(as)->rec;
		idle = !(DEVP(as)->play.active);
		pause = as->record_as->info.pause;
	}
	ASSERT(cp != NULL);

	/* If already active, paused, or nothing queued to the device, done */
	if (cp->active || pause || (cp->cmdptr == NULL))
		return;

	cp->active = TRUE;

	if (idle) {
		ATRACE(amd_start,
			(as == as->play_as ? ATR_STARTPLAY : ATR_STARTREC),
			__LINE__);

		CSR(as)->cr = AUDIO_UNPACK_REG(AUDIO_INIT_INIT);
		CSR(as)->dr = AUDIO_INIT_BITS_ACTIVE;
	}
	return;
}

/*
 * The next routine is used to stop reads or writes.  All we do is turn
 * off the active bit.  If there is currently no i/o active in the other
 * direction, then the interrupt routine will disable the chip.
 */
static void
amd_stop(as)
	aud_stream_t	*as;
{
	int	s;

	ASSERT(as != NULL);

	s = splaudio();		/* read/reset error flag atomically */
	if (as == as->play_as)
		DEVP(as)->play.active = FALSE;
	else
		DEVP(as)->rec.active = FALSE;

	ATRACE(amd_stop, (as == as->play_as ? ATR_STOPPLAY : ATR_STOPREC),
		__LINE__);

	(void) splx(s);
	return;
}


/* Get or set a particular flag value */
static unsigned int
amd_setflag(as, op, val)
	aud_stream_t	*as;
	enum aud_opflag op;
	unsigned int	val;
{
	amd_stream_t	*cp;

	ASSERT(as != NULL);
	cp = (as == as->play_as) ? &DEVP(as)->play : &DEVP(as)->rec;
	ASSERT(cp != NULL);

	switch (op) {
	case AUD_ERRORRESET: {
		int	s;

		s = splaudio();		/* read/reset error flag atomically */
		val = cp->error;
		cp->error = FALSE;
		(void) splx(s);
		break;
	    }

	/* GET only */
	case AUD_ACTIVE:
		val = cp->active;
		break;
	}
	return (val);
}


/* Get or set device-specific information in the audio state structure */
static aud_return_t
amd_setinfo(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	aud_stream_t		*play_as = as->play_as;
	aud_stream_t		*record_as = as->record_as;
	amd_dev_t		*devp;
	struct aud_79C30_chip	*chip;
	audio_info_t		*ip;
	unsigned int		sample_rate, channels, precision, encoding;
	int			error = 0;
	int			s;

	ASSERT(as != NULL);
	devp = DEVP(as);
	ASSERT(devp != NULL);

	/* Set device-specific info into device-independent structure */
	play_as->info.samples = devp->play.samples;
	record_as->info.samples = devp->rec.samples;
	play_as->info.active = devp->play.active;
	record_as->info.active = devp->rec.active;

	/* If getinfo, 'mp' is NULL...we're done */
	if (mp == NULL)
		return (AUDRETURN_NOCHANGE);

	chip = devp->chip;
	ASSERT(chip != NULL);

	s = splaudio();		/* load chip registers atomically */
	ip = (audio_info_t *)mp->b_cont->b_rptr;

	/*
	 * If any new value matches the current value, there
	 * should be no need to set it again here.
	 * However, it's work to detect this so don't bother.
	 *
	 * Also, if AMD_CHIP is defined, apps may have set things via
	 * AUDIOSETREG; it is convenient to allow GETLEVEL/SETLEVEL to
	 * reset the gains & outport to what the driver thinks they are.
	 *
	 * We must return an error if apps try and change the encoding
	 * on the control device
	 */
	if (Modify(ip->play.gain))
		play_as->info.gain = amd_play_gain(chip, ip->play.gain);

	if (Modify(ip->record.gain))
		record_as->info.gain = amd_record_gain(chip, ip->record.gain);

	if (Modify(ip->monitor_gain)) {
		devp->hwi_state.monitor_gain =
			amd_monitor_gain(chip, ip->monitor_gain);
	}


	if (Modify(ip->play.port)) {
		switch (ip->play.port) {
		case AUDIO_SPEAKER:
		case AUDIO_HEADPHONE:
#if defined(CPU_SUN4M_690)
			/*
			 * XXX - Only headphone jack on SPARCserver 600 series,
			 * so don't allow any changes from the default value.
			 */
			if (cpu != CPU_SUN4M_690) {
				play_as->info.port =
					amd_outport(chip, ip->play.port);
			} else {
				error = EINVAL;
			}
#else
			play_as->info.port = amd_outport(chip, ip->play.port);
#endif
			break;

		default:
			error = EINVAL;
			break;
		}
	}

	/*
	 * Set the sample counters atomically, returning the old values.
	 */
	if (Modify(ip->play.samples) || Modify(ip->record.samples)) {
		if (play_as->info.open) {
			play_as->info.samples = devp->play.samples;
			if (Modify(ip->play.samples))
				devp->play.samples = ip->play.samples;
		}
		if (record_as->info.open) {
			record_as->info.samples = devp->rec.samples;
			if (Modify(ip->record.samples))
				devp->rec.samples = ip->record.samples;
		}

	}

	if (Modify(ip->play.sample_rate))
		sample_rate = ip->play.sample_rate;
	else if (Modify(ip->record.sample_rate))
		sample_rate = ip->record.sample_rate;
	else
		sample_rate = as->info.sample_rate;
	if (Modify(ip->play.channels))
		channels = ip->play.channels;
	else if (Modify(ip->record.channels))
		channels = ip->record.channels;
	else
		channels = as->info.channels;
	if (Modify(ip->play.precision))
		precision = ip->play.precision;
	else if (Modify(ip->record.precision))
		precision = ip->record.precision;
	else
		precision = as->info.precision;
	if (Modify(ip->play.encoding))
		encoding = ip->play.encoding;
	else if (Modify(ip->record.encoding))
		encoding = ip->record.encoding;
	else
		encoding = as->info.encoding;

	/*
	 * If setting to the same existing format, don't do anything. Otherwise,
	 * check and see if this is a supported format
	 */
	if ((sample_rate == as->info.sample_rate) &&
			(channels == as->info.channels) &&
			(precision == as->info.precision) &&
			(encoding == as->info.encoding)) {

		(void) splx(s);
		iocp->ioc_error = error;
		return (AUDRETURN_CHANGE);
	} else if ((sample_rate != 8000) ||
			(channels != 1) ||
			(precision != 8) ||
			((encoding != AUDIO_ENCODING_ULAW) &&
			(encoding != AUDIO_ENCODING_ALAW))) {

		error = EINVAL;
		(void) splx(s);
		iocp->ioc_error = error;
		return (AUDRETURN_CHANGE);
	}
	if (Modify(ip->play.encoding)) {
		/*
		 * If a process wants to modify the play format, another
		 * process can not have it open for recording, but it can
		 */
		if (record_as->info.open && play_as->info.open &&
				(record_as->readq != play_as->readq)) {
			error = EBUSY;
			goto playdone;
		}
		/*
		 * Do not allow format changes on the control channel
		 */
		if ((as != play_as) && (as != record_as)) {
			error = EINVAL;
			goto playdone;
		}
		switch (ip->play.encoding) {
		case AUDIO_ENCODING_ULAW:
			play_as->info.encoding = AUDIO_ENCODING_ULAW;
			record_as->info.encoding = AUDIO_ENCODING_ULAW;

			/* Tell the chip to accept the gain registers */
			chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
			chip->dr = (AUDIO_MMR1_BITS_u_LAW |
			    AUDIO_MMR1_BITS_LOAD_GR |
			    AUDIO_MMR1_BITS_LOAD_GER |
			    AUDIO_MMR1_BITS_LOAD_GX |
			    AUDIO_MMR1_BITS_LOAD_STG);
			break;

		case AUDIO_ENCODING_ALAW:
			play_as->info.encoding = AUDIO_ENCODING_ALAW;
			record_as->info.encoding = AUDIO_ENCODING_ALAW;

			/* Tell the chip to accept the gain registers */
			chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
			chip->dr = (AUDIO_MMR1_BITS_A_LAW |
			    AUDIO_MMR1_BITS_LOAD_GR |
			    AUDIO_MMR1_BITS_LOAD_GER |
			    AUDIO_MMR1_BITS_LOAD_GX |
			    AUDIO_MMR1_BITS_LOAD_STG);
			break;

		default:
			error = EINVAL;
			break;
		}
	playdone: ;
	}

	if (Modify(ip->record.encoding)) {
		/*
		 * If a process wants to modify the recording format, another
		 * process can not have it open for playing, but it can
		 */
		if (play_as->info.open && record_as->info.open &&
				(record_as->readq != play_as->readq)) {
			error = EBUSY;
			goto recdone;
		}
		/*
		 * Do not allow format changes on the control channel
		 */
		if ((as != play_as) && (as != record_as)) {
			error = EINVAL;
			goto recdone;
		}
		switch (ip->record.encoding) {
		case AUDIO_ENCODING_ULAW:
			record_as->info.encoding = AUDIO_ENCODING_ULAW;
			play_as->info.encoding = AUDIO_ENCODING_ULAW;

			/* Tell the chip to accept the gain registers */
			chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
			chip->dr = (AUDIO_MMR1_BITS_u_LAW |
			    AUDIO_MMR1_BITS_LOAD_GR |
			    AUDIO_MMR1_BITS_LOAD_GER |
			    AUDIO_MMR1_BITS_LOAD_GX |
			    AUDIO_MMR1_BITS_LOAD_STG);
			break;

		case AUDIO_ENCODING_ALAW:
			record_as->info.encoding = AUDIO_ENCODING_ALAW;
			play_as->info.encoding = AUDIO_ENCODING_ALAW;

			/* Tell the chip to accept the gain registers */
			chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
			chip->dr = (AUDIO_MMR1_BITS_A_LAW |
			    AUDIO_MMR1_BITS_LOAD_GR |
			    AUDIO_MMR1_BITS_LOAD_GER |
			    AUDIO_MMR1_BITS_LOAD_GX |
			    AUDIO_MMR1_BITS_LOAD_STG);
			break;

		default:
			error = EINVAL;
			break;
		}
	recdone: ;
	}
	(void) splx(s);
	iocp->ioc_error = error;
	return (AUDRETURN_CHANGE);
}


/*
 * This routine is called whenever a new command is added to the cmd chain.
 * Since the virtual dma controller simply uses the driver's cmd chain,
 * all we have to do is make sure that the virtual controller has the
 * start address right.
 */
/*ARGSUSED*/
static void
amd_queuecmd(as, cmdp)
	aud_stream_t	*as;
	aud_cmd_t	*cmdp;
{
	amd_stream_t	*cp;

	ASSERT(as != NULL);
	ASSERT(cmdp != NULL);
	cp = (amd_stream_t *)as;
	ASSERT(cp != NULL);

	/*
	 * Each AMD command is its own "packet".  Setting this here makes
	 * the interrupt routine (C and Assemby versions) that much
	 * shorter.
	 */
	cmdp->lastfragment = cmdp;

	/*
	 * If the virtual controller command list is NULL, then the
	 * interrupt routine is probably disabled.  In the event that it
	 * is not, setting a new command list below is safe at low
	 * priority.
	 */
	if (cp->cmdptr == NULL) {
		cp->cmdptr = cmdp;
		ATRACE(amd_queuecmd,
		    (as == as->play_as ? ATR_FIRSTPLAY : ATR_FIRSTREC),
		    __LINE__);
		if (!cp->active)
			amd_start(as); /* go, if not paused */
	}
	return;
}


/*
 * Flush the device's notion of queued commands.
 */
static void
amd_flushcmd(as)
	aud_stream_t	*as;
{
	ASSERT(as != NULL);

	if (as == as->play_as)
		DEVP(as)->play.cmdptr = NULL;
	else
		DEVP(as)->rec.cmdptr = NULL;
	return;
}

/*
 * This is the interrupt routine used by the level four interrupt.  It is
 * scheduled by the level 13 interrupt routine, simply calls out to the
 * generic audio driver routines to process finished play and record
 * buffers.
 */
static int
amd_intr4()
{
	/* XXX - for now, there's only one audio device */
	aud_stream_t	*as = (aud_stream_t *)&amd_devices[0].play.as;
	aud_stream_t	*record_as = as->record_as;
	aud_stream_t	*play_as = as->play_as;

	/* Process Record events: call out if finished cmd block or error */
	if ((record_as->cmdlist.head != NULL &&
	    record_as->cmdlist.head->done) ||
	    DEVP(as)->rec.error) {
		ATRACE(amd_intr4, 'rint', 0);
		audio_process_record(record_as);
	}

	/* Process Play events: call out if finished cmd block or error */
	if ((play_as->cmdlist.head != NULL &&
	    play_as->cmdlist.head->done) ||
	    DEVP(as)->play.error) {
		ATRACE(amd_intr4, 'pint', 0);
		audio_process_play(play_as);
	}
	return (TRUE);
}


/*
 * Initialize the audio chip to a known good state.
 *
 * The audio outputs are usually high impedance.  This causes a problem
 * with the current hardware, since the high impedance line picks up lots
 * of noise.  The lines are always high impedance if the chip is idle (as
 * on reset or after setting the INIT register to 0).  We can lower the
 * impedance of one output, and hence the noise, by making the chip
 * active and selecting that output.  However, the other output will be
 * floating, and hence noisy.
 */
static void
amd_devinit(as)
	aud_stream_t		*as;
{
	struct aud_79C30_chip	*chip;
	amd_dev_t		*devp;
	aud_stream_t		*play_as = as->play_as;
	aud_stream_t		*record_as = as->record_as;

	ASSERT(as != NULL);

	devp = DEVP(as);
	chip = CSR(as);
	ASSERT(devp != NULL);
	ASSERT(chip != NULL);

	/* Make the chip inactive and turn off the INT pin. */
	chip->cr = AUDIO_UNPACK_REG(AUDIO_INIT_INIT);
	chip->dr = AUDIO_INIT_BITS_ACTIVE | AUDIO_INIT_BITS_INT_DISABLED;

	/*
	 * Set up the multiplexer.  We use the Bb port for both reads
	 * and writes.  We also enable interrupts for the port.  Note
	 * that we also have to enable the INT pin using the INIT
	 * register to get an interrupt.
	 */
	chip->cr = AUDIO_UNPACK_REG(AUDIO_MUX_MCR1);
	chip->dr = AUDIO_MUX_PORT_BA | (AUDIO_MUX_PORT_BB << 4);

	chip->cr = AUDIO_UNPACK_REG(AUDIO_MUX_MCR2);
	chip->dr = 0;

	chip->cr = AUDIO_UNPACK_REG(AUDIO_MUX_MCR3);
	chip->dr = 0;

	chip->cr = AUDIO_UNPACK_REG(AUDIO_MUX_MCR4);
	chip->dr = AUDIO_MUX_MCR4_BITS_INT_ENABLE;

	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_FTGR);
	chip->dr = 0;
	chip->dr = 0;

	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_ATGR);
	chip->dr = 0;
	chip->dr = 0;

	/* Init the gain registers */
	play_as->info.gain = amd_play_gain(chip, play_as->info.gain);
	record_as->info.gain =
	    amd_record_gain(chip, record_as->info.gain);

	(void) amd_monitor_gain(chip, devp->hwi_state.monitor_gain);
	(void) amd_outport(chip, play_as->info.port);

	/* Tell the chip to accept the gain registers */
	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR1);
	chip->dr = AUDIO_MMR1_BITS_LOAD_GR | AUDIO_MMR1_BITS_LOAD_GER |
	    AUDIO_MMR1_BITS_LOAD_GX | AUDIO_MMR1_BITS_LOAD_STG;

	return;
}


/* Set output port to external jack or built-in speaker */
static unsigned int
amd_outport(chip, val)
	struct aud_79C30_chip	*chip;
	unsigned int		val;
{
	ASSERT(chip != NULL);

	/* AINB is the analog input port; LS is the built-in speaker */
	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_MMR2);
	chip->dr = AUDIO_MMR2_BITS_AINB |
	    (val == AUDIO_SPEAKER ? AUDIO_MMR2_BITS_LS : 0);

	return (val);
}


/*
 * AM79C30 gain coefficient tables
 *
 * The record and monitor levels can range from -18dB to +12dB.
 * The play level is the sum of both gr and ger and can range
 * from -28dB to +30dB.  Such low output levels are not particularly
 * useful, however, so the range supported by the driver is -13dB to +30dB.
 *
 * Further, since ger also amplifies the monitor path, it is held at +5dB
 * except at extremely high output levels.
 *
 * The gain tables have been chosen to give the most granularity in
 * the upper half of the dynamic range, since small changes at low
 * levels are virtually indistiguishable.  Since the maximum output
 * gains are only needed for extremely quiet recordings, less
 * granularity is provided at the extreme upper end as well.
 */

struct aud_gainset {
	unsigned char	coef[2];
};

/* This is the table for record, monitor, and 1st-stage play gains */
static struct aud_gainset grtab[] = {
	/* Infinite attenuation is treated as a special case */
	{0x08, 0x90},			/* infinite attenuation */

	{0x7c, 0x8b},			/* -18.  dB */
	{0x35, 0x8b},			/* -17.  dB */
	{0x24, 0x8b},			/* -16.  dB */
	{0x23, 0x91},			/* -15.  dB */
	{0x2a, 0x91},			/* -14.  dB */
	{0x3b, 0x91},			/* -13.  dB */
	{0xf9, 0x91},			/* -12.  dB */
	{0xb6, 0x91},			/* -11.  dB */
	{0xa4, 0x91},			/* -10.  dB */
	{0x32, 0x92},			/*  -9.  dB */
	{0xaa, 0x92},			/*  -8.  dB */
	{0xb3, 0x93},			/*  -7.  dB */
	{0x91, 0x9f},			/*  -6.  dB */
	{0xf9, 0x9b},			/*  -5.  dB */
	{0x4a, 0x9a},			/*  -4.  dB */
	{0xa2, 0xa2},			/*  -3.  dB */
	{0xa3, 0xaa},			/*  -2.  dB */
	{0x52, 0xbb},			/*  -1.  dB */
	{0x08, 0x08},			/*   0.  dB */
	{0xb2, 0x4c},			/*   0.5 dB */
	{0xac, 0x3d},			/*   1.  dB */
	{0xe5, 0x2a},			/*   1.5 dB */
	{0x33, 0x25},			/*   2.  dB */
	{0x22, 0x22},			/*   2.5 dB */
	{0x22, 0x21},			/*   3.  dB */
	{0xd3, 0x1f},			/*   3.5 dB */
	{0xa2, 0x12},			/*   4.  dB */
	{0x1b, 0x12},			/*   4.5 dB */
	{0x3b, 0x11},			/*   5.  dB */
	{0xc3, 0x0b},			/*   5.5 dB */
	{0xf2, 0x10},			/*   6.  dB */
	{0xba, 0x03},			/*   6.5 dB */
	{0xca, 0x02},			/*   7.  dB */
	{0x1d, 0x02},			/*   7.5 dB */
	{0x5a, 0x01},			/*   8.  dB */
	{0x22, 0x01},			/*   8.5 dB */
	{0x12, 0x01},			/*   9.  dB */
	{0xec, 0x00},			/*   9.5 dB */
	{0x32, 0x00},			/*  10.  dB */
	{0x21, 0x00},			/*  10.5 dB */
	{0x13, 0x00},			/*  11.  dB */
	{0x11, 0x00},			/*  11.5 dB */
	{0x0e, 0x00},			/*  12.  dB */
};

/*
 * This is the table for 2nd-stage play gain.
 * Note that this gain stage affects the monitor volume also.
 */
static struct aud_gainset gertab[] = {
	{0x1f, 0x43},			/*   5.  dB */
	{0x1f, 0x33},			/*   5.5 dB */
	{0xdd, 0x40},			/*   6.  dB */
	{0xdd, 0x11},			/*   6.5 dB */
	{0x0f, 0x44},			/*   7.  dB */
	{0x1f, 0x41},			/*   7.5 dB */
	{0x1f, 0x31},			/*   8.  dB */
	{0x20, 0x55},			/*   8.5 dB */
	{0xdd, 0x10},			/*   9.  dB */
	{0x11, 0x42},			/*   9.5 dB */
	{0x0f, 0x41},			/*  10.  dB */
	{0x1f, 0x11},			/*  10.5 dB */
	{0x0b, 0x60},			/*  11.  dB */
	{0xdd, 0x00},			/*  11.5 dB */
	{0x10, 0x42},			/*  12.  dB */
	{0x0f, 0x11},			/*  13.  dB */
	{0x00, 0x72},			/*  14.  dB */
	{0x10, 0x21},			/*  15.  dB */
	{0x00, 0x22},			/*  15.9 dB */
	{0x0b, 0x00},			/*  16.9 dB */
	{0x0f, 0x00},			/*  18.  dB */
};

#define	GR_GAINS	((sizeof (grtab) / sizeof (grtab[0])) - 1)
#define	GER_GAINS	(sizeof (gertab) / sizeof (gertab[0]))
#define	PLAY_GAINS	(GR_GAINS + GER_GAINS - 1)

/*
 * Convert play gain to chip values and load them.
 * Keep the 2nd stage gain at its lowest value if possible, so that monitor
 * gain isn't affected.
 * Return the closest appropriate gain value.
 */
static unsigned int
amd_play_gain(chip, val)
	struct aud_79C30_chip	*chip;
	unsigned int		val;
{
	int			gr;
	int			ger;

	ASSERT(chip != NULL);

	ger = 0;		/* assume constant 2nd stage gain */
	if (val == 0) {
		gr = -1;	/* first gain entry is infinite attenuation */
	} else {
		/* Scale gain range to available values */
		gr = ((val * PLAY_GAINS) + (PLAY_GAINS / 2)) /
		    (AUDIO_MAX_GAIN + 1);

		/* Scale back to full range for return value */
		if (val != AUDIO_MAX_GAIN)
			val = ((gr * AUDIO_MAX_GAIN) + (AUDIO_MAX_GAIN / 2)) /
			    PLAY_GAINS;

		/* If gr is off scale, increase 2nd stage gain */
		if (gr >= GR_GAINS) {
			ger = gr - GR_GAINS + 1;
			gr = GR_GAINS - 1;
		}
	}

	/* Load output gain registers */
	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_GR);
	chip->dr = grtab[++gr].coef[0];
	chip->dr = grtab[gr].coef[1];
	chip->cr = AUDIO_UNPACK_REG(AUDIO_MAP_GER);
	chip->dr = gertab[ger].coef[0];
	chip->dr = gertab[ger].coef[1];
	return (val);
}


/*
 * Convert record and monitor gain to chip values and load them.
 * Return the closest appropriate gain value.
 */
static unsigned
amd_gx_gain(chip, val, reg)
	struct aud_79C30_chip	*chip;
	unsigned		val;
	unsigned		reg;
{
	int			gr;

	ASSERT(chip != NULL);

	if (val == 0) {
		gr = -1;	/* first gain entry is infinite attenuation */
	} else {
		/* Scale gain range to available values */
		gr = ((val * GR_GAINS) + (GR_GAINS / 2)) /
		    (AUDIO_MAX_GAIN + 1);

		/* Scale back to full range for return value */
		if (val != AUDIO_MAX_GAIN)
			val = ((gr * AUDIO_MAX_GAIN) + (AUDIO_MAX_GAIN / 2)) /
			    GR_GAINS;
	}

	/* Load gx or stg registers */
	chip->cr = reg;
	chip->dr = grtab[++gr].coef[0];
	chip->dr = grtab[gr].coef[1];

	return (val);
}


#if defined(AMD_C_TRAP)

/* Redefine spl calls so that trace macros don't spl() from interrupt handler */
#define	splaudio()	0
#define	splx(X)		0

/*
 * Level 13 intr handler implements a pseudo DMA device for the AMD79C30.
 *
 * NOTE: This routine has a number of unreferenced labels corresponding to
 *	branch points in the assembly language routine (amd_intr.s).
 */
int
amd_intr()
{
	amd_dev_t		*devp;
	struct aud_79C30_chip	*chip;
	struct aud_cmd		*cmdp;
	int			int_active;
#define	Interrupt	1
#define	Active		2

	/*
	 * Figure out which chip interrupted.
	 * Since we only have one chip, we punt and assume device zero.
	 *
	 * XXX - how would we differentiate between chips??
	 */
	devp = &amd_devices[0];
	chip = devp->chip;
	int_active = chip->ir;		/* clear interrupt condition */

	int_active = 0;

	/*
	 * Process record IO
	 */
	if (devp->rec.active) {
		cmdp = devp->rec.cmdptr;

		/*
		 * A normal buffer must have at least 1 sample of data.
		 * A zero length buffer must have had the skip flag set.
		 * A skipped (possibly zero length) command block is used
		 * to synchronize the audio stream.
		 */
		while (cmdp != NULL && (cmdp->skip || cmdp->done)) {
			cmdp->done = TRUE;
			cmdp = cmdp->next;
			devp->rec.cmdptr = cmdp;
			int_active |= Interrupt;
		}

		/* Check for flow error */
		if (cmdp == NULL) {
			/* Flow error condition */
			devp->rec.error = TRUE;
			devp->rec.active = FALSE;
			int_active |= Interrupt;
			goto play;
		}

		/* Transfer record data */
		int_active |= Active;		/* note device active */
		devp->rec.samples++;		/* bump sample count */
		*cmdp->data++ = chip->bbrb;

		/* Notify driver of end of buffer condition */
		if (cmdp->data >= cmdp->enddata) {
			cmdp->done = TRUE;
			devp->rec.cmdptr = cmdp->next;
			int_active |= Interrupt;
		}
	}

play:
	if (devp->play.active) {
		cmdp = devp->play.cmdptr;

		/* Ignore null and non-data buffers */
		while (cmdp != NULL && (cmdp->skip || cmdp->done)) {
			cmdp->done = TRUE;
			cmdp = cmdp->next;
			devp->play.cmdptr = cmdp;
			int_active |= Interrupt;
		}

		/* Check for flow error */
		if (cmdp == NULL) {
			/* Flow error condition */
			devp->play.error = TRUE;
			devp->play.active = FALSE;
			int_active |= Interrupt;
			goto done;
		}

		/* Transfer play data */
		int_active |= Active;
		devp->play.samples++;
		chip->bbrb = *cmdp->data++;

		/* Notify driver of end of buffer condition */
		if (cmdp->data >= cmdp->enddata) {
			cmdp->done = TRUE;
			devp->play.cmdptr = cmdp->next;
			int_active |= Interrupt;
		}
	}

done:
	/* If no IO is active, shut down device interrupts */
	if (!(int_active & Active)) {
		chip->cr = AUDIO_UNPACK_REG(AUDIO_INIT_INIT);
		chip->dr = AUDIO_INIT_BITS_ACTIVE |
		    AUDIO_INIT_BITS_INT_DISABLED;
	}

	/* Schedule a lower priority interrupt, if necessary */
	if (int_active & Interrupt) {
#if defined(AUDIO_ASM)
		{
			extern int psr;

			*(unsigned char *)INTREG_ADDR |= IR_SOFT_INT4;
			psr = 0;
		}
#else
		{
			extern int set_intreg();

			set_intreg(IR_SOFT_INT4, TRUE);
		}
#endif
	}
	return (TRUE);
}
#undef	splaudio
#undef	splx

#endif /* AMD_C_TRAP */
