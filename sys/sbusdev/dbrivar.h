/*
 * @(#) dbrivar.h 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc.
 */

#ifndef _sbusdev_dbrivar_h
#define	_sbusdev_dbrivar_h

/*
 * This file describes the ATT T5900FC (DBRI) ISDN chip and declares
 * parameters and data structures used by the audio driver.
 */

#ifdef KERNEL

#if !defined(DBRI_NOSETPORT)
#define	DBRI_NOSETPORT		/* Make this the default for now */
#endif


/*
 * This is the size of the STREAMS buffers for receive.
 *
 * XXX - For now, this is set to 1K so that reader processes like
 * soundtool that require some real-time response to input don't have to
 * wait for larger buffers to fill up.  Eventually, there should be an
 * ioctl that tells the driver to send all available data upstream.  In
 * this way, the buffering for 'normal' processes can be increased
 * without sacrificing the performance of real-time display processes.
 */
#define	DBRI_RECV_BSIZE	(1024)		/* Receive buffer size */

/* N.B. High and low water marks must fit into unsigned shorts */
/* XXX - 4.x water mark bugs need to be considered if these change */
#define	DBRI_HIWATER	(57000)
#define	DBRI_LOWATER	(32000)

#define	DBRI_CMDPOOL	(20)		/* command block buffer pool */
#define	DBRI_RECBUFS	(16)		/* # of record command blocks */

#define	DBRI_DEFAULT_GAIN (128)		/* gain initialization */

/*
 * Device minor numbers, the clone device becomes either DBRI_AUDIO_RW or
 * DBRI_AUDIO_RO depending on the open flags. DBRI_AUDIOCTL has been
 * changed from 0x80 to allow more minor devices to be added without
 * kludges. This breaks compatibility with the previous audio.c.
 *
 * Note: When new channel types are added to dbri_conf.c they must also
 * be added here as a new minor device. A device special file also needs
 * to be made in the /dev directory. The minor device number in the
 * /dev/directory must be the number below << 2 as the unit number is
 * encoded in the bottom two bits.
 */
typedef	enum {
	DBRI_MINOR_NODEV =		0,

	DBRI_MINOR_AUDIO_RW =		1,
	DBRI_MINOR_AUDIO_RO =		2,
	DBRI_MINOR_AUDIOCTL =		3,

	DBRI_MINOR_TE_B1_RW =		4,
	DBRI_MINOR_TE_B1_RO =		5,
	DBRI_MINOR_TE_B2_RW =		6,
	DBRI_MINOR_TE_B2_RO =		7,
	DBRI_MINOR_TE_D =		8,
	DBRI_MINOR_TEMGT =		9,

	DBRI_MINOR_NT_B1_RW =		10,
	DBRI_MINOR_NT_B1_RO =		11,
	DBRI_MINOR_NT_B2_RW =		12,
	DBRI_MINOR_NT_B2_RO =		13,
	DBRI_MINOR_NT_D =		14,
	DBRI_MINOR_NTMGT =		15,

	DBRI_MINOR_DBRIMGT =		16,

	DBRI_MINOR_TE_B1_56_RW =	17,
	DBRI_MINOR_TE_B1_56_RO =	18,
	DBRI_MINOR_TE_B1_TR_RW =	19,
	DBRI_MINOR_TE_B1_TR_RO =	20,
	DBRI_MINOR_NT_B1_56_RW =	21,
	DBRI_MINOR_NT_B1_56_RO =	22,
	DBRI_MINOR_NT_B1_TR_RW =	23,
	DBRI_MINOR_NT_B1_TR_RO =	24,
	DBRI_MINOR_TE_B2_56_RW =	25,
	DBRI_MINOR_TE_B2_56_RO =	26,
	DBRI_MINOR_TE_B2_TR_RW =	27,
	DBRI_MINOR_TE_B2_TR_RO =	28,
	DBRI_MINOR_NT_B2_56_RW =	29,
	DBRI_MINOR_NT_B2_56_RO =	30,
	DBRI_MINOR_NT_B2_TR_RW =	31,
	DBRI_MINOR_NT_B2_TR_RO =	32,
	DBRI_MINOR_TE_H_RW =		33,
	DBRI_MINOR_TE_H_RO =		34,
	DBRI_MINOR_NT_H_RW =		35,
	DBRI_MINOR_NT_H_RO =		36,
} dbri_minor_t;


/*
 * Note that the unit number is encoded into the upper 2 bits of the minor
 * device number. The lower order bits specify the type of channel.
 */
#define	MAXDBRI			4			/* max # of DBRI's */
#define	MINORDEV(dev)		(minor(dev) & 0x3f)	/* get minor dev */
#define	MINORUNIT(dev)		(minor(dev) >> 6)	/* get minor unit # */
#define	dbrimakedev(maj, unit, min)	\
	makedev((maj), (((unit) << 6) | ((min) & 0x3f))) /* make a new dev_t */


/* DBRI channels */
typedef enum {
	DBRI_NO_CHNL = -2,
	DBRI_HOST_CHNL = -1,

	/* TE channel defines */
	DBRI_TE_D_OUT = 0,
	DBRI_TE_D_IN,
	DBRI_TE_B1_OUT,
	DBRI_TE_B1_IN,
	DBRI_TE_B2_OUT,
	DBRI_TE_B2_IN,
	DBRI_TE_H_OUT,
	DBRI_TE_H_IN,
	DBRI_TE_B1_56_OUT,
	DBRI_TE_B1_56_IN,
	DBRI_TE_B2_56_OUT,
	DBRI_TE_B2_56_IN,
	DBRI_TE_B1_TR_OUT,
	DBRI_TE_B1_TR_IN,
	DBRI_TE_B2_TR_OUT,
	DBRI_TE_B2_TR_IN,

	/* NT channel defines */
	DBRI_NT_D_OUT,
	DBRI_NT_D_IN,
	DBRI_NT_B1_OUT,
	DBRI_NT_B1_IN,
	DBRI_NT_B2_OUT,
	DBRI_NT_B2_IN,
	DBRI_NT_H_OUT,
	DBRI_NT_H_IN,
	DBRI_NT_B1_56_OUT,
	DBRI_NT_B1_56_IN,
	DBRI_NT_B2_56_OUT,
	DBRI_NT_B2_56_IN,
	DBRI_NT_B1_TR_OUT,
	DBRI_NT_B1_TR_IN,
	DBRI_NT_B2_TR_OUT,
	DBRI_NT_B2_TR_IN,

	/* CHI channel defines */
	DBRI_AUDIO_OUT,
	DBRI_AUDIO_IN,

	/*
	 * Not user visible channels. (?)
	 * Used for data and control pipes dealing with device control.
	 */
	DBRI_AUDIOCTL,			/* actually MMCODEC control */
	DBRI_TEMGT,			/* generic ISDN control on TE */
	DBRI_NTMGT,			/* generic ISDN control on NT */
	DBRI_DBRIMGT,			/* DBRI specific control */

	DBRI_LAST_CHNL
	/* DO NOT PUT ANY ENTRIES AFTER THIS LAST DUMMY ENTRY */
} dbri_chnl_t;

#define	DBRI_NSTREAM 	((int)DBRI_LAST_CHNL)	/* # of channels available */
						/*
						 * This will be increased later
						 * by an offset
						 */

/*
 * Data mode to be used by the pipe.  Note that this does not apply to
 * fixed pipes.
 */
typedef	enum {
		DBRI_MODE_UNKNOWN = -1,		/* mode predefined by def */
		DBRI_MODE_XPRNT = DBRI_SDP_TRANSPARENT,	/* Transparent mode */
		DBRI_MODE_HDLC = DBRI_SDP_HDLC,		/* HDLC mode */
		DBRI_MODE_HDLC_D = DBRI_SDP_TE_DCHNL,	/* HDLC-D mode */
		DBRI_MODE_SER = DBRI_SDP_SERIAL,	/* Serial to serial */
		DBRI_MODE_FIXED = DBRI_SDP_FIXED	/* Fixed mode */
} dbri_mode_t;


typedef struct dbri_conn_req {
	dbri_chnl_t		xchan;		/* transmit channel type */
	dbri_chnl_t		rchan;		/* receive channel type */
	dbri_mode_t		mode;		/* SDP command mode */
} dbri_conn_req_t;


typedef struct dbri_port_tab {
	isdn_port_t		port;		/* isdn port */
	dbri_chnl_t		xchan;		/* dbri transmit port */
	dbri_chnl_t		rchan;		/* dbri receive port */
} dbri_port_tab_t;


typedef struct dbri_chan_tab {
	dbri_chnl_t		chan;		/* dbri channel */
	isdn_port_t		port;		/* port associated w/chan */
	unsigned int		bpipe;		/* base pipe of channel */
	unsigned int		cycle;		/* bit offset of time slot */
	unsigned int		len;		/* bit length of time slot */
	int			dir;		/* direction of this channel */
	dbri_mode_t		mode;		/* pipe mode - default HDLC */
	int			samplerate;	/* sample frames per second */
	int			channels;	/* channels per sample frame */
	dbri_chnl_t		in_as;		/* input data stream */
	dbri_chnl_t		out_as;		/* output data stream */
	dbri_chnl_t		control_as;	/* control data stream */
	aud_streamtype_t	audtype;	/* audio type of stream */
	dbri_minor_t		minordev;	/* dev minor #...well almost */
	isdn_interface_t	isdntype;	/* ISDN type of stream */
	int			numbufs;	/* Number of descriptors */
	char			*name;		/* Name string for printfs */
	int			input_size;	/* Default record buffer size */
} dbri_chan_tab_t;


#define	DBRI_MAX_CMDS		(512)		/* max cmds in Q */
#define	DBRI_MAX_CMDSIZE	(3)		/* max # of words in a cmd */
/*
 * Limit to ensure we can insert a JMP back to the head of list.  Note
 * that the JMP cmd is two words long.
 */
#define	DBRI_END_CMDLIST	(DBRI_MAX_CMDS - DBRI_MAX_CMDSIZE - 2 - 1)

/* DBRI internal command queue structure */
typedef struct dbri_chip_cmdq {
	unsigned int	cmd[DBRI_MAX_CMDS]; /* space for command list */
	unsigned int	cmdhp;		/* next command to add to list */
} dbri_chip_cmdq_t;


typedef struct {
	unsigned int	berr;		/* detectible bit errors */
	unsigned int	ferr;		/* frame sync errors */
} primitives_t;

/* Status for TE, NT, and CHI channels */
typedef struct serial_status {
	unsigned int		te_cmd;		/* TE command */
	struct dbri_code_sbri	te_sbri;	/* SBRI intr te status */
	primitives_t		te_primitives;	/* primtvs. for TE */

	unsigned int		nt_cmd;		/* NT command */
	struct dbri_code_sbri	nt_sbri;	/* SBRI intr nt status */
	primitives_t		nt_primitives;	/* primtvs. for NT */

	unsigned int		chi_cmd;	/* CHI command */
	unsigned int		chi_cdeccmd;	/* CDEC command */
	unsigned int		chi_cdm;	/* CDM command */
	unsigned int		chi_dd_val;	/* CHI Dummy Data value */
	unsigned int		chi_flags;	/* state info */
	unsigned int		chi_state;	/* current CHI state */
	unsigned int		chi_nofs;	/* number of nofs interrupts */
	int			ibeg;		/* # times IBEG intr */
	int			iend;		/* # times IEND intr */
	int			cmdi;		/* cmdi intr timer */
	int			pal_present;	/* TRUE if present */
	int			mmcodec_ts_start;	/* cycle of CODEC */
} serial_status_t;

/* Possible CHI state flags */
#define	CHI_FLAG_CODEC_RPTD	0x01
#define	CHI_FLAG_HAVE_PAL	0x02
#define	CHI_FLAG_OPEN_SLEEP	0x04
#define	CHI_FLAG_SER_STS	0x08

/* Possible CHI Bus states */
#define	CHI_NO_STATE		0
#define	CHI_NO_DEVICES		1
#define	CHI_IN_DATA_MODE	2
#define	CHI_IN_CTRL_MODE	3
#define	CHI_WAIT_DCB_LOW	4
#define	CHI_WAIT_DCB_HIGH	5
#define	CHI_GOING_TO_DATA_MODE	6
#define	CHI_CODEC_CALIBRATION	7
#define	CHI_GOING_TO_CTRL_MODE	8
#define	CHI_POWERED_DOWN	9

/* frame lengths */
#define	CHI_DATA_MODE_LEN	128
#define	CHI_CTRL_MODE_LEN	256

typedef enum {		DBRI_NO_TAG, DBRI_MM1_CTRL1W, DBRI_MM1_CTRL2W,
			DBRI_MM1_CTRL1R, DBRI_MM1_CTRL2R,
			DBRI_MM1_CTRL3R, DBRI_MM1_CTRL4R,
			DBRI_MM1_DATA, DBRI_MM2_CTRL1W,
			DBRI_MM2_CTRL2W, DBRI_MM2_CTRL1R,
			DBRI_MM2_CTRL2R, DBRI_MM2_CTRL3R,
			DBRI_MM2_CTRL4R, DBRI_MM2_DATA,
			DBRI_S_CHNL, DBRI_Q_CHNL, DBRI_NT_TE,
			DBRI_NT_CHI, DBRI_TE_NT, DBRI_TE_CHI,
			DBRI_CHI_NT, DBRI_CHI_TE
} short_pipe_id_t;


/* Status for 16 long pipes and 16 short pipes */
typedef struct pipe_tab {
	aud_stream_t		*as;		/* stream assoc w/port */
	short_pipe_id_t		pid;		/* pipe id */
	unsigned int		sdp;		/* setup data pipe cmd */
	unsigned int		ssp;		/* set short pipe cmd */
	union dbri_dtscmd	dts;		/* define time slot cmd */
	union dbri_dtstsd	in_tsd;		/* input time slot desc */
	union dbri_dtstsd	out_tsd;	/* output time slot desc */
	int			monflag;	/*
						 * monitor flag indicator. TRUE
						 * means monitor pipe attached
						 * to this base pipe.
						 */
	dbri_chnl_t		xchan;		/* transmit channel type */
	dbri_chnl_t		rchan;		/* receive channel type */
} pipe_tab_t;

typedef struct isdn_var {
	enum isdn_ph_param_maint	maint;	/* Maintenance mode */
	enum isdn_ph_param_asmb		asmb;	/* Act. State Machine Type */
	unsigned char	message_type;	/* 0 if signals, else M_PROTO */
	unsigned int	norestart;	/* XXX Enable for DBRI restart hack */
	unsigned int	power;		/* ISDN_PH_PARAM_POWER_OFF or ...ON */
	unsigned int	enable;		/* if (enable && power) enable intf */
	int	t101;			/* NT ASM */
	int	t102;			/* NT ASM */
	int	t103;			/* TE ASM */
	int	t104;			/* TE ASM */
} isdn_var_t;

/* Reserved pipes */
#define	DBRI_PIPE_TE_D_IN	0
#define	DBRI_PIPE_TE_D_OUT	1
#define	DBRI_PIPE_NT_D_OUT	2
#define	DBRI_PIPE_NT_D_IN	3
#define	DBRI_PIPE_LONGFREE	4		/* initial start of free list */
#define	DBRI_PIPE_CHI		16		/* base pipe for CHI */
/*
 * These next few dedicated short pipes are needed as they contain s/w
 * status.  They should never be used for any other purpose.  If any
 * additional short pipes are needed for status insert them before the
 * define for the freelist and modify the start of the freelist.
 */
#define	DBRI_PIPE_DM_T_5_8	17		/* Data Mode xmit TS's 5-8 */
#define	DBRI_PIPE_DM_R_5_6	18		/* Data Mode recv TS's 5-6 */
#define	DBRI_PIPE_DM_R_7_8	19		/* Data Mode recv TS's 7-8 */
#define	DBRI_PIPE_CM_T_1_4	20		/* Cntrl Mode xmit TS's 1-4 */
#define	DBRI_PIPE_CM_R_1_2	21		/* Cntrl Mode recv TS's 1-2 */
#define	DBRI_PIPE_CM_R_3_4	22		/* Cntrl Mode recv TS's 3-4 */
#define	DBRI_PIPE_SB_PAL_T	23		/* SBox ID PAL transmit */
#define	DBRI_PIPE_SB_PAL_R	24		/* SBox ID PAL receive */
#define	DBRI_PIPE_CM_R_7	25		/* To read MMCODEC Manu info */
#define	DBRI_PIPE_DUMMYDATA	26		/* to xmit dummy data */
#define	DBRI_PIPE_SHORTFREE	27		/* initial start of free list */
#define	DBRI_MAX_PIPES		32		/* total number of pipes */
						/* also used for invalid pipe */
#define	DBRI_BAD_PIPE		DBRI_MAX_PIPES	/* total number of pipes */

/* DBRI pipe types */
#define	DBRI_LONGPIPE		0		/* Long pipe type */
#define	DBRI_SHORTPIPE		1		/* Short pipe type */

#define	ISPIPEINUSE(a)		((a)->dts.r.id == DBRI_DTS_ID)
#define	ISHWCONNECTED(as)	(ISDATASTREAM(as) && \
					(AsToDs(as)->pipe != DBRI_BAD_PIPE))
#define	ISTEDCHANNEL(as)	((AsToDs(as)->pipe == DBRI_PIPE_TE_D_IN) || \
	(AsToDs(as)->pipe == DBRI_PIPE_TE_D_OUT))
#define	ISNTDCHANNEL(as)	((AsToDs(as)->pipe == DBRI_PIPE_NT_D_IN) || \
	(AsToDs(as)->pipe == DBRI_PIPE_NT_D_OUT))
#define	ISXMITDIRECTION(as)	(((dbri_dev_t *)	\
	(as)->v->devdata)->ptab[AsToDs(as)->pipe].sdp & DBRI_SDP_D)
#define	ISAUDIOCONNECTION(a)	((a == DBRI_AUDIO_OUT) || (a == DBRI_AUDIO_IN))

#define	DBRI_NO_IO		0x0	/* Not input or output */
#define	DBRI_IN			0x1	/* input direction */
#define	DBRI_OUT		0x2	/* output direction */
#define	DBRI_GOODCONN		1	/* good connection return val */
#define	DBRI_BADCONN		2	/* bad connection return value */


/* Command list status and info within a specific stream */
/* 
 * Note that each message descriptor (tmd/rmd) MUST start on
 * a 16 byte alignment for DBRI V5 silicon. This is a h/w 
 * requirement! An ASSERT has been added to the driver to 
 * check for this 
 */
#define	DBRI_MD_ALIGN	16		/* md's must be 16 byte aligned */

typedef struct dbri_cmd {
	aud_cmd_t 	cmd;		/* generic driver's generic command */
	struct dbri_cmd	*nextio;	/* next IO command */
	caddr_t		sb_addr;	/* 4m only, but keep same size */
	union {
		dbri_tmd_t	t;	/* transmit message descriptor */
		dbri_rmd_t	r;	/* receive message descriptor */
	} md;
} dbri_cmd_t;

/* Stream specific status info within a particular controller */
typedef struct dbri_stream {
	aud_stream_t	as;		/* HW independent state */
	dbri_cmd_t	*cmdptr;	/* cmd chain list head */
	dbri_cmd_t	*cmdlast;	/* last IO command in list */
	unsigned int	samples;	/* number samples converted */
	unsigned int	recv_eol;	/* indicates receive hit EOL */
	unsigned int	audio_uflow;	/* audio over/under-flow */
	unsigned int	last_flow_err;	/* XXX DBRI[de] bug 3034 */
	unsigned int	pipe;		/* index into hardware pipe table */
	isdn_info_t	i_info;		/* ISDN state and error information */
	isdn_var_t	i_var;		/* driver's ISDN private variables */
	dbri_stat_t	d_stats;	/* DBRI Statistics */
	mblk_t		*mp;		/* current ioctl message pointer */
} dbri_stream_t;

#define	AsToDs(as) ((dbri_stream_t *)(void *)(as))
#define	DsToAs(as) ((aud_stream_t *)(void *)(as))


#define	DBRI_NUM_INTQS		10	/* number of interrupt queues */

typedef struct dbri_intr_struct {
	dbri_intq_t	*intq_bp;	/* interrupt queue base ptr */
	dbri_intq_t	*curqp;		/* intr qptr to current qstruct */
	int		off;		/* offset into qstruct */
} dbri_intr_struct_t;


/* Controller specific status and info on a per controller basis */
typedef struct dbri_dev {
	aud_state_t	hwi_state;	/* HW Independant chip State */
	int		intrlevel;	/* Unit-specific interrupt level */
	dbri_reg_t	*chip;		/* addr of device registers */
	dbri_intr_struct_t intq;	/* base of entire intq struct */
	int		openinhibit;	/* FALSE if opens on device allowed */
	dbri_chip_cmdq_t *cmdqp;	/* DBRI chip command qptr */
	caddr_t		mem_base;	/* base of kernel memory */
	int		mem_size;	/* to free memory on unload */
	caddr_t		sb_addr;	/* 4m only, but keep same size */
	int		addr_offset;	/* offset to add for DBRI address */
	pipe_tab_t 	ptab[DBRI_INT_MAX_CHNL]; /* pipe status table */
	serial_status_t	ser_sts;	/* serial status table */
	dbri_stream_t	ioc[DBRI_NSTREAM]; /* Per stream info */
	struct dev_info	*dbri_dev;	/* ptr to dev vector info */
	int		init_chip;	/* TRUE if chip already init */
	int		init_cmdq;	/* TRUE if cmdq already init */
	unsigned int 	dbri_flags;	/* State info */
	int		keep_alive_running; /* Sanity ref cnt if NT|TE used */
	char		dbri_version;	/* DBRI Revision letter */
	/* XXX error counters per controller go here */
} dbri_dev_t;

#define	DBRI_ADDR(d, a)		((caddr_t)a + d->addr_offset)
#define	KERN_ADDR(d, a)		((caddr_t)a - d->addr_offset)


/* dbri routines */
int			dbri_open();
void			dbri_close();
aud_return_t		dbri_ioctl();
void			dbri_start();
void			dbri_stop();
unsigned int		dbri_setflag();
aud_return_t		dbri_setinfo();
void			dbri_queuecmd();
void			dbri_flushcmd();
aud_stream_t *		dbri_minortoas();
int			dbri_intr();

/* routines in dbri_subr.c */
void		dbri_initchip();
int		dbri_con_stream();
int		dbri_xcon_stream();
void		audio_ser_to_ser_config();
int		dbri_disconnect();
int		dbri_chnl_activated();
unsigned int	dbri_setup_chnl();
void		dbri_setup_sdp();
void		dbri_setup_ntte();
int		dbri_setup_dts();
int		dbri_find_oldnext();
void		dbri_remove_dts();
unsigned int	dbri_getpipe();
unsigned int	dbri_findshortpipe();
void		dbri_command();
int		dbri_check_req();
int		dbri_chnl_activated();
int		dbri_tsd_overlap();
void		dbri_te_timer();
void		dbri_nt_timer();
void		dbri_keep_alive();
void		dbri_panic();

int		audio_con_stream();
void		audio_xcon_stream();
int		mmcodec_getdev();
int		mmcodec_check_audio_config();
void		mmcodec_set_audio_config();
void            audio_pause_record();
void            audio_pause_play();
void            audio_resume_record();
void            audio_resume_play();


/* routines in dbri_isr.c */
void		dbri_sbri_intr();
void		dbri_xmit_intr();
void		dbri_recv_intr();

/* routines in audio.c */
void		audio_sendsig();

extern int	Dbri_debug;
extern int	Dbri_panic;
extern int	Ndbri;
extern int	Default_T101_timer;
extern int	Default_T102_timer;
extern int	Default_T103_timer;
extern int	Default_T104_timer;
extern enum isdn_ph_param_asmb	Default_asmb;
extern int	Default_power;
extern int	Keepalive_timer;
extern int	Nstreams;
extern dbri_dev_t	*Dbri_devices;
extern dbri_chan_tab_t	Chan_tab[];
extern dbri_port_tab_t	Port_tab[];

#endif /* KERNEL */

#endif /* !_sbusdev_dbrivar_h */
