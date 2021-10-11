/*
 * @(#) isdnio.h 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc.
 */

/* 
 * NOTE: ISDN functionality is not currently supported in s/w.
 */

/*
 * Predefined from_to connections for the NT, TE, and CHI interfaces.  To
 * define connections not listed here use the ISDN_CREATPORT ioctl.
 */
typedef enum {
	ISDN_PORT_NONE = 0x0,		/* no port specified */
	ISDN_NONSTD,			/*
					 * non-standard port type used
					 * with SETPORT and RELPORT after
					 * a CREATPORT has returned
					 * successfully.
					 */
	ISDN_HOST,			/* Host port define, currently SBus */

	ISDN_CTLR_MGT,			/* ISDN Controller Management */
	ISDN_TE_MGT,			/* ISDN BRI TE Management */
	ISDN_NT_MGT,			/* ISDN BRI NT Management */
	ISDN_PRI_MGT,			/* ISDN PRI Management */

	/* NT port defines */
	ISDN_NT_D,
	ISDN_NT_B1,
	ISDN_NT_B2,
	ISDN_NT_H,
	ISDN_NT_S,
	ISDN_NT_Q,
	ISDN_NT_B1_56,
	ISDN_NT_B2_56,
	ISDN_NT_B1_TR,
	ISDN_NT_B2_TR,

	/* TE port defines */
	ISDN_TE_D,
	ISDN_TE_B1,
	ISDN_TE_B2,
	ISDN_TE_H,
	ISDN_TE_S,
	ISDN_TE_Q,
	ISDN_TE_B1_56,
	ISDN_TE_B2_56,
	ISDN_TE_B1_TR,
	ISDN_TE_B2_TR,

	/* Codec defines */
	ISDN_CHI,

	/*
	 * Primary rate definitions can be added here. ex:
	 * ISDN_PRI_D = 0x15,
	 * ISDN_PRI_NT_B1...BN,
	 * ISDN_PRI_TE_B1...BN,
	 */
	ISDN_LAST_PORT,
	/* DO NOT PUT ANY NEW ENTRIES AFTER THIS LAST DUMMY ENTRY */
} isdn_port_t;

/*
 * ISDN specific connection request ioctl structures
 */
#ifdef	DBRI_UNSUPPORTED
/*
 * The following ioctls are not documented or supported and will 
 * go away in future releases. They will be replaced with a 
 * documented and supported interface. 
 */
#define	ISDN_SETPORT	_IOW(A, 5, isdn_conn_req_t) /* set-up predefined port */
#define	ISDN_RELPORT	_IOW(A, 6, isdn_conn_req_t) /* rel predefined port */
#define	ISDN_MODPORT	_IOW(A, 7, isdn_conn_req_t) /* mod predefined port */
#define	ISDN_GETPORTTAB	_IOR(A, 9, dbri_porttable_t) /* list of ports in use */
#endif	DBRI_UNSUPPORTED
#define	ISDN_STATUS	_IOWR(A, 8, isdn_info_t) /* get ISDN status */

/*
 * ISDN_PH_ACTIVATE_REQ - CCITT PH-ACTIVATE.req can only be used with a
 * file descriptor that is connected to a D-Channel.
 */
#define	ISDN_PH_ACTIVATE_REQ	_IO(A, 10)	   /* Activate TE interface */

/*
 * ISDN_MPH_DEACTIVATE_REQ - CCITT PH-ACTIVATE.req can only be used with
 * a file descriptor that is connected to a D-Channel. This ioctl is only
 * legal on NT D-Channels.
 */
#define	ISDN_MPH_DEACTIVATE_REQ	_IO(A, 11)	   /* deactivate ISDN intface */

/*
 * ISDN_MESSAGE_SET - Enable/disable the reception of CCITT primitive
 * messages as M_PROTO messages on a D-Channel.
 * 		Argument == 0, disables messages, uses S_MSG/SIGPOLL instead.
 * 		Argument == 1, enables CCITT messages, uses M_PROTO.
 * An ISDN driver will send M_PROTO messages up-stream using the data
 * structure isdn_message_t.
 *
 * Vendor specific messages will always use M_PROTO.
 */
#define	ISDN_MESSAGE_SET	_IOW(A, 12, int)
typedef enum {
	ISDN_MESSAGE_SET_SIGNAL = 0,	/* Use signals instead of messages */
	ISDN_MESSAGE_SET_PROTO,		/* Driver sends M_PROTO messages */
} isdn_message_set_t;

/*
 * ISDN_PH_PARAM_POWER_* - Turn power for an ISDN interface on and off. This
 *	is equivalent to inserting or removing the ISDN cable from
 *	the ISDN equipment on an ISDN-line powered TE.
 *	1=power on, 0=power off.
 *	Interfaces that do not support this ioctl return -1 and errno
 *	set to EXIO.
 */
#define ISDN_PH_PARAM_POWER_OFF		(0)
#define ISDN_PH_PARAM_POWER_ON		(1)

/*
 * ISDN_PH_SET_PARAM - Set an ISDN device parameter.
 */
#define ISDN_SET_PARAM	_IOW(A, 16, isdn_ph_param_t)
#define ISDN_GET_PARAM	_IOWR(A, 16, isdn_ph_param_t)

typedef enum {
	ISDN_PARAM_NONE = 0,
	ISDN_NT_PH_PARAM_T101,	/* NT Timer, 5-30 s, in milliseconds */
	ISDN_NT_PH_PARAM_T102,	/* NT Timer, 25-100 ms, in milliseconds */
	ISDN_TE_PH_PARAM_T103,	/* TE Timer, 5-30 s, in milliseconds */
	ISDN_TE_PH_PARAM_T104,	/* TE Timer, 500-1000 ms, in milliseconds */
	ISDN_PH_PARAM_end_of_timers = 99, /* highest possible timer parameter */

	ISDN_PH_PARAM_MAINT,	/* Manage the TE Maintenence Channel */
	ISDN_PH_PARAM_ASMB,	/* Modify Activation State Machine Behavoir */
	ISDN_PH_PARAM_POWER,	/* ISDN_PH_PARAM_POWER_* */
	ISDN_PH_PARAM_PAUSE,	/* Paused if == 1, else not paused == 0 */

	ISDN_PH_PARAM_vendor = 1000, /* Vendor specific params start at 1000 */
} isdn_ph_param_tag_t;

#define ISDN_PH_PARAM_VENDOR(x) \
	((isdn_ph_param_tag_t)((int)ISDN_PH_PARAM_vendor+(int)(x)))

/*
 * Modify activation state machine behavior.
 * This parameter takes effect immediately.
 */
enum isdn_ph_param_asmb {
	ISDN_TE_PH_PARAM_ASMB_CCITT88,	/* 1988 bluebook */
	/*
	 * Conformance Test Suite 2, used by CNET for France Telecom testing.
	 */
	ISDN_TE_PH_PARAM_ASMB_CTS2,
};

/*
 * This parameter takes effect the next time the device is opened. XXX?
 */
enum isdn_ph_param_maint {
	/*
	 * ISDN_PH_PARAM_MAINT:
	 *
	 * If bit 8 is zero, F(A) will be zero in all conditions
	 */
	ISDN_PH_PARAM_MAINT_OFF,

	/*
	 * ISDN_PH_PARAM_MAINT:
	 *
	 * If bit 8 is 1 and there is no source for Q-channel then F(A) will
	 * echo the received F(A)
	 */
	ISDN_PH_PARAM_MAINT_ECHO,

	/*
	 * ISDN_PH_PARAM_MAINT:
	 *
	 * If bit 8 is 1, and the TE is transmitting Q channel data, then when
	 * a 1 is received in the proper place in the multi-frame the Q-data bit
	 * will go out in the F(A) bit of the current frame.
	 */
	ISDN_PH_PARAM_MAINT_ON,
};


typedef struct isdn_ph_param {
	isdn_ph_param_tag_t	tag;
	union {
		unsigned int			us;	/* micro seconds */
		unsigned int			ms;	/* Timer value in ms */
		unsigned int			flag;	/* Boolean */
		enum isdn_ph_param_asmb		asmb;
		enum isdn_ph_param_maint	maint;
		struct {
			isdn_port_t		port;	/* Port to Pause */
			int			paused;	/* TRUE or FALSE */
		} pause;
		unsigned int	reserved[2];	/* reserved future expansion */
	} value;
} isdn_ph_param_t;

/*
 * ISDN_ACTIVATION_STATUS - Query the current activation state of an
 * interface.  "type" must be set to indicate the interface to query.
 *
 * type == ISDN_TYPE_SELF may be used to get the activation status of the
 * interface connected to the file descriptor used in the ioctl.
 */
#define	ISDN_ACTIVATION_STATUS	_IOWR(A, 13, isdn_activation_status_t)

/*
 * ISDN_SET_LOOPBACK - Set the specified interface into remote loopback
 * mode.
 *
 * ISDN_RESET_LOOPBACK - Clear the specified loopbacks on the specified
 * interface.
 */
#define	ISDN_SET_LOOPBACK	_IOW(A, 14, isdn_loopback_request_t)
#define	ISDN_RESET_LOOPBACK	_IOW(A, 15, isdn_loopback_request_t)

/*
 * Data mode to be used by the pipe.  Note that this does not apply to
 * fixed pipes.
 */
typedef	enum {
		ISDN_MODE_UNKNOWN = -1,		/* mode predefined by def */
		ISDN_MODE_HDLC = 0x1,		/* HDLC mode */
		ISDN_MODE_TRANSPARENT = 0x2,	/* Transparent mode */
} isdn_mode_t;

/*
 * Indicates if this is a one-way data pipe or a two way data stream. The
 * audio semantics have been retained. Note that if ISDN_ONEWAY_STREAM is
 * choosen then the direction of the single data pipe will be from
 * transmit_errors to recvport.
 */
typedef	enum {
		ISDN_ONEWAY_STREAM = 1,		/* One way data stream */
		ISDN_TWOWAY_STREAM = 2,		/* Two way data stream */
} isdn_streamdir_t;


/*
 * This is the main structure that is passed in with the SETPORT and
 * RELPORT ioctls. Note that once a CREATPORT returns success, the user
 * should simply use the SETPORT and RELPORT ioctl's on the new device.
 * The port numbers of the stdport structure used in this case should be
 * set to ISDN_NONSTD.  The standard port type simply uses the defines at
 * the top of this header file. These ports have predefined length and
 * cycle fields.
 *
 *
 * *** Note that the xmit and receive ports are DBRI relative.  Thus a
 * write channel from the host to the TE B1 channel would have a xmit TE
 * port and a receive HOST port even though the data is being transmitted
 * from the host.
 */
typedef struct isdn_conn_req {
	int			unit;		/* unit # 0-3 */
	isdn_port_t		xmitport;	/* transmit port end type */
	isdn_port_t		recvport;	/* receive port end type */
	isdn_mode_t		mode;		/* data mode */
	isdn_streamdir_t	dir;		/* uni/bi-directional */
	int			reserved[2];	/* reserved for future use */
} isdn_conn_req_t;

typedef enum isdn_activation_state {
	ISDN_OFF = 0,		/* Interface is powered down */
	ISDN_UNPLUGGED,		/* Power but no-physical connection */
	ISDN_DEACTIVATED,	/* Activation is permitted */
	ISDN_ACTIVATE_REQ,	/* Attempting to activate */
	ISDN_ACTIVATED,		/* Interface is activated */
} isdn_activation_state_t;

typedef enum {
	ISDN_TYPE_UNKNOWN = -1,		/* Not known or applicable */
	ISDN_TYPE_SELF = 0,		/*
					 * For queries, application may
					 * put this value into "type" to
					 * query the state of the file
					 * descriptor used in an ioctl.
					 */
	ISDN_TYPE_OTHER,		/* Not an ISDN interface */
	ISDN_TYPE_TE,
	ISDN_TYPE_NT,
	ISDN_TYPE_PRI,
} isdn_interface_t;

enum isdn_iostate {
	ISDN_IO_UNKNOWN = -1,		/* I/O state not known or applicable */
	ISDN_IO_STOPPED,		/* DMA is not enabled */
	ISDN_IO_READY,			/* DMA is enabled */
};

#define	ISDN_PROTO_MAGIC	0x6973646e	/* "isdn" */

/*
 * TE sends: PH-AI, PH-DI, MPH-AI, MPH-DI, MPH_EI1, MPH_EI2, MPH_II_C, MPH_II_D
 * NT sends: PH_AI, PH_DI, MPH_AI, MPH_DI, MPH_EI1
 */
typedef enum isdn_message_type {
	VPH_VENDOR = 0,			/* Vendor specific messages */

	PH_AI,				/* Physical: Activation Ind */
	PH_DI,				/* Physical: Deactivation Ind */

	MPH_AI,				/* Management: Activation Ind */
	MPH_DI,				/* Management: Deactivation Ind */
	MPH_EI1,			/* Management: Error 1 Indication */
	MPH_EI2,			/* Management: Error 2 Indication */
	MPH_II_C,			/* Management: Info Ind, connection */
	MPH_II_D,			/* Management: Info Ind, disconn. */

}	isdn_message_type_t;

typedef struct isdn_message {
	unsigned int		magic;		/* ISDN_PROTO_MAGIC */
	isdn_interface_t	type;		/* Interface type */
	isdn_message_type_t	message;	/* CCITT Primitive or Vendor */
	unsigned int		vendor[5];	/* Vendor specific content */
}	isdn_message_t;

typedef struct isdn_activation_status {
	isdn_interface_t		type;
	enum isdn_activation_state	activation;
}	isdn_activation_status_t;

typedef enum {
	ISDN_LOOPBACK_LOCAL,
	ISDN_LOOPBACK_REMOTE,
} isdn_loopback_type_t;

typedef enum {
	ISDN_LOOPBACK_B1 = 0x1,
	ISDN_LOOPBACK_B2 = 0x2,
	ISDN_LOOPBACK_D = 0x4,
	ISDN_LOOPBACK_E_ZERO = 0x8,
	ISDN_LOOPBACK_S = 0x10,
	ISDN_LOOPBACK_Q = 0x20,
} isdn_loopback_chan_t;

typedef struct isdn_loopback_request {
	isdn_loopback_type_t		type;
	int 				channels;
}	isdn_loopback_request_t;

/*
 * ISDN_STATUS ioctl uses this data structure
 */
typedef struct isdn_info {
	isdn_interface_t		type;

	/*
	 * Activation State Machine information
	 */
	isdn_activation_state_t	activation;

	/*
	 * Counters for physical layer ASM primitives
	 */
	unsigned int	ph_ai;		/* Physical: Activation Ind */
	unsigned int	ph_di;		/* Physical: Deactivation Ind */
	unsigned int	mph_ai;		/* Management: Activation Ind */
	unsigned int	mph_di;		/* Management: Deactivation Ind */
	unsigned int	mph_ei1;	/* Management: Error 1 Indication */
	unsigned int	mph_ei2;	/* Management: Error 2 Indication */
	unsigned int	mph_ii_c;	/* Management: Info Ind, connection */
	unsigned int	mph_ii_d;	/* Management: Info Ind, disconn. */

	/*
	 * Per channel IO Statistics
	 *	XXX Per D/B channel? What about PRI?
	 */
	enum isdn_iostate	iostate;
	struct isdn_io_stats {
		unsigned int	packets;/* Number of packets transferred */
		unsigned int	octets;	/* Number of octets transferred */
		unsigned int	errors;	/* Number of errors encountered */
	}	transmit,		/* Transmission statistics */
		receive;		/* Receive statistics */
} isdn_info_t;
