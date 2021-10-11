#ident "@(#) dbri.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

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
#include <sys/file.h>
#include <sys/debug.h>
#include <machine/intreg.h>
#include <machine/mmu.h>
#include <sundev/mbvar.h>

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>

#define	DBRI_UNSUPPORTED		/* Defines SETPORT, RELPORT */
#include <sun/isdnio.h>

#include <sun/dbriio.h>
#include <sbusdev/dbrivar.h>
#include "audiodebug.h"

unsigned int	dbri_monitor_gain();
unsigned int	dbri_outport();
unsigned int	dbri_play_gain();
unsigned int	dbri_record_gain();
unsigned char	dbri_output_muted();
void		dbri_chil_intr();
void		dbri_error_msg();
void		dbri_fxdt_intr();
void		mmcodec_init_pipes();
void		mmcodec_reset();
void		mmcodec_setup_ctrl_mode();
unsigned int	dbri_inport();
void		dbri_te_ph_activate_req();
void		dbri_nt_ph_activate_req();
void		dbri_nt_mph_deactivate_req();
void		dbri_pr_dbri_cmd();
void		dbri_xmitqcmd();
void		dbri_recvqcmd();
int		dbri_loopback();


/*
 * dbri_minortoas - Search channel table for a match on the minor device
 * number then grab the audio stream from that channel.
 */
aud_stream_t *
dbri_minortoas(driver, minordev)
	dbri_dev_t	*driver;
	int		minordev;
{
	int		i;

	for (i = 0; i < Nstreams; ++i) {
		if ((int) Chan_tab[i].minordev == minordev)
			break;
	}

	dprintf("dbri_minortoas: minordev = 0x%x chnl = %d \n",
		(unsigned int)minordev, i);
	if (i == Nstreams)
		return (NULL);

	return (&driver->ioc[i].as);
} /* dbri_minortoas */


/*
 * dbri_open - Set device structure ptr and call generic routine.
 */
int
dbri_open(q, dev, flag, sflag)
	queue_t		*q;
	dev_t		dev;
	int		flag;
	int		sflag;
{
	dbri_dev_t	*driver;
	aud_stream_t	*as;
	isdn_conn_req_t	conn;
	int		unit;
	dev_t		origdev;
	int		i;
	int		s;
	int		busy = 0;
	dbri_chan_tab_t	*ctep;	/* Channel Table Entry Pointer */
	int		error = 0;

	origdev = dev;
	unit = MINORUNIT(dev);

	if ((unit < 0) || (unit >= Ndbri)) {
		dprintf("dbri: Invalid unit number 0x%x.\n",
			(unsigned int)unit);
		u.u_error = ENODEV;
		return (OPENFAIL);
	}
	driver = &Dbri_devices[unit];

	if (driver->openinhibit) {
		/*
		 * This unit is unusable for some reason such as a hardware
		 * failure.
		 */
		u.u_error = ENXIO;
		return (OPENFAIL);
	}

	dprintf("dbri_open: q=0x%x, dev=0x%x, flag=0x%x, sflag=0x%x\n",
		(unsigned int)q, (unsigned int)dev, (unsigned int)flag,
		(unsigned int)sflag);

	/*
	 * XXX - This is necessary for use with the aclone driver
	 * since we *must* return a different minor number than
	 * was passed to us.  It could be implemented cleaner.
	 */
	if (MINORDEV(dev) > Nstreams) {
		/*
		 * XXX What is the logic for the minor number calculation
		 * XXX in this case?
		 */
		dev = dbrimakedev(major(dev), unit, MINORDEV(dev) - Nstreams);
	}

	/*
	 * Handle clone device opens
	 */
	if (sflag == CLONEOPEN) {
		switch (MINORDEV(dev)) {
		case 0:				/* Normal clone open */
		case DBRI_MINOR_AUDIO_RW:	/* Audio clone open */
			if (flag & FWRITE) {
				dev = dbrimakedev(major(dev), unit,
						    DBRI_MINOR_AUDIO_RW);
				break;
			}
			/* fall through */
		case DBRI_MINOR_AUDIO_RO:	/* Audio clone open */
			dev = dbrimakedev(major(dev), unit,
					    DBRI_MINOR_AUDIO_RO);
			break;

		default:
			u.u_error = ENODEV;
			return (OPENFAIL);
		}
	} else if (minor(dev) == 0) {
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
	
	/*
	 * Check for legal device.
	 */
	if ((as = dbri_minortoas(driver, MINORDEV(dev))) == NULL) {
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	/*
	 * "as" is not the play_as if the aud_stream is a read-only
	 * data device.
	 */
	if (ISDATASTREAM(as) && ((flag & (FREAD|FWRITE)) == FREAD)) {
		dev = dbrimakedev(major(dev), unit,
			as->record_as->info.minordev);
		if ((as = dbri_minortoas(driver, MINORDEV(dev))) == NULL) {
			u.u_error = ENODEV;
			return (OPENFAIL);
		}
	}

	/*
	 * If there are still open streams and initchip is FALSE this
	 * means that we have done a reset of DBRI. Do not allow any
	 * more streams to open the device until the last of the
	 * original streams finally closes the device.
	 */
	s = splr(driver->intrlevel);
	for (i = 0; i < Nstreams; ++i) {
		aud_stream_t	*asp;

		asp = &driver->ioc[i].as;
		if (asp->info.open)
			++busy;
	}
	(void) splx(s);
	if (busy && !driver->init_chip) {
		dprintf("dbri:\tChip reset. All open streams must\n");
		dprintf("\tbe closed before new open()'s.\n");
		ATRACE(dbri_open, '!rdy', busy);
		u.u_error = EBUSY;
		return (OPENFAIL);
	}

	if (audio_open(as, q, flag, sflag) == OPENFAIL) {
		ATRACE(dbri_open, 'fail', as);
		return (OPENFAIL);
	}

	if (as->writeq == NULL) {
		/* XXX This should never happen */
		ATRACE(dbri_open, ATR_WHY, as->writeq);
		u.u_error = EBUSY;
		call_debug("as->writeq == NULL");
		return (OPENFAIL);
	}

	/* Associate a stream with the appropriate hardware. */
	s = splr(driver->intrlevel);

	/*
	 * Loop thru conf table searching for matching minordev.
	 */
	for (i = 0, ctep = NULL; i < Nstreams; ++i) {
		if ((u_int)Chan_tab[i].minordev == MINORDEV(dev)) {
			ctep = &Chan_tab[i];
dprintf("Minor %d = Chan_tab[%d] {\
chan:%d, port:%d, bpipe:%d, cycle:%d, len:%d, dir:%d, \
mode:0x%x, samplerate:%d, channels:%d, in_as:%d, \
out_as:%d, control_as:%d, audtype:%d, minordev:%d, \
isdntype:%d, numbufs:%d, name:%s, input_size:%d}\n",
MINORDEV(dev), i,
ctep->chan, ctep->port, ctep->bpipe, ctep->cycle, ctep->len, ctep->dir,
ctep->mode, ctep->samplerate, ctep->channels, ctep->in_as, ctep->out_as,
ctep->control_as, ctep->audtype, ctep->minordev, ctep->isdntype, ctep->numbufs,
ctep->name, ctep->input_size);
			break;
		}
	}

	/*
	 * Minor number not found in configuration table.
	 */
	if (ctep == NULL) {
		u.u_error = ENODEV;
		return (OPENFAIL);
	}

	/*
	 * Form the defaults for a connection request here
	 */
	conn.unit = unit;
	conn.xmitport = ISDN_HOST;	/* read from host perspective */
	conn.recvport = ctep->port;
	if (ctep->dir == DBRI_OUT) {
		conn.dir = ISDN_TWOWAY_STREAM;
	} else {
		conn.dir = ISDN_ONEWAY_STREAM;
	}
	/*
	 * Convert DBRI mode from channel table to ISDN mode type
	 * to compose a generic ISDN connection request.
	 */
	switch (ctep->mode) {
	case DBRI_MODE_XPRNT:
		conn.mode = ISDN_MODE_TRANSPARENT;
		break;
	case DBRI_MODE_HDLC:
	case DBRI_MODE_HDLC_D:
		conn.mode = ISDN_MODE_HDLC;
		break;
	case DBRI_MODE_SER:
	case DBRI_MODE_UNKNOWN:
	default:
		conn.mode = ISDN_MODE_UNKNOWN;
		dprintf("dbri_open: unknown mode %x\n", ctep->mode);
		break;
	} /* switch */

	/*
	 * Adjustment for unidirectional write only connection request.
	 */
	if ((ctep->dir == DBRI_OUT) && ((flag & FREAD) == 0)) {
		conn.dir = ISDN_ONEWAY_STREAM;
		conn.xmitport = conn.recvport;	/* swap ports for write only */
		conn.recvport = ISDN_HOST;
	}

	if (ISDATASTREAM(as)) {
		/* connect up the h/w for a data stream */
		if (dbri_con_stream(as, &conn, &error) == DBRI_BADCONN) {
			ATRACEI(dbri_open, ATR_WHY, minor(dev));
			/* Close to clean up and indicate stream not busy */
			audio_close(as->writeq, 0); /* flag doesn't matter */
			u.u_error = error;
			return (OPENFAIL);
		}
	}

	ATRACEI(dbri_open, 'port', minor(dev));
	(void) splx(s);

	/*
	 * NOTE: If this is a cloneopen and the minor passed to us and
	 * the minor we return are the same, we will end up corrupting
	 * the vnode/snode structures and eventually crash the system.
	 */
	if (sflag == CLONEOPEN && dev == origdev) {
		printf("dbri_open: clone open returns original minor dev, good luck!\n");
	}
	ASSERT(q->q_ptr != NULL);

	ATRACEI(dbri_ioctl, 'dev ', dev);
	return (minor(dev));
} /* dbri_open */


void
dbri_close(as)
	aud_stream_t	*as;
{
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;
	int		s;

	dprintf("dbri_close: as=0x%x\n", (unsigned int)as);
	if (!ISDATASTREAM(as))
		return;		/* No h/w to disconnect so just return */

	s = splr(driver->intrlevel);

	/* XXX - check what cleanup needs to be done here */
	AsToDs(as)->samples = 0;
	AsToDs(as)->recv_eol = 0;
	AsToDs(as)->audio_uflow = 0;
	AsToDs(as)->last_flow_err = 0;
	bzero((caddr_t)&AsToDs(as)->d_stats, (unsigned)sizeof(dbri_stat_t));

	/*
	 * Disassociate the stream from the HW.
	 */
	(void) dbri_xcon_stream(as, (isdn_conn_req_t *) NULL);
	(void) splx(s);
	return;
} /* dbri_close */


/*
 * Process ioctls not already handled by the generic audio handler.
 */
aud_return_t
dbri_ioctl(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	aud_return_t	change;
	isdn_conn_req_t	*conp;
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;
	int		s;
	int		error;

	dprintf("dbri_ioctl:  as=%x, control_as=%x\n", (unsigned int)as,
	    (unsigned int)as->control_as);

	s = splr(driver->intrlevel);
	change = AUDRETURN_NOCHANGE;	/* detect device state change */
	ATRACEI(dbri_ioctl, 'ioct', iocp->ioc_cmd);

	switch (iocp->ioc_cmd) {
	case AUDIO_GETDEV:
		{
		int		*ip;

		ip = ((int *)mp->b_cont->b_rptr);
		mp->b_cont->b_wptr += sizeof (int);
		*ip = mmcodec_getdev(driver);
		iocp->ioc_count = sizeof (int);
		}
		break;

	case ISDN_PH_ACTIVATE_REQ:
		as = as->play_as;		/* get control stream */
		dprintf("before test activation request\n");

		if (!ISHWCONNECTED(as)) {
			dprintf("dbri: error in D-ch act. req. (no pipe)\n");
			iocp->ioc_error = EINVAL;
			break;
		}

		if (AsToDs(as)->i_var.power != ISDN_PH_PARAM_POWER_ON) {
			iocp->ioc_error = ENXIO;
			break;
		} else if (ISTEDCHANNEL(as)) {
			audio_sendsig(as); /* XXX broken D-ch protocol */
			dbri_te_ph_activate_req(as);	/* Activate TE */
		} else if (ISNTDCHANNEL(as)) {
			dbri_nt_ph_activate_req(as);	/* Activate NT */
		} else {
			iocp->ioc_error = EINVAL;
			break;
		}
		break;

	case ISDN_MPH_DEACTIVATE_REQ:
		as = as->play_as;		/* get control stream */

		if (AsToDs(as)->i_var.power != ISDN_PH_PARAM_POWER_ON) {
			iocp->ioc_error = ENXIO;
			break;
		} else if (ISNTDCHANNEL(as) && ISHWCONNECTED(as)) {
			dbri_nt_mph_deactivate_req(as);	/* Deactivate NT */
		} else {
			/*
			 * There is no such thing as PH_DEACTIVATE_REQ
			 * for TEs, so this handles that also.
			 */
			iocp->ioc_error = EINVAL;
			break;
		}
		break;

#ifdef	DBRI_UNSUPPORTED
	case ISDN_SETPORT:			/* connect stream to h/w */
		conp = (isdn_conn_req_t *) mp->b_cont->b_rptr;
		ATRACEI(dbri_ioctl, 'setp', (int) conp);
/*
 * XXX This needs to be uncommented out as soon as sparcphone uses DBRIMGT
 * 		if (as != &driver->ioc[DBRI_DBRIMGT].as) {
 * 			iocp->ioc_error = EPERM;
 * 			break;
 * 		} 
 */
		if (dbri_con_stream(as, conp, &error) == DBRI_BADCONN) {
			dprintf("dbri: Setup port request failed\n");
			iocp->ioc_error = error;
			break;
		}
		change = AUDRETURN_CHANGE;
		break;

	case ISDN_RELPORT:	/* disconnect stream from h/w */
		conp = (isdn_conn_req_t *)mp->b_cont->b_rptr;
		ATRACEI(dbri_ioctl, 'relp', (int) conp);
/* 
 * XXX This needs to be uncommented out as soon as sparcphone uses DBRIMGT
 * 		if (as != &driver->ioc[DBRI_DBRIMGT].as) {
 * 			iocp->ioc_error = EPERM;
 * 			break;
 * 		} 
 */
		if (dbri_xcon_stream(as, conp) == DBRI_BADCONN) {
			dprintf("dbri: Release port request failed\n");
			iocp->ioc_error = EINVAL;
			break;
		}
		change = AUDRETURN_CHANGE;
		break;
#endif	DBRI_UNSUPPORTED

	case ISDN_SET_PARAM:
		{
		isdn_ph_param_t *ip;
#define IOCEQ(x) (as == driver->ioc[(int)x].as.play_as)
#define IOCEQMGT(x) (as == driver->ioc[(int)x].as.control_as)

		ATRACEI(dbri_ioctl, 'dev ', as->info.minordev);

		ip = ((isdn_ph_param_t *)mp->b_cont->b_rptr);
		switch (ip->tag) {
		case ISDN_NT_PH_PARAM_T101:	/* Set */
			if (IOCEQ(DBRI_NT_D_OUT) || IOCEQMGT(DBRI_NTMGT)) {
				AsToDs(as)->i_var.t101 = ip->value.ms;
				change = AUDRETURN_CHANGE;
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
		case ISDN_NT_PH_PARAM_T102:	/* Set */
			if (IOCEQ(DBRI_NT_D_OUT) || IOCEQMGT(DBRI_NTMGT)) {
				AsToDs(as)->i_var.t102 = ip->value.ms;
				change = AUDRETURN_CHANGE;
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
		case ISDN_TE_PH_PARAM_T103:	/* Set */
			if (IOCEQ(DBRI_TE_D_OUT) || IOCEQMGT(DBRI_TEMGT)) {
				AsToDs(as)->i_var.t103 = ip->value.ms;
				change = AUDRETURN_CHANGE;
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
		case ISDN_TE_PH_PARAM_T104:	/* Set */
			if (IOCEQ(DBRI_TE_D_OUT) || IOCEQMGT(DBRI_TEMGT)) {
				AsToDs(as)->i_var.t104 = ip->value.ms;
				change = AUDRETURN_CHANGE;
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
		case ISDN_PH_PARAM_MAINT:	/* Set */
			if (IOCEQMGT(DBRI_TEMGT) || IOCEQMGT(DBRI_NTMGT)) {
				dbri_stream_t	*ds;

				ds = AsToDs(as);
				if (IOCEQMGT(DBRI_TEMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_TE_D_OUT]
						.as.play_as);
				else if (IOCEQMGT(DBRI_NTMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_NT_D_OUT]
						.as.play_as);
				switch (ip->value.maint) {
				case ISDN_PH_PARAM_MAINT_OFF:
				case ISDN_PH_PARAM_MAINT_ECHO:
				case ISDN_PH_PARAM_MAINT_ON:
					ds->i_var.maint = ip->value.maint;
					change = AUDRETURN_CHANGE;
				default:
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
					break;
				}
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
		case ISDN_PH_PARAM_ASMB:	/* Set */
		{
			/*
			 * If optional behaviors are allowed for NT side,
			 * then add more conditionals here.
			 */
			dbri_stream_t	*ds;

			if (!IOCEQMGT(DBRI_TEMGT)) {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
				break;
			}

			ds = AsToDs(driver->ioc[(int)DBRI_TE_D_OUT].as.play_as);
			switch (ip->value.maint) {
			case ISDN_TE_PH_PARAM_ASMB_CCITT88:
			case ISDN_TE_PH_PARAM_ASMB_CTS2:
				ds->i_var.asmb = ip->value.asmb;
				change = AUDRETURN_CHANGE;
				break;
			default:
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
				break;
			}
			break;
		}

		case ISDN_PH_PARAM_POWER:	/* Set */
			if ( IOCEQMGT(DBRI_TEMGT) || IOCEQMGT(DBRI_NTMGT)) {
				dbri_stream_t	*ds;

				ds = AsToDs(as);
				if (IOCEQMGT(DBRI_TEMGT)) {
					ds = AsToDs(
						driver->ioc[(int)DBRI_TE_D_OUT]
						.as.play_as);
				} else if (IOCEQMGT(DBRI_NTMGT)) {
					ds = AsToDs(
						driver->ioc[(int)DBRI_NT_D_OUT]
						.as.play_as);
				}

				switch (ip->value.flag) {
				case ISDN_PH_PARAM_POWER_OFF:
				case ISDN_PH_PARAM_POWER_ON:
					if (ds->i_var.power != ip->value.flag) {
						(void)dbri_power(ds, ip->value.flag);
						change = AUDRETURN_CHANGE;
					}
					break;
				default:
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
					break;
				}
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;

		case ISDN_PH_PARAM_PAUSE:	/* Pause IO */
			{
			/*
			 * XXX Do B1 and B1_56 point to the
			 * XXX same place?
			 */
			switch (ip->value.pause.port) {
			case ISDN_NT_B1:
			case ISDN_NT_B2:
			case ISDN_NT_H:
				if (IOCEQMGT(DBRI_DBRIMGT)
				    || IOCEQMGT(DBRI_TEMGT)) {
					as = driver->ioc[(int)ip->value.pause.port].as.play_as;
				} else {
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
				}
				break;

			case ISDN_TE_B1:
			case ISDN_TE_B2:
			case ISDN_TE_H:
				if (IOCEQMGT(DBRI_DBRIMGT)
				    || IOCEQMGT(DBRI_TEMGT)) {
					as = driver->ioc[(int)ip->value.pause.port].as.play_as;
				} else {
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
				}
				break;

			default:
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
				break;
			}

			if (iocp->ioc_error != EINVAL) {
				if (ip->value.pause.paused) {
					audio_pause_record(as);
					audio_pause_play(as);
				} else {
					audio_resume_record(as);
					audio_resume_play(as);
				}
			}
			break;
			}
#ifdef DBRI_UNSUPPORTED
		case ISDN_PH_PARAM_GLITCH:	/* DBRI specific, unsupported */
			{
			unsigned int	save_sts;

			if (IOCEQ(DBRI_TE_D_OUT) || IOCEQ(DBRI_NT_D_OUT)) {
				save_sts = driver->chip->n.sts;
				if (IOCEQ(DBRI_TE_D_OUT)) {
					driver->chip->n.sts &= ~DBRI_STS_T;
					DELAY((int)ip->value.us);
					driver->chip->n.sts = save_sts;
				} else {
					driver->chip->n.sts &= ~DBRI_STS_N;
					DELAY((int)ip->value.us);
					driver->chip->n.sts = save_sts;
				}
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
			}

		case ISDN_PH_PARAM_NORESTART:	/* DBRI specific, unsupported */
			{
			if (IOCEQ(DBRI_TE_D_OUT) || (IOCEQ(DBRI_NT_D_OUT))) {
				AsToDs(as)->i_var.norestart =
					(ip->value.flag != 0);
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;
			}
#endif DBRI_UNSUPPORTED
		default:
			iocp->ioc_error = EINVAL;
			change = AUDRETURN_NOCHANGE;
			break;
		}
		break;
		}

	case ISDN_GET_PARAM:
		{
		isdn_ph_param_t *ip;

		ATRACEI(dbri_ioctl, 'dev ', as->info.minordev);

		ip = ((isdn_ph_param_t *)mp->b_cont->b_rptr);
		iocp->ioc_count = sizeof (isdn_ph_param_t);

		switch (ip->tag) {
		case ISDN_NT_PH_PARAM_T101:	/* Query */
			if (IOCEQ(DBRI_NT_D_OUT) || IOCEQ(DBRI_NTMGT)) {
				/* XXX get value from D-Ch */
				ip->value.ms = AsToDs(as)->i_var.t101;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
		case ISDN_NT_PH_PARAM_T102:	/* Query */
			if (IOCEQ(DBRI_NT_D_OUT) || IOCEQMGT(DBRI_NTMGT)) {
				/* XXX get value from D-Ch */
				ip->value.ms = AsToDs(as)->i_var.t102;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
		case ISDN_TE_PH_PARAM_T103:	/* Query */
			if (IOCEQ(DBRI_TE_D_OUT) || IOCEQMGT(DBRI_TEMGT)) {
				/* XXX get value from D-Ch */
				ip->value.ms = AsToDs(as)->i_var.t103;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
		case ISDN_TE_PH_PARAM_T104:	/* Query */
			if (IOCEQ(DBRI_TE_D_OUT) || IOCEQMGT(DBRI_TEMGT)) {
				/* XXX get value from D-Ch */
				ip->value.ms = AsToDs(as)->i_var.t104;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
		case ISDN_PH_PARAM_MAINT:	/* Query */
			if (IOCEQMGT(DBRI_TEMGT) || IOCEQMGT(DBRI_NTMGT)) {
				dbri_stream_t	*ds;

				ds = AsToDs(as);
				if (IOCEQMGT(DBRI_TEMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_TE_D_OUT]
						.as.play_as);
				else if (IOCEQMGT(DBRI_NTMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_NT_D_OUT]
						.as.play_as);
				ip->value.maint = ds->i_var.maint;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
		case ISDN_PH_PARAM_ASMB:	/* Query */
		{
			dbri_stream_t	*ds;

			if (!IOCEQMGT(DBRI_TEMGT)) {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
				break;
			}

			ds = AsToDs(driver->ioc[(int)DBRI_TE_D_OUT].as.play_as);
			ip->value.asmb = ds->i_var.asmb;
			break;
		}

		case ISDN_PH_PARAM_POWER:	/* Query */
			if (IOCEQMGT(DBRI_TEMGT) || IOCEQMGT(DBRI_NTMGT)) {
				dbri_stream_t	*ds;

				ds = AsToDs(as);
				if (IOCEQMGT(DBRI_TEMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_TE_D_OUT]
						.as.play_as);
				else if (IOCEQMGT(DBRI_NTMGT))
					ds = AsToDs(
						driver->ioc[(int)DBRI_NT_D_OUT]
						.as.play_as);
				ip->value.flag = ds->i_var.power;
			} else {
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
			}
			break;

		case ISDN_PH_PARAM_PAUSE:	/* Query paused state */
			{
			/*
			 * XXX Do B1 and B1_56 point to the
			 * XXX same place?
			 */
			switch (ip->value.pause.port) {
			case ISDN_NT_B1:
			case ISDN_NT_B2:
			case ISDN_NT_H:
				if (IOCEQMGT(DBRI_DBRIMGT)
				    || IOCEQMGT(DBRI_TEMGT)) {
					as = driver->ioc[(int)ip->value.pause.port].as.play_as;
				} else {
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
				}
				break;

			case ISDN_TE_B1:
			case ISDN_TE_B2:
			case ISDN_TE_H:
				if (IOCEQMGT(DBRI_DBRIMGT)
				    || IOCEQMGT(DBRI_TEMGT)) {
					as = driver->ioc[(int)ip->value.pause.port].as.play_as;
				} else {
					iocp->ioc_error = EINVAL;
					change = AUDRETURN_NOCHANGE;
				}
				break;

			default:
				iocp->ioc_error = EINVAL;
				change = AUDRETURN_NOCHANGE;
				break;
			}

			if (iocp->ioc_error != EINVAL) {
				if (as->play_as->info.pause !=
				    as->record_as->info.pause) {
					/* XXX This does not seem good */
					printf("isdn pause play != record\n");;
				}
				ip->value.pause.paused = as->info.pause;
			}
			break;
			}

#ifdef DBRI_UNSUPPORTED
		case ISDN_PH_PARAM_NORESTART:	/* Query */
			{
			if (IOCEQ(DBRI_TE_D_OUT) || (IOCEQ(DBRI_NT_D_OUT))) {
				ip->value.flag = AsToDs(as)->i_var.norestart;
			} else {
				iocp->ioc_error = EINVAL;
			}
			break;
			}
#endif /* DBRI_UNSUPPORTED */

		default:
			iocp->ioc_error = EINVAL;
			break;
		}
		break;
		}

	case ISDN_MESSAGE_SET:
		{
		int		*ip;

		ip = ((int *)mp->b_cont->b_rptr);

		/*
		 * XXX - If stream is not a D-Channel, or other control
		 * channel, then this ioctl should probably be rejected.
		 */

		if (IOCEQMGT(DBRI_TEMGT) || IOCEQMGT(DBRI_NTMGT)) {
			as = as->play_as;	/* Get D-channel as */

			switch (*ip) {
			case ISDN_MESSAGE_SET_SIGNAL:
				AsToDs(as)->i_var.message_type = 0;
				break;
			case ISDN_MESSAGE_SET_PROTO:
				AsToDs(as)->i_var.message_type = M_PROTO;
				break;
			default:
				iocp->ioc_error = EINVAL;
				break;
			}
		} else {
			/* Not TE or NT so it's invalid */
			iocp->ioc_error = EINVAL;
			break;
		}
		}
		break;

	case ISDN_ACTIVATION_STATUS:			/* get ISDN status */
		{
		isdn_activation_status_t	*ip;

		ip = ((isdn_activation_status_t *)mp->b_cont->b_rptr);

		switch (ip->type) {
		case ISDN_TYPE_SELF:	/* This stream */
			as = as->play_as;
			if ((as !=
			    driver->ioc[(int)DBRI_TE_D_OUT].as.play_as) &&
			    (as != driver->ioc[(int)DBRI_NT_D_OUT].as.play_as))
				iocp->ioc_error = EINVAL;
			break;

		case ISDN_TYPE_TE:
			as = driver->ioc[(int)DBRI_TE_D_OUT].as.play_as;
			break;

		case ISDN_TYPE_NT:
			as = driver->ioc[(int)DBRI_NT_D_OUT].as.play_as;
			break;

		default:
			iocp->ioc_error = EINVAL;
			break;
		}

		if (iocp->ioc_error != EINVAL) {
			ip->type = AsToDs(as)->i_info.type;
			ip->activation = AsToDs(as)->i_info.activation;
			iocp->ioc_count = sizeof (isdn_activation_status_t);
		} else {
			ip->type = ISDN_TYPE_UNKNOWN;
		}
		}
		break;

	case ISDN_STATUS:			/* get ISDN status */
		{
		isdn_info_t	*ip;
		aud_stream_t	*das;

		ip = ((isdn_info_t *)mp->b_cont->b_rptr);

		ATRACEI(dbri_ioctl, 'dev ', as->info.minordev);

		switch (ip->type) {
		case ISDN_TYPE_SELF:	/* Self */
			switch (AsToDs(as->play_as)->i_info.type) {
			case ISDN_TYPE_TE:
				das = driver->ioc[(int)DBRI_TE_D_OUT].as.play_as;
				break;

			case ISDN_TYPE_NT:
				das = driver->ioc[(int)DBRI_NT_D_OUT].as.play_as;
				break;
			default:
				iocp->ioc_error = EINVAL;
				break;
			}
			break;

		case ISDN_TYPE_TE:
			das = driver->ioc[(int)DBRI_TE_D_OUT].as.play_as;
			break;

		case ISDN_TYPE_NT:
			das = driver->ioc[(int)DBRI_NT_D_OUT].as.play_as;
			break;

		default:
			iocp->ioc_error = EINVAL;
			break;
		}

		if (iocp->ioc_error != EINVAL) {
			iocp->ioc_count = sizeof (isdn_info_t);
			*ip = AsToDs(as)->i_info;

			ip->type = AsToDs(as)->i_info.type;

			ip->activation = AsToDs(das)->i_info.activation;
			ip->ph_ai = AsToDs(das)->i_info.ph_ai;
			ip->ph_di = AsToDs(das)->i_info.ph_di;
			ip->mph_ai = AsToDs(das)->i_info.mph_ai;
			ip->mph_di = AsToDs(das)->i_info.mph_di;
			ip->mph_ei1 = AsToDs(das)->i_info.mph_ei1;
			ip->mph_ei2 = AsToDs(das)->i_info.mph_ei2;
			ip->mph_ii_c = AsToDs(das)->i_info.mph_ii_c;
			ip->mph_ii_d = AsToDs(das)->i_info.mph_ii_d;

			ip->iostate = AsToDs(as)->i_info.iostate;

			printf("play xmit %d, record xmit %d\n",
				AsToDs(as)->i_info.transmit.packets,
				AsToDs(as->record_as)->i_info.transmit.packets);

			ip->transmit.packets =
				AsToDs(as)->i_info.transmit.packets;
			ip->transmit.octets =
				AsToDs(as)->i_info.transmit.octets;
			ip->transmit.errors =
				AsToDs(as)->i_info.transmit.errors;
			ip->receive.packets =
				AsToDs(as->record_as)->i_info.receive.packets;
			ip->receive.octets =
				AsToDs(as->record_as)->i_info.receive.octets;
			ip->receive.errors =
				AsToDs(as->record_as)->i_info.receive.errors;

		}
		}
		break;

	case ISDN_SET_LOOPBACK:
		{
		isdn_loopback_request_t	*lr;

		lr = ((isdn_loopback_request_t *)mp->b_cont->b_rptr);

		if (dbri_loopback(as, lr, 0) == FALSE) {
			iocp->ioc_error = EINVAL;
			break;
		}
		}
		break;

	case ISDN_RESET_LOOPBACK:
		{
		isdn_loopback_request_t	*lr;

		lr = ((isdn_loopback_request_t *)mp->b_cont->b_rptr);

		if (dbri_loopback(as, lr, 1) == FALSE) {
			iocp->ioc_error = EINVAL;
			break;
		}
		}
		break;

#ifdef	DBRI_UNSUPPORTED
	case ISDN_MODPORT:			/* mod predefined port */
			/* XXX - nothing for now */
#endif	DBRI_UNSUPPORTED

	default:
		dprintf("dbri: Unsupported ioctl\n");
		ATRACEI(dbri_ioctl, 'bioc', iocp->ioc_cmd);
		iocp->ioc_error = EINVAL;
		break;
	} /* switch */

	(void) splx(s);
	return (change);
} /* dbri_ioctl */


/*
 * dbri_start - used to start/resume reads or writes for an entire list.
 *
 * NOTE: This is not called to CONTINUE lists from dbri_queuecmd.
 */
void
dbri_start(as)
	aud_stream_t		*as;
{
	int		s;
	dbri_chip_cmd_t	dbricmd;
	pipe_tab_t	*pipep;
	dbri_dev_t	*driver;
	dbri_cmd_t	*dcp;

	s = splstr();

	if ((AsToDs(as)->cmdptr == NULL) || as->info.pause) {
		/* No commands queued up */
		ATRACEI(dbri_start, ATR_NONE, as->info.pause);
		(void) splx(s);
		return;
	}
	if ((AsToDs(as)->pipe == DBRI_BAD_PIPE)) {
		/* Bad ju-ju */
		ATRACEI(dbri_start, 'Bpip', as);
		(void) splx(s);
		return;
	}
	ATRACEI(dbri_start, '[go]', AsToDs(as)->pipe);

	driver = (dbri_dev_t *)as->v->devdata;
	pipep = &driver->ptab[AsToDs(as)->pipe];
	pipep->sdp &= ~DBRI_SDP_CMDMASK;	/* kill off any cmd bits set */
	/* XXXPAS potentially kill of CLR bit */
	pipep->sdp |= (DBRI_SDP_PTR | DBRI_SDP_CLR);

	dbricmd.cmd_wd1 = pipep->sdp;

	/*
	 * Scan list for 1st non-complete MD.
	 *
	 * Note: the first thing on the chain could be a done, skip, etc.
	 *
	 * Note: This is necessary for the transmit direction
	 * because when paused, we end up with 'done' descriptors
	 * and the next dbri_start will have to skip over them.
	 */
	for (dcp = AsToDs(as)->cmdptr;
	    dcp != NULL;
	    dcp = dcp->nextio) {

		/*
		 * cmd.lastfragment is not set until packets are
		 * assembled in the ISR.
		 */
		if (dcp->cmd.skip || dcp->cmd.done)
			continue;
		if (dcp->md.r.f.com)
			continue;

		break;
	}

	if (dcp == NULL) {
		ATRACEI(dbri_start, 'bore', 0);
		dprintf("dbri_start: pipe%u, nothing interesting to do\n",
			AsToDs(as)->pipe);
		(void) splx(s);
		return;
	}

	dprintf("dbri_start: sdp %u\n", AsToDs(as)->pipe);

	dbricmd.cmd_wd2 = (unsigned int)DBRI_ADDR(driver, &dcp->md.t);
	dbri_command(driver, dbricmd);		/* issue SDP command */
	ATRACEI(dbri_start, 'sdp ', dbricmd.cmd_wd2);

	dbricmd.cmd_wd1 = DBRI_CMD_PAUSE;	/* PAUSE command */
	dbri_command(driver, dbricmd);

	as->info.active = TRUE;

	/* On receive when we finally issue a SDP, clear eolflag */
	if (AsToDs(as)->recv_eol)
		AsToDs(as)->recv_eol = FALSE;

	(void) splx(s);
	return;
} /* dbri_start */


/*
 * dbri_stop - stop reads or writes. Simply issues a SDP command with a
 * NULL pointer causing data to stop at the end of the current buffer.
 */
void
dbri_stop(as)
	aud_stream_t	*as;
{
	dbri_chip_cmd_t	cmd;
	pipe_tab_t	*pipep;
	dbri_dev_t	*driver;
	int		s;

	s = splstr();

	if (AsToDs(as)->cmdptr == NULL) {
		(void) splx(s);
		return;				/* No commands queued up */
	}
	if ((AsToDs(as)->pipe == DBRI_BAD_PIPE)) {
		/* Bad ju-ju */
		ATRACEI(dbri_stop, 'Bpip', as);
		(void) splx(s);
		return;
	}

	driver = (dbri_dev_t *)as->v->devdata;
	pipep = &driver->ptab[AsToDs(as)->pipe];

	if (pipep->sdp == 0) {
		ATRACEI(dbri_stop, '-na-', AsToDs(as)->pipe);
		(void) splx(s);
		return;
	}
	ATRACEI(dbri_stop, 'STOP', AsToDs(as)->pipe);

	pipep->sdp &= ~DBRI_SDP_CMDMASK;	/* kill off any cmd bits set */
	pipep->sdp |= (DBRI_SDP_PTR | DBRI_SDP_CLR);
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;			/* NULL address */
	dbri_command(driver, cmd);		/* issue SDP command */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	as->info.active = FALSE;
	AsToDs(as)->recv_eol = TRUE;	/* for start to get called later */

	/* XXX There is a delay in dbri_flushcmd before releasing memory */

	(void) splx(s);
	return;
} /* dbri_stop */


/* dbri_setflag - Get or set a particular flag value */
unsigned int
dbri_setflag(as, op, val)
	aud_stream_t		*as;
	enum aud_opflag		op;
	unsigned int		val;
{
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;

	switch (op) {
	case AUD_ERRORRESET:
		{
		int	s;

		s = splr(driver->intrlevel); /* reset error flag atomically */
		val = AsToDs(as)->audio_uflow;
		AsToDs(as)->audio_uflow = FALSE;
		(void) splx(s);
		break;
		}

	/* GET only */
	case AUD_ACTIVE:
		/*
		 * XXX - need to use status here like OVRN/UNDR and if
		 * not paused. Actually, a new routine.
		 */
		val = as->info.active;
		break;
	}

	return (val);
} /* dbri_setflag */


/*
 * dbri_setinfo - Get or set device-specific information in the
 *	audio state structure. Returns TRUE if there is an error.
 */
aud_return_t
dbri_setinfo(as, mp, iocp)
	aud_stream_t		*as;
	mblk_t			*mp;
	struct iocblk		*iocp;
{
	int		s;
	dbri_stream_t	*iocp_out;
	dbri_stream_t	*iocp_in;
	dbri_stream_t	*iocp_as;
	dbri_dev_t	*driver;
	audio_info_t	*ip;
	int		error, set_config;
	unsigned int	sample_rate, channels, precision, encoding;
	unsigned int	gain;
	unsigned char	balance;

	error = 0;
	driver = (dbri_dev_t *)(as->v->devdata);

	iocp_out = AsToDs(as->play_as);
	iocp_in = AsToDs(as->record_as);
	iocp_as = AsToDs(as);

	/* Set device-specific info into device-independent structure */

	iocp_out->as.info.samples = iocp_out->samples;
	iocp_in->as.info.samples = iocp_in->samples;

	/* If getinfo, 'mp' is NULL...we're done */
	if (mp == NULL)
		return (AUDRETURN_NOCHANGE); 		/* no error */

	s = splr(driver->intrlevel);	/* load chip registers atomically */
	ip = (audio_info_t *)mp->b_cont->b_rptr;

	if (iocp_out == &driver->ioc[(int) DBRI_AUDIO_OUT]) {
		if ((Modify(ip->play.gain)) || (Modifyc(ip->play.balance))) {
			if (Modify(ip->play.gain))
				gain = ip->play.gain;
			else
				gain = iocp_out->as.info.gain;
			if (Modifyc(ip->play.balance))
				balance = ip->play.balance;
			else
				balance = iocp_out->as.info.balance;
			iocp_out->as.info.gain =
				dbri_play_gain(driver, gain, balance);
			iocp_out->as.info.balance = balance;
		}

		if (Modifyc(ip->output_muted)) {
			driver->hwi_state.output_muted =
				dbri_output_muted(driver, ip->output_muted);
		}

		if (Modify(ip->play.port)) {
			iocp_out->as.info.port =
					dbri_outport(driver, ip->play.port);
		}
	}

	if (iocp_in == &driver->ioc[(int) DBRI_AUDIO_IN]) {
		if ((Modify(ip->record.gain))||(Modifyc(ip->record.balance))) {
			if (Modify(ip->record.gain))
				gain = ip->record.gain;
			else
				gain = iocp_in->as.info.gain;
			if (Modifyc(ip->record.balance))
				balance = ip->record.balance;
			else
				balance = iocp_in->as.info.balance;
			iocp_in->as.info.gain =
				dbri_record_gain(driver, gain, balance);
			iocp_in->as.info.balance = balance;
		}

		if (Modify(ip->monitor_gain)) {
			driver->hwi_state.monitor_gain =
				dbri_monitor_gain(driver, ip->monitor_gain);
		}
		if (Modify(ip->record.port)) {
			switch (ip->record.port) {
			case AUDIO_MICROPHONE:
			case AUDIO_LINE_IN:
				iocp_in->as.info.port =
					dbri_inport(driver, ip->record.port);
				break;
			default:
				error = EINVAL;
				break;
			}
		}
	}

	/*
	 * Set the sample counters atomically, returning the old values.
	 */
	if (iocp_out->as.info.open) {
		iocp_out->as.info.samples = iocp_out->samples;
		if (Modify(ip->play.samples))
			iocp_out->samples = ip->play.samples;
	}

	if (iocp_in->as.info.open) {
		iocp_in->as.info.samples = iocp_in->samples;
		if (Modify(ip->record.samples))
			iocp_in->samples = ip->record.samples;
	}

	/*
	 * First, get the right values to use. XXX - as an optimization,
	 * if all new values matching existing ones, we should just 
	 * return
	 */
	if (Modify(ip->play.sample_rate))
		sample_rate = ip->play.sample_rate;
	else if (Modify(ip->record.sample_rate))
		sample_rate = ip->record.sample_rate;
	else
		sample_rate = iocp_as->as.info.sample_rate;
	if (Modify(ip->play.channels))
		channels = ip->play.channels;
	else if (Modify(ip->record.channels))
		channels = ip->record.channels;
	else
		channels = iocp_as->as.info.channels;
	if (Modify(ip->play.precision))
		precision = ip->play.precision;
	else if (Modify(ip->record.precision))
		precision = ip->record.precision;
	else
		precision = iocp_as->as.info.precision;
	if (Modify(ip->play.encoding))
		encoding = ip->play.encoding;
	else if (Modify(ip->record.encoding))
		encoding = ip->record.encoding;
	else
		encoding = iocp_as->as.info.encoding;

	/*
	 * Now, if setting the same existing format, or not touching
	 * the "big four" parameters, don't do anything
	 */
	if ((sample_rate == iocp_as->as.info.sample_rate) &&
			(channels == iocp_as->as.info.channels) &&
			(precision == iocp_as->as.info.precision) &&
			(encoding == iocp_as->as.info.encoding)) {

		set_config = FALSE;
	} else if (Modify(ip->play.sample_rate) || Modify(ip->play.precision) ||
		Modify(ip->play.channels) || Modify(ip->play.encoding)) {

		/*
		 * So, a process wants to modify the play format - check
		 * and see if another process has it open for recording.
		 * If not, see if the new requested config is possible
		 * (Can't make changes on the control device however)
		 */
		if (iocp_in->as.info.open && iocp_out->as.info.open &&
				(iocp_in->as.readq != iocp_out->as.readq)) {
			dprintf("setinfo:play change but already recording\n");
			error = EBUSY;
		} else if ((iocp_as != iocp_in) && (iocp_as != iocp_out)) {
			error = EINVAL;
		} else {
			set_config = TRUE;
			error = mmcodec_check_audio_config(driver,
				sample_rate, channels, precision, encoding);
		}
	} else if (Modify(ip->record.sample_rate) ||
		Modify(ip->record.precision) ||
		Modify(ip->record.channels) ||
		Modify(ip->record.encoding)){

		/*
		 * A process wants to modify the recording format - check
		 * to see if someone else is playing. If not, see if the
		 * new requested config is possible
		 */
		if (iocp_in->as.info.open && iocp_out->as.info.open &&
				(iocp_in->as.readq != iocp_out->as.readq)) {
			dprintf("setinfo:record change but already playing\n");
			error = EBUSY;
		} else if ((iocp_as != iocp_in) && (iocp_as != iocp_out)) {
			error = EINVAL;
		} else {
			set_config = TRUE;
			error = mmcodec_check_audio_config(driver,
				sample_rate, channels, precision, encoding);
		}
	} else {
		set_config = FALSE;		/* don't know what to do */
	}
	if (!error && set_config) {
		/*
		 * Update the "real" info structure (and others) accordingly
		 */
		ip->play.sample_rate = ip->record.sample_rate = sample_rate;
		ip->play.channels = ip->record.channels = channels;
		ip->play.precision = ip->record.precision = precision;
		ip->play.encoding = ip->record.encoding = encoding;
		mmcodec_set_audio_config(driver, as,
				sample_rate, channels, precision, encoding);
		/*
		 * Don't ack the ioctl until it's actually done; save the
		 * mp on this stream and when it's finished, then ack
		 */
		iocp_as->mp = mp;
		splx(s);
		return (AUDRETURN_DELAYED);
	}
	(void) splx(s);
	iocp->ioc_error = error;
	return (AUDRETURN_CHANGE);
}


/*
 * This routine is called whenever a new packet is added to the cmd chain.
 * It ties in the chip specific message descriptors then tells the chip to go.
 */
void
dbri_queuecmd(as, cmdp)
	aud_stream_t	*as;
	aud_cmd_t	*cmdp;
{
	dbri_dev_t	*driver = (dbri_dev_t *)(as->v->devdata);
	int		s;

	s = splr(driver->intrlevel);

	if (as == as->play_as) {
		dbri_xmitqcmd(as, cmdp); /* fill and chain up xmit md's */
	} else if (as == as->record_as) {
		dbri_recvqcmd(as, cmdp); /* fill and chain up recv md's */
	} else {
		(void) printf("dbriqueuecmd: Non play or record as = 0x%x\n",
			(unsigned int)as);
	}

	(void) splx(s);
	return;
} /* dbri_queuecmd */


/*
 * dbri_recvqcmd - Fill in a receive md's in the dbri command structure
 */
void
dbri_recvqcmd(as, cmdp)
	aud_stream_t	*as;
	aud_cmd_t	*cmdp;
{
	dbri_cmd_t	*dcp;
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;

	ATRACEI(dbri_recvqcmd, 'strt', cmdp);
	for (dcp = (dbri_cmd_t *)cmdp;
	    dcp != NULL;
	    dcp = (dbri_cmd_t *)dcp->cmd.next) {

		/*
		 * Initialize bit fields and pointers
		 */
		dcp->nextio = NULL;
		dcp->md.r.word32[0] = 0;	/* clear out all bits */
		dcp->md.r.f.bufp = 0;		/* buffer pointer */
		dcp->md.r.f.fp = NULL; 		/* forward md pointer */
		dcp->md.r.word32[3] = 0;	/* clear out all bits */

		/* Set bit fields and pointers */
#ifdef	sun4m
		/* XXX - add failure recovery code */
#ifndef lint	/* XXX lint barfs on arg 1 */
		dcp->sb_addr = (caddr_t)mb_nbmapalloc((struct map *)dvmamap,
			(caddr_t)dcp->cmd.data,
			as->input_size,
			MDR_BIGSBUS|MB_CANTWAIT,
			(func_t)0, (caddr_t)0);
#endif /* lint */
		if (dcp->sb_addr == (caddr_t)0) {
			printf("dbri_recvqcmd: out of DVMA resources!\n");
			return;
		}
		dcp->md.r.f.bufp = (unsigned char *)dcp->sb_addr;
#else	sun4m
		dcp->md.r.f.bufp = dcp->cmd.data; /* buffer pointer */
#endif	/* sun4m */
		dcp->md.r.f.fint = 1;		/* Frame interrupt */

		ASSERT((dcp->cmd.enddata - dcp->cmd.data) >= as->input_size);

		dcp->md.r.f.bcnt = as->input_size;

		if (AsToDs(as)->cmdptr == NULL) {
			AsToDs(as)->cmdptr = dcp;
			AsToDs(as)->cmdlast = dcp;
			ATRACEI(dbri_recvqcmd, ATR_FIRST, AsToDs(as)->pipe);
		} else {
			AsToDs(as)->cmdlast->md.r.f.fp =
				(dbri_rmd_t *)DBRI_ADDR(driver, &(dcp->md.r));
			/*
			 * The dbri_cmd_t is now visible to DBRI.  Append
			 * to end of IO list.
			 */
			ATRACEI(dbri_recvqcmd, 'next', AsToDs(as)->cmdlast);
			AsToDs(as)->cmdlast->nextio = dcp;
			AsToDs(as)->cmdlast = dcp;
		}
	}

	/*
	 * Don't start IO if paused.
	 * Only when EOL is set do we issue a SDP.
	 */
	if (!as->info.pause && (AsToDs(as)->recv_eol))
		dbri_start(as);		/* issues SDP with new chain */

	return;
} /* dbri_recvqcmd */


/*
 * dbri_xmitqcmd - Fill in a transmit md's in the dbri command structure.
 *
 * Currently chucks packets when D-channels are not activated.
 */
void
dbri_xmitqcmd(as, cmdp)
	aud_stream_t	*as;
	aud_cmd_t	*cmdp;
{
	dbri_cmd_t	*dcp;	/* general purpose DBRI command pointer */
	struct {
		dbri_cmd_t	*head;		/* 1st fragment in packet */
		dbri_cmd_t	*tail;		/* last fragment in packet */
	}	packet;
	dbri_dev_t	*driver;		/* Driver data structure */
	int	activated;			/* Serial interface state */
	enum {NOTSET, IDLE, CONTINUE, START} action; /* Action needed */
	pipe_tab_t	*pipep;
	int		notify;

	action = NOTSET;
	packet.head = NULL;
	packet.tail = NULL;
	driver = (dbri_dev_t *)as->v->devdata;

	/*
	 * Determine the state of the serial interface
	 */
	pipep = &driver->ptab[AsToDs(as)->pipe];
	activated = dbri_chnl_activated(as, pipep->xchan, pipep->rchan);

	/*
	 * Process a chain of one or more fragments making a single
	 * packet.
	 *
	 * audio_process_play() hides zero length fragments that are
	 * part of a non-zero length packet.
	 */
	ATRACEI(dbri_xmitqcmd, 'pipe', AsToDs(as)->pipe);

	for (dcp = (dbri_cmd_t *)cmdp;
	    dcp != NULL;
	    dcp = (dbri_cmd_t *)dcp->cmd.next) {
		int	fraglen; /* length of a frag */

		dcp->nextio = NULL;
		fraglen = dcp->cmd.enddata - dcp->cmd.data;
		if (fraglen > DBRI_MAXPACKET) {
			(void) printf("dbri: fraglen is %d\n", fraglen);
			ASSERT(fraglen <= DBRI_MAXPACKET);
		}

		if (dcp->cmd.done || dcp->cmd.skip || !activated) {
			dcp->cmd.done = TRUE;
			dcp->cmd.skip = TRUE;

			dcp->md.t.word32[0] = 0; /* Clear bit fields */
			dcp->md.t.f.bufp = NULL; /* data */
			dcp->md.t.f.fp = NULL; /* forward md pointer */
			dcp->md.t.f.status = 0;	/* status */

			if (!activated) {
				ATRACEI(dbri_xmitqcmd, '!act', dcp);
			} else {
				ATRACEI(dbri_xmitqcmd, 'skip', dcp);
			}
		} else {
			/*
			 * This fragment is part of a packet to transmit.
			 */
			dcp->md.t.word32[0] = 0; /* Clr out bit fields */
			dcp->md.t.f.fp = NULL; 	/* forward md pointer */
			dcp->md.t.f.status = 0;	/* status */
			dcp->md.t.f.cnt = fraglen;
			dcp->md.t.f.idl = 0; /* 1st frag gets fixed later */
			dcp->md.t.f.fcnt = 0; /* 1st frag gets fixed later */

			ATRACEI(dbri_xmitqcmd, 'dcp ', dcp);
			ATRACEI(dbri_xmitqcmd, 'frag', fraglen);

#ifdef	sun4m
#ifndef lint	/* XXX - lint barfs on arg 1 */
			dcp->sb_addr = (caddr_t)mb_nbmapalloc(
				(struct map *)dvmamap,
				(caddr_t)dcp->cmd.data,
				fraglen,
				MDR_BIGSBUS|MB_CANTWAIT,
				(func_t)0, (caddr_t)0);
#endif /* lint */
			if (dcp->sb_addr == (caddr_t)0) {
				printf("dbri_xmitqcmd: out of DVMA space!\n");
				return;
			}
			dcp->md.t.f.bufp = (unsigned char *)dcp->sb_addr;
#else	/* sun4m */
			dcp->md.t.f.bufp = dcp->cmd.data;
#endif	/* sun4m */

			vac_flush((caddr_t)dcp->cmd.data,
			    dcp->cmd.enddata - dcp->cmd.data);

			ASSERT(dcp->cmd.lastfragment != NULL);

			if (&dcp->cmd == dcp->cmd.lastfragment) {
				dcp->md.t.f.eof = 1;	/* Tag last pkt */
				dcp->md.t.f.fint = 1;	/* Frame intr */
			}

			/*
			 * Link the fragments together
			 */
			if (packet.head == NULL) {
				packet.head = dcp;
				packet.tail = dcp;
				packet.tail->md.t.f.fp = 0;
			} else {
				packet.tail->nextio = dcp;
				packet.tail->md.t.f.fp =
					(dbri_tmd_t *)DBRI_ADDR(driver,
								&dcp->md.t);
				packet.tail = dcp;
			}

			if (Dbri_debug > 1)
				dbri_pr_dbri_cmd(dcp, 0);
		}
	}

	if (packet.head) {
		if (ISTEDCHANNEL(as)) {
			/*
			 * CCITT Fascicle III.8 - Rec. I.430, p180, 6.1.4
			 *
			 * The priority is set on the first packet fragment.
			 * Always negotiate access.
			 */
			packet.head->md.t.f.idl = 1;

			/*
			 * Use the correct priority for this SAPI.
			 * SAPI 0, call control, uses Class 1.
			 * All other SAPIs get Class 2.
			 */
			if ((packet.head->cmd.data[0] & 0xfc) == 0)
				packet.head->md.t.f.fcnt = DBRI_D_CLASS_1;
			else
				packet.head->md.t.f.fcnt = DBRI_D_CLASS_2;
		} else {
			ASSERT(packet.tail && packet.tail->md.t.f.eof);
			/*
			 * The "fcnt" field only takes effect in fragments
			 * with eof set and HDLC mode.
			 *
			 * For HDLC mode, send idles between frames.
			 */
			packet.tail->md.t.f.idl = 1;
			packet.tail->md.t.f.fcnt = 1;
		}

		if (AsToDs(as)->cmdptr == NULL) {
			AsToDs(as)->cmdptr = packet.head;
			AsToDs(as)->cmdlast = packet.tail;
			ATRACEI(dbri_xmitqcmd, ATR_FIRST, packet.head);
			action = START;
		} else {
			ATRACEI(dbri_xmitqcmd, 'apnd', packet.head);
			ATRACEI(dbri_xmitqcmd, '(to)', AsToDs(as)->cmdlast);
			AsToDs(as)->cmdlast->nextio = packet.head;

			AsToDs(as)->cmdlast->md.t.f.fp =
				(dbri_tmd_t *)DBRI_ADDR(driver,
							&packet.head->md.t);

			AsToDs(as)->cmdlast = packet.tail;
			action = CONTINUE;
		}
		if (packet.tail->md.t.f.eof != 1) {
			dprintf("NO EOF! head=%x tail=%x\n",
				(unsigned int)packet.head,
				(unsigned int)packet.tail);
			call_debug("no eof");
		}
	} else {
		action = IDLE;
		ATRACEI(dbri_xmitqcmd, 'nada', AsToDs(as)->pipe);
	}

	if (as->info.pause) {		/* don't start IO if paused */
		action = IDLE;
	}

	/* Give md's to chip to start or continue I/O */
	switch (action) {
	case CONTINUE:
		{
			dbri_chip_cmd_t	cmd;

			ATRACEI(dbri_xmitqcmd, 'cdp ', AsToDs(as)->pipe);
			cmd.cmd_wd1 = DBRI_CMD_CDP | AsToDs(as)->pipe;
			dbri_command(driver, cmd); /* issue CDP command */

			as->info.active = TRUE;
		}
		break;

	case START:
		dbri_start(as);		/* issues SDP with new chain */
		break;

	case IDLE:
		/* XXX - need to insure that garbage is collected */
		notify = audio_xmit_garbage_collect(as);
		if (notify)
			audio_sendsig(as->control_as);
		break;
	case NOTSET:
		(void) printf("dbri: dbri_xmitqcmd with no action given\n");
		break;
	}

	return;
} /* dbri_xmitqcmd */


/*
 * dbri_flushcmd - Flush the device's notion of queued commands.
 * AUD_STOP() mush be called before this routine.
 */
void
dbri_flushcmd(as)
	aud_stream_t	*as;
{
	dbri_cmd_t	*dcp;

	dprintf("dbri_flush: pipe=%d\n", AsToDs(as)->pipe);

	/*
	 * XXX - This delay is bogus and needs to be reworked but makes
	 *	things a little safer for now. It's purpose is to insure
	 *	that all SBus accesses by the chip are complete *BEFORE*
	 *	freeing up memory.
	 */
	DELAY(250);
	for (dcp = AsToDs(as)->cmdptr;
	    dcp != NULL;
	    dcp = dcp->nextio) {
#ifdef	sun4m
		if (dcp->sb_addr != 0) {
#ifndef lint	/* XXX lint barfs on arg 1 */
			mb_mapfree(dvmamap, (int *)&dcp->sb_addr);
#endif /* lint */
		}
#endif	sun4m
	}

	AsToDs(as)->cmdptr = NULL;
	AsToDs(as)->cmdlast = NULL;

	return;
} /* dbri_flushcmd */


/*
 * Send a M_ERROR message up the specified stream.
 */
void
dbri_error_msg(as, msg)
	aud_stream_t	*as;	/* always points to write side */
	int		msg;
{
	mblk_t		*notify_mp;

	/* If stream is not open, simply return */
	if (as->readq == 0)
		return;

	/* Init a message to send a SIGPOLL upstream */
	if ((notify_mp = allocb(sizeof (char), BPRI_HI)) == NULL)
		return;

	notify_mp->b_datap->db_type = msg;
	*notify_mp->b_wptr++ = SIGPOLL;

	/* Signal the specified stream */
	putnext(as->readq, notify_mp);

	return;
} /* dbri_error_msg */


/*
 * TE sends: PH-AI, PH-DI, MPH-AI, MPH-DI, MPH_EI1, MPH_EI2, MPH_II_C, MPH_II_D
 * NT sends: PH_AI, PH_DI, MPH_AI, MPH_DI, MPH_EI
 */

/*
 * Send a CCITT PH or MPH indication upstream.
 */
void
dbri_send_primitive(as, message)
	aud_stream_t		*as;
	isdn_message_type_t	message;
{
	mblk_t			*primitive_mp;
	dbri_stream_t		*ds = AsToDs(as);
	isdn_message_t		*ip;

	if (ds->i_var.message_type != M_PROTO) {
		audio_sendsig(as);
		if (as != as->control_as)
			audio_sendsig(as->control_as);
		return;
	}

	if ((primitive_mp = allocb(sizeof (isdn_message_t), BPRI_HI)) == NULL) {
		ATRACE(dbri_send_primitive, 'fail', as);
		return;
	}

	primitive_mp->b_datap->db_type = ds->i_var.message_type;
	ip = (isdn_message_t *)primitive_mp->b_wptr;
	primitive_mp->b_wptr += sizeof (*ip);

	bzero((caddr_t)ip, sizeof (*ip));
	ip->magic = ISDN_PROTO_MAGIC;
	ip->type = ds->i_info.type;
	ip->message = message;

	if (as->control_as != NULL && as->control_as->info.open) {
		as = as->control_as;
		putnext(as->readq, primitive_mp);
		ATRACE(dbri_send_primitive, 'sent', as);
	}

	return;
}


/*
 * MPH-ACTIVATE Indication
 */
void
dbri_primitive_mph_ai(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_ai;
	ATRACE(dbri_primitive_mph_ai, 'prim', as);
	dbri_send_primitive(as, MPH_AI);
	return;
}


/*
 * MPH-DEACTIVATE Indication
 */
void
dbri_primitive_mph_di(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_di;
	ATRACE(dbri_primitive_mph_di, 'prim', as);
	dbri_send_primitive(as, MPH_DI);
	return;
}


void
dbri_primitive_mph_ei1(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_ei1;
	ATRACE(dbri_primitive_mph_ei1, 'prim', as);
	dbri_send_primitive(as, MPH_EI1);
	return;
}


void
dbri_primitive_mph_ei2(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_ei2;
	ATRACE(dbri_primitive_mph_ei2, 'prim', as);
	dbri_send_primitive(as, MPH_EI2);
	return;
}


void
dbri_primitive_mph_ii_c(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_ii_c;
	ATRACE(dbri_primitive_mph_ii_c, 'prim', as);
	dbri_send_primitive(as, MPH_II_C);
	return;
}


void
dbri_primitive_mph_ii_d(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.mph_ii_d;
	ATRACE(dbri_primitive_mph_ii_d, 'prim', as);
	dbri_send_primitive(as, MPH_II_D);
	return;
}


void
dbri_primitive_ph_ai(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.ph_ai;
	ATRACE(dbri_primitive_ph_ai, 'prim', as);
	dbri_send_primitive(as, PH_AI);
	return;
}


void
dbri_primitive_ph_di(as)
	aud_stream_t    *as;
{
	++AsToDs(as)->i_info.ph_di;
	ATRACE(dbri_primitive_ph_ai, 'prim', as);
	dbri_send_primitive(as, PH_DI);
	return;
}


/*
 * dbri_loopback - Set up a loopback (flag = 0) or clear loopback
 * (flag = 1).
 */
int
dbri_loopback(as, lr, flag)
	aud_stream_t		*as;
	isdn_loopback_request_t	*lr;
	int			flag;
{
	int		lb_bits; /* loopback bits for NT or TE channel */
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;
	serialp = &driver->ser_sts;
	lb_bits = 0;

	/*
	 * The request must occur on a D channel on either the NT or the
	 * TE interface.
	 */
	if (!ISTEDCHANNEL(as) && !ISNTDCHANNEL(as))
		return (FALSE);

	/*
	 * Check for a request that specifies channels we are not
	 * supporting.
	 */
	if (lr->channels &
	    ~(ISDN_LOOPBACK_D | ISDN_LOOPBACK_B1 | ISDN_LOOPBACK_B2))
		return (FALSE);

	/*
	 * Make a mask for the proper loopback bits in the NT/TE command
	 */
	if (lr->channels & (int)ISDN_LOOPBACK_D)
		lb_bits |= DBRI_NTE_LLB_D;
	if (lr->channels & (int)ISDN_LOOPBACK_B1)
		lb_bits |= DBRI_NTE_LLB_B1;
	if (lr->channels & (int)ISDN_LOOPBACK_B2)
		lb_bits |= DBRI_NTE_LLB_B2;

	switch (lr->type) {
	case ISDN_LOOPBACK_LOCAL:
		/* Bits are already okay, don't move them */
		break;

	case ISDN_LOOPBACK_REMOTE:
		/* Shift bits over 3 positions to get Remote mask */
		lb_bits <<= 3;
		break;

	default:
		return (FALSE);
	}

	/*
	 * Build a DBRI TE or NT command and execute it.
	 */
	if (ISTEDCHANNEL(as)) {
		if (flag)
			serialp->te_cmd &= ~(lb_bits);
		else
			serialp->te_cmd |= lb_bits;
		cmd.cmd_wd1 = serialp->te_cmd;
	} else if (ISNTDCHANNEL(as)) {
		if (flag)
			serialp->nt_cmd &= ~(lb_bits);
		else
			serialp->nt_cmd |= lb_bits;
		cmd.cmd_wd1 = serialp->nt_cmd;
	} else {
		return (FALSE);
	}
	dbri_command(driver, cmd);

	return (TRUE);
} /* dbri_loopback */
