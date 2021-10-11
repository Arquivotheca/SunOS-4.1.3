#ident "@(#) dbri_isr.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

/*
 * Interrupt service routines for the AT&T DBRI chip.
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
#include <sundev/mbvar.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <machine/mmu.h>

#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>
#include <sun/isdnio.h>
#include <sun/dbriio.h>
#include <sbusdev/dbrivar.h>
#include "audiodebug.h"


void	dbri_receive_eol();
void	dbri_chil_intr();
void	dbri_error_msg();
void	dbri_fxdt_intr();
void	dbri_hold_f3();
void	dbri_hold_g1();
void	dbri_primitive_ph_ai();
void	dbri_primitive_ph_di();
void	dbri_primitive_mph_ai();
void	dbri_primitive_mph_di();
void	dbri_primitive_mph_ei1();
void	dbri_primitive_mph_ei2();
void	dbri_primitive_mph_ii_c();
void	dbri_pr_packet();
void	dbri_primitive_mph_ii_d();
void	dbri_bri_up();
void	dbri_bri_down();


int	Dbri_rverbose = 0;	/* Receive packet-level debugging */
int	Dbri_debug_sbri = 0;	/* SBRI Activation/Deactivation messages */

#define R3(v) driver->chip->r.reg3=(v&0xffffff)<<8

/*
 * Need to remember that interrupts can occur without any devices open as
 * they are enabled at the end of the attach routine. Only upon unload
 * will they be disabled.
 */
int
dbri_intr()
{
	int		s;
	int		unit;
	unsigned int	intr_reg;		/* intr register status */
	dbri_intrq_ent_t intr;			/* interrupt queue word */
	dbri_dev_t	*driver;
	pipe_tab_t	*pipep;
	serial_status_t	*serialp;
	int		serviced = 0;

	s = splstr();

	driver = &Dbri_devices[0];
	for (unit = 0; unit < Ndbri; unit++, driver++) {
		if ((driver == NULL) || (driver->chip == NULL))
			continue;

		intr_reg = driver->chip->n.intr; /* auto clr on read */

		if (intr_reg)
			serviced = 1;

		if (!(intr_reg & (DBRI_INTR_REQ |
				    DBRI_INTR_LATE_ERR |
				    DBRI_INTR_BUS_GRANT_ERR |
				    DBRI_INTR_BURST_ERR |
				    DBRI_INTR_MRR_ERR))) {
			/* No interrupt from this DBRI */
			continue;
		}

		if (intr_reg & DBRI_INTR_LATE_ERR)  {
			/*
			 * XXX - There is nothing we are going to be able to do
			 * about this one.  If the system is really
			 * hosed, we're sure to hit more serious
			 * interrupts later.  So just re-enable DBRI
			 * master mode and continue on.
			 */
			ATRACEI(dbri_intr, 'LATE', intr_reg);
			printf("dbri: Multiple Late Error on SBus");
			driver->chip->n.sts &= ~(DBRI_STS_D);
		}

		if (intr_reg & DBRI_INTR_BUS_GRANT_ERR)  {
			/*
			 * XXX - we don't know where this occurred so we
			 * cannot set any error flags to let a user
			 * process know that there is a data
			 * discontinuity.  This is BAD.
			 */
			ATRACEI(dbri_intr, 'BGNT', intr_reg);
			printf("dbri: Lost Bus Grant on SBus");
		}

		if (intr_reg & DBRI_INTR_BURST_ERR)  {
			/*
			 * XXX - This is mostly informational since
			 * nothing bad happens until we get the MRR
			 * interrupt.  For now, ignore it.  We set the
			 * burst-mode enables at initialization time and
			 * when this interrupt occurs DBRI automatically
			 * figures out the next smaller size burst to
			 * use.  We probably do not want to re-enable
			 * burst sizes that have failed.
			 */
			ATRACEI(dbri_intr, 'BRST', intr_reg);
			printf("dbri: Burst Error on SBus\n");
		}

		if (intr_reg & DBRI_INTR_MRR_ERR) {
			ATRACEI(dbri_intr, 'MRR ', intr_reg);
			printf("dbri: Multiple Error Ack on SBus\n");

			if (driver->chip->n.sts & DBRI_STS_S) {
				printf("dbri: 16-word burst disabled\n");
				driver->chip->n.sts &= ~(DBRI_STS_S|DBRI_STS_D);
			} else if (driver->chip->n.sts & DBRI_STS_E) {
				printf("dbri: 8-word bursts disabled\n");
				driver->chip->n.sts &= ~(DBRI_STS_E|DBRI_STS_D);
			} else if (driver->chip->n.sts & DBRI_STS_G) {
				printf("dbri: 4-word bursts disabled\n");
				driver->chip->n.sts &= ~(DBRI_STS_G|DBRI_STS_D);
			} else {
				printf("dbri: single-word transfer failed?!\n");
				driver->chip->r.reg3 = 0xbfbfbf00;
				driver->chip->n.sts &= ~(DBRI_STS_D);
			}
		}

		/*
		 * If we don't have to check the queue, don't go to memory.
		 */
		if (!(intr_reg & DBRI_INTR_REQ))
			continue;

		intr = driver->intq.curqp->intr[driver->intq.off];
		if (intr.f.channel >= DBRI_INT_MAX_CHNL)
			pipep = 0;
		else
			pipep = &driver->ptab[intr.f.channel];

		while (intr.f.ibits == DBRI_INT_IBITS) {

			intr.f.ibits = unit;	/* XXX debug */
			driver->chip->r.reg3 = intr.word32 & ~0xFF; /* XXX */

			/* Clear out IBITS in interrupt queue */
			driver->intq.curqp->intr[driver->intq.off].f.ibits = 0;

			switch (intr.f.code) {
			case DBRI_INT_BRDY:	/* Receive buffer ready */
				ATRACEI(dbri_recv_intr, ATR_BRDY, intr.word32);
				dbri_recv_intr(driver, intr);
				break;

			case DBRI_INT_MINT:	/* Marked interrupt in RD/TD */
				ATRACEI(dbri_intr, 'MINT', intr.word32);
				/* XXX - Currently not used (masked) */
				dprintf("dbri_intr: intr=0x%x MINT interrupt\n",
					intr.word32);
				break;

			case DBRI_INT_IBEG:	/* Flag to idle transition */
				serialp = &driver->ser_sts;
				serialp->ibeg++;
				ATRACEI(dbri_intr, 'IBEG', intr.word32);
				break;

			case DBRI_INT_IEND:	/* Idle to flag transition */
				serialp = &driver->ser_sts;
				serialp->iend++;
				ATRACEI(dbri_intr, 'IEND', intr.word32);
				break;

			case DBRI_INT_EOL:	/* End of list */
				ATRACEI(dbri_intr, ATR_EOL, intr.word32);

				/* Check if stream closed or invalid EOL */
				if (pipep->as == NULL) {
					ATRACEI(dbri_intr, 'EOL-',
						intr.f.channel);
					break;
				}

				/* check if receive direction */
				if (!ISXMITDIRECTION(pipep->as))
					dbri_receive_eol(AsToDs(pipep->as));
				pipep->as->info.active = FALSE;

				break;

			case DBRI_INT_CMDI: /* 1st word cmd read by DBRI */
				ATRACEI(dbri_intr, 'CMDI', (int)intr.word32);
				/*
				 * If REX commands are ever used for
				 * synchronization, they will be processed
				 * here.
				 */
				driver->ser_sts.cmdi++;
				break;

			case DBRI_INT_XCMP: /* Xmit Frame complete */
				ATRACEI(dbri_intr, ATR_XCMP,
					(int)intr.word32);
				dbri_xmit_intr(driver, intr);
				break;

			case DBRI_INT_SBRI: /* BRI status change info */
				ATRACEI(dbri_intr, 'SBRI', intr.word32);
				dbri_sbri_intr(driver, intr);
				break;

			case DBRI_INT_FXDT: /* Fixed data change */
				ATRACEI(dbri_intr, 'FXDT', intr.word32);

				/*
				 * XXX - this is in dbri_mmcodec.c.  It
				 * will get moved into this file with the
				 * rest of the handlers later
				 */
				dbri_fxdt_intr(driver, intr);
				break;

			case DBRI_INT_CHIL: /* CHI lost frame sync */
				ATRACEI(dbri_intr, 'CHIL', intr.word32);
				if (intr.f.channel == DBRI_INT_CHI_CHNL) {
					dbri_chil_intr(driver, intr);
				} else {
					/*
					 * XXX need to match code here to
					 * XXX DBRI 2/27/92 spec
					 */
					ATRACEI(dbri_intr, 'COLL', intr.word32);
					if (!ISTEDCHANNEL(pipep->as)) {
printf("dbri: Unrecoverable TE D-channel collision on an illegal channel!\n");
						break;
					}
printf("dbri: Unrecoverable TE D-channel collision!\n");
					dbri_stop(pipep->as);
					DELAY(250);
					dbri_start(pipep->as);
				}
				break;

			case DBRI_INT_DBYT: /* Dropped byte frame slip */
				ATRACEI(dbri_intr, 'DBYT', intr.word32);
				/*
				 * This can only happen on a
				 * serial-to-serial pipe.
				 *
				 * XXX - Do we need to set an error flag
				 * somewhere?
				 */
				printf("dbri: Dropped byte frame slip.\n");
				break;

			case DBRI_INT_RBYT: /* Repeated byte frame slip */
				ATRACEI(dbri_intr, 'RBYT', intr.word32);
				/*
				 * This can only happen on a
				 * serial-to-serial pipe.
				 *
				 * XXX - Do we need to set an error flag
				 * somewhere?
				 */
				printf("dbri: Repeated byte frame slip\n");
				break;

			case DBRI_INT_LINT:	/* Lost interrupt */
				ATRACEI(dbri_intr, 'LINT', intr.word32);
				printf("dbri: Lost interrupts\n");
				break;

			case DBRI_INT_UNDR:	/* DMA underrun */
				ATRACEI(dbri_intr, 'UNDR', intr.word32);

				dprintf("dbri_intr: intr = 0x%x DMA underrun\n",
					intr.word32);
				break;

			default:
				ATRACEI(dbri_intr, 'unkn', intr.word32);
				printf("dbri: No interrupt code given\n");
				break;
			} /* switch */

			/*
			 * Advance intqp to point to next available
			 * intr status word
			 */
			if (driver->intq.off == (DBRI_MAX_QWORDS - 1)) {
				driver->intq.curqp = ((dbri_intq_t *)
					KERN_ADDR(driver,
					driver->intq.curqp->nextq));
				driver->intq.off = 0;
			} else {
				driver->intq.off++;
			}

			/* get next intr status word */
			intr = driver->intq.curqp->intr[driver->intq.off];
			if (intr.f.channel >= DBRI_INT_MAX_CHNL)
				pipep = 0;
			else
				pipep = &driver->ptab[intr.f.channel];
		} /* while */
	} /* for each interface */

	(void) splx(s);
	return (serviced);
} /* dbri_intr */


void
dbri_receive_eol(ds)
	dbri_stream_t	*ds;
{
	dbri_cmd_t	*dcp;
	int		n;
	int		neof = 0;
	int		ncom = 0;
	int		done = 0;

	ATRACEI(dbri_receive_eol, ATR_EOL, ds->pipe);
	AsToDs(ds->as.control_as)->d_stats.recv_eol++;
	ds->recv_eol = TRUE;

	if (!ds->cmdptr) {
		/*
		 * There are no receive buffers on the list.  Call
		 * process_record to try to queue some more (if that is
		 * appropriate.)
		 */
		ATRACEI(dbri_receive_eol, ATR_DROPPACKET, ds->pipe);
		if (ds->as.info.active)
			ds->audio_uflow = TRUE;
		audio_process_record((aud_stream_t *)ds);
		return;
	}

	/*
	 * Scan the receive buffer list for complete frames. If there are
	 * some complete frames, then normal processing will clear the
	 * EOL condition. If the list is full of completed fragments, but
	 * no completed frames, and the freelist is empty, then special
	 * action needs to be taken.
	 */
	for (dcp = ds->cmdptr, n = 0; dcp; dcp = dcp->nextio, ++n) {
		if (dcp->md.r.f.com)
			ncom++;
		if (dcp->md.r.f.eof)
			neof++;
		if (dcp->cmd.done)
			done++;
	}

	dprintf("dbri:Rcv EOL; %d frags, %d complete, %d marked eof, %d done\n",
		    n, ncom, neof, done);

	/*
	 * If there is no hope of audio_process_record solving this EOL
	 * problem, then mark the entire list for discarding.
	 */
	if ((ncom == n) && (neof == 0) && (ds->as.cmdlist.free == NULL)) {
		int	total = 0;

		ATRACEI(dbri_receive_eol, ATR_NONE, ds->pipe);
		(void) printf("dbri: REOL: %d frags, %d com, %d eof.\n",
		    n, ncom, neof);
		/*
		 * The receive list has been completely filled with a "huge"
		 * packet (huge means larger that the list.)
		 */
		for (dcp = ds->cmdptr, n = 0;
		    dcp != NULL;
		    dcp = dcp->nextio, ++n) {
			dcp->cmd.done = TRUE;
			dcp->cmd.skip = TRUE;
			total += dcp->md.r.f.cnt;
#ifdef	sun4m
#ifndef lint	/* XXX lint barfs on arg 1 */
			mb_mapfree(dvmamap, (int *)&dcp->sb_addr);
#endif /* lint */
#endif	/* sun4m */
		}
		ds->d_stats.recv_error_octets += total;
	}

	audio_process_record((aud_stream_t *)ds);

	return;
}


/* dbri_recv_intr - process a single packet or a single audio buffer. */
void
dbri_recv_intr(driver, intr)
	dbri_dev_t	*driver;
	dbri_intrq_ent_t intr;			/* interrupt queue word */
{
	unsigned int	status;
	pipe_tab_t	*pipep;
	dbri_cmd_t	*dcp;
	struct {
		dbri_cmd_t *head;
		dbri_cmd_t *tail;
	}		packet;
	aud_stream_t	*as;
	dbri_stream_t	*ds;
	int		error = FALSE;
	unsigned int	cnt;
	int		total = 0;
	int		hdlc;
#if defined(DBRI_SWFCS)
	extern int	Dbri_fcscheck;
#endif

	pipep = &driver->ptab[intr.f.channel];
	as = pipep->as;

	/* Check if stream closed */
	if (as == NULL) {
		/* Some other piece of code needs to flush the aud_cmd list. */
		ATRACEI(dbri_recv_intr, '-na-', intr.word32);
		return;
	}

	/*
	 * "Processed" commands never show up on the DBRI's IO list.
	 * Completed packets are removed from the DBRI IO list by this
	 * routine.
	 */
	ds = AsToDs(as); /* Assign dbri stream pointer */

	/* Just go through list to end to check for bad status */
	dcp = ds->cmdptr;	/* current head of DBRI cmdlist */
	if (dcp == NULL) {
		ATRACEI(dbri_recv_intr, 'null', intr.word32);
		return;
	}
	if (!dcp->md.r.f.com) {
		/*
		 * XXX - may want to put a printf here since this if
		 * we get a BRDY but the first one isn't complete, then
		 * something is very wrong
		 */
		ATRACEI(dbri_recv_intr, 'ncmp', dcp->md.r.word32[0]);
		return;
	}

	packet.head = dcp;	/* Any new packet will start here */
	packet.tail = NULL;

	if (dcp == NULL) {
		driver->chip->r.reg3 = 0xd0a00000;
		ATRACEI(dbri_recv_intr, 'emty', 0);
	}

	/*
	 * Search for a valid EOF
	 */
	while (dcp != NULL && dcp->md.r.f.com) {	/* buffer complete */
		if (dcp->md.r.f.eof) {
			ATRACEI(dbri_recv_intr, 'eof ', dcp);
			packet.tail = dcp; /* Save pointer to end of packet */
			break;
		} else {
			driver->chip->r.reg3 = 0x00feeb00;
			ATRACEI(dbri_recv_intr, '!eof', dcp);
		}
		dcp = dcp->nextio;
	}

	if (packet.tail == NULL)  {
		ATRACEI(dbri_recv_intr, 'part', dcp);
		return;
	}

	/*
	 * There is at least one complete packet on the receive chain.
	 * Process one packet only.
	 */

	/*
	 * Start and end of packet have been found. Mark fragments as
	 * done and set lastfragment pointer.
	 */
	for (dcp = packet.head; dcp != NULL; dcp = dcp->nextio) {
		dcp->cmd.done = TRUE;
		dcp->cmd.lastfragment = &packet.tail->cmd;

#ifdef	sun4m
#ifndef lint	/* XXX lint barfs on arg 1 */
		mb_mapfree(dvmamap, (int *)&dcp->sb_addr);
#endif /* lint */
#endif
#ifdef	sun4c
		/*
		 * XXX - Could probably do this after we check for errors
		 * and if we aren't going to send the data anywhere, don't
		 * bother wasting the cycles
		 */
		vac_flush((caddr_t)dcp->cmd.data,
		    dcp->cmd.enddata - dcp->cmd.data);
#endif

		if (dcp == packet.tail)
			break;
	}

	/*
	 * Process receive errors
	 */
	status = packet.tail->md.r.f.status; 		/* check status */

	if (Dbri_rverbose) {
		dbri_pr_packet(packet.head, 1);
		(void) printf ("\tstatus: %x\n", status);
	}

	if ((pipep->sdp & DBRI_SDP_MODEMASK) != DBRI_SDP_TRANSPARENT)
		hdlc = TRUE;
	else
		hdlc = FALSE;

	if (status & (DBRI_RMD_CRC|DBRI_RMD_BBC|DBRI_RMD_ABT|DBRI_RMD_OVRN)) {


		if (Dbri_rverbose) {
			/*
			 * %b format spec: number, then string; 1st byte
			 * of string is base, each bit spec is "\XXname"
			 * where XX is octal (power of 2)+1
			 */
			(void) printf("[RE%b:%u]",
				    status, "\20\10CRC\7BBC\6ABT\4OVRN",
				    intr.f.channel);
		}

		if (status & DBRI_RMD_OVRN) {
			ds->audio_uflow = TRUE;
			ds->d_stats.dma_underflow++;
		}
		if (status & DBRI_RMD_CRC) {
			error = TRUE;
			ds->d_stats.crc_error++;
		}
		if (status & DBRI_RMD_BBC) {
			error = TRUE;
			ds->d_stats.badbyte_error++;
		}
		if (status & DBRI_RMD_ABT) {
			error = TRUE;
			ds->d_stats.abort_error++;
		}
	}

#if defined(DBRI_SWFCS)
	if (Dbri_fcscheck && !error && hdlc) {
		error = dbri_checkfcs(packet.head);

		if (error)
			printf("dbri: Dropped packet with bad SW FCS\n");
	}
#endif

	/*
	 * Second pass thru data to adjust data pointers,
	 * and adjust sample count.
	 */
	total = 0;

	for (dcp = packet.head; dcp; dcp = dcp->nextio) {
		ASSERT(dcp->md.r.f.com);

		cnt = dcp->md.r.f.cnt;

		/*
		 * For HDLC mode, remove the 2-byte CRC at the end of the
		 * frame.
		 *
		 * XXX - Will CRC ever be given to driver users?
		 */
		if (hdlc) {
			/*
			 * If HDLC mode, remove CRC from end of packet.
			 * If the last fragment contains one byte, it is
			 * from the CRC
			 */
			if (dcp->md.r.f.eof)
				cnt -= (dcp->md.r.f.cnt == 1) ? 1 : 2;
			else if (dcp->nextio->md.r.f.eof &&
				    dcp->nextio->md.r.f.cnt == 1)
				cnt -= 1;
		}

		if (error)
			dcp->cmd.skip = TRUE;

		dcp->cmd.data += cnt;
		total += cnt;

		if (dcp == packet.tail)
			break;
	}
	ASSERT(dcp != NULL);	/* above loop stays on last cmd */

	/*
	 * Update dbri stream structure command pointers
	 * Delete the complete packet from the DBRI IO list.
	 *
	 * The "processed" fragment doesn't show up on the DBRI IO list
	 * even though the DBRI still partially "owns" it.  This is
	 * correct behavior.
	 */
	ds->cmdptr = packet.tail->nextio;
	if (packet.tail->nextio == NULL)
		ds->cmdlast = NULL;

	/*
	 * Updata audio samples counter
	 */
	if (!hdlc) {
		if (as->info.channels == 0 || (as->info.precision/8) == 0)
			total /= 1; /* XXX */
		else
			total /= (as->info.channels * (as->info.precision/8));
	} else {
		total = 1;	/* packets */
	}

	if (error) {
		ATRACEI(dbri_recv_intr, 'ERR ', ds->cmdptr);
		ds->d_stats.recv_error_octets += total;
		ds->i_info.receive.errors += 1;
	} else {
		ATRACEI(dbri_recv_intr, 'totl', total);
		ds->i_info.receive.octets += total;
		ds->i_info.receive.packets++;
	}

	if (!error || !hdlc)
		ds->samples += total;

	audio_process_record(pipep->as);

	return;
} /* dbri_recv_intr */


/*
 * dbri_xmit_intr - process a single packet or a single audio buffer
 */
void
dbri_xmit_intr(driver, intr)
	dbri_dev_t	*driver;
	dbri_intrq_ent_t intr;			/* interrupt queue word */
{
	dbri_cmd_t	*dcp;
	unsigned int	status;
	pipe_tab_t	*pipep;
	dbri_stream_t	*ds;
	dbri_cmd_t	*err_dcp = NULL;
	int		hdlc;
	int		total;
	int		octets;

	pipep = &driver->ptab[intr.f.channel];

	/* Check if stream closed */
	if (pipep->as == NULL) {
		ATRACEI(dbri_xmit_intr, 'bad!', intr.word32);
		driver->chip->r.reg3 = 0x00dead00;
		dprintf("dbri_xmit_intr: XCMP on closed stream, 0x%x\n",
			intr.word32);
		return;
	}
	ds = AsToDs(pipep->as);		/* Get dbri stream pointer */

	/*
	 * Go thru list to end to check for bad status marking all md's done
	 */
	dcp = ds->cmdptr;		/* head of DBRI cmdlist */

	/*
	 * Check if 1st cmd already processed.
	 */
	while ((dcp != NULL) && dcp->cmd.processed) {
		/*
		 * NB: Device routines follow the nextio list, the aud_cmd
		 * "next" pointer is irrelevent here.
		 */
		dcp = dcp->nextio;
	}

	if (dcp == NULL) {
		dprintf("dbri_xmit_intr: dcp == NULL\n");
		return;
	}

	if ((pipep->sdp & DBRI_SDP_MODEMASK) != DBRI_SDP_TRANSPARENT)
		hdlc = TRUE;
	else
		hdlc = FALSE;

	status = dcp->md.t.f.status;
	octets = 0;
	total = 0;
	while ((dcp != NULL) && (status & (DBRI_TMD_TBC | DBRI_TMD_ABT))) {
		int	samples;

		dcp->cmd.done = TRUE;

		if (status & DBRI_TMD_UNR) {
			if (!hdlc && !ds->last_flow_err) {
				ds->audio_uflow = TRUE;
			}
		}
		/*
		 * XXX DBRI[de] DBRI HW BUG #3034, BugId 1089299
		 * DBRI continues to report flow errors when only one error
		 * ocurrs. last_flow_err is used to supress these
		 * repeated error reports.
		 */
		switch (driver->dbri_version) {
		case 'd':
		case 'e':
			ds->last_flow_err = status & DBRI_TMD_UNR;
			break;
		default:
			ds->last_flow_err = 0;
			break;
		} /* switch */

		if (((status & DBRI_TMD_ABT) &&
		    ((pipep->sdp & DBRI_SDP_MODEMASK) !=
		    DBRI_SDP_TRANSPARENT)) ||
		    (status & DBRI_TMD_UNR)) {
			if (err_dcp == NULL) /* Grab first error status */
				err_dcp = dcp;
		}
#ifdef	sun4m
		if (dcp->sb_addr != 0) {
#ifndef lint	/* XXX lint barfs on arg 1 */
			mb_mapfree(dvmamap, (int *)&dcp->sb_addr);
#endif /* lint */
		}
#endif
		/*
		 * Updata audio samples counter
		 */
		samples = dcp->md.t.f.cnt;
		octets += samples;
		if (!hdlc) {
			if ((pipep->as->info.precision/8) == 0)
				samples /= 1; /* XXX */
			else
				samples /= pipep->as->info.channels *
					(pipep->as->info.precision/8);
		} else {
			samples = 1; /* packets */
		}
		total += samples;

		if (dcp->md.t.f.eof)
			break;

		dcp = dcp->nextio;
		if (dcp != NULL)
			status = dcp->md.t.f.status;
	}

	if (dcp == NULL) {
		dprintf("dbri_intr: dcp NULL on transmit\n");
		return;
	}

	/* If there was an error, retrieve the md where the error occurred */
	if (err_dcp != NULL) {
		dcp = err_dcp;
		status = dcp->md.t.f.status;

		if (status & DBRI_TMD_UNR) {
			ds->d_stats.dma_underflow++;
			ds->samples += total;
		}
		if (status & DBRI_TMD_ABT)
			ds->d_stats.abort_error++;
		ds->i_info.transmit.errors++;
	} else {
		ds->i_info.transmit.packets++;
		ds->i_info.transmit.octets += octets;
		ds->samples += total; /* adjust sample count */
	}

	/* Update dbri stream structure command pointers */
	ds->cmdptr = dcp;
	if (dcp == NULL)
		ds->cmdlast = NULL;

	audio_process_play(pipep->as); /* EOF - release xmit buffers */

	return;
} /* dbri_xmit_intr */


static char *
pr_code(code)
	int	code;
{
	static char	*codes[16] = {
		"****", "BRDY", "MINT", "IBEG",
		"IEND", "EOL", "CMDI", "****",
		"XCMP", "SBRI", "FXDT", "CHIL",
		"DBYT", "RBYT", "LINT", "UNDR",
	};

	return (codes[code]);
}


static char *
pr_channel(ch)
	int	ch;
{
	static char	buf[100];
	caddr_t		tbuf = buf;		/* for lint */

	switch (ch) {
	case DBRI_INT_TE_CHNL:
		return ("TE_status");
	case DBRI_INT_NT_CHNL:
		return ("NT_status:");
	case DBRI_INT_CHI_CHNL:
		return ("CHI_status:");
	case DBRI_INT_REPORT_CHNL:
		return ("Report_Command_channel_intr_status");
	case DBRI_INT_OTHER_CHNL:
		return ("Other_status");
	default:
		(void) sprintf(tbuf, "Channel_%d", ch);
		return (buf);
	}
}


static void
pr_interrupt(intr)
	dbri_intrq_ent_t	*intr;
{
	static char	*tss[8] = {
		"G1/F1",	/* 0 */
		"**/**",	/* 1 */
		"**/F8",	/* 2 */
		"**/F3",	/* 3 */
		"**/F4",	/* 4 */
		"**/F5",	/* 5 */
		"G2/F6",	/* 6 */
		"G3/F7",	/* 7 */
	};

	(void) printf("I=%d channel=%s code=%s, field=0x%x",
		    intr->f.ibits,
		    pr_channel((int)intr->f.channel),
		    pr_code((int)intr->f.code),
		    (unsigned int)intr->f.field);

	if (intr->f.code != DBRI_INT_SBRI) {
		(void) printf("\n");
		return;
	}

	(void) printf(" %b %s\n",
		    intr->f.field,
		    "\20\13vta\12berr\11ferr\10mfm\07fsc\06rif4\05rif0\04act",
		    tss[intr->code_sbri.tss]);
}

/*
 * dbri_sbri_intr - process a SBRI interrupt DBRI always has the say as
 * to what the current state is. However, old state must be known in
 * order to properly implement Primitives.
 */
void
dbri_sbri_intr(driver, intr)
	dbri_dev_t	*driver;
	dbri_intrq_ent_t intr;
{
	aud_stream_t	*as;
	pipe_tab_t	*pipep;
	serial_status_t	*serialp;

	serialp = &driver->ser_sts;

	/*
	 * Detectible Bit Error
	 */
	if (intr.code_sbri.berr) {
		ATRACEI(dbri_sbri_intr, 'BERR', intr.word32);
		if (intr.f.channel == DBRI_INT_TE_CHNL)
			serialp->te_primitives.berr++;
		else if (intr.f.channel == DBRI_INT_NT_CHNL)
			serialp->nt_primitives.berr++;
		/* XXX - any other action? */
	}

	/*
	 * Frame Sync Error
	 */
	if (intr.code_sbri.ferr) {
		ATRACEI(dbri_sbri_intr, 'FERR', intr.word32);
		if (intr.f.channel == DBRI_INT_TE_CHNL)
			serialp->te_primitives.ferr++;
		else if (intr.f.channel == DBRI_INT_NT_CHNL)
			serialp->nt_primitives.ferr++;
		/* XXX - any other action? */
	}

	if (Dbri_debug_sbri)
		pr_interrupt(&intr);

	if (intr.f.channel == DBRI_INT_TE_CHNL) {
		struct dbri_code_sbri   old_te_sbri;

		old_te_sbri = serialp->te_sbri;
		serialp->te_sbri = intr.code_sbri;

		pipep = &driver->ptab[DBRI_PIPE_TE_D_IN];

		/* don't do anything if pipe not connected */
		if (!ISPIPEINUSE(pipep))
			return;

		as = pipep->as->play_as; /* get control as */

		/*
		 * If new state is not F3, then remove PH-ACTIVATEreq
		 * since it's only use, for the TE, is to get out of F3.
		 */
		if (serialp->te_sbri.tss != DBRI_TEINFO0_F3)
			dbri_hold_f3(driver);

		/*
		 * XXX DBRI hangs if synchronization is lost.
		 * XXX So, flush queues and stop IO if this is a transition
		 * XXX out of F7 and this hack is enabled.
		 */
		if ((AsToDs(as)->i_var.norestart == 0) &&
		    (old_te_sbri.tss == DBRI_TEINFO3_F7) &&
		    (serialp->te_sbri.tss != DBRI_TEINFO3_F7)) {
			ATRACEI(dbri_sbri_intr, 'Thck', intr.word32);
			dbri_bri_down(driver, ISDN_TYPE_TE);
		}

		if (old_te_sbri.tss != serialp->te_sbri.tss) {
			switch (serialp->te_sbri.tss) {
			case DBRI_TEINFO0_F1:
#ifdef lint			/* XXX - Because the next part is if'd out */
				dbri_primitive_mph_ii_d(as);
#endif
#if 0
				AsToDs(as)->i_info.activation = ISDN_UNPLUGGED;
				/*
				 * F1 and F2 are not distinguishable by
				 * the driver
				 *
				 * f3_f1	mph-ii(d)
				 * f4_f1	mph-ii(d), mph-di, ph-di
				 * f5_f1	mph-ii(d), mph-di, ph-di
				 * f6_f1	mph-ii(d), mph-di, ph-di
				 * f7_f1	mph-ii(d), mph-di, ph-di
				 * f8_f1	mph-ii(d), mph-di, ph-di
				 */
				dbri_primitive_mph_ii_d(as);
				if (old_te_sbri.tss != DBRI_TEINFO0_F3) {
					dbri_bri_down(driver, ISDN_TYPE_TE);
					dbri_primitive_mph_di(as);
					dbri_primitive_ph_di(as);
				}
				break;
#else /* 0 */
				/*
				 * Given the implementation of this
				 * driver and the implementation of DBRI,
				 * transitions to F1 reported by DBRI are
				 * to be interpreted as transitions to
				 * F3.
				 */
				serialp->te_sbri.tss = DBRI_TEINFO0_F3;
				/* FALL THROUGH */
#endif /* !0 */

			case DBRI_TEINFO0_F3:
				AsToDs(as)->i_info.activation =
					ISDN_DEACTIVATED;
				/*
				 * f1_f3	mph-ii(c)
				 * f4_f3	mph-di, ph-di	// T3 expiry
				 * f5_f3	mph-di, ph-di	// T3 expiry
				 * f6_f3	mph-di, ph-di	// T3 expiry
				 * 			or recv I0
				 * f7_f3	mph-di, ph-di	// receive I0
				 * f8_f3	mph-di, ph-di, mph-ei2
				 */
				if (old_te_sbri.tss != DBRI_TEINFO0_F1) {
					dbri_bri_down(driver, ISDN_TYPE_TE);
					dbri_primitive_mph_di(as);
					dbri_primitive_ph_di(as);
					if (old_te_sbri.tss ==
					    DBRI_TEINFO0_F8) {
						dbri_primitive_mph_ei2(as);
					}
				} else {
					/* really from F2 */
#if 0
					/*
					 * DBRI "falsely" reports F1 when
					 * T3 expires and then driver
					 * toggles the T bit in reg0.
					 *
					 * Since dbri_primitive_mph_ii_c
					 * should only/always happen once
					 * immediately after open, this
					 * line has been moved into the
					 * TE initialization code in
					 * dbri_setup_ntte().
					 */
					dbri_primitive_mph_ii_c(as);
#endif	/* 0 */
				}
				break;

			case DBRI_TEINFO1_F4:
				AsToDs(as)->i_info.activation =
					ISDN_ACTIVATE_REQ;
				break;

			case DBRI_TEINFO0_F5:
				AsToDs(as)->i_info.activation =
					ISDN_ACTIVATE_REQ;
				break;

			case DBRI_TEINFO3_F6:
				/* AsToDs(as)->i_info.activation=no change; */
#if 0
				untimeout((int(*)())dbri_te_timer, (caddr_t)as);
#endif

				/*
				 * f1_f6	mph-ii(c)
				 * f7_f6	mph-ei1	// report error
				 * f8_f6	mph-ei2	// report recovery
				 * 		from error
				 * f3_f6	no-action
				 * f5_f6	no-action
				 */
				switch (old_te_sbri.tss) {
				case DBRI_TEINFO0_F1:
					/* really from F2 */
					dbri_primitive_mph_ii_c(as);
					break;
				case DBRI_TEINFO3_F7:
					dbri_primitive_mph_ei1(as);
					break;
				case DBRI_TEINFO0_F8:
					dbri_primitive_mph_ei2(as);
					break;
				}
				break;

			case DBRI_TEINFO3_F7:
				AsToDs(as)->i_info.activation = ISDN_ACTIVATED;
				untimeout((int(*)())dbri_te_timer, (caddr_t)as);

				/*
				 * f1_f7	mph-ii(c), ph-ai, mph-ai
				 * f3_f7	ph-ai, mph-ai
				 * f5_f7	ph-ai, mph-ai
				 * f6_f7	ph-ai, mph-ai, mph-ei2
				 * f8_f7	ph-ai, mph-ai, mph-ei2
				 */

				if (old_te_sbri.tss == DBRI_TEINFO0_F1) {
					/* really from F2 */
					dbri_primitive_mph_ii_c(as);
				}
				dbri_bri_up(driver, ISDN_TYPE_TE);
				dbri_primitive_ph_ai(as);
				dbri_primitive_mph_ai(as);

				if ((old_te_sbri.tss == DBRI_TEINFO3_F6) ||
				    (old_te_sbri.tss == DBRI_TEINFO0_F8)) {
					/* recovered from error */
					dbri_primitive_mph_ei2(as);
				}
				break;

			case DBRI_TEINFO0_F8:
				/*
				 * AsToDs(as)->i_info.activation = no change
				 *
				 * f7_f8	mph-e11
				 * f6_f8	mph-ei1
				 */

				/* report error */
				dbri_primitive_mph_ei1(as);

				break;
			}	/* switch */
		}
	} else {		/* NT channel */
		struct dbri_code_sbri   old_nt_sbri;

		serialp = &driver->ser_sts;
		old_nt_sbri = serialp->te_sbri;
		serialp->nt_sbri = intr.code_sbri;

		pipep = &driver->ptab[DBRI_PIPE_NT_D_IN];

		/* don't do anything if pipe not connected */
		if (!ISPIPEINUSE(pipep))
			return;

		as = pipep->as->play_as; /* get control as */

		/*
		 * If new state is not G1, then remove PH-ACTIVATEreq
		 * since it's only use, for the NT, is to get out of G1.
		 */
		if (serialp->nt_sbri.tss != DBRI_NTINFO0_G1)
			dbri_hold_g1(driver);


		/* If no state change, then no actions taken */
		if (old_nt_sbri.tss != serialp->nt_sbri.tss) {
			/*
			 * XXX DBRI hangs if synchronization is lost.
			 * XXX So, flush queues and stop IO if this is a
			 * XXX transition G3 and this hack is enabled.
			 */
			if ((AsToDs(as)->i_var.norestart == 0) &&
			    (old_nt_sbri.tss == DBRI_NTINFO4_G3) &&
			    (serialp->nt_sbri.tss != DBRI_NTINFO4_G3)) {
				ATRACEI(dbri_sbri_intr, 'Nhck', intr.word32);
				dbri_bri_down(driver, ISDN_TYPE_NT);
			}

			switch (serialp->nt_sbri.tss) {
			case DBRI_NTINFO0_G1:
				AsToDs(as)->i_info.activation =
					ISDN_DEACTIVATED;
				/*
				 * g2_g1  (really g4_g1)
				 * g3_g1  (really g4_g1)
				 * g4_g1  (this will appear as either
				 * 	g2_g1, or g3_g1)
				 */
				break;

			case DBRI_NTINFO2_G2:
				AsToDs(as)->i_info.activation =
					ISDN_DEACTIVATED;
				if (old_nt_sbri.tss == DBRI_NTINFO0_G1) {
					untimeout((int(*)())dbri_nt_timer,
						(caddr_t)as);
					timeout((int(*)())dbri_nt_timer,
						(caddr_t)as,
						hz*AsToDs(as)->i_var.t101/1000);
					ATRACEI(dbri_sbri_intr,
						't101',
						AsToDs(as)->i_var.t101);
				} else if (old_nt_sbri.tss ==
				    DBRI_NTINFO4_G3) {
					dbri_primitive_mph_di(as);
					dbri_primitive_mph_ei1(as);
				} else if (old_nt_sbri.tss == DBRI_NTINFO0_G4) {
					/* XXX - start timer T1 */
				}

				break;

			case DBRI_NTINFO4_G3:
				AsToDs(as)->i_info.activation = ISDN_ACTIVATED;
				dbri_bri_up(driver, ISDN_TYPE_NT);
				if (old_nt_sbri.tss == DBRI_NTINFO2_G2) {
					/* XXX - cancel T1 */
					dbri_bri_up(driver, ISDN_TYPE_NT);
					dbri_primitive_ph_ai(as);
					dbri_primitive_mph_ai(as);
				}
				break;

			case DBRI_NTINFO0_G4:
				AsToDs(as)->i_info.activation =
					ISDN_DEACTIVATED;
				/*
				 * XXX - DBRI does not generate
				 * transitions to this state. Transition
				 * to G4 is done only through expiry of
				 * T1 or receipt of MPH-DEACTIVATEreq.
				 */
				break;
			}
		}
	}
	return;
}				/* dbri_sbri_int */


void
dbri_pr_dbri_cmd(dcp, receive)
	dbri_cmd_t	*dcp;
	int		receive;
{
	(void) printf("0x%x:\n", (unsigned int)dcp);

	(void) printf("\tcmd.data=0x%x", (unsigned int)dcp->cmd.data);
	(void) printf(" cmd.enddata=0x%x", (unsigned int)dcp->cmd.enddata);
	(void) printf(" cmd.next=0x%x", (unsigned int)dcp->cmd.next);
	(void) printf(" cmd.lastfragment=0x%x\n",
		    (unsigned int)dcp->cmd.lastfragment);

	(void) printf("\tcmd.skip=%d", dcp->cmd.skip);
	(void) printf(" cmd.done=%d", dcp->cmd.done);
	(void) printf(" cmd.iotype=%d\n", dcp->cmd.iotype);
	(void) printf("\tcmd.processed=%d", dcp->cmd.processed);
	(void) printf(" cmd.dihandle=0x%x", (unsigned int)dcp->cmd.dihandle);
	(void) printf(" nextio=0x%x\n", (unsigned int)dcp->nextio);

	if (receive) {
		(void) printf("\tmd.r.f {");
		(void) printf(" eof=%d", dcp->md.r.f.eof);
		(void) printf(" com=%d", dcp->md.r.f.com);
		(void) printf(" cnt=%d", dcp->md.r.f.cnt);
		(void) printf(" status=0x%x\n", dcp->md.r.f.status);
		(void) printf("\tbufp=0x%x", (unsigned int)dcp->md.r.f.bufp);
		(void) printf(" fp=0x%x", (unsigned int)dcp->md.r.f.fp);
		(void) printf(" fint=%d", dcp->md.r.f.fint);
		(void) printf(" mint=%d", dcp->md.r.f.mint);
		(void) printf(" bcnt=%d", dcp->md.r.f.bcnt);
		(void) printf(" }\n");
	} else {
		(void) printf("\tmd.t.f {\n");
		(void) printf("\teof=%d", dcp->md.t.f.eof);
		(void) printf(" dcrc=%d", dcp->md.t.f.dcrc);
		(void) printf(" cnt=%d", dcp->md.t.f.cnt);
		(void) printf(" fint=%d", dcp->md.t.f.fint);
		(void) printf(" mint=%d", dcp->md.t.f.mint);
		(void) printf(" idl=%d\n", dcp->md.t.f.idl);
		(void) printf("\tfcnt=%d", dcp->md.t.f.fcnt);
		(void) printf(" bufp=0x%x\n", (unsigned int)dcp->md.t.f.bufp);
		(void) printf("\tfp=0x%x", (unsigned int)dcp->md.t.f.fp);
		(void) printf(" status=0x%x\n", dcp->md.t.f.status);
		(void) printf("\t}\n");
	}
}


void
dbri_pr_packet(dcp, receive)
	dbri_cmd_t	*dcp;
	int		receive;
{
	unsigned char	*p;
	unsigned char	*end;

	if (receive)
		(void) printf("RECEIVE PACKET:\n");
	else
		(void) printf("TRANSMIT PACKET:\n");

	for (dcp = dcp; dcp; dcp = dcp->nextio) {	/* buffer complete */
		dbri_pr_dbri_cmd(dcp, receive);

		p = dcp->cmd.data;
		if (receive) {
			end = p + dcp->md.r.f.cnt;
		} else {
			end = p + dcp->md.t.f.cnt;
		}
		(void) printf("%x: ", (unsigned int)p);
		while (p < end) {
			if (((int)p % 16) == 0) {
				(void) printf("\n%x: ", (unsigned int)p);
			}
			(void) printf("%x ", (unsigned int)*p);
			++p;
		}
		(void) printf("\n%x; \n", (unsigned int)p);

		if (dcp == (dbri_cmd_t *)dcp->cmd.lastfragment)
			break;
	}
	return;
}


#if defined(DBRI_SWFCS)
static unsigned short fcstab[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48,	0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
};


/*
 * dbri_checkfcs - paranoid re-checking of the FCS in a HDLC packet
 * to inform us of bugs in DBRI.  Returns TRUE if there is an error.
 */
int
dbri_checkfcs(dcp)
	dbri_cmd_t *dcp;	/* DBRI Command Pointer */
{
	register int databytes;
	register unsigned short fcs = 0xffff; /* Initial FCS value */
	register unsigned char *p;
	unsigned short dbri_fcs;
	unsigned long first4bytes;
	int error = FALSE;

	p = dcp->cmd.data;

	/*
	 * There are five types of fragment:
	 * 1) Data only
	 * 2) Data and complete FCS
	 * 3) Data and 1 byte of FCS
	 * 4) One byte of FCS only
	 * 5) FCS only
	 */
	databytes = dcp->md.r.f.cnt;
	if (databytes > 4) {
		first4bytes = (*p << 24) | (*(p+1) << 16) | (*(p+2) << 8) |
			(*(p+3));
	}
	if (dcp->md.r.f.eof) {
		if (databytes == 1) {
			/* Type 4 */
			databytes = 0;
		} else if (databytes >= 2) {
			/* Type 2 and Type 5 */
			databytes -= 2;
		} else {
			panic("dbri: unexpected receive packet type");
		}
	} else if (dcp->nextio->md.r.f.eof && (dcp->nextio->md.r.f.cnt == 1)) {
		/* Type 3 */
		--databytes;
	} else {
		/* Not eof and next is not eof: Type 1 */
		;
	}


	while (dcp != NULL && dcp->md.r.f.com) {
		while (databytes-- > 0)
			fcs = (fcs >> 8) ^ fcstab[(fcs ^ *p++) & 0xff];

		if (dcp->md.r.f.eof)
			break;

		/*
		 * Before advancing to the next fragment, check for an FCS byte
		 * in current fragment.
		 */
		if (!dcp->md.r.f.eof &&
		    dcp->nextio->md.r.f.eof &&
		    dcp->nextio->md.r.f.cnt == 1) {
			dbri_fcs = *p++ << 8;
		}

		if (dcp->nextio == NULL)
			break;

		dcp = dcp->nextio;
		p = dcp->cmd.data;
		databytes = dcp->md.r.f.cnt;

		if (dcp->md.r.f.eof) {
			if (databytes == 1) {
				/* Type 4 */
				databytes = 0;
			} else if (databytes >= 2) {
				/* Type 2 and Type 5 */
				databytes -= 2;
			} else {
				panic("dbri: unexpected receive packet type");
			}
		} else if (dcp->nextio->md.r.f.eof &&
		    (dcp->nextio->md.r.f.cnt == 1)) {
			/* Type 3 */
			--databytes;
		} else {
			/* Not eof and next is not eof: Type 1 */
			;
		}
	}
	/* p is left pointing at the first FCS octet */

	/*
	 * Calculate SW FCS
	 */
	fcs ^= 0xffff;
	fcs = ((fcs & 0xff) << 8) | ((fcs >> 8) & 0xff);

	/*
	 * Retrieve HW FCS
	 */
	if (dcp->md.r.f.cnt > 1) {
		dbri_fcs = *p++ << 8;
		dbri_fcs |= *p++;
	} else {
		dbri_fcs = *p++ << 8;
		if (dcp->md.r.f.eof || dcp->nextio == NULL ||
		    !dcp->nextio->md.r.f.eof)
			panic("dbri: unexpected error while getting fcs");
		p = dcp->nextio->cmd.data;
		dbri_fcs |= *p++;
	}

	if (fcs != dbri_fcs) {
		error = TRUE;	/* Discard this packet */
		(void) printf("dbri: fcs mismatch, fcs: %x != dbri_fcs: %x\n",
		    fcs, dbri_fcs);
		(void) printf("dbri: first4bytes = %x\n", first4bytes);
	}

	return (error);
}
#endif /* DBRI_SWFCS */
