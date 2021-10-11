/*      @(#)fddi_types.h 1.1 92/07/30 Copyright Sun Micro"             */

/* This file contains fundamental definitions that are required by both the
 * host driver and the firmware but are not directly related to host/controller
 * communication.
 */

typedef unsigned char	uchar_t;
typedef unsigned short	ushort_t;
typedef unsigned int	uint_t;
typedef unsigned long	ulong_t;
typedef unsigned char	*charp_t;

typedef void    (*fptrv_t) ();	/* pointer to function returning void */
typedef int     (*fptri_t) ();	/* pointer to function returning int */

typedef long	function_t;	/* used to manipulate pROBE and pSOS data */

typedef enum {
	False = 0, True = 1,
	Clear = 0, Set = 1,
} boolean_t;


/*
 * MAC (Media Access Controller) address - 6 octets
 */
typedef struct {
	unsigned char	octet[6];
} mac_addr_t;

typedef struct {
	unsigned char	octet[2];	/* Short network address */
} short_mac_addr_t;

/* ECM state machine state names indicating station connection status: */
typedef enum {
	ECS_Out,		/* EC0 */
	ECS_In,			/* EC1 */
	ECS_Trace,		/* EC2 */
	ECS_Leave,		/* EC3 */
	ECS_Path_Test,		/* EC4 */
	ECS_Insert,		/* EC5 */
	ECS_Check,		/* EC6 */
	ECS_Deinsert		/* EC7 */
} ec_state_t;

typedef enum {
	ECStatus_Clear,
	ECStatus_ConnectSuccessful,
	ECStatus_ConnectFailed,
	ECStatus_DisconnectRequestedBeforeConnectCompleted,
	ECStatus_DisconnectSuccessful,
	ECStatus_SelfDisconnected,
	ECStatus_StuckBypass
} ec_status_t;

/* PCM state machine state names indicating PHY connection status */ 
typedef enum {
	PCS_Off,	/* PC0 */
	PCS_Break,	/* PC1 */
	PCS_Trace,	/* PC2 */
	PCS_Reject=PCS_Trace,	/* PC2 */
	PCS_Connect,	/* PC3 */
	PCS_Next,	/* PC4 */
	PCS_Signal,	/* PC5 */
	PCS_Join,	/* PC6 */
	PCS_Verify,	/* PC7 */
	PCS_Active,	/* PC8 */
	PCS_Maint,	/* PC9 */
} pc_state_t;

/* CFM state machine state names indicating station configuration: */
typedef enum {
	CFS_Isolated,	/* SC0 */
	CFS_WrapA,	/* SC1 */
	CFS_WrapB,	/* SC2 */
	CFS_WrapAB,	/* SC3 */
	CFS_ThruA,	/* SC4 */
	CFS_ThruB,	/* SC5 */
	CFS_ThruAB,	/* SC6 */
} cf_state_t;

/* RMT state machine state names indicating ring management: */
typedef enum {
	RMS_Isolated,	/* RM0 */
	RMS_Non_Op,	/* RM1 */
	RMS_Ring_Op,	/* RM2 */
	RMS_Detect,	/* RM3 */
	RMS_Non_Op_Dup,	/* RM4 */
	RMS_Ring_Op_Dup,/* RM5 */
	RMS_Directed,	/* RM6 */
	RMS_Rm_Trace,	/* RM7 */
} rm_state_t;

/* Optical Bypass switch state */
typedef enum {
	OBP_None,
	OBP_ExistsOut,
	OBP_ExistsIn,
} obp_state_t;


/* Strcutre for caching the mac hdr big-endian to little-endian conv */
struct cache_st {
	ulong_t	wrd;
	ulong_t sw_wrd;
};
/* Macros to convert from Big-endian to Little endian */
#define BE_TO_LE_WRD(in_addr,s_wrd)  { \
		ulong_t		wrd_cnt,tmp_wrd=0; \
		uchar_t		*addr; \
		addr = (uchar_t *)in_addr; \
		for (s_wrd=0,wrd_cnt=0; wrd_cnt < 4; wrd_cnt++) { \
			tmp_wrd = (ulong_t)( ((swap[(*addr)&0xf]) << 4) | \
			 	(swap[(((*addr)&0xf0) >> 4)]) );  \
			s_wrd = (s_wrd << 8) | tmp_wrd; \
			tmp_wrd=0; \
			addr ++; \
		} \
	}

/*
#define BE_TO_LE_MAC(iaddr,oaddr)  { \
		uchar_t *addr,*o_addr; \
		ulong_t byt_cnt; \
		addr = (uchar_t *)iaddr; \
		o_addr = (uchar_t *)oaddr; \
		for (byt_cnt=0; byt_cnt < 6; byt_cnt++) { \
			*(o_addr) = ( ((swap[(*addr)&0xf]) << 4) | \
		 	(swap[(((*addr)&0xf0) >> 4)]) );  \
			addr++; \
			o_addr++; \
		} \
	}
*/
