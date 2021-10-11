#ident	"@(#)espvar.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988-1991 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_adapters_espvar_h
#define	_scsi_adapters_espvar_h
/*
 * Emulex ESP (Enhanced Scsi Processor) Definitions,
 * Software && Hardware.
 */

/*
 * Compile options
 */

#define	ESPDEBUG		/* turn on debugging code */
#define	ESP_TEST_FAST_SYNC		/* turn on fast synch. test code */
/* #define	ESP_TEST_TIMEOUT	/* turn on timeout test code */
/* #define	ESP_TEST_PARITY		/* turn on parity test code */
/* #define	ESP_NEW_HW_DEBUG	/* turn on debug code for new h/w */

/*
 * General SCSI includes
 */

#include <scsi/scsi.h>

/*
 * Include hardware definitions for the ESP generation chips.
 */

#include <scsi/adapters/espreg.h>

/*
 * Software Definitions
 */

/*
 * Data Structure for this Host Adapter.
 */


struct esp {
	/*
	 * Configuration information for this host adapter
	 */

	/*
	 * This structure must be first.
	 *
	 * Each host adapter will export the address of this structure,
	 * which defines function entry points, as well as a splx() cookie
	 * needed to mask interrupts for this device, to the library. The
	 * library will use the address of this structure to to form
	 * SCSI device addresses- the address of this structure will
	 * be encoded in the 'a_cookie' field of the SCSI device address.
	 *
	 * All requests are defined such that the SCSI address is either
	 * a formal paramter, or contained within a formal parameter.
	 *
	 * Therefore, each function entry in the driver will know that
	 * the 'a_cookie' field points to a transport structure, which
	 * will then allow each function to retrieve a pointer to the
	 * correct softc structure.
	 */

	struct scsi_transport	e_tran;

	/*
	 * Next in a linked list of host adapters
	 */

	struct esp	*e_next;

	/*
	 * Cross reference to autoconfig structures
	 */

#ifdef	OPENPROMS
	struct dev_info		*e_dev;	/* reference to devinfo structure */
#else	/* OPENPROMS */
	struct mb_driver	*e_dev;	/* reference to mb_driver structure */
#endif	/* OPENPROMS */

	/*
	 * Unit number
	 */

	u_char e_unit;

	/*
	 * Type byte for this host adapter (53C90, 53C90A, ESP-236)
	 */

	u_char	e_type;

	/*
	 * value for configuration register 1.
	 * Also contains Initiator Id.
	 */

	u_char	e_espconf;

	/*
	 * value for configuration register 2 (ESP100A)
	 */

	u_char	e_espconf2;

	/*
	 * value for configuration register 3 (ESP236)
	 */

	u_char	e_espconf3[NTARGETS];

	/*
	 * differential bus flag
	 */
	u_char  e_differential;

	/*
	 * clock conversion register value for this host adapter.
	 * clock cycle value * 1000 for this host adapter,
	 * to retain 5 significant digits.
	 */

	u_char	e_clock_conv;
	u_short	e_clock_cycle;

	/*
	 * selection timeout register value
	 */

	u_char	e_stval;

	/*
	 * State of the host adapter
	 */

	u_char	e_state;	/* state of the driver */
	u_char	e_laststate;	/* last state of the driver */
	u_char	e_stat;		/* soft copy of status register */
	u_char	e_intr;		/* soft copy of interrupt register */
	u_char	e_step;		/* soft copy of step register */
	u_char	e_sdtr;		/*
				 * Count of sync data negotiation messages:
				 * zeroed for every selection attempt,
				 * every reconnection, and every disconnect
				 * interrupt. Each SYNCHRONOUS DATA TRANSFER
				 * message, both coming from the target, and
				 * sent to the target, causes this tag to be
				 * incremented. This allows the received
				 * message handling to determine whether
				 * a received SYNCHRONOUS DATA TRANSFER
				 * message is in response to one that we
				 * sent.
				 */

	/*
	 * Scratch Buffer, allocated out of iopbmap for commands
	 * The same size as the ESP's fifo.
	 */

	u_char		*e_cmdarea;

	/*
	 * Message handling: enough space is reserved for the expected length
	 * of all messages we could either send or receive.
	 *
	 * For sending, we expect to send only SYNCHRONOUS extended messages
	 * (5 bytes). We keep a history of the last message sent, and in order
	 * to control which message to send, an output message length is set
	 * to indicate whether and how much of the message area is to be used
	 * in sending a message. If a target shifts to message out phase
	 * unexpectedly, the default action will be to send a MSG_NOP message.
	 *
	 * After the successful transmission of a message, the initial message
	 * byte is moved to the e_last_msgout area for tracking what was the
	 * last message sent.
	 */

#define	OMSGSIZE	6
	u_char		e_cur_msgout[OMSGSIZE];
	u_char		e_last_msgout;
	u_char		e_omsglen;


	/*
	 * We expect, at, most, to receive a maximum of 7 bytes
	 * of an incoming extended message (MODIFY DATA POINTER),
	 * and thus reserve enough space for that.
	 */

#define	IMSGSIZE	8
	u_char		e_imsgarea[IMSGSIZE];
#define	e_last_msgin	e_imsgarea[0]

	/*
	 * These are used to index how far we've
	 * gone in receiving incoming  messages.
	 */

	u_char		e_imsglen;
	u_char		e_imsgindex;

	/*
	 * Target information
	 *	Synchronous SCSI Information,
	 *	Disconnect/reconnect capabilities
	 *	Noise Susceptibility
	 */

	u_char	e_offset[NTARGETS];	/* synchronous offset */
	u_char	e_period[NTARGETS];	/* synchronous periods */
	u_char	e_backoff[NTARGETS];	/* synchronous period compensation */
	u_char	e_default_period;	/* default sync period */
	u_char  e_req_ack_delay;	/* req/ack delay on FAS SCSI */
#define	DEFAULT_REQ_ACK_DELAY_101 0x20	/* delay assert period 1 cycle */
#define	DEFAULT_REQ_ACK_DELAY_236 0x60	/* delay assert period 1 cycle */

	/*
	 * This u_char is a bit map for targets
	 * whose SYNC capabilities are known.
	 */

	u_char e_sync_known;

	/*
	 * This u_char is a bit map for targets who
	 * don't appear to be able to disconnect.
	 */

	u_char	e_nodisc;

	/*
	 * This u_char is a bit map for targets
	 * who seem to be susceptible to noise.
	 */

	u_char	e_weak;

	/*
	 * This u_char is a bit map for targets
	 * that have been successfully probed
	 * talked to before.
	 */


	u_char	e_targets;

	/*
	 * different platforms may support different sbus burst sizes
	 */

	u_char	e_burstsizes;

	/*
	 * Instrumentation
	 */

	u_long e_npolling;	/* number of polling commands stored up */
	u_long e_ncmds;		/* number of commands stored here at present */
	u_long e_ndisc;		/* number of disconnected cmds at present */
	u_long e_preempt;	/* number of times a select was preempted */
	u_long e_disconnects;	/* number of disconnects */
	u_long e_nlinked;	/* number of linked commands */
	u_long e_nsync;		/* number of synchronous scsi data xfers */

	/*
	 * Hardware pointers
	 */

	/*
	 * Pointer to mapped in ESP registers
	 */

	struct espreg		*e_reg;

	/*
	 * Pointer to mapped in DMA Gate Array registers
	 */

	struct dmaga		*e_dma;
	u_long			e_lastdma;	/* last dma address */
	u_long			e_lastcount;	/* last dma count */

	/*
	 * DMA base value for this host adapter.
	 * Addresses passed in SCSI command offsets,
	 * if less than mmu_ptob(dvmasize) are offsets
	 * from the base of DVMA[]. The following base
	 * value is or'd in with this to get the true
	 * dma value.
	 */

	long			e_dma_base;

	/*
	 * Ugly, and space consumptive, but extremely unambiguous.
	 *
	 * If the slot is NULL, then the Host Adapter believes that
	 * that Target/Lun can accept a command.
	 *
	 * Addressing for using the _slot tags is fixed by (target<<3)|lun.
	 * The value ((short) -1) is reserved to undefine any _slot tag.
	 *
	 */

	short			e_last_slot;	/* last active target/lun */
	short			e_cur_slot;	/* current active target/lun */

	/*
	 * 'splx' interrupt mask for the specific target
	 */

	int			e_spl;
	struct scsi_cmd	*	e_slots[NTARGETS*NLUNS_PER_TARGET];

#define	NPHASE	16
#ifdef	ESPDEBUG
	/*
	 * SCSI analyzer function data structures.
	 */
	int	e_xfer;				/* size of current transfer */
	short	e_phase_index;			/* next entry in table */
	struct	scsi_phases { 			/* SCSI analyzer structure */
		short	e_save_state;
		short	e_save_stat;
		int	e_val1, e_val2;
	} e_phase[NPHASE];
#endif	/* ESPDEBUG */
};

#ifdef	ESPDEBUG
/*
 * Log state and phase history of activity
 */
#define	LOG_STATE(arg0, arg1, arg2, arg3) { \
	esp->e_phase[esp->e_phase_index].e_save_state = arg0; \
	esp->e_phase[esp->e_phase_index].e_save_stat = arg1; \
	esp->e_phase[esp->e_phase_index].e_val1 = arg2; \
	esp->e_phase[esp->e_phase_index].e_val2 = arg3; \
	esp->e_phase_index = (++esp->e_phase_index) & (NPHASE-1); \
};
#else	/* ESPDEBUG */
#define	LOG_STATE(arg0, arg1, arg2, arg3) {};
#endif	/* ESPDEBUG */

/*
 * Representations of Driver states (stored in tags e_state && e_laststate).
 */

/*
 * Totally idle. There may or may not disconnected commands still
 * running on targets.
 */

#define	STATE_FREE	0x00

/*
 * Selecting States. These states represent a selection attempt
 * for a target.
 */

#define	STATE_SELECTING		0xE0
#define	STATE_SELECT_NORMAL	(1<<5)
#define	STATE_SELECT_N_STOP	(2<<5)
#define	STATE_SELECT_N_SENDMSG	(3<<5)
#define	STATE_SELECT_NOATN	(4<<5)

/*
 * When the driver is neither idle nor selecting, it is in one of
 * the information transfer phases. These states are not unique
 * bit patterns- they are simple numbers used to mark transitions.
 * They must start at 1 and proceed sequentially upwards and
 * match the indexing of function vectors declared in the function
 * esp_phasemanage().
 */

#define	STATE_ITPHASES	0x1F

/*
 * These states cover finishing sending a command out (if it wasn't
 * sent as a side-effect of selecting), or the case of starting
 * a command that was linked to the previous command (i.e., no
 * selection phase for this particular command as the target
 * remained connected when the previous command completed).
 */

#define	ACTS_CMD_START		0x01
#define	ACTS_CMD_DONE		0x02

/*
 * These states are the begin and end of sending out a message.
 * The message to be sent is stored in the field e_msgout (see above).
 */

#define	ACTS_MSG_OUT		0x03
#define	ACTS_MSG_OUT_DONE	0x04

/*
 * These states are the beginning, middle, and end of incoming messages.
 *
 */

#define	ACTS_MSG_IN		0x05
#define	ACTS_MSG_IN_MORE	0x06
#define	ACTS_MSG_IN_DONE	0x07


/*
 * This state is reached when the target may be getting
 * ready to clear the bus (disconnect or command complete).
 */

#define	ACTS_CLEARING		0x08


/*
 * These states elide the begin and end of a DATA phase
 */

#define	ACTS_DATA		0x09
#define	ACTS_DATA_DONE		0x0A

/*
 * This state indicates that we were in status phase. We handle status
 * phase by issuing the ESP command 'CMD_COMP_SEQ' which causes the
 * ESP to read the status byte, and then to read a message in (presumably
 * one of COMMAND COMPLETE, LINKED COMMAND COMPLETE or LINKED COMMAND
 * COMPLETE WITH FLAG).
 *
 * This state is what is expected to follow after the issuance of the
 * ESP command 'CMD_COMP_SEQ'.
 */

#define	ACTS_C_CMPLT		0x0B

/*
 * Hiwater mark of vectored states
 */

#define	ACTS_ENDVEC		0x0B

/*
 * This state is used by the driver to indicate that it doesn't know
 * what the next state is, and that it should look at the ESP's status
 * register to find out what SCSI bus phase we are in in order to select
 * the next state to transition to.
 */

#define	ACTS_UNKNOWN		0x1A

/*
 * This state is used by the driver to indicate that it
 * is in the middle of processing a reselection attempt.
 */

#define	ACTS_RESEL		0x1B

/*
 * This state is used by the driver to indicate that a self-inititated
 * Bus reset is in progress.
 */

#define	ACTS_RESET		0x1C


/*
 * This state is used by the driver to indicate to itself that it is
 * in the middle of aborting things.
 */

#define	ACTS_ABORTING		0x1D

/*
 * This state is used by the driver to indicate to itself that it is
 * in the middle of spanning a target driver completion call.
 */

#define	ACTS_SPANNING		0x1E

/*
 * This state is used by the driver to just hold the state of
 * the softc structure while it is either aborting or resetting
 * everything.
 */

#define	ACTS_FROZEN		0x1F

/*
 * These additional states are only used by the scsi bus analyzer.
 */
#define	ACTS_PREEMPTED		0x21
#define	ACTS_PROXY		0x22
#define	ACTS_SYNCHOUT		0x23
#define	ACTS_CMD_LOST		0x24
#define	ACTS_DATAOUT		0x25
#define	ACTS_DATAIN		0x26
#define	ACTS_STATUS		0x27
#define	ACTS_DISCONNECT		0x28
#define	ACTS_NOP		0x29
#define	ACTS_REJECT		0x2A
#define	ACTS_RESTOREDP		0x2B
#define	ACTS_SAVEDP		0x2C
#define	ACTS_BAD_RESEL		0x2D
#define	ACTS_LOG		0x0F

/*
 * Interrupt dispatch actions
 */

#define	ACTION_RETURN		-1	/* return from interrupt */
#define	ACTION_FINSEL		0	/* finish selection */
#define	ACTION_RESEL		1	/* handle reselection */
#define	ACTION_PHASEMANAGE	2	/* manage phases */
#define	ACTION_FINISH		3	/* this command done */
#define	ACTION_FINRST		4	/* finish reset recovery */
#define	ACTION_SEARCH		5	/* search for new command to start */
#define	ACTION_ABORT_CURCMD	6	/* abort current command */
#define	ACTION_ABORT_ALLCMDS	7	/* abort all commands */
#define	ACTION_RESET		8	/* reset bus */
#define	ACTION_SELECT		9	/* handle selection */

/*
 * Proxy command definitions.
 *
 * At certain times, we need to run a proxy command for a target
 * (if only to select a target and send a message).
 *
 * We use the tail end of the cdb that is internal to the scsi_cmd
 * structure to store the proxy code, the proxy data (e.g., the
 * message to send).
 *
 * We also store a boolean result code in this area so that the
 * user of a proxy command knows whether it succeeded.
 */

/*
 * Offsets into the cmd_db[] array for proxy data
 */

#define	ESP_PROXY_TYPE		CDB_GROUP0
#define	ESP_PROXY_RESULT	ESP_PROXY_TYPE+1
#define	ESP_PROXY_DATA		ESP_PROXY_RESULT+1

/*
 * Currently supported proxy types
 */

#define	ESP_PROXY_SNDMSG	1

/*
 * Reset actions
 */

#define	ESP_RESET_ESP		0x1	/* reset ESP chip */
#define	ESP_RESET_DMA		0x2	/* reset DMA gate array */
#define	ESP_RESET_BRESET	0x4	/* reset SCSI bus */
#define	ESP_RESET_IGNORE_BRESET	0x8	/*
					 * ignore SCSI Bus RESET interrupt
					 * while resetting bus.
					 */
#define	ESP_RESET_SCSIBUS	(ESP_RESET_BRESET|ESP_RESET_IGNORE_BRESET)
#define	ESP_RESET_SOFTC		0x10	/* reset SOFTC structure */

#define	ESP_RESET_HW		(ESP_RESET_ESP|ESP_RESET_DMA|ESP_RESET_SCSIBUS)
#define	ESP_RESET_ALL		(ESP_RESET_HW|ESP_RESET_SOFTC)

#define	ESP_RESET_MSG		0x20

/*
 * Debugging macros and defines
 */

#ifdef	ESPDEBUG

#define	INFORMATIVE	(espdebug)
#define	DEBUGGING	(espdebug > 1)

#define	EPRINTF(str)		if (espdebug > 1) eprintf(esp, str)
#define	EPRINTF1(str, a)	if (espdebug > 1) eprintf(esp, str, a)
#define	EPRINTF2(str, a, b)	if (espdebug > 1) eprintf(esp, str, a, b)
#define	EPRINTF3(str, a, b, c)	if (espdebug > 1) eprintf(esp, str, a, b, c)
#define	EPRINTF4(str, a, b, c, d)	\
	if (espdebug > 1) eprintf(esp, str, a, b, c, d)
#define	EPRINTF5(str, a, b, c, d, e)	\
	if (espdebug > 1) eprintf(esp, str, a, b, c, d, e)

#define	IPRINTF(str)		if (espdebug) eprintf(esp, str)
#define	IPRINTF1(str, a)	if (espdebug) eprintf(esp, str, a)
#define	IPRINTF2(str, a, b)	if (espdebug) eprintf(esp, str, a, b)
#define	IPRINTF3(str, a, b, c)	if (espdebug) eprintf(esp, str, a, b, c)
#define	IPRINTF4(str, a, b, c, d)	\
	if (espdebug) eprintf(esp, str, a, b, c, d)
#define	IPRINTF5(str, a, b, c, d, e)	\
	if (espdebug) eprintf(esp, str, a, b, c, d, e)

#else	/* ESPDEBUG */

#define	EPRINTF(str)
#define	EPRINTF1(str, a)
#define	EPRINTF2(str, a, b)
#define	EPRINTF3(str, a, b, c)
#define	EPRINTF4(str, a, b, c, d)
#define	EPRINTF5(str, a, b, c, d, e)
#define	IPRINTF(str)
#define	IPRINTF1(str, a)
#define	IPRINTF2(str, a, b)
#define	IPRINTF3(str, a, b, c)
#define	IPRINTF4(str, a, b, c, d)
#define	IPRINTF5(str, a, b, c, d, e)

#endif	/* ESPDEBUG */

/*
 * Shorthand macros and defines
 */

#define	NODISC(tgt)		(esp->e_nodisc & (1<<(tgt)))
#define	SYNC_KNOWN(tgt)		(esp->e_sync_known & (1<<(tgt)))
#define	CURRENT_CMD(esp)	((esp)->e_slots[(esp)->e_cur_slot])

#define	SLOT(sp)		((short) (Tgt((sp))<<3|(Lun((sp)))))
#define	NEXTSLOT(slot)		((slot)+1) & ((NTARGETS*NLUNS_PER_TARGET)-1)
#define	FIFO_CNT(ep)		((ep)->esp_fifo_flag & 0x1f)
#define	MY_ID(esp)		((esp)->e_espconf & ESP_CONF_BUSID)
#define	INTPENDING(esp)		((esp)->e_dma->dmaga_csr&DMAGA_INT_MASK)

#define	Tgt(sp)	((sp)->cmd_pkt.pkt_address.a_target)
#define	Lun(sp)	((sp)->cmd_pkt.pkt_address.a_lun)

#define	New_state(esp, state)	\
	(esp)->e_laststate = (esp)->e_state, (esp)->e_state = (state)

#define	ESP_PREEMPT(esp)	\
	New_state((esp), STATE_FREE), (esp)->e_last_slot = (esp)->e_cur_slot, \
	(esp)->e_cur_slot = UNDEFINED,  (esp)->e_preempt++

#define	CNUM	(esp->e_unit)
#define	TRUE		1
#define	FALSE		0
#define	UNDEFINED	-1

/*
 * Some manifest miscellaneous constants
 */

#define	MEG		(1000 * 1000)
#define	FIVE_MEG	(5 * MEG)
#define	TEN_MEG		(10 * MEG)
#define	TWENTY_MEG	(20 * MEG)
#define	TWENTYFIVE_MEG	(25 * MEG)
#define	ESP_FREQ_SLOP	(25000)


#endif	/* !_scsi_adapters_espvar_h */
