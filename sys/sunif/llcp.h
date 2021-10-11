/*      @(#)llcp.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * header file for Low Level Command Protocol (llcp) 
 */


typedef enum {
		HIO_HLCP,
		HIO_LLCP
}hostio_t;	/* can either be in hlcp mode or llcp mode */

/*
 * llcp command set: commands issued by host to controller
 */
typedef enum {
	/* 
	 * controller independent commands
	 */
	LLCP_CMD_COM = 0x0,		/* command complete */
	LLCP_RESET,			/* software reset of controller */
	LLCP_STRT_INIT,			/* initiate intialization sequence */
	LLCP_GET_INFO,			/* get info structure from controller */
	LLCP_SEND_INFO,			/* send host struct to controller */
	LLCP_GET_PKT,			/* get packet of data from controller */
	LLCP_SEND_PKT,			/* send packet of data to controller */
	LLCP_ERROR,			/* force controller to error state */
	LLCP_ENA_NET,			/* initiate execution from second prom*/

	/*
 	 * controller specific commands (ie. for NARYA)
 	 */
	LLCP_DWNLOAD = 0x10000000,	/* initiate download to controller */
	LLCP_EX_USER,			/* controller to execute in user mode */
	LLCP_EX_SUPER,			/* cntrlr to execute in super mode */
	LLCP_GOTO_HLCP,			/* controller to goto hlcp */
	LLCP_SET_VBR,			/* set vector base register */

	/*
 	 * diagnostic controller specific commands (ie. for NARYA)
 	 */
	 LLCP_WARM_START= 0x80000000,	/** WARM start NP **/

         LLCP_RAMTEST,			/** puts NP in RAM test state  **/
         LLCP_NP_SLEEP			/** puts NP to sleep           **/	
} cmd_t;


/*
 * controller states
 */
typedef enum {
	LLCP_NOT_RDY = 0x5678,	/* controller not ready state */
	LLCP_INIT1,		/* initialization state - xfer addr */
	LLCP_INIT2,		/* initialization state - get cntrlr info */
	LLCP_INIT3,		/* initialization state - send host info */
	LLCP_RDY,		/* ready to accept commands state */
	LLCP_BUSY,		/* busy executing command state */
	LLCP_HLCP,		/* finished setting HLCP transition */
	LLCP_ERR,		/* h/w error or error in protocol */
	LLCP_ANY_STATE		/* any of the above states */
} state_t;


/* 
 * fault codes: indicate particular fault in error state or command
 *		succeed, fail, or invalid command from other states
 */
typedef enum {
	LLCP_FC_OK = 0x0,	/* command succeeded */
	LLCP_FC_NO_COM,		/* command did not complete */
	LLCP_FC_INV_CMD,	/* command not implemented or not valid */

	/*
	 * codes 256 and up reserved for diagnostics
	 */
	LLCP_FC_FATAL = 0x100,	/* fatal hardware failure  */
	LLCP_FC_NOPROM2         /* no second prom available  */
} fault_t;

/* 
 * memory type - indicates use of dma or shared memory
 */
typedef enum {
	DMA_MEM,		/* controller uses dma for data transfers */
	SHARED_MEM		/* controller used shared mem for data xfers */
} mem_t;


/* 
 * address type - indicates use of long or short network addresses 
 */
typedef enum {
	SHORT_ADDR,		/* short 16 bit MAC address */
	LONG_ADDR		/* long 48 bit MAC address */
} llcpaddr_t;


/*
 * llcp struct: shared memory interface between intelligent network 
 *              controllers and main host.
 */
typedef struct {
	cmd_t	cmd;		/* reg for commands and synchronization  */
				/* writes !0 cmd, waits for response of 0 */
	state_t	state;		/* reg indicating current state of controller */
	unsigned char	*addr;	/* reg to transfer either host or cntr addr */
	long	len;		/* reg to transfer length of data transferred */
	fault_t	fault;		/* indicates fail. fault code on err state */
} llcp_reg_t;


#define	LLCP_VERSION 	1	/* llcp version number which should be bumped */
				/* when the interface changes */
#define	VER_STR		20	/* length of version strings */
#define	LLCP_FAIL	0	/* llcp routine failed */
#define	LLCP_SUCC	1	/* llcp routine succeeded */
#define	LLCP_T_RESET	4000000	/* 4 second reset cmd timeout */
#define	LLCP_RST_TIME	20000000 /* 20 seconds max time for LLCP_NOT_RDY */
				/* additional 20 sec max time for LLCP_INIT1 */
#ifdef PROMCODE 
#define LLCP_BUF_SIZ	1600	/* XXX DO NOT INCREASE as there are */ 
				/* hardwired dependencies in the nd code */
#else
#define LLCP_BUF_SIZ	4500	/* compatible with new nfs packet sizes */ 
#endif
#define LLCP_OFFBUS	2000	/* time for host cpu to stay off bus */ 
				/* while accessing llcp shared memory regs*/

/* 
 * controller information sent to Host
 */
typedef struct {
	int len;		/* length of structure */
	int llcp_ver;		/* version of llcp */
	llcpaddr_t addrtyp;	/* type of netword address that follows */
	char net_laddr[6];	/* network long 48 bit MAC address */
	char net_saddr[2];	/* network short 16 bit MAC address */
	unsigned char *buf;	/* address of llcp buffer */
	mem_t memtyp;		/* indicates dma or shared memory */
	unsigned char *phys_addr;	/* physical address of shared mem */
	long mem_size;		/* size of shared memory */
	unsigned char *strtadr;	/* firmware start address */
	unsigned timeout;	/* timeout value for commands */
	int fw_ver;		/* firmware version */
	int hw_ver;		/* hardware version */
	int max_pkt;		/* maximum packet size */
} llcp_cntrlr_info_t;

/* 
 * host information sent to controller
 */
typedef struct {
	int len;		/* length of structure */
	int llcp_ver;		/* version of llcp */
	llcpaddr_t addrtyp;	/* type of netword address that follows */
	char net_laddr[6];	/* network long 48 bit MAC address */
	char net_saddr[2];	/* network short 16 bit MAC address */
	unsigned char *buf;	/* addr of llcp buffer */
	mem_t memtyp;		/* indicates dma or shared memory */
	char host_arch[VER_STR];/* host machine type */
	int prom_rev;		/* host prom revision */
		/* from boot (contrlr, unit, partition */
	int	ctlr;		/* Controller number or address */
	int	unit;		/* Unit number within controller */
	long    partition;	/* Partition number within unit */
} llcp_host_info_t;


/*
 * complete state of all llcp information
 */

typedef struct {
	llcp_reg_t *regp;
	llcp_cntrlr_info_t *cinfop;
	llcp_host_info_t *hinfop;
} llcp_info_t;


/*
 * LLCP Error Message Defines
 */
#define E_LLCP_DMA 	0	/* cntrl mem type (dma/shared) set incorrectly*/
#define E_LLCP_RDY 	1	/* controller not responding */
#define E_LLCP_INV 	2	/* invalid controller command */
#define E_LLCP_EST 	3	/* error string */

