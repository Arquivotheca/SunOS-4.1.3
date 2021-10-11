/*****************************************************************************

  static char sccsid[] = "@(#)entest.h	1.1 7/30/92 17:35:49, Copyright Sun Microsystems";

  Date:		Tue Oct 31 14:58:17 1989
  File:		entest.h
  Author:	Anantha Srirama
  Description:	This file contains the generic defines used only in entest.

  Copyright (c) 1989 Sun Microsystems, Inc.

*****************************************************************************/
#define		DEVICE_NAME	"fddi0"
#define		TOTAL_PASS	25
#define		ERROR_LIMIT	5


/* 
 * FDDI definitions for driver
 */
#define FDDI_MASK	0xF0FF


/* 
 * Error code definitions passed to send_message
 */
#define	NO_SOCKET	1	/* Socket open failed			*/
#define	NO_FDDI_IF	2	/* FDDI not in system			*/
#define NIT_OPEN_FAIL	3	/* /dev/nit open failed 		*/
#define NIT_RMSGD_FAIL	5	/* message discard operation failed	*/
#define FILT_PUSH_FAIL	6	/* filter push failed			*/
#define SET_FILT_FAIL	7	/* could not set the filter with ioctl	*/
#define NIT_BIND_FAIL	8	/* could not bind /dev/nit to FDDI	*/
#define NIT_FLUSH_FAIL	9	/* could not flush readside of stream	*/
#define IF_ADDR_FAIL	10	/* could not get FDDI interface address */
#define SA_MISMATCH	11	/* bad SA in SIF response packet	*/
#define EXIT_ON_ERROR	12	/* exiting test on first error		*/
#define TOOMANY_ERR	13	/* too many errors during testing	*/
#define SEND_FAIL	14	/* could not send packet		*/
#define RECV_FAIL	15	/* could not receive packet		*/
#define RECV_TMOUT	16	/* Receive timeout			*/
#define RECONFIG_ERROR  17	/* (Ioctl) reconfig error               */ 


/*
 * Firmware Status
 */
#define RECONFIGURED	0

/*
 * Extended header
 */
typedef struct {
    mac_addr_t da;
    mac_addr_t sa;
    u_short    type;
} machdr_t;


/* 
 * FDDI SMT (Station ManagemenT) frame format
 */
typedef struct {
    u_char	frame_class;
    u_char	frame_type;
    u_short	smt_version;
    u_long	transaction_id;
    u_char	station_id[8];
    u_short	pad;
    u_short	info_field_len;
} smthdr_t;


/*
 * SMT SIF response frame format
 */
typedef struct {
    machdr_t machdr;
    smthdr_t smt;
    u_char sif_info_field[0x116A]		/* max allowed in FDDI spec */
} sif_resp_t;


/* 
 * MAC (Media Access Controller) Frame Class.  The FDDI standard specifies
 * the FC to be an octet, but here we are programming it as two octets.
 * This works out fine because, the FDDI driver will strip out the most
 * significant octet when building the actual packet.  This also explains
 * why the machdr_t has a 'type' field of 2 octets when it should have been 1
 */
#define	MAC_FC		0xF041


/* 
 * SMT (Station ManagemenT) Frame Class for SIF (Station Information Frame)
 */
#define SMT_SIF_CLASS	0x02


/* 
 * SMT (Station Management) Frame Type for a SIF Request Frame
 */
#define	SMT_SIF_REQUEST	0x02

/*
 * SMT Frame Type for SIF Response Frame
 */
#define SMT_SIF_RESPONSE 0x03


/*
 * SMT verion ID
 */
#define	SMT_VERSION	1


/*
 * Paramter type for MAC Neighbor field in the Information Field of SIF Resp.
 */
#define MAC_NEIGHBOR	0x7


/* 
 * Generic defines
 */
#define ENABLE	0
#define DISABLE	1
