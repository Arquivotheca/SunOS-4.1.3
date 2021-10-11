#ident  "@(#) dbri_subr.c 1.1@(#) Copyright (c) 1989-92 Sun Microsystems, Inc."

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/limits.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/ioccom.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/file.h>

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>
#include <sun/isdnio.h>
#include <sun/dbriio.h>
#include <sbusdev/dbrivar.h>
#include "audiodebug.h"

unsigned int	dbri_monitor_gain();
unsigned int	dbri_outport();
unsigned int	dbri_play_gain();
unsigned int	dbri_record_gain();
void		dbri_chil_intr();
void		dbri_error_msg();
void		dbri_fxdt_intr();
void		mmcodec_init_pipes();
void		mmcodec_reset();
void		mmcodec_setup_ctrl_mode();
void		dbri_te_unplug();
void		dbri_primitive_mph_ei2();
void		dbri_primitive_mph_ii_c();
void		dbri_primitive_ph_di();
void		dbri_primitive_mph_di();
void		dbri_primitive_mph_ei2();
void		dbri_primitive_mph_ei2();
void		dbri_primitive_mph_ei2();
void		dbri_bri_down();
void		dbri_bri_up();

extern char	*dbri_channel_name();
extern char	*dbri_mode_name();


/*
 * Reset the DBRI and MMCODEC chips to a known safe state.  Chip needs to
 * be in a fully reset state. This is currently called at attach time and
 * unload time. When called from attach the streams have just been setup
 * but nothing is connected. At UNLOAD time the chip could really be in
 * any state.
 */
void
dbri_initchip(driver)
	dbri_dev_t	*driver;
{
	int		i;
	int		unload;
	dbri_reg_t	*chip = driver->chip;
	dbri_intq_t	*qp, *nqp;
	pipe_tab_t	*pipep;
	dbri_chip_cmd_t	cmd;

	unload = driver->init_chip;		/* load or unload? */
	chip->n.sts = DBRI_STS_R;		/* reset DBRI chip */
	driver->init_chip = FALSE;

	/*
	 * Kill off all timers
	 */
	driver->keep_alive_running = FALSE;
	untimeout((int(*)())dbri_keep_alive, (caddr_t)driver);

	pipep = &driver->ptab[DBRI_PIPE_TE_D_IN];
	if (ISPIPEINUSE(pipep))
		untimeout((int(*)())dbri_te_timer,
			    (caddr_t)pipep->as->play_as);
	pipep = &driver->ptab[DBRI_PIPE_NT_D_IN];
	if (ISPIPEINUSE(pipep))
		untimeout((int(*)())dbri_nt_timer,
			    (caddr_t)pipep->as->play_as);

	/*
	 * Clear out structures.  This also intializes the command
	 * queue to all DBRI WAIT's since the WAIT opcode is 0
	 */
	bzero((caddr_t)driver->intq.intq_bp,
		DBRI_NUM_INTQS * sizeof (dbri_intq_t));
	bzero((caddr_t)driver->cmdqp, sizeof (dbri_chip_cmdq_t));

	/* Circularly chain up the intr queues and set status ptrs */
	qp = driver->intq.intq_bp;
	for (i = 0; i < (DBRI_NUM_INTQS - 1); ++i, qp++) {
		nqp = qp + 1;	/* can't do this calc in DBRI_ADDR */
		qp->nextq = (dbri_intq_t *)DBRI_ADDR(driver, nqp);
	}
	/* last pts back to 1st */
	qp->nextq = (dbri_intq_t *)DBRI_ADDR(driver, driver->intq.intq_bp);

	driver->intq.curqp = driver->intq.intq_bp;
	driver->intq.off = 0;

	/* Now set status pointers for the command queue */
	driver->cmdqp->cmdhp = 0;
	driver->init_cmdq = FALSE;

	bzero((caddr_t) &driver->ser_sts, sizeof (serial_status_t));
	bzero((caddr_t) driver->ptab, sizeof (driver->ptab));
	for (i = 0; i < 10; i++) {			/* loop up to 2.5ms */
		DELAY(250);				/* don't bog down bus */
		if (!(chip->n.sts & DBRI_STS_R)) 	/* brk on reset done */
			break;
	}
	if (chip->n.sts & DBRI_STS_R) {
		(void) printf("DBRI: error, unable to reset chip\n");
		return;
	}
	driver->init_chip = TRUE;
	/*
	 * If we are unloading the driver, just return
	 */
	if (unload)
		return;

	mmcodec_reset(driver);

	dprintf("dbri_attach: intr queue setup\n");
	cmd.cmd_wd1 = DBRI_CMD_IIQ;
	cmd.cmd_wd2 = ((unsigned int)DBRI_ADDR(driver, driver->intq.curqp));
	dbri_command(driver, cmd);	/* Enable interrupts */

	chip->n.sts |= (DBRI_STS_G|DBRI_STS_E);

	return;
} /* dbri_initchip */


void
dbri_pr_conn_req(crp)
	dbri_conn_req_t	*crp;
{
	dprintf("xchan=%s rchan=%s mode=%s\n",
		dbri_channel_name((int)crp->xchan),
		dbri_channel_name((int)crp->rchan),
		dbri_mode_name(crp->mode));
	return;
}


/*
 * dbri_con_stream - the top level routine that is called to connect
 * up a system level stream. This involves determining what individual
 * DBRI h/w streams must be setup and invoking the routines to actually
 * configure the h/w and then pointing them to the software streams.
 * Called from both dbri_ioctl and dbri_open. Note that the open call
 * must form it's own conp struct.
 *
 * NOTE: There is no need to add support for fixed data pipes.
 */
int
dbri_con_stream(as, conp, error)
	aud_stream_t	*as;
	isdn_conn_req_t	*conp;
	int		*error;
{
	unsigned int	pipe;
	unsigned int	opipe;
	aud_stream_t	*oas;
	pipe_tab_t	*pipep;
	dbri_conn_req_t	dcon;
	dbri_dev_t	*driver;
	dbri_stream_t	*dsp;
	aud_stream_t	*p_as, *r_as;

	*error = 0;
	if ((conp->unit < 0) || (conp->unit >= Ndbri)) {
		dprintf("dbri: Unit # %d invalid \n", conp->unit);
		ATRACEI(dbri_con_stream, 'badu', as);
		*error = ENODEV;
		return (DBRI_BADCONN);
	}
	driver = &Dbri_devices[conp->unit];
	dprintf("dbri_con_stream: driver address=0x%x \n",
		(unsigned int)driver);

	/* Check the port values to insure that they are sane */
	if (((int)conp->xmitport >= (int)ISDN_LAST_PORT) ||
	    ((int)conp->xmitport < (int)ISDN_PORT_NONE) ||
	    ((int)conp->recvport >= (int)ISDN_LAST_PORT) ||
	    ((int)conp->recvport < (int)ISDN_PORT_NONE)) {
		dprintf("dbri: Invalid transmit or receive port\n");
		ATRACEI(dbri_con_stream, 'badp', conp);
		*error = ENODEV;
		return (DBRI_BADCONN);
	}

	/*
	 * Setup DBRI h/w specific connection request from generic ISDN
	 * connection request.
	 */
	dcon.xchan = Port_tab[(int)conp->xmitport].xchan;
	dcon.rchan = Port_tab[(int)conp->recvport].rchan;

	/*
	 * Assign mode
	 *
	 * NB: this doesn't deal with fixed pipes
	 */
	if ((conp->xmitport != ISDN_HOST) && (conp->recvport != ISDN_HOST)) {
		dcon.mode = DBRI_MODE_SER;
	} else if (conp->mode == ISDN_MODE_TRANSPARENT) {
		dcon.mode = DBRI_MODE_XPRNT;
	} else {
		/* Grab mode of channel not connected to host from table */
		if (dcon.xchan == DBRI_HOST_CHNL)
			dcon.mode = Chan_tab[(int)dcon.rchan].mode;
		else
			dcon.mode = Chan_tab[(int)dcon.xchan].mode;
	}

	dprintf("dbri_con_stream: as=0x%x as->out=0x%x as->in=0x%x\n",
		(unsigned int)as, (unsigned int)as->play_as,
		(unsigned int)as->record_as);

	if (ISDATASTREAM(as)) {
		/* Get appropriate input or output stream for data streams */
		as = (dcon.xchan == DBRI_HOST_CHNL) ? as->record_as
						    : as->play_as;
	}

	/* Check sanity of connection request */
	dprintf("dbri_con_stream: first as=0x%x\n", (unsigned int)as);
	dbri_pr_conn_req(&dcon);

	if (dbri_check_req(as, conp, &dcon, error) == DBRI_BADCONN) {
		ATRACEI(dbri_con_stream, 'badc', as);
		return (DBRI_BADCONN);
	}

	/* If audio setup CHI interface first */
	if (ISAUDIOCONNECTION(dcon.xchan) || ISAUDIOCONNECTION(dcon.rchan)) {
		/* 
		 * For a serial to serial connection on the control stream
		 * we need to mark the audio streams as using serial to 
		 * serial pipes so that DBRI can be the master when we
		 * get a fxdt interrupt.
		 */
		if (ISCONTROLSTREAM(as)) {
			driver->ser_sts.chi_flags |= CHI_FLAG_SER_STS;
			audio_ser_to_ser_config(as, driver);
			/*
			 * XXX Could be a race if connection succeeds after
			 * this times out.
			 */
			if (driver->ser_sts.chi_state != CHI_IN_DATA_MODE) {
				driver->ser_sts.chi_flags &= ~CHI_FLAG_SER_STS;
				*error = ENODEV;
				return (DBRI_BADCONN);
			}
		} else {
			driver->ser_sts.chi_flags &= ~CHI_FLAG_SER_STS;
			if (audio_con_stream(driver, as, error) == DBRI_BADCONN)
				return (DBRI_BADCONN);
		}
	}

	/* connect up DBRI serial side of pipe */
	if ((pipe = dbri_setup_chnl(as, &dcon, error)) == DBRI_BAD_PIPE) {
		if (ISCONTROLSTREAM(as) && 
		    (ISAUDIOCONNECTION(dcon.xchan) || 
		    ISAUDIOCONNECTION(dcon.rchan))) {
			driver->ser_sts.chi_flags &= ~CHI_FLAG_SER_STS;
			audio_ser_to_ser_config(as, driver);
		}
		ATRACEI(dbri_con_stream, 'badp', as);
		return (DBRI_BADCONN);
	}

	pipep = &driver->ptab[pipe];
	AsToDs(as)->pipe = pipe;	/* save pipe index */
					/* XCARE for serial mode connections */
	pipep->as = as;			/* point back to as structure */
	pipep->xchan = dcon.xchan;
	pipep->rchan = dcon.rchan;
	as->mode = (dcon.mode == DBRI_MODE_XPRNT) ? AUDMODE_AUDIO
						    : AUDMODE_HDLC;

	dsp = &driver->ioc[(int)dcon.xchan];
	dprintf("dbri_con_stream: pipe=%u, dsp=0x%x, head=0x%x, tail=0x%x\n",
		pipe, (unsigned int)dsp, (unsigned int)&dsp->cmdptr,
		(unsigned int)&dsp->cmdlast);

	/* for two way stream need to connect another pipe in other dir */
	if (conp->dir == ISDN_TWOWAY_STREAM) {
		/*
		 * swap dir of channels requested and adjust mode for
		 * TE-D out
		 */
		dcon.rchan = Port_tab[(int)conp->xmitport].rchan;
		dcon.xchan = Port_tab[(int)conp->recvport].xchan;

		/*
		 * D-channel only case where modes are different in each
		 * dir
		 */
		if (dcon.xchan == DBRI_TE_D_OUT)
			dcon.mode = Chan_tab[(int)dcon.xchan].mode;

		opipe = pipe;	/* save old pipe and "as" for error case */
		oas = as;
		/* For serial to serial pipes leave "as" intact */
		if (dcon.mode != DBRI_MODE_SER) {
			as = (dcon.xchan == DBRI_HOST_CHNL) ? as->record_as
							    : as->play_as;
		}
		dprintf("dbri_con_stream: second as=0x%x\n", (unsigned int)as);

		pipe = dbri_setup_chnl(as, &dcon, error);
		if (pipe == DBRI_BAD_PIPE) {
			dbri_remove_dts(oas, opipe);	/* xcon 1st pipe */
			pipep = &driver->ptab[opipe];
			AsToDs(oas)->pipe = DBRI_BAD_PIPE;
			pipep->as = (aud_stream_t *)NULL;
			pipep->xchan = DBRI_NO_CHNL;
			pipep->rchan = DBRI_NO_CHNL;
			oas->mode = AUDMODE_NONE;
			ATRACEI(dbri_con_stream, 'bdp2', as);
			if (ISCONTROLSTREAM(as) && 
			    (ISAUDIOCONNECTION(dcon.xchan) || 
			    ISAUDIOCONNECTION(dcon.rchan))) {
				driver->ser_sts.chi_flags &= ~CHI_FLAG_SER_STS;
				audio_ser_to_ser_config(as, driver);
			}
			return (DBRI_BADCONN);
		}
		pipep = &driver->ptab[pipe];
		AsToDs(as)->pipe = pipe;	/* save pipe index */
					/* XCARE for serial mode connections */
		pipep->as = as;		/* point back to as structure */
		pipep->xchan = dcon.xchan;
		pipep->rchan = dcon.rchan;
		as->mode = (dcon.mode == DBRI_MODE_XPRNT) ? AUDMODE_AUDIO
							    : AUDMODE_HDLC;
	}

	/* Marks ports that serial to serial connections use as busy */
	if (dcon.mode == DBRI_MODE_SER) {
		/* set first direction */
		p_as = &driver->ioc[(int)dcon.xchan].as;
		r_as = &driver->ioc[(int)dcon.rchan].as;

		p_as->play_as->info.open = TRUE;
		r_as->record_as->info.open = TRUE;

		/* if bidirectional set second direction */
		if (conp->dir == ISDN_TWOWAY_STREAM) {
			p_as = &driver->ioc[(int)dcon.rchan].as;
			r_as = &driver->ioc[(int)dcon.xchan].as;

			p_as->play_as->info.open = TRUE;
			r_as->record_as->info.open = TRUE;
		}
	}

	return (DBRI_GOODCONN);
} /* dbri_con_stream */


/*
 * dbri_xcon_stream - disconnect a stream from the hardware.  For a
 * serial to serial connection 2 way stream all the disconnect work is
 * done in this routine. For a data stream 2 way connection, this routine
 * is called twice by the close routine.
 */
int
dbri_xcon_stream(as, conp)
	aud_stream_t	*as;
	isdn_conn_req_t *conp;			/* NULL for data streams */
{
	int		s;
	unsigned int	pipe;
	pipe_tab_t 	*pipep;
	dbri_chnl_t	xchan, rchan;
	dbri_dev_t	*driver;
	aud_stream_t	*p_as, *r_as;

	s = splstr();
	if (ISDATASTREAM(as)) {
		int	result;

		/* data stream so get information from as struct */
		if (!ISHWCONNECTED(as)) {
			(void) splx(s);
			return (DBRI_BADCONN);
		}

		/*
		 * For a data stream only a single direction stream is
		 * dismantled at a time as called by the close routine.
		 */
		driver = (dbri_dev_t *) as->v->devdata;

		/*
		 * If this is a D channel and the pipe doesn't point to
		 * itself, there are other pipes connected. Therefore,
		 * disconnect those pipes too.
		 */
		pipe = AsToDs(as)->pipe;
		ATRACEI(dbri_xcon_stream, 'data', pipe);
		pipep = &driver->ptab[pipe];

		if (ISTEDCHANNEL(as) || ISNTDCHANNEL(as)) {
			int		inpipe;
			unsigned int	opipe;
			aud_stream_t	*oas;

			/*
			 * Disconnect all channels hooked to the
			 * particular D-channel and then send a
			 * signal upstream with M_ERROR set.
			 */
			inpipe = (pipep->dts.r.vi == DBRI_DTS_VI)? TRUE: FALSE;
			while ((inpipe && (pipep->dts.r.oldin != pipe)) ||
			    (!inpipe && (pipep->dts.r.oldout != pipe))) {
				if (inpipe)
					opipe = pipep->dts.r.oldin;
				else
					opipe = pipep->dts.r.oldout;
				oas = driver->ptab[opipe].as;
				if (dbri_disconnect(driver, oas, opipe) ==
				    DBRI_BADCONN) {
					dprintf("dbrixcon: failed.\n");
					(void) splx(s);
					return (DBRI_BADCONN);
				}
				/*
				 * Now disable any further reading or writing
				 * to the stream. Check if we want to also
				 * send a M_HANGUP message upstream.
				 */
				dbri_error_msg(oas, M_ERROR);
			}
		}
		result = dbri_disconnect(driver, as, pipe);
		(void) splx(s);
		return (result);
	}

	/*
	 * It's a control stream; must use connection request
	 */
	if ((conp->unit < 0) || (conp->unit >= Ndbri)) {
		dprintf("dbri: Unit # %d invalid \n", conp->unit);
		(void) splx(s);
		return (DBRI_BADCONN);
	}
	driver = &Dbri_devices[conp->unit];
	xchan = Port_tab[(int)conp->xmitport].xchan;
	rchan = Port_tab[(int)conp->recvport].rchan;

	/*
	 * XXX - want findshortpipe to deal with data streams also.  This is
	 * the loophole so if process dies then a control stream would be
	 * able to remove the channel. (This means long pipes) Also need
	 * to set pids
	 */
	pipe = dbri_findshortpipe(as, rchan, xchan);
	if (pipe == DBRI_BAD_PIPE) {
		dprintf("dbri: could not find pipe\n");
		(void) splx(s);
		return (DBRI_BADCONN);
	}

	/* Disconnect first pipe */
	if (dbri_disconnect(driver, as, pipe) == DBRI_BADCONN) {
		(void) splx(s);
		return (DBRI_BADCONN);
	}

	if (conp->dir == ISDN_TWOWAY_STREAM) {
		rchan = Port_tab[(int)conp->xmitport].rchan;
		xchan = Port_tab[(int)conp->recvport].xchan;
		pipe = dbri_findshortpipe(as, rchan, xchan);
		if (pipe == DBRI_BAD_PIPE) {
			dprintf("dbri: could not find pipe\n");
			(void) splx(s);
			return (DBRI_BADCONN);
		}
		if (dbri_disconnect(driver, as, pipe) == DBRI_BADCONN) {
			(void) splx(s);
			return (DBRI_BADCONN);
		}
	}

	/* Mark ports that serial to serial connections used as not busy */
	/* clear first direction */
	p_as = &driver->ioc[(int)xchan].as;
	r_as = &driver->ioc[(int)rchan].as;

	p_as->play_as->info.open = FALSE;
	r_as->record_as->info.open = FALSE;

	/* if bidirectional clear second direction */
	if (conp->dir == ISDN_TWOWAY_STREAM) {
		p_as = &driver->ioc[(int)rchan].as;
		r_as = &driver->ioc[(int)xchan].as;

		p_as->play_as->info.open = FALSE;
		r_as->record_as->info.open = FALSE;
	}

	/* If audio setup restore CHI interface so that CODEC is master */
	if (ISAUDIOCONNECTION(xchan) || ISAUDIOCONNECTION(rchan)) {
		driver->ser_sts.chi_flags &= ~CHI_FLAG_SER_STS;
		audio_ser_to_ser_config(as, driver);
		p_as = &driver->ioc[(int) DBRI_AUDIO_OUT].as;
		audio_sendsig(p_as->control_as);
		wakeup((caddr_t)p_as->control_as); /* wakeup audio_open() */
	}
	(void) splx(s);
	return (DBRI_GOODCONN);
} /* dbri_xcon_stream */


void
audio_ser_to_ser_config(as, driver)
	aud_stream_t	*as;
	dbri_dev_t	*driver;
{
	int	i;
	int	s;

	mmcodec_set_audio_config(driver, as, 8000, 1, 8, AUDIO_ENCODING_ULAW);
	/* 
	 * XXX - Hack to delay up to 500ms waiting for
	 * CHI setup to complete.
	 */
	/* XXX - Check on this...I think this is dangerous */
	s = splx(0);			/* lower intr level */
	for (i=0; i < 5000; ++i) {
		DELAY(100);		/* Delay 100 Us */
		/* check if setup complete and successful */
		if (driver->ser_sts.chi_state==CHI_IN_DATA_MODE)
			break;
	}
	(void) splx(s);			/* restore intr level */
} /* audio_ser_to_ser_config */


void
dbri_hold_f3(driver)
	dbri_dev_t	*driver;
{
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;

	serialp = &driver->ser_sts;
	if ((serialp->te_cmd & DBRI_NTE_ACT) == 0) {
		ATRACEI(dbri_hold_f3, 'noop', serialp->te_cmd);
		return;
	}

	serialp->te_cmd &= ~(DBRI_NTE_ACT|DBRI_NTE_IRM_STATUS);
	cmd.cmd_wd1 = serialp->te_cmd; 		/* NT command */
	dbri_command(driver, cmd);
	/* XXX must insure that the command has been processed */
	cmd.cmd_wd1 = DBRI_CMD_REX | DBRI_CMDI
		| (((driver-Dbri_devices) & 0x3f) << 10) /* 6 bit unit num */
		| 1;		/* 10 bit tag */
	dbri_command(driver, cmd);

	return;
}


void
dbri_exit_f3(driver)
	dbri_dev_t	*driver;
{
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;

	serialp = &driver->ser_sts;
	if (serialp->te_cmd & DBRI_NTE_ACT) {
		ATRACEI(dbri_exit_f3, 'noop', serialp->te_cmd);
		return;
	}

	serialp->te_cmd |= DBRI_NTE_ACT;
	serialp->te_cmd &= ~DBRI_NTE_IRM_STATUS;
	cmd.cmd_wd1 = serialp->te_cmd; 		/* NT command */
	dbri_command(driver, cmd);
	/* XXX must insure that the command has been processed */
	cmd.cmd_wd1 = DBRI_CMD_REX | DBRI_CMDI
		| (((driver-Dbri_devices) & 0x3f) << 10) /* 6 bit unit num */
		| 2;		/* 10 bit tag */
	dbri_command(driver, cmd);

	return;
}


void
dbri_hold_g1(driver)
	dbri_dev_t	*driver;
{
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;

	serialp = &driver->ser_sts;
	if ((serialp->nt_cmd & DBRI_NTE_ACT) == 0) {
		ATRACEI(dbri_hold_g1, 'noop', serialp->nt_cmd);
		return;
	}

	serialp->nt_cmd &= ~(DBRI_NTE_ACT|DBRI_NTE_IRM_STATUS);
	cmd.cmd_wd1 = serialp->nt_cmd; 		/* NT command */
	dbri_command(driver, cmd);
	/* XXX must insure that the command has been processed */
	cmd.cmd_wd1 = DBRI_CMD_REX | DBRI_CMDI
		| (((driver-Dbri_devices) & 0x3f) << 10) /* 6 bit unit num */
		| 3;		/* 10 bit tag */
	dbri_command(driver, cmd);

	return;
}


void
dbri_exit_g1(driver)
	dbri_dev_t	*driver;
{
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;

	serialp = &driver->ser_sts;
	if (serialp->nt_cmd & DBRI_NTE_ACT) {
		ATRACEI(dbri_exit_g1, 'noop', serialp->nt_cmd);
		return;
	}

	serialp->nt_cmd |= DBRI_NTE_ACT;
	serialp->nt_cmd &= ~DBRI_NTE_IRM_STATUS;
	cmd.cmd_wd1 = serialp->nt_cmd; 		/* NT command */
	dbri_command(driver, cmd);
	/* XXX must insure that the command has been processed */
	cmd.cmd_wd1 = DBRI_CMD_REX | DBRI_CMDI
		| (((driver-Dbri_devices) & 0x3f) << 10) /* 6 bit unit num */
		| 4;		/* 10 bit tag */
	dbri_command(driver, cmd);

	return;
}


int
dbri_disconnect(driver, as, pipe)
	dbri_dev_t	*driver;
	aud_stream_t	*as;
	unsigned int	pipe;
{
	int		s;
	pipe_tab_t 	*pipep;
	serial_status_t	*serialp;

	s = splstr();
	pipep = &driver->ptab[pipe];

	/*
	 * Clear out SDP command to gracefully stop data flow
	 *
	 * XXX - Needed?
	 */
	dbri_stop(as);

	pipep->sdp = 0;					/* clr sdp mem copy */

	/* if this is a base pipe disable the serial interface */
	serialp = &driver->ser_sts;
	switch (pipe) {
		case DBRI_PIPE_TE_D_IN:
			/* check if D_OUT in use ... if so break */
			pipep = &driver->ptab[DBRI_PIPE_TE_D_OUT];
			if (pipep->sdp != 0)
				break;

			/*FALLTHROUGH*/

		case DBRI_PIPE_TE_D_OUT:
			/* check if D_IN in use ... if so break */
			pipep = &driver->ptab[DBRI_PIPE_TE_D_IN];
			if (pipep->sdp != 0)
				break;

			dbri_te_unplug(driver);
			DELAY(250);		/* XXX - delay for nice exit */

			if (driver->init_chip)	/* Disable interface */
				dbri_disable_te(driver);
			break;

		case DBRI_PIPE_NT_D_IN:
			/* check if D_OUT in use ... if so break */
			pipep = &driver->ptab[DBRI_PIPE_NT_D_OUT];
			if (pipep->sdp != 0)
				break;

			/*FALLTHROUGH*/

		case DBRI_PIPE_NT_D_OUT:
			/* check if D_IN in use ... if so break */
			pipep = &driver->ptab[DBRI_PIPE_NT_D_IN];
			if (pipep->sdp != 0)
				break;

			/*
			 * Disable NT serial interface
			 */
			dbri_hold_g1(driver);

			if (driver->init_chip) {	/* disable intf */
				dbri_disable_nt(driver);
				/* XXX Should relay control be in disable_nt? */
				driver->chip->n.sts |= DBRI_STS_F;
			}
			serialp->nt_cmd = 0; 			/* clr cmd */
			serialp->nt_sbri.tss = DBRI_NTINFO0_G1;

			/*
			 * If NT D-channels not in use then kill alive
			 * timer and switch in relays.
			 */
			driver->keep_alive_running = FALSE;
			untimeout((int(*)())dbri_keep_alive, (caddr_t)driver);
			break;

		default:
			break;
	} /* switch */

	/* Now remove the dts timeslot definitions */
	if (pipep->monflag) {
		/*
		 * clear flag so that when monitor pipe is removed the
		 * monitor pipe can remove this pipe too.
		 */
		pipep->monflag = FALSE;
	} else {
		/* disconnect up DBRI serial side of pipe */
		dbri_remove_dts(as, pipe);
	}

	if (ISAUDIOCONNECTION(pipep->xchan) || ISAUDIOCONNECTION(pipep->rchan))
		audio_xcon_stream(driver, as);

	pipep->xchan = DBRI_NO_CHNL;	/* clear out pipe indicators */
	pipep->rchan = DBRI_NO_CHNL;
	pipep->as = (aud_stream_t *) NULL;
	pipep->pid = DBRI_NO_TAG;
	AsToDs(as)->pipe = DBRI_BAD_PIPE;
	as->mode = AUDMODE_NONE;

	(void) splx(s);
	return (DBRI_GOODCONN);
} /* dbri_disconnect */


/*
 * dbri_setup_chnl - calls the routines to get a new pipe, and then setup
 * the in memory copy of the sdp, dts, and te/nt commands.  Then issues
 * the sdp, dts, and te/nt commands when needed.
 */
unsigned int
dbri_setup_chnl(as, dcon, error)
	aud_stream_t	*as;
	dbri_conn_req_t	*dcon;
	int		*error;
{
	int		s;
	unsigned int	npipe;
	pipe_tab_t	*pipep;
	serial_status_t	*serialp;
	dbri_chip_cmd_t	cmd;
	dbri_chnl_t	channel;
	dbri_dev_t	*driver;

	*error = 0;
	s = splstr();
	driver = (dbri_dev_t *)as->v->devdata;

	/*
	 * Grab any non-host channel.
	 */
	channel = (dcon->xchan != DBRI_HOST_CHNL) ? dcon->xchan
						    : dcon->rchan;

	/* get a new pipe */
	npipe = dbri_getpipe(driver, channel, dcon->mode);
	if (npipe == DBRI_BAD_PIPE) {
		dprintf("dbri_setup_chnl: No available pipes\n");
		*error = EPIPE;
		(void) splx(s);
		return (DBRI_BAD_PIPE);
	}
	pipep = &driver->ptab[npipe];
	serialp = &driver->ser_sts;
	dbri_setup_sdp(driver, npipe, dcon, Chan_tab[(int)channel].dir);
	dprintf("dbri_setup_chnl: pipe=%u sdp=0x%x\n", npipe, pipep->sdp);

	/*
	 * If this is a receive pipe then allow first SDP to point to
	 * posted receive descriptor chain instead of pointing to NULL.
	 */
	if ((Chan_tab[(int)channel].dir == DBRI_IN) && (dcon->mode != DBRI_MODE_SER)) {
		AsToDs(as)->pipe = npipe;

		/*
		 * Force EOL precondition so that initial SDP is issued.
		 */
		AsToDs(as)->recv_eol = TRUE;

		dprintf("(dbri_setup_chnl:%u)", npipe);
		audio_process_record(as);
	} else {
		cmd.cmd_wd1 = pipep->sdp;
		cmd.cmd_wd2 = 0;		/* NULL address */
		dbri_command(driver, cmd);	/* issue SDP command */

		cmd.cmd_wd1 = DBRI_CMD_PAUSE;	/* PAUSE command */
		dbri_command(driver, cmd);
	}

	/* attach pipe & setup dts */
	if (dbri_setup_dts(as, npipe, channel) == DBRI_BADCONN) {
		pipep->dts.word32 = 0;		/* clear pipe in use */
		pipep->in_tsd.word32 = 0;
		pipep->out_tsd.word32 = 0;
		pipep->sdp = 0;
		*error = EPIPE;
		(void) splx(s);
		return (DBRI_BAD_PIPE);
	}

	/* For serial to serial connections setup other tsd of dts command */
	if (dcon->mode == DBRI_MODE_SER) {
		channel = dcon->rchan; /* get recv channel this time */
		if (dbri_setup_dts(as, npipe, channel) == DBRI_BADCONN) {
			pipep->dts.word32 = 0;		/* clear pipe in use */
			pipep->in_tsd.word32 = 0;
			pipep->out_tsd.word32 = 0;
			pipep->sdp = 0;
			*error = EPIPE;
			(void) splx(s);
			return (DBRI_BAD_PIPE);
		}
	}

	dprintf("dbri_setup_chnl: pipep=0x%x dts=0x%x\n", (unsigned int)pipep,
		pipep->dts.word32);
	dprintf("dbri_setup_chnl: in_tsd=0x%x out_tsd=0x%x\n",
		pipep->in_tsd.word32, pipep->out_tsd.word32);
	cmd.cmd_wd1 = pipep->dts.word32;	/* DTS command */
	cmd.cmd_wd2 = pipep->in_tsd.word32;
	cmd.cmd_wd3 = pipep->out_tsd.word32;
	dbri_command(driver, cmd);

	/* Setup serial interface if necessary and issue commands */
	dbri_setup_ntte(as, channel, serialp);

	(void) splx(s);
	return (npipe);				/* return pipe number */
} /* dbri_setup_chnl */


/*
 * dbri_setup_sdp - sets up the in memory copy of the sdp and te/nt
 * commands
 */
void
dbri_setup_sdp(driver, pipe, dcon, dir)
	dbri_dev_t	*driver;
	unsigned int	pipe;
	dbri_conn_req_t	*dcon;
	int		dir;
{
	pipe_tab_t	*pipep;

	pipep = &driver->ptab[pipe];
	pipep->sdp = 	DBRI_CMD_SDP | 		/* SDP command */
			/* XXX5 - don't enable UNDR interrupt */
			DBRI_SDP_EOL |		/* Enable EOL intr */
						/* LSBit first */
			(unsigned int)dcon->mode |
			DBRI_SDP_PTR |		/* NULL pointer */
			DBRI_SDP_CLR |		/* Clear pipe */
			pipe;			/* new pipe number */

	if (ISAUDIOCONNECTION(dcon->xchan) || ISAUDIOCONNECTION(dcon->rchan) ||
	    (dcon->mode == DBRI_MODE_XPRNT))    /*
						 * single new line to reverse 
						 * B-channels too.
						 */
		pipep->sdp |= DBRI_SDP_B;	/* CHI is always MSB */

	if ((dir == DBRI_OUT) && (dcon->mode != DBRI_MODE_SER))
		pipep->sdp |= DBRI_SDP_D;	/* set transmit direction */

	return;
} /* dbri_setup_sdp */


/*
 * dbri_setup_ntte - setup serial interface but *DO NOT* issue activate
 * command.  Also start timeout timers.
 */
void
dbri_setup_ntte(as, channel, serialp)
	aud_stream_t	*as;
	dbri_chnl_t	channel;
	serial_status_t	*serialp;
{
	dbri_chip_cmd_t	cmd;
	pipe_tab_t	*pipep1, *pipep2;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;
	if ((channel == DBRI_TE_D_IN) || (channel == DBRI_TE_D_OUT)) {
		/* make sure both pipes setup before enabling interface */
		pipep1 = &driver->ptab[DBRI_PIPE_TE_D_OUT];
		pipep2 = &driver->ptab[DBRI_PIPE_TE_D_IN];

		if ((pipep1->dts.r.id != DBRI_DTS_ID) ||
		    (pipep2->dts.r.id != DBRI_DTS_ID))
			return;

		dbri_disable_te(driver);
		driver->ser_sts.te_sbri.tss = DBRI_TEINFO0_F1;
		AsToDs(as)->i_info.activation = ISDN_DEACTIVATED;

		/*
		 * Because of the DBRI implementation, it is proper and
		 * required to send MPH-INFORMATION.ind(connected) here.
		 */
		dbri_primitive_mph_ii_c(as);

		/*
		 * Enable TE interface.  This is only done once initially
		 * and then is not touched again until close is called.
		 * This enables F3 as well as allows NT to activate the
		 * interface.
		 *
		 * Note: The exception is on expiry of T3. In that case,
		 * DBRI_STS_T may be toggled to force DBRI TE into F3,
		 * depending on the state of the activation state
		 * machine.
		 */
		dbri_enable_te(driver); /* enable TE interface */

		/* setup TE command here */
		serialp->te_cmd = (DBRI_CMD_TE | /* TE command */
				    DBRI_NTE_IRM_STATUS | /* force immed intr */
				    DBRI_NTE_IRM_SBRI | /* enable SBRI intrs */
						    /* Not NT command */
						    /* No fixed timing */
						    /* No 0's in E chnl */
						    /* No IFA */
						    /* No Activate intf */
						    /* No multiframe */
						    /* No loopback */
						    /* No force activate */
				    DBRI_NTE_ABV); /* Detect ABV's */

		if ( AsToDs(as)->i_var.maint == ISDN_PH_PARAM_MAINT_OFF)
			serialp->te_cmd &= ~DBRI_TE_QE;
		else
			serialp->te_cmd |= DBRI_TE_QE;

		cmd.cmd_wd1 = serialp->te_cmd; 		/* TE command */
		dbri_command(driver, cmd);

		dprintf("dbri_setup_ntte: serialp=0x%x te_cmd=0x%x\n",
			(unsigned int)serialp, serialp->te_cmd);

	} else if ((channel == DBRI_NT_D_IN) || (channel == DBRI_NT_D_OUT)) {
		/*
		 * check if interface already enabled
		 */

		/* make sure both pipes setup before enabling interface */
		pipep1 = &driver->ptab[DBRI_PIPE_NT_D_OUT];
		pipep2 = &driver->ptab[DBRI_PIPE_NT_D_IN];

		if ((pipep1->dts.r.id != DBRI_DTS_ID) ||
		    (pipep2->dts.r.id != DBRI_DTS_ID))
			return;

		driver->ser_sts.nt_sbri.tss = DBRI_NTINFO0_G1;

		dbri_enable_nt(driver);	/* NT on but not active  */

		if (!driver->keep_alive_running) {
			timeout((int(*)())dbri_keep_alive, (caddr_t)driver,
				hz*Keepalive_timer/1000);
		}
		driver->keep_alive_running = TRUE;
		driver->chip->n.sts &= ~(DBRI_STS_F|DBRI_STS_X);

		/* setup NT command) here */
		serialp->nt_cmd = DBRI_CMD_NT | 	/* NT command */
				DBRI_NTE_IRM_STATUS |	/* force immed intr */
				DBRI_NTE_IRM_SBRI |	/* enable SBRI intrs */
				DBRI_NTE_ISNT |		/* NT command */
							/* Adaptive timing */
							/* No 0's in E chnl */
							/* No IFA */
							/* intf inactive */
							/* No multiframe */
							/* No loopback */
							/* No force activate */
				DBRI_NTE_ABV;		/* Detect ABV's */

		if ( AsToDs(as)->i_var.maint == ISDN_PH_PARAM_MAINT_OFF)
			serialp->nt_cmd &= ~DBRI_NT_MFE;
		else
			serialp->nt_cmd |= DBRI_NT_MFE;

		dprintf("dbri_setup_ntte: serialp=0x%x nt_cmd=0x%x\n",
			(unsigned int)serialp, serialp->nt_cmd);

		cmd.cmd_wd1 = serialp->nt_cmd; 		/* NT command */
		dbri_command(driver, cmd);
	}

	return;
} /* dbri_setup_ntte */


/*
 * dbri_setup_dts - Find place to attach pipe in chain to setup DTS
 * command .  words. Also patch up oldpipe and nextpipe in memory copies
 * to reflect the fact that the new pipe was inserted.
 */
int
dbri_setup_dts(as, pipe, channel)
	aud_stream_t	*as;
	unsigned int	pipe;
	dbri_chnl_t	channel;
{
	unsigned int	offset;
	unsigned int	length;
	int		dir;
	unsigned int	oldpipe;
	unsigned int	nextpipe;
	unsigned int	bpipe;
	unsigned int	monpipe = 0;
	pipe_tab_t	*monpipep;
	pipe_tab_t	*nextpipep;
	pipe_tab_t	*oldpipep;
	pipe_tab_t	*bpipep;
	unsigned int	mode = DBRI_DTS_SINGLE;	/* bias to single mode */
	dbri_dev_t	*driver;
	pipe_tab_t	*pipep;

	extern unsigned	int get_aud_pipe_length();
	extern unsigned	int get_aud_pipe_cycle();

	driver = (dbri_dev_t *)as->v->devdata;
	pipep = &driver->ptab[pipe];
	offset = Chan_tab[(int)channel].cycle;	/* Set time slot bit offset */
	length = Chan_tab[(int)channel].len;	/* Set time slot length */
	dir = Chan_tab[(int)channel].dir;	/* set direction of channel */
	bpipe = Chan_tab[(int)channel].bpipe;	/* get base pipe */
	bpipep = &driver->ptab[bpipe];		/* set base pipe pointer */

	/*
	 * These fields are not found in the Channel table for audio
	 * data, get them from the current audio configuration
	 * information
	 */
	if (ISAUDIOCONNECTION(channel)) {
		offset = get_aud_pipe_cycle(as, pipe, channel);
		length = get_aud_pipe_length(as, pipe, channel);
	}

	/*
	 * If this is the base pipe or there is only a single pipe on the
	 * tsd queue. If so we don't need to traverse the linked list to
	 * discover oldpipe and nextpipe.
	 */
	if ((bpipe == pipe) ||
	    ((dir == DBRI_IN) && (bpipep->in_tsd.r.next == bpipe)) ||
	    ((dir == DBRI_OUT) && (bpipep->out_tsd.r.next == bpipe))) {
		oldpipe = bpipe;		/* output pipe */
		nextpipe = bpipe;		/* next pipe */
	} else {				/* more than 1 pipe in queue */
		/*
		 * check bit offsets as well as setup oldpipe, nextpipe,
		 * and monpipe.
		 */
		if (dbri_find_oldnext(as, bpipe, dir, length, offset, &oldpipe,
		    &nextpipe, &monpipe) == DBRI_BADCONN)
			return (DBRI_BADCONN);
	}

	/*
	 * If monitor pipe and the modes do not match we have a problem
	 * We can also only have a single monitor pipe attached to a main
	 * pipe.
	 */
	if (monpipe != 0) {
		monpipep = &driver->ptab[monpipe];	/* get mon pipe ptr */

		/* check that modes match */
		if ((monpipep->sdp & DBRI_SDP_MODEMASK) == DBRI_SDP_FIXED) {
			dprintf("dbri: timeslot already in use\n");
			return (DBRI_BADCONN);
		} else if (((pipep->sdp&DBRI_SDP_MODEMASK) !=
		    DBRI_SDP_SERIAL) &&
		    ((monpipep->sdp & DBRI_SDP_MODEMASK) !=
		    (pipep->sdp & DBRI_SDP_MODEMASK))) {
			dprintf("dbri: channel modes conflict \n");
			return (DBRI_BADCONN);
		}

		/* check if already one monitor pipe hooked up. */
		if (monpipep->monflag) {
			dprintf("dbri_setup_dts: chnl %d: Mon pipe in use\n",
				channel);
			return (DBRI_BADCONN);
		}

		monpipep->monflag = TRUE; /* tag main pipe having monitor */
		mode = DBRI_DTS_MONITOR;
	} else {
		/*
		 * not a monitor pipe so patch up old and next pipe dts's
		 * and tsd's.
		 */
		oldpipep = &driver->ptab[oldpipe];	/* get old pipe ptr */
		nextpipep = &driver->ptab[nextpipe];	/* get next pipe ptr */
		if (dir == DBRI_IN) {
			oldpipep->in_tsd.r.next = pipe;
			nextpipep->dts.r.oldin = pipe;
		} else {
			oldpipep->out_tsd.r.next = pipe;
			nextpipep->dts.r.oldout = pipe;
		}
	}

	/*
	 * Setup dts command words
	 *
	 * Note that for serial to serial connections 1/2 of the dts
	 * command can already been setup. This is why we don't zero out
	 * anything. Upon release of tsd everything is cleared.
	 */
	pipep->dts.r.cmd = DBRI_OPCODE_DTS;		/* DTS command */
	pipep->dts.r.id = DBRI_DTS_ID;			/* Add timeslot */
	pipep->dts.r.pipe = pipe;			/* pipe # */
	if (dir == DBRI_IN) {
		pipep->dts.r.vi = DBRI_DTS_VI;
		pipep->dts.r.oldin = oldpipe;

		pipep->in_tsd.r.len = length;
		pipep->in_tsd.r.cycle = offset;
		pipep->in_tsd.r.mode = mode;
		pipep->in_tsd.r.mon = monpipe;
		pipep->in_tsd.r.next = nextpipe;	/* next pipe */
	} else {
		pipep->dts.r.vo = DBRI_DTS_VO;
		pipep->dts.r.oldout = oldpipe;

		pipep->out_tsd.r.len = length;
		pipep->out_tsd.r.cycle = offset;
		pipep->out_tsd.r.mode = mode;
		pipep->out_tsd.r.mon = monpipe;
		pipep->out_tsd.r.next = nextpipe;	/* next pipe */
	}

	return (DBRI_GOODCONN);
} /* dbri_setup_dts */


/*
 * dbri_find_oldnext - finds the oldpipe, nextpipe, and monitor pipes for
 * a list of 2 or more pipes. (ie determines where in DTS linked list of
 * tsd's new pipe should go.
 */
int
dbri_find_oldnext(as, bpipe, dir, len, cycle, oldpipe, nextpipe, monpipe)
	aud_stream_t	*as;
	unsigned int	bpipe;
	int		dir;
	unsigned int	len;
	unsigned int	cycle;
	unsigned int	*oldpipe, *nextpipe, *monpipe;
{
	union dbri_dtstsd ttsd;
	int		minvalue = INT_MAX; /* larger than any cycle */
	unsigned int	tpipe = bpipe;
	pipe_tab_t	*tpipep;
	unsigned int	minpipe = bpipe;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;

	/*
	 * Skip over pipe 16 since it doesn't have a len or cycle
	 */
	if (bpipe == DBRI_PIPE_CHI) {
		tpipep = &driver->ptab[bpipe];
		ttsd.word32 = (dir == DBRI_IN)	? tpipep->in_tsd.word32
						: tpipep->out_tsd.word32;
		tpipe = ttsd.chi.next;
		/* If this is the only one, then it is next and old as well */
		if (tpipe == DBRI_PIPE_CHI) {
			*oldpipe = DBRI_PIPE_CHI;
			*nextpipe = DBRI_PIPE_CHI;
			return (DBRI_GOODCONN);
		}

		/*
		 * CHI transmit pipes can't start at zero, they must
		 * start at the length of the frame so that they actually
		 * *start* at zero - receive pipes can start at zero.
		 * Having a cycle of eg.128 imply the beginning of the
		 * frame really screws up the list management logic too
		 * Notice the receive side will go through the regular
		 * code.
		 *
		 * XXX - This could be cleaner
		 */
		if (cycle == CHI_DATA_MODE_LEN) {
			*oldpipe = DBRI_PIPE_CHI;
			*nextpipe = tpipe;
			return (DBRI_GOODCONN);
		} else if (ttsd.r.cycle == CHI_DATA_MODE_LEN) {
			*oldpipe = tpipe;
			*nextpipe = ttsd.r.next;
			return (DBRI_GOODCONN);
		}
	}

	/*
	 * This first loop looks for potential monitor pipes, overlaps in
	 * timeslots and finds the minimum bit offset value in the
	 * circular linked list.
	 */
	do {
		tpipep = &driver->ptab[tpipe];
		ttsd.word32 = (dir == DBRI_IN)	? tpipep->in_tsd.word32
						: tpipep->out_tsd.word32;

		/*
		 * Check if we timeslots match..perhaps monitor pipe
		 * candidate
		 *
		 * XXX - for CHI, len and cycle are not there, but are
		 * reserved fields of zero; this works since they won't
		 * match
		 */
		if ((ttsd.r.len == len) && (ttsd.r.cycle == cycle)) {
			/* Monitor pipes must tap off of an input pipe */
			if (dir == DBRI_OUT) {
				dprintf("dbri: Channel in use.\n");
				return (DBRI_BADCONN);
			}

			*oldpipe = tpipep->dts.r.oldin;
			*nextpipe = ttsd.r.next;
			*monpipe = tpipe;
			return (DBRI_GOODCONN);
		}

		/* check for illegal overlapping offsets and cycles */
		if (dbri_tsd_overlap(cycle, (cycle+len), ttsd.r.cycle,
				    (ttsd.r.cycle + ttsd.r.len))) {
			dprintf("attach_pipe: cycle %u, len overlap %u\n",
				cycle, len);
			return (DBRI_BADCONN);
		}

		/* get minimum cycle bit offset in list */
		if (ttsd.r.cycle < minvalue) {
			minvalue = ttsd.r.cycle;
			minpipe = tpipe;
		}
		tpipe = ttsd.r.next;
	} while (tpipe != bpipe);

	/*
	 * Find spot in list to insert before.  Note that we are starting
	 * with the minimum offset element in list.
	 */
	tpipe = minpipe;
	do {
		tpipep = &driver->ptab[tpipe];
		ttsd.word32 = (dir == DBRI_IN)	? tpipep->in_tsd.word32
						: tpipep->out_tsd.word32;

		if ((int)cycle < ttsd.r.cycle)
			break;

		tpipe = ttsd.r.next;	/* advance to next pipe in list */

		if ((bpipe == DBRI_PIPE_CHI) && (tpipe == DBRI_PIPE_CHI))
			break;

	} while (tpipe != minpipe);

	/* Now set oldpipe and nextpipe variables */
	*nextpipe = tpipe;
	tpipep = &driver->ptab[tpipe];
	*oldpipe = (dir == DBRI_IN) ? tpipep->dts.r.oldin
				    : tpipep->dts.r.oldout;

	return (DBRI_GOODCONN);
} /* dbri_find_oldnext */


/*
 * dbri_remove_dts - disconnect up DBRI serial side of pipe
 */
void
dbri_remove_dts(as, pipe)
	aud_stream_t	*as;
	unsigned int	pipe;
{
	dbri_chip_cmd_t	cmd;
	unsigned int	basepipe;
	unsigned int	oldpipe;
	unsigned int	nextpipe;
	pipe_tab_t 	*pipep;
	pipe_tab_t 	*basepipep;
	pipe_tab_t 	*oldpipep;
	pipe_tab_t 	*nextpipep;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;
	/* check if this was a monitor pipe. */
	pipep = &driver->ptab[pipe];
	if ((pipep->dts.r.vi == DBRI_DTS_VI) &&
	    (pipep->in_tsd.r.mode == DBRI_DTS_MONITOR)) {
		/*
		 * Monitor pipe so clear out monflag field of underlying
		 * base pipe if it is set, otherwise remove the underlying
		 * base pipe itself.
		 */
		basepipe = pipep->in_tsd.r.mon;
		basepipep = &driver->ptab[basepipe];
		if (basepipep->monflag) {
			basepipep->monflag = FALSE;
			return;
		}

		/*
		 * Kill off underlying base pipe, find oldpipe and next pipe
		 * and patch them up
		 */
		if (basepipep->dts.r.vi == DBRI_DTS_VI) {
			oldpipe = basepipep->dts.r.oldin;
			oldpipep = &driver->ptab[oldpipe];
			nextpipe = basepipep->in_tsd.r.next;
			nextpipep = &driver->ptab[nextpipe];
			oldpipep->in_tsd.r.next = basepipep->in_tsd.r.next;
			nextpipep->dts.r.oldin = basepipep->dts.r.oldin;
		}
		if (basepipep->dts.r.vo == DBRI_DTS_VO) {
			oldpipe = basepipep->dts.r.oldout;
			oldpipep = &driver->ptab[oldpipe];
			nextpipe = basepipep->out_tsd.r.next;
			nextpipep = &driver->ptab[nextpipe];
			oldpipep->out_tsd.r.next = basepipep->out_tsd.r.next;
			nextpipep->dts.r.oldout = basepipep->dts.r.oldout;
		}

		/* The main pipe needs to be cleared out */
		basepipep->dts.r.id = ~DBRI_DTS_ID & 1;
		cmd.cmd_wd1 = basepipep->dts.word32;	/* DTS cmd */
		cmd.cmd_wd2 = basepipep->in_tsd.word32;
		cmd.cmd_wd3 = basepipep->out_tsd.word32;
		dbri_command(driver, cmd);

		/* clear out in memory copy of the dts command */
		pipep->dts.word32 = 0;
		pipep->in_tsd.word32 = pipep->out_tsd.word32 = 0;
	}

	/* find neighbors oldpipe and next pipe and patch them up */
	if (pipep->dts.r.vi == DBRI_DTS_VI) {
		oldpipe = pipep->dts.r.oldin;
		oldpipep = &driver->ptab[oldpipe];
		nextpipe = pipep->in_tsd.r.next;
		nextpipep = &driver->ptab[nextpipe];
		oldpipep->in_tsd.r.next = pipep->in_tsd.r.next;
		nextpipep->dts.r.oldin = pipep->dts.r.oldin;
	}
	if (pipep->dts.r.vo == DBRI_DTS_VO) {
		oldpipe = pipep->dts.r.oldout;
		oldpipep = &driver->ptab[oldpipe];
		nextpipe = pipep->out_tsd.r.next;
		nextpipep = &driver->ptab[nextpipe];
		oldpipep->out_tsd.r.next = pipep->out_tsd.r.next;
		nextpipep->dts.r.oldout = pipep->dts.r.oldout;
	}

	/* Now clear the add/modify bit and issue the DTS command */
	pipep->dts.r.id = ~DBRI_DTS_ID & 1;
	cmd.cmd_wd1 = pipep->dts.word32;		/* DTS command */
	cmd.cmd_wd2 = pipep->in_tsd.word32;
	cmd.cmd_wd3 = pipep->out_tsd.word32;
	dbri_command(driver, cmd);

	/* clear out in memory copy of the dts and sdp command */
	pipep->dts.word32 = 0;
	pipep->in_tsd.word32 = 0;
	pipep->out_tsd.word32 = 0;
	pipep->sdp = 0;

	return;
} /* dbri_remove_dts */


/*
 * dbri_getpipe - Get an available free pipe.  This routine allocates the
 * appropriate type of pipe {long or short} based on the data channel.
 * Note that if the channel is of an unknown type then a short pipe is
 * automatically allocated. This could be a problem in the future when
 * unknown pipes are setup by the user to the host.
 */
unsigned int
dbri_getpipe(driver, chan, mode)
	dbri_dev_t	*driver;
	dbri_chnl_t	chan;
	dbri_mode_t	mode;
{
	unsigned int	pipe;
	unsigned int	maxpipe;
	pipe_tab_t 	*pipep;
	int		pipetype;

	/* D-channels are required to use predetermined pipes */
	if ((chan == DBRI_TE_D_OUT) ||
	    (chan == DBRI_TE_D_IN) ||
	    (chan == DBRI_NT_D_OUT) ||
	    (chan == DBRI_NT_D_IN)) {
		pipe = Chan_tab[(int)chan].bpipe;
		pipep = &driver->ptab[pipe];
		if (ISPIPEINUSE(pipep)) {
			dprintf("dbri_getpipe: Pipe %d already in use.\n",
				(int)pipe);
			return (DBRI_BAD_PIPE);		/* no free pipes */
		}
		pipep->dts.r.id = DBRI_DTS_ID; /* clear and mark pipe in use */
		return (pipe);
	}

	/* Non-predetermined pipes */
	if ((mode == DBRI_MODE_SER) || (mode == DBRI_MODE_FIXED)) {
		pipetype = DBRI_SHORTPIPE;
	} else {
		pipetype = DBRI_LONGPIPE;
	}

	if (pipetype == DBRI_LONGPIPE) {
		pipe = DBRI_PIPE_LONGFREE;
		maxpipe = DBRI_PIPE_CHI;
	} else {
		pipe = DBRI_PIPE_SHORTFREE;
		maxpipe = DBRI_MAX_PIPES;
	}

	/* Go down list of pipes looking for one not in use. */
	for (; pipe < maxpipe; pipe++) {
		pipep = &driver->ptab[pipe];
		if (!ISPIPEINUSE(pipep))
			break;		/* free pipe available */
	}

	if (pipe == maxpipe)
		return (DBRI_BAD_PIPE);	/* no free pipes */

	pipep->dts.r.id = DBRI_DTS_ID; /* clear and mark pipe in use */

	return (pipe);
} /* dbri_getpipe */


/*
 * dbri_findshortpipe - searches for a short pipe in use that matches
 * channels requested
 */
unsigned int
dbri_findshortpipe(as, rchan, xchan)
	aud_stream_t	*as;
	dbri_chnl_t	rchan, xchan;
{
	unsigned	pipe;
	pipe_tab_t 	*pipep;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;

	/* search short pipe list pointers for "as" to find a match. */
	for (pipe = DBRI_PIPE_CHI; pipe < DBRI_MAX_PIPES; pipe++) {
		pipep = &driver->ptab[pipe];
		if ((pipep->as == as) &&
		    (xchan == pipep->xchan) &&
		    (rchan == pipep->rchan)) {
			break;		/* found a match */
		}
	}
	if (pipe == DBRI_BAD_PIPE)
		return (DBRI_BAD_PIPE);	/* cannot find connection */

	pipep = &driver->ptab[pipe];
	if (!ISPIPEINUSE(pipep))
		return (DBRI_BAD_PIPE);

	return (pipe);
} /* dbri_findshortpipe */


static struct {
	int	length;
	char	*name;
}	dbri_cmd_info[16] = {
	{1, "WAIT"},
	{1, "PAUSE"},
	{2, "JMP"},
	{2, "IIQ"},
	{1, "REX"},
	{2, "SDP"},
	{1, "CDP"},
	{3, "DTS"},
	{2, "SSP"},
	{1, "CHI"},
	{1, "NT"},
	{1, "TE"},
	{1, "CDEC"},
	{3, "TEST"},	/* SBus TEST/COPY is the only 3 word test command */
	{1, "CDM"},
	{0, "undefined"}
};


void
dbri_command(driver, cmd)
	dbri_dev_t	*driver;
	dbri_chip_cmd_t	cmd;
{
	unsigned int	unprocessed;
	dbri_chip_cmdq_t *cmdq;	/* base of cmdq */
	int		command_length;
	unsigned int	dbri_pc;
	unsigned int	*reg_cmdqp;
	unsigned int	tmp_cmdp;

	cmdq = driver->cmdqp;	/* base of cmdq */

	/* Protect this routine if the chip has not initialized successfully */
	if (!driver->init_chip) {
		dprintf("**Entering dbri_command but chip not init \n");
		return;
	}

	command_length = dbri_cmd_info[DBRI_OP(cmd.cmd_wd1)].length;

	/*
	 * See if there is sufficient space in the queue for the command.
	 */

	/* Find DBRI's current position in the command queue */
	if (!driver->init_cmdq) {
		dbri_pc = 0;	/* Not initialized yet, it will be at 0 */
	} else {
		reg_cmdqp = (unsigned *)KERN_ADDR(driver,
		    driver->chip->n.cmdqp);
		dbri_pc = reg_cmdqp - &cmdq->cmd[0];
		if (dbri_pc > DBRI_MAX_CMDS) {
			dbri_panic(driver, "DBRI Reg8 bad!\n");
			return;
		}
	}

	/* Calculate space at end of queue. cmdq->cmdhp always points to WAIT */
	unprocessed = (cmdq->cmdhp >= dbri_pc)
		? cmdq->cmdhp - dbri_pc
		: DBRI_MAX_CMDS - (dbri_pc - cmdq->cmdhp);

	/*
	 * If the command queue is almost full, DBRI has not been processing
	 * commands and this indicates a serious problem.
	 *
	 * XXX - bad test
	 */
	if (unprocessed > (DBRI_MAX_CMDS - 10 /* XXX - magic number */)) {
		(void) printf("dbri_command: *** No space in cmdq.\n");
		(void) printf("\tcmd=0x%x 0x%x\n",
			cmd.cmd_wd1, cmd.cmd_wd2);
		(void) printf("r8:0x%x index:%u, cmdhp:%u, MAX=%d unprocd:%u\n",
			    (unsigned int)reg_cmdqp, dbri_pc, cmdq->cmdhp,
			    DBRI_MAX_CMDS, unprocessed);
		return;		/* XXX - may want to return error here */
	}

	/*
	 * If at end of command queue, insert a JMP command, to wrap
	 * around to head of the command queue, followed by a WAIT
	 * command.
	 */
	if ((cmdq->cmdhp+command_length) > (DBRI_MAX_CMDS-DBRI_CMD_JMP_LEN)) {
		cmdq->cmd[0] = DBRI_CMD_WAIT; /* jump to WAIT */

		/*
		 * Stuff the JMP command into the current word and 1 more
		 */
		cmdq->cmd[cmdq->cmdhp + 1] = ((unsigned)
		    DBRI_ADDR(driver, &cmdq->cmd[0]));
		cmdq->cmd[cmdq->cmdhp] = DBRI_CMD_JMP;

		/*
		 * Point back to head of list
		 */
		cmdq->cmdhp = 0;
	}

	/*
	 * Write the command plus a WAIT command into the queue.  Insert
	 * commands last to first to avoid race with DBRI.
	 */
	switch (command_length) {
	case 1:
		if (DBRI_OP(cmd.cmd_wd1) == DBRI_OP(DBRI_CMD_WAIT)) {
			/* Don't advance pointer on WAIT command. */
			cmdq->cmd[cmdq->cmdhp] = cmd.cmd_wd1;
		} else {
			tmp_cmdp = cmdq->cmdhp + 1;
			cmdq->cmdhp = tmp_cmdp;
			cmdq->cmd[tmp_cmdp] = DBRI_CMD_WAIT;
			cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd1;
		}
		break;

	case 2:
		tmp_cmdp = cmdq->cmdhp + 2;
		cmdq->cmdhp = tmp_cmdp;
		cmdq->cmd[tmp_cmdp] = DBRI_CMD_WAIT;
		cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd2;
		cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd1;
		break;

	case 3:
		tmp_cmdp = cmdq->cmdhp + 3;
		cmdq->cmdhp = tmp_cmdp;
		cmdq->cmd[tmp_cmdp] = DBRI_CMD_WAIT;
		cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd3;
		cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd2;
		cmdq->cmd[--tmp_cmdp] = cmd.cmd_wd1;
		break;

	default:
		dbri_panic(driver, "illegal dbri_command");
		return;
	}			/* switch */

	if (!driver->init_cmdq) {
		driver->chip->n.cmdqp = ((unsigned)
					    DBRI_ADDR(driver, &cmdq->cmd[0]));
		driver->init_cmdq = TRUE;
		dprintf("dbri_command: cmdq is 0x%x, reg8 set to 0x%x\n",
			(unsigned int)&cmdq->cmd[0], driver->chip->n.cmdqp);
	} else {
		driver->chip->n.sts |= DBRI_STS_P;	/* Queue ptr valid */
	}

	return;
} /* dbri_command */


#define dbri_valid_channel(c) (((int)c) >= ((int)DBRI_HOST_CHNL) && \
	(((int)c) < (int)DBRI_LAST_CHNL))
/*
 * dbri_check_req - Checks on sanity of connection setup
 */
int
dbri_check_req(as, conp, dcon, error)
	aud_stream_t	*as;
	isdn_conn_req_t	*conp;
	dbri_conn_req_t	*dcon;
	int		*error;
{
	pipe_tab_t	*pipep;
	dbri_dev_t	*driver;
	aud_stream_t	*p_as, *r_as;

	if (as == NULL) {
		*error = ENODEV;
		return (DBRI_BADCONN);
	}

	switch (dcon->mode) {
	case DBRI_MODE_UNKNOWN:
	case DBRI_MODE_XPRNT:
	case DBRI_MODE_HDLC:
	case DBRI_MODE_HDLC_D:
	case DBRI_MODE_SER:
	case DBRI_MODE_FIXED:
		break;
	default:
		*error = EINVAL;
		return (DBRI_BADCONN);
	}

	driver = (dbri_dev_t *)as->v->devdata;
	/* Check permissions of operations */
	if (as->openflag == FREAD) {			/* Read only stream */
		if (conp->dir != ISDN_ONEWAY_STREAM) {
			dprintf("dbri: dev open O_READ yet 2 way strm reqd.\n");
			*error = EACCES;
			return (DBRI_BADCONN);
		}
		/* check host dir is read */
		if (conp->xmitport != ISDN_HOST) {
			dprintf("dbri: transmit port not equal to host.\n");
			*error = EACCES;
			return (DBRI_BADCONN);
		}
	} else if (as->openflag == FWRITE) {		/* Write only stream */
		if (conp->dir != ISDN_ONEWAY_STREAM) {
			dprintf("dbri: open O_WRITE yet 2-way stream req\n");
			*error = EACCES;
			return (DBRI_BADCONN);
		}
		/* check host direction is write */
		if (conp->recvport != ISDN_HOST) {
			dprintf("dbri: receive port not equal to host.\n");
			*error = EACCES;
			return (DBRI_BADCONN);
		}
	} else {					/* Read/write stream */
		if (conp->dir != ISDN_TWOWAY_STREAM) {
			dprintf("dbri: device opened RW yet one-way request\n");
			*error = EACCES;
			return (DBRI_BADCONN);
		}
	}

	if (ISHWCONNECTED(as)) {
		dprintf("dbri: H/W channel already connected \n");
		*error = EEXIST;
		return (DBRI_BADCONN);
	}

	/* Check if data stream for serial to serial connections */
	if ((dcon->mode == DBRI_MODE_SER) && ISDATASTREAM(as)) {
		dprintf("dbri: Data channel req non-data connection.\n");
		*error = ENOSTR;
		return (DBRI_BADCONN);		/* Must be a control stream */
	}

	/* Check if control stream for data connection */
	if ((dcon->mode != DBRI_MODE_SER) && !ISDATASTREAM(as)) {
		dprintf("dbri: Control channel req data connection.\n");
		*error = ENOSTR;
		return (DBRI_BADCONN);		/* Must be a control stream */
	}

	/*
	 * Check that D channel requests are bidirectional and one side
	 * of the pipe's datastream goes to the host.
	 */
	if ((conp->xmitport == ISDN_TE_D) ||
	    (conp->xmitport == ISDN_NT_D) ||
	    (conp->recvport == ISDN_TE_D) ||
	    (conp->recvport == ISDN_NT_D)) {
		/* D channel request must be bidirectional */
		if (conp->dir != ISDN_TWOWAY_STREAM) {
			dprintf("dbri: D-ch req not bidirectional.\n");
			*error = EINVAL;
			return (DBRI_BADCONN);
		}
		/* One side of D channel must go to the host */
		if ((conp->xmitport != ISDN_HOST) &&
		    (conp->recvport != ISDN_HOST)) {
			dprintf("dbri: D-ch req not connected to host.\n");
			*error = EINVAL;
			return (DBRI_BADCONN);
		}
	} else {
		/* Check that base pipes of xmit and recv channels are setup */
		if ((dcon->xchan != DBRI_NO_CHNL) &&
		    (dcon->xchan != DBRI_HOST_CHNL)) {
			pipep = &driver->ptab[Chan_tab[(int)dcon->xchan].bpipe];
			if (!ISPIPEINUSE(pipep)) {
				dprintf("dbri: base channel not setup\n");
				*error = ENOENT;
				return (DBRI_BADCONN);
			}
		}
		if ((dcon->rchan != DBRI_NO_CHNL) &&
		    (dcon->rchan != DBRI_HOST_CHNL)) {
			pipep = &driver->ptab[Chan_tab[(int)dcon->rchan].bpipe];
			if (!ISPIPEINUSE(pipep)) {
				dprintf("dbri: base channel not setup\n");
				*error = ENOENT;
				return (DBRI_BADCONN);
			}
		}
	}

	/*
	 * Here we check for channels not currently implemented.
	 */
	if ((dcon->xchan == DBRI_NO_CHNL) || (dcon->rchan == DBRI_NO_CHNL)) {
		dprintf("dbri: channel request not implemented.\n");
		*error = ENXIO;
		return (DBRI_BADCONN);
	}

	/* If audio, check that mode is transparent */
	if ((dcon->mode != DBRI_MODE_SER) &&
	    (ISAUDIOCONNECTION(dcon->xchan) || ISAUDIOCONNECTION(dcon->rchan))){
		if (dcon->mode != DBRI_MODE_XPRNT) {
			dprintf("dbri_check_req: audio !transparent.\n");
			*error = EINVAL;
			return (DBRI_BADCONN);
		}
	}

	/* Checks on serial to serial connections to insure ports are not busy*/
	if (dcon->mode == DBRI_MODE_SER) {
		/* check first direction */
		p_as = &driver->ioc[(int)dcon->xchan].as;
		r_as = &driver->ioc[(int)dcon->rchan].as;

		if (p_as->play_as->info.open || r_as->record_as->info.open) {
			*error = EBUSY;
			return (DBRI_BADCONN);
		}
		/* now check second direction */
		if (conp->dir == ISDN_TWOWAY_STREAM) {
			p_as = &driver->ioc[(int)dcon->rchan].as;
			r_as = &driver->ioc[(int)dcon->xchan].as;

			if (p_as->play_as->info.open || 
			    r_as->record_as->info.open) {
				*error = EBUSY;
				return (DBRI_BADCONN);
			}
		}
	}

	return (DBRI_GOODCONN);			/* Everything ok */
} /* dbri_check_req */


/*
 * dbri_chnl_activated - Checks that the base channel is activated and
 * returns true.
 *
 * XXX - May have to be expanded to also check for CHI active
 */
int
dbri_chnl_activated(as, xchan, rchan)
	aud_stream_t	*as;
	dbri_chnl_t	xchan;
	dbri_chnl_t	rchan;
{
	unsigned	bpipe;
	aud_stream_t	*bas;
	pipe_tab_t	*pipep;
	dbri_dev_t	*driver;

	driver = (dbri_dev_t *)as->v->devdata;

	/* Grab mode of channel not connected to host from table */
	if ((xchan != DBRI_HOST_CHNL) && (xchan != DBRI_NO_CHNL))
		bpipe = Chan_tab[(int)xchan].bpipe;
	else if ((rchan != DBRI_HOST_CHNL) && (rchan != DBRI_NO_CHNL))
		bpipe = Chan_tab[(int)rchan].bpipe;
	else {
		dprintf("dbri_chnl_activated: both channels invalid\n");
		return (FALSE);
	}

	if ((bpipe == DBRI_PIPE_TE_D_IN) ||
	    (bpipe == DBRI_PIPE_TE_D_OUT) ||
	    (bpipe == DBRI_PIPE_NT_D_IN) ||
	    (bpipe == DBRI_PIPE_NT_D_OUT)) {
		/* need to get the stream of the base channel */
		pipep = &driver->ptab[bpipe];
		bas = pipep->as;
		if (AsToDs(bas->play_as)->i_info.activation != ISDN_ACTIVATED)
			return (FALSE);
	}

	/*
	 * Note that the default value is used for CHI.
	 */

	return (TRUE);
} /* dbri_chnl_activated */


/*
 * dbri_tsd_overlap - check if anywhere the values of a1 thru a2
 * inclusive lie within the values of x1 thru x2 inclusive.
 */
int
dbri_tsd_overlap(a1, a2, x1, x2)
	unsigned int a1;
	unsigned int a2;
	unsigned int x1;
	unsigned int x2;
{
	return (((a1 < x1) && (a2 > x1)) || ((a1 > x1) && (a1 < x2)));
} /* dbri_tsd_overlap */



/*
 * dbri_te_timer - expiry of T3.
 */
void
dbri_te_timer(as)
	aud_stream_t	*as;
{
	dbri_dev_t	*driver;
	serial_status_t	*serialp;
	int		s;

	/* Don't do anything if H/W not connected */
	if ((as == (aud_stream_t *) NULL) || !ISHWCONNECTED(as))
		return;

	driver = (dbri_dev_t *)as->v->devdata;
	serialp = &driver->ser_sts;

	s = splr(driver->intrlevel);
	ATRACE(dbri_te_timer, 'timr', serialp->te_sbri.tss);

	switch (serialp->te_sbri.tss) {
	case DBRI_TEINFO3_F6:
		if (AsToDs(as)->i_var.asmb == ISDN_TE_PH_PARAM_ASMB_CTS2) {
			/* FALLTHROUGH */
		} else {
			break;
		}
		/* FALLTHROUGH */
	case DBRI_TEINFO0_F1:	/* CCITT: "impossible", it happens in DBRI */
	case DBRI_TEINFO0_F3:	/* no effect */
	case DBRI_TEINFO3_F7:	/* no effect */
		(void) splx(s);
		return;				/* Sending data */

	case DBRI_TEINFO0_F8:	/* blue book says "no effect", FT disagrees */
		if ((AsToDs(as)->i_var.asmb == ISDN_TE_PH_PARAM_ASMB_CTS2) &&
		    AsToDs(as)->i_info.activation == ISDN_ACTIVATED) {
			(void) splx(s);
			return;				/* Sending data */
		}
		break;

	default:
		break;
	}

	serialp->te_sbri.tss = DBRI_TEINFO0_F3;
	AsToDs(as)->i_info.activation = ISDN_DEACTIVATED;

	/*
	 * Force DBRI's TE state machine into F3 by resetting the TE
	 * activation bit and then setting.
	 *
	 * NB: Given DBRI's implementation, DBRI will report a transition
	 * to F1. This transition is to be interpreted as a transition to
	 * F3.
	 */
	dbri_disable_te(driver);
	dbri_hold_f3(driver);
	DELAY(250); /* XXX Stoltz bets that 2 frame times must go by */
	dbri_enable_te(driver);

	dbri_bri_down(driver, ISDN_TYPE_TE);
	dbri_primitive_mph_di(as);
	dbri_primitive_ph_di(as);

	(void) splx(s);
	return;
} /* dbri_te_timer */


/*
 * dbri_nt_timer - doubles up for both T1 and T2 timer expirations.  This
 * assumes that both timers cannot be operating simultaneously. If this
 * is not the case there must be two separate functions to deactivate the
 * NT interface.
 */
void
dbri_nt_timer(as)
	aud_stream_t	*as;
{
	dbri_dev_t	*driver;
	serial_status_t	*serialp;
	int		s;

	if ((as == (aud_stream_t *) NULL) || !ISHWCONNECTED(as))
		return;

	driver = (dbri_dev_t *)as->v->devdata;
	s = splr(driver->intrlevel);
	serialp = &driver->ser_sts;

	ATRACEI(dbri_nt_timer, 'timr', serialp->nt_sbri.tss);

	if (serialp->nt_sbri.tss == DBRI_NTINFO4_G3) {
		/* If T101 or T102 expire in G3, then ignore them */
		(void) splx(s);
		return;				/* Sending data */
	} else if (serialp->nt_sbri.tss == DBRI_NTINFO2_G2) {
		/* This is T1 expiry */
		AsToDs(as)->i_info.activation = ISDN_DEACTIVATED;
		dbri_bri_down(driver, ISDN_TYPE_NT);
		dbri_primitive_ph_di(as);

		dbri_disable_nt(driver);
		dbri_hold_g1(driver);
		DELAY(250); /* XXX Stoltz bets that 2 frame times must go by */
		dbri_enable_nt(driver);

		untimeout((int(*)())dbri_nt_timer, (caddr_t)as);
		timeout((int(*)())dbri_nt_timer,
			(caddr_t)as,
			hz*AsToDs(as)->i_var.t102/1000);
		ATRACEI(dbri_nt_timer, 't102', AsToDs(as)->i_var.t102);

		driver->ser_sts.nt_sbri.tss = DBRI_NTINFO0_G4;
	} else if (driver->ser_sts.nt_sbri.tss == DBRI_NTINFO0_G4) {
		dbri_enable_nt(driver);	/* This is T2 expiry */
	} else {
		/*
		 * XXX if we get here then a bug has to be fixed.
		 * Since both timers above leave the interface
		 * ON, make sure that it is on now.
		 */
		dbri_enable_nt(driver);
		ATRACEI(dbri_nt_timer, 't10?', 0);
	}
	(void) splx(s);

	return;
} /* dbri_nt_timer */


/*
 * dbri_keep_alive - keep alive timer routine that reads reg0 every 4
 * seconds to insure that there is activity on DBRI.  This prevents the
 * relays from bypassing the host.
 */
void
dbri_keep_alive(driver)
	dbri_dev_t	*driver;
{
	unsigned int	tmp;

	tmp = driver->chip->n.sts;	/* dummy read to keep chip alive */
#ifdef lint
	tmp = tmp;
#endif

	/* restart keep alive timer */
	if (driver->keep_alive_running) {
		timeout((int(*)())dbri_keep_alive, (caddr_t)driver,
			hz*Keepalive_timer/1000);
	}

	return;
} /* dbri_keep_alive */


/*
 * dbri_te_unplug - adjusts the state of the isdnstate variable to
 * reflect an unplugged condition as well as conditionally signals the
 * user program and disables the "permit-activation" bit if this has not
 * been previously done.
 */
void
dbri_te_unplug(driver)
	dbri_dev_t	*driver;
{
	pipe_tab_t	*pipep;
	aud_stream_t	*as;

	pipep = &driver->ptab[DBRI_PIPE_TE_D_IN];

	/*
	 * XXX - Need to have semaphore valid variable to check if
	 * structure is valid.
	 */
	if (pipep->as == NULL)
		return;

	as = pipep->as->play_as;		/* get control as */

	untimeout((int(*)())dbri_te_timer, (caddr_t)as);

	if (AsToDs(as)->i_info.activation != ISDN_UNPLUGGED) {
		/*
		 * Only signal upper layer when there is a change from
		 * active to not active.
		 */
		dprintf("dbri: TE Interface down\n");
		audio_sendsig(as);	/* XXX - done during close, bogus */
	}
	AsToDs(as)->i_info.activation = ISDN_UNPLUGGED;

	dbri_hold_f3(driver);

	return;
} /* dbri_te_unplug */


/*
 * dbri_ph_activate_req - Activate TE
 */
void
dbri_te_ph_activate_req(as)
	aud_stream_t	*as;
{
#ifdef AUDIOTRACE
	serial_status_t	*serialp;
#endif
	dbri_dev_t	*driver;
	int		s;

	driver = (dbri_dev_t *)(as->v->devdata);
	s = splr(driver->intrlevel);

#ifdef AUDIOTRACE
	serialp = &driver->ser_sts;
#endif

	ATRACEI(dbri_te_ph_activate_req, 'act+', serialp->te_cmd);
	if ((AsToDs(as)->i_info.activation != ISDN_ACTIVATED) &&
	    (AsToDs(as)->i_info.activation != ISDN_ACTIVATE_REQ)) {
		/*
		 * Activate TE interface
		 *
		 * XXX - Should not do anything unless in F3
		 */
		dbri_exit_f3(driver);

		dprintf("TE activation request\n");

		AsToDs(as)->i_info.activation = ISDN_ACTIVATE_REQ;

		untimeout((int(*)())dbri_te_timer, (caddr_t)as);
		timeout((int(*)())dbri_te_timer,
			(caddr_t)as,
			hz*AsToDs(as)->i_var.t103/1000);
		ATRACEI(dbri_te_ph_activate_req,
			't103', AsToDs(as)->i_var.t103);
	}

	(void) splx(s);
	return;
}


/*
 * Activate NT interface
 */
void
dbri_nt_ph_activate_req(as)
	aud_stream_t	*as;
{
	dbri_dev_t	*driver;
	int		s;

	driver = (dbri_dev_t *)(as->v->devdata);
	s = splr(driver->intrlevel);

	ATRACEI(dbri_nt_ph_activate_req, 'act+', as);

	dbri_exit_g1((dbri_dev_t *)(as->v->devdata));

	untimeout((int(*)())dbri_nt_timer, (caddr_t)as);
	timeout((int(*)())dbri_nt_timer,
		(caddr_t)as,
		hz*AsToDs(as)->i_var.t101/1000);
	ATRACEI(dbri_nt_ph_activate_req, 't101', AsToDs(as)->i_var.t101);
	(void) splx(s);

	return;
}


/*
 * dbri_nt_mph_deactivate_req - Deactivate NT
 */
void
dbri_nt_mph_deactivate_req(as)
	aud_stream_t	*as;
{
	dbri_dev_t	*driver;
	int		s;

	driver = (dbri_dev_t *)(as->v->devdata);
	s = splr(driver->intrlevel);

	ATRACEI(dbri_nt_mph_deactivate_req, 'act-', driver->chip->n.sts);
	/*
	 * XXX need to examine the following conditional, was something
	 * XXX else intended?
	 */
	if ((driver->ser_sts.nt_sbri.tss == DBRI_NTINFO0_G4) ||
	    (driver->ser_sts.nt_sbri.tss == DBRI_NTINFO0_G4)) {
		ATRACEI(dbri_nt_mph_deactivate_req, 'noop',
			driver->ser_sts.nt_sbri.tss);
		(void) splx(s);
		return;
	}

	dbri_disable_nt(driver);
	dbri_hold_g1(driver);
	dbri_bri_down(driver, ISDN_TYPE_NT);

	driver->ser_sts.nt_sbri.tss = DBRI_NTINFO0_G4;
	AsToDs(as)->i_info.activation = ISDN_DEACTIVATED;
	dbri_primitive_ph_di(as);

	untimeout((int(*)())dbri_nt_timer, (caddr_t)as);
	timeout((int(*)())dbri_nt_timer,
		(caddr_t)as,
		hz*AsToDs(as)->i_var.t102/1000);
	ATRACEI(dbri_nt_mph_deactivate_req, 't102', AsToDs(as)->i_var.t102);
	(void) splx(s);

	return;
}


/*
 * dbri_panic - Print out a message and either cause a system panic or
 * deactivate the current DBRI unit until the next time the system is
 * booted/driver is reloaded.
 */
void
dbri_panic(driver, message)
	dbri_dev_t *driver;
	char *message;
{
	dbri_reg_t *chip = driver->chip;
	int i;

	/*
	 * Prevent new opens on the device
	 */
	driver->openinhibit = TRUE;

	/*
	 * Reset the chip and disallow it to become an SBus master
	 */
	chip->n.sts = DBRI_STS_R;	/* reset DBRI chip */
	DELAY(2);			/* XXX - For safety? */
	driver->init_chip = FALSE;	/* no longer initialized */
	chip->n.sts |= (DBRI_STS_F|DBRI_STS_D);

	if (Dbri_panic) {
		panic(message);
		/* The system will probably not get to this point */
	}

	printf("dbri: %s\n", message);

	/*
	 * Send M_ERROR messages up all streams associated with this
	 * device.
	 */
	for (i = 0; i < Nstreams; i++)
		dbri_error_msg(driver->ioc[i].as.play_as, M_ERROR);

	/*
	 * Device will no longer be used by this instance of the driver
	 *
	 * XXX - Is this all we need to do?
	 */
	return;
}


int
dbri_power_subr(as, arg)
	aud_stream_t	*as;
	void		*arg;
{
	unsigned int	power;

	power = (unsigned int)arg;

	AsToDs(as)->i_var.power = power;
	AsToDs(as->play_as)->i_var.power = power;
	AsToDs(as->record_as)->i_var.power = power;
	AsToDs(as->control_as)->i_var.power = power;

	if ((power == ISDN_PH_PARAM_POWER_OFF) && !ISCONTROLSTREAM(as)) {
		/* Send an M_HANGUP upstream */
		dbri_error_msg(as, M_HANGUP);
	} else {
		dbri_error_msg(as, M_UNHANGUP);
	}
	/* XXX For powerup case, should M_UNHANGUP be sent? */
	return (0);
}

	
#define IOCEQ(x) (ds == AsToDs(driver->ioc[(int)x].as.play_as))

/* 
 * Turn interface "power" on or off.
 * XXX For the NT, should this routine control the relay?
 */
int
dbri_power(ds, power_state)
	dbri_stream_t	*ds;	/* pointer to the interface's D-channel */
	unsigned int	power_state;
{
	isdn_interface_t	bri_type;
	dbri_dev_t		*driver;
	
	driver = (dbri_dev_t *) DsToAs(ds)->v->devdata;

	/* Check for invalid args or redundant work */
	if (power_state != ISDN_PH_PARAM_POWER_OFF && power_state != ISDN_PH_PARAM_POWER_ON)
		return (0);
	if (!IOCEQ(DBRI_NT_D_OUT) && !IOCEQ(DBRI_TE_D_OUT))
		return (0);
	if (ds->i_var.power == power_state)
		return (1);

	ds->i_var.power = power_state;

	if (IOCEQ(DBRI_NT_D_OUT)) {
		bri_type = ISDN_TYPE_NT;
		driver->ioc[(int)DBRI_NTMGT].i_var.power = power_state;
	} else {
		bri_type = ISDN_TYPE_TE;
		driver->ioc[(int)DBRI_TEMGT].i_var.power = power_state;
	}

	ATRACEI(dbri_power, 'juce', power_state);

	if (power_state == ISDN_PH_PARAM_POWER_OFF)
		dbri_bri_down(driver, bri_type);
	
	if (power_state == ISDN_PH_PARAM_POWER_ON) {
		if (bri_type == ISDN_TYPE_NT)
			dbri_enable_nt(driver);
		else
			dbri_enable_te(driver);
	}

	(void) dbri_bri_func(driver, bri_type, dbri_power_subr,
		(void *)power_state);

	if (power_state == ISDN_PH_PARAM_POWER_ON)
		dbri_bri_up(driver, bri_type);

	return (1);
}
#undef IOCEQ

/*
 * An interface is being brought up or down, perform the appropriate actions
 * on an aud_stream on that interface.
 *
 * If the interface is being brought down (arg == 0), stop IO on the pipe
 * and flush all pending output.
 *
 * If the interface is coming up, reset the eol state and queue record buffers.
 */
/*ARGSUSED*/
int
dbri_bri_up_down_subr(as, arg)
	aud_stream_t	*as;
	void		*arg;
{
	int	up = (int)arg;
	extern void	audio_flush_cmdlist();

	if (up) {
		if (as == as->record_as) {
			ATRACEI(dbri_bri_up, 'rego', as);
			AsToDs(as)->recv_eol = TRUE;
			audio_process_record(as);
		}
	} else {
		ATRACEI(dbri_bri_down, 'bri-', as);
		dbri_stop(as);
		audio_flush_cmdlist(as);
	}
	return (0);
}

/*
 * Flush all pipes associated with a TE or NT.
 * XXX Should we flush output pipes only? Abort input pipes?
 */
void
dbri_bri_down(driver, bri_type)
	dbri_dev_t	*driver;
	isdn_interface_t	bri_type;
{
	if (bri_type != ISDN_TYPE_TE && bri_type != ISDN_TYPE_NT)
		return;

	(void) dbri_bri_func(driver, bri_type, dbri_bri_up_down_subr,
		(void *)0);
}

/*
 * Restart all receive pipes associated with a TE or NT.
 */
void
dbri_bri_up(driver, bri_type)
	dbri_dev_t	*driver;
	isdn_interface_t	bri_type;
{
	ATRACEI(dbri_bri_up, 'bri+', bri_type);

	if (bri_type != ISDN_TYPE_TE && bri_type != ISDN_TYPE_NT)
		return;
	(void) dbri_bri_func(driver, bri_type, dbri_bri_up_down_subr,
		(void *)1);
}

/*
 * Run a function on all open BRI aud_streams. Return code is OR of all
 * return codes.
 */
int
dbri_bri_func(driver, bri_type, func, arg)
	dbri_dev_t		*driver;
	isdn_interface_t	bri_type;
	int			(*func)();
	void			*arg;
{
	int		i;
	int		r = 0;

	if (bri_type != ISDN_TYPE_TE && bri_type != ISDN_TYPE_NT)
		return (r);

	for (i = 0; i < Nstreams; ++i) {
		aud_stream_t		*asp;

		if (bri_type != Chan_tab[i].isdntype)
			continue;

		asp = &driver->ioc[i].as;

		if (!ISHWCONNECTED(asp))
			continue;
		r |= func(asp, arg);
	}
	return (r);
}

dbri_enable_te(driver)
	dbri_dev_t	*driver;
{
	dbri_stream_t	*ds;

	ds = &driver->ioc[(int)DBRI_TE_D_OUT];

	ds->i_var.enable = TRUE;
	if (ds->i_var.power == ISDN_PH_PARAM_POWER_ON)
		driver->chip->n.sts |= DBRI_STS_T;
}

dbri_disable_te(driver)
	dbri_dev_t	*driver;
{
	dbri_stream_t	*ds;

	ds = &driver->ioc[(int)DBRI_TE_D_OUT];

	ds->i_var.enable = FALSE;
	driver->chip->n.sts &= ~DBRI_STS_T;
}

dbri_enable_nt(driver)
	dbri_dev_t	*driver;
{
	dbri_stream_t	*ds;

	ds = &driver->ioc[(int)DBRI_NT_D_OUT];

	ds->i_var.enable = TRUE;
	if (ds->i_var.power == ISDN_PH_PARAM_POWER_ON)
		driver->chip->n.sts |= DBRI_STS_N;
}

dbri_disable_nt(driver)
	dbri_dev_t	*driver;
{
	dbri_stream_t	*ds;

	ds = &driver->ioc[(int)DBRI_NT_D_OUT];

	ds->i_var.enable = FALSE;
	driver->chip->n.sts &= ~DBRI_STS_N;
}
