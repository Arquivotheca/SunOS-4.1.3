/*      @(#)commands.h 1.1 92/07/30 Copyright Sun Micro"             */

/* Command Set used for Host driver requests to Narya
 *	and for Narya F/W requests to the Host driver
 */

/* There are two command classes: data transmission requests and indications,
 *	and directives from the driver or status/emergency reports from
 *	the Narya F/W.
 *
 *	The commands are issued by allocating a Cmd_buff from Np_ram,
 *	filling in the relevant fields, and posting the Cmd_buff pointer
 *	onto the appropriate queue; command posting implies interrupting the
 *	command recipient. These commands are of varying length, and the
 *	Np_ram pool for these blocks will be managed as two circular buffers,
 *	one for the Host and one for Narya F/W.
 *
 * Narya Command Reception: all commands are received by the MSGin
 *	process (or ISR if tolerable); this process exhaustively reads
 *	the command queue. For each queue entry, the MSGin sorts on the
 *	command type, digests the contents into internal structures, releases
 *	the Cmd_buff, and posts these digested commands via message exchanges
 *	to the interested processes (e.g. receive_proc, xmit_proc, smt_proc,
 *	misc_proc, plm_proc, etc.).
 */

/* NOTE: For the time being (while we are only duplicating ethernet
 *	capabilities), there will be no FDDI specific information in any
 *	commands.
 */

typedef struct {
	enum {
		/* Host to Narya Commands */
		TRANSMIT,
		NET_CONFIG,
		CTLR_CONFIG,
		INQUIRE_STATUS,

		/* Narya to Host Commands */
		RECEIVE,
		RELEASE_XMIT_BUFFER,	/* Frees transmit buffers and
					 * acknowledge
					 */
		STATUS,
		CMNDS_ACK,		/* command receipt acknowledgement */

		GET_NETMAP_ENT,		/* Get a network map entry */
		NETMAP_ENT,		/* Provide a netmap entry to the host */
		SMT_CONFIG,		/* Command to set smt frame switches */
	} type;

	unsigned long	command_id;	/* used in any correspondence
					 * pertaining to this command
					 */
	enum {
		IFN_fddi = 0,		/* FDDI */
		IFN_ether,		/* Ethernet */
		IFN_serial,		/* Serial Line Internet Proto.*/
		IFN_hss,		/* High Speed Serial */
	} interface;			/* Interface name */

	union {
		struct {
			int	fc;	/* frame control */

			/* Source address comes from a separate
			 * NET_CONFIG command.
			 */

			/* Destination address is the first thing in the packet
			 * the supplied fc indicates long or short addresses.
			 */

			/* XXX - IMPORTANT QUESTION
			 *
			 * For the moment we assume that we can map all of a
			 * packet into DVMA space at one time. If this
			 * assumption is incorrect, the command semantics
			 * will have to change. Worst case would be the max
			 * packet (4500 bytes) contained in about 40 112 byte
			 * mbufs each on a different kernel page, thereby
			 * requiring 40 pages of DVMA space (gak!).
			 */
#define MAX_DMASEG 6	/* N.B. Cannot be > value in dma.h */
			int		nbuf;
			/* FDDI: sync, async, FC bits, etc... */
			struct {
				unsigned char	*buffer;
				unsigned int	length;
			 } bufdesc[MAX_DMASEG];
		} transmit;

		struct {
			/* It is the driver's responsibility to associate
			 * xmit_command_id with the TRANSMIT command's original
			 * command_id and release the appropriate DVMA mappings.
			 */
			unsigned long	xmit_command_id;
			enum { COPY_OK, COPY_FAILED }	success;
			unsigned char	*failed_buffer;
		} release_xmit_buffer;

#define MAX_RECV_BUF 6	/* N.B. Cannot be > value in dma.h */
		struct {
			 int			fc;
			 int	 		nbuf;
			 /* FDDI: sync, async, FC bits, etc... */
			 enum {RCV_ERRORS, RCV_OK}	status;
			 struct {
				unsigned char	*buffer;
				unsigned short	length;
			 } bufdesc[MAX_RECV_BUF];
		} receive;

		struct {
			/* Source addresses for long and short address forms */
			enum {
				IFA_Up,		/* Bring interface up */
				IFA_Down,	/* Bring interface down */
				IFA_Mode_change,/* Change interface mode */
				IFA_GAT_add,	/* Add to Group Address Table */
				IFA_GAT_delete,	/* Delete from G.A.T. */
				IFA_GAT_reset	/* Clear entire table */
			} action;		/* Interface Action */
			enum {
				Mode_Non_promiscuous = 0,
				Mode_Promiscuous = 1, /* Receive All packets */
			} mode;
			mac_addr_t	mac_addr_long;
			short_mac_addr_t	mac_addr_short;
		} net_config;

		struct fddi_ctlr_config {
			int	fddi_tvx, 	/* time between end delims */
				fddi_treq,
				fddi_tmax;	/* TMAX value */
		} ctlr_config;

		struct fddi_smt_config{
			boolean_t	resp_frame_sw, 
						nif_frame_sw,
						disable_fw_agent;
		} smt_config;

		struct {
			/* Freq at which to return FW status to host */
			/* 0 for this value means stop sending status*/
			int	freq_status; 
		} inq_status;

		struct fddi_status_struct {
			/* What is the Configuration and Ring Stability */
			int		ring_operational;
			ec_state_t	ecm_state;
			ec_status_t	ecm_status;
			cf_state_t	cfm_state;
			rm_state_t	rmt_state;
			struct {
				short	dummy;
				uchar_t	remotephytype;
				uchar_t	state;
			} pcma;
			struct {
				short	dummy;
				uchar_t	remotephytype;
				uchar_t	state;
			} pcmb;
			obp_state_t	obp_state;	/* Optical bypass */

			/* Exceptional condition counts for RECV and XMIT */
			ulong_t	ring_toggles;	/* # of ring up events */
			ulong_t	receive_full;	/* buffer ram is full */
			ulong_t	receive_error;	/* packets received in error */
			ulong_t	transmit_abort;	/* # of xmit abort conditions */

			/* Throughput statistics */
			ulong_t		packets_xmit,	/* XXX redundant */
					bytes_xmit,	/* XXX redundant */
					packets_recv,	/* XXX redundant */
					bytes_recv;	/* XXX redundant */
		} status;

	} req;
} command_t;

struct fddi_netmap_entry {
	union {
		int 	station_info; /* Coded info of station */
			struct {
				char	untopology,
					dntopology,
					topology,
					dupladdress;
			} smt_s;
		} smtu;
	union {
		int 	phya_info;
		struct {
			uchar_t	phya_connectstate,
				phya_remotephytype;
				short	phya_dummy;
			} phya_s;
		} phya_u;
	union {
		int	phyb_info;
		struct {
			uchar_t	phyb_connectstate,
				phyb_remotephytype;
				short	phyb_dummy;
			} phyb_s;
		} phyb_u;
	int		t_req,		/* requested trt */
			tneg,		/* negotiated trt */
			tmax,		/* my tmax value */
			tvx,		/* my tvx value */
			f_ct,		/* Frame count */
			e_ct,		/* Error count */
			l_ct;		/* Lost packet count */
	mac_addr_t	net_addr,	/* address of station */
				us_neighbor,	/* address of USN */
				ds_neighbor;	/* address of DSN */
	int		smt_revision;
} ;

#define FDDI_TIMER_CONFIG 1
#define FDDI_SMT_SWITCHES 2
