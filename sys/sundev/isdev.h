/*	@(#)isdev.h	1.1 7/30/92 SMI */

/*
 * Device registers for the Sun VME IPI String Controller or Channel.
 */

#ifndef _isdev_h
#define _isdev_h 

#define ISC_MAX_CMD	64		/* maximum command packet size */
#define ISC_MAX_RESP	72		/* maximum response packet size */

#define ISC_REG_SIZE	0x400		/* size of shared memory */

struct	is_reg {
	u_long		dev_bir;	/* Board ID register */
	u_long		dev_csr;	/* CSR - Status register */
	u_long		dev_cmdreg;	/* Command register */ 
	u_long		dev_resp;	/* Response register (read only) */ 
	u_long		dev_vector;	/* VME interrupt vector */
	u_long		dev_aux[3];	/* reserved */
	u_long		dev_cmd_pkt[ISC_MAX_CMD/sizeof(u_long)];  
					/* command packet */
	u_long		dev_resp_pkt[ISC_MAX_RESP/sizeof(u_long)];
					/* response packet */
	u_char		reserved[ISC_REG_SIZE - 8*sizeof(long) 
				- ISC_MAX_CMD - ISC_MAX_RESP];	/* reserved */
};

/*
 * Board ID for Panther-.
 */
#define PM_ID		(('P'<<24) | ('A'<<16) | ('N'<<8) | '-')

/*
 * CSR Bit Definitions
 */
#define CSR_RESET	0x01		/* reset - other bits not valid */
#define CSR_EICRNB	0x02		/* En int on cmd register not busy */
#define CSR_EIRRV	0x04		/* En int on response register valid */
#define CSR_CRBUSY	0x08		/* Command Register Busy */ 

#define CSR_RRVLID	0x10		/* Response Register Valid */ 

#define CSR_ERROR	0x0100		/* Error */ 
#define CSR_MRINFO	0x0200		/* More Info */ 

#define CSR_FAULT	0xff0000	/* mask for fault code */
#define CSR_FAULT_SHIFT	16		/* shift for fault code */

/*
 * Error codes in CSR_FAULT field.  Valid if CSR_ERROR set.
 */
#define	CSR_FAULT_VME_B	1		/* Can't DMA - VME Bus error*/
#define CSR_FAULT_VME_T	2		/* Can't DMA - VME Timeout */
#define CSR_INVAL_CMD	3		/* Invalid command register write */

#define CSR_LED0	0x01000000	/* LED 0 */
#define CSR_LED1	0x02000000
#define CSR_LED2	0x04000000
#define CSR_LED3	0x08000000
#define CSR_LED4	0x10000000
#define CSR_LED5	0x20000000
#define CSR_LED6	0x40000000
#define CSR_LED7	0x80000000	/* LED 7 */

#endif	/* !_isdev_h */ 
