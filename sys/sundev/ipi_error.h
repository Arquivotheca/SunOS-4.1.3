/* @(#)ipi_error.h	1.1 7/30/92 */ 

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _ipi_error_h
#define	_ipi_error_h

/*
 * Definitions for IPI error table.
 */

#define	IE_MASK		0xf	/* mask for result */

/*
 * result/action code
 */
#define IE_RESP_EXTENT	1	/* handle response extent */
#define	IE_MESSAGE	2	/* print a message */
#define	IE_STAT_OFF	3	/* addressee's status bits became false */
#define	IE_STAT_ON	4	/* addressee's status bits became true */
#define IE_IML		5	/* IML slave */
#define IE_RESET	6	/* re-initialize addressee */
#define IE_QUEUE_FULL	7	/* queue full - limit new commands */
#define	IE_READ_LOG	8	/* read log from controller */
#define IE_ABORTED	9	/* handle aborted command */

/*
 * Flags - currently limited to upper 24 bits, allowing driver to use lower 8.
 *	Lower 4 bits used by action code above.
 */
#define	IE_PRINT	0x80000000	/* print entire response */
#define IE_DUMP		0x40000000	/* dump entire response packet */
#define IE_MSG		0x20000000	/* print single message */
#define	IE_CMD		0x10000000	/* error indicates command failed */
#define IE_RETRY	0x08000000	/* driver should retry */
#define IE_ASYNC	0x04000000	/* from an asynchronous interrupt */
#define IE_FAC		0x02000000	/* for a facility (not the slave) */
#define IE_INIT_STAT	0x01000000	/* initial status (don't print) */
#define IE_CORR		0x00800000	/* correctable data error */
#define IE_UNCORR	0x00400000	/* uncorrectable data error */	
#define IE_RE_INIT	0x00200000	/* ctlr/device must be re-initialized */
#define IE_RECOVER	0x00100000	/* device in recovery mode */
#define IE_BUSY		0x00080000	/* busy status changed */
#define	IE_ONLINE	0x00040000	/* online/offline status changed */
#define	IE_READY	0x00020000	/* ready status changed */
#define	IE_PAV		0x00010000	/* P-available status changed */
#define IE_TRACE_RECOV	0x00008000	/* Print message each recovery step */
#define	IE_RECOVER_ACTV	0x00004000	/* recovery routine on stack */
#define	IE_FIRST_PASS	0x00002000	/* run error routine in first pass */

/*
 * Flags used for controller/facility status.
 */
#define IE_STAT_MASK \
	(IE_ONLINE | IE_PAV | IE_READY | IE_BUSY | IE_RECOVER | IE_RE_INIT)

/*
 * Macros to determine whether status is sufficient to send commands.
 * IE_STAT_READY(flags) is true if the device can accept commands.
 * IE_STAT_PRESENT(flags) is true if the device is logically present.
 */
#define IE_STAT_PRESENT(flags) \
		(((flags) & (IE_ONLINE | IE_PAV | IE_READY)) \
			 == (IE_ONLINE | IE_PAV | IE_READY))
#define IE_STAT_READY(flags) \
		(((flags) & IE_STAT_MASK) == (IE_ONLINE | IE_PAV | IE_READY))

/*
 * Flags affecting handling for the whole response.
 */
#define IE_RESP_MASK	(IE_PRINT | IE_CMD | IE_RETRY | IE_DUMP \
	| IE_INIT_STAT | IE_CORR | IE_UNCORR)

/*
 * Flags affecting handling of the parameter.
 */
#define IE_PARM_MASK	(IE_FAC | IE_MSG | IE_FIRST_PASS)

/*
 * Flags affecting only the handling for one matched compare in the parameter.
 */
#define IE_BIT_MASK	(IE_ONLINE | IE_PAV | IE_READY | IE_BUSY | IE_MSG)

/*
 * Header file of IPI error definitions for building error handling tables.
 * 
 * The macros used here must be defined by the including program.
 *
 * IPI_ERR_PARM(slave-id, fac-id, flags, message)
 *		defines the start of a section for a particular parameter.
 *		the slave-id and fac-id are the parameter IDs for the slave
 *		and facilities, respectively.
 *		
 *		Flags supplied for this macro affect all bits for the parameter.
 *
 * IPI_ERR_BIT(byte, bit, flags, message)
 *		defines the flags and message for a bit in the current parm.
 * 		The byte number is relative to the IPI spec way of
 *		numbering.  Byte 0 is the parameter ID.
 */
#define	IE_DBG	IE_MSG 	/* define as zero if no debugging messages */

/* #define	IE_DBG	0 */	/* define as IE_MSG if debugging messages needed */
#define IPI_ERR_TABLE_INIT() \
\
IPI_ERR_PARM(RESP_EXTENT, RESP_EXTENT, IE_RESP_EXTENT|IE_FIRST_PASS, \
					"Response Extent") \
\
IPI_ERR_PARM(IPI_S_VEND, IPI_F_VEND, IE_PRINT|IE_CMD|IE_DUMP, "vendor unique") \
 \
IPI_ERR_PARM(IPI_S_MESSAGE, IPI_F_MESSAGE, 0, "Message/Microcode Exception") \
IPI_ERR_BIT(1, 7, IE_PRINT,		"Microcode data not accepted") \
IPI_ERR_BIT(1, 6, IE_IML | IE_PRINT,	"Request Master to IML slave") \
IPI_ERR_BIT(1, 5, IE_PRINT,		"Slave unable to IML") \
IPI_ERR_BIT(1, 4, IE_MESSAGE, 		"Message") \
IPI_ERR_BIT(1, 3, IE_CMD | IE_PRINT,	"Microcode Execution Error") \
IPI_ERR_BIT(1, 2, IE_MESSAGE | IE_CMD,	"Failure Message") \
IPI_ERR_BIT(1, 1, IE_PRINT,		"Port Disable Pending") \
IPI_ERR_BIT(1, 0, IE_PRINT,		"Port Response") \
IPI_ERR_BIT(2, 7, IE_PRINT,		"Facility Status") \
 \
IPI_ERR_PARM(IPI_S_INTREQ, IPI_F_INTREQ, IE_CMD, "Intervention Required") \
IPI_ERR_BIT(1, 7, IE_STAT_OFF | IE_PAV,		"Not P-Available") \
IPI_ERR_BIT(1, 6, IE_STAT_OFF | IE_READY,	"Not Ready") \
IPI_ERR_BIT(1, 5, IE_STAT_OFF | IE_PAV,		"Not P-Available Transition") \
IPI_ERR_BIT(1, 4, IE_STAT_OFF | IE_READY,	"Not Ready Transition") \
IPI_ERR_BIT(1, 3, IE_MSG,			"Physical Link Failure") \
IPI_ERR_BIT(1, 2, IE_IML | IE_PRINT, 	"Attribute Table may be corrupted") \
IPI_ERR_BIT(1, 1, IE_STAT_ON | IE_BUSY,	"Busy") \
 \
IPI_ERR_PARM(IPI_S_ALTPORT, IPI_F_ALTPORT, IE_CMD | IE_MSG, \
					"Alternate Port Exception") \
IPI_ERR_BIT(1, 7, 0,				"Priority Reserve Issued") \
IPI_ERR_BIT(1, 6, 0,				"Attributes Updated") \
IPI_ERR_BIT(1, 5, 0,				"Initialization Completed") \
IPI_ERR_BIT(1, 4, 0,				"Format Completed") \
IPI_ERR_BIT(1, 3, 0,			"Facility switched to other port") \
 \
 \
IPI_ERR_PARM(IPI_S_MCH_EXC, IPI_F_MCH_EXC, 0,	"Machine Exception") \
IPI_ERR_BIT(1, 7, IE_STAT_OFF | IE_BUSY,		"No longer busy") \
IPI_ERR_BIT(1, 6, IE_STAT_ON | IE_PAV,		"P-Available") \
IPI_ERR_BIT(1, 5, IE_STAT_ON | IE_READY | IE_PAV,	"Ready") \
IPI_ERR_BIT(1, 4, IE_RETRY | IE_PRINT,		"Operation Timeout") \
IPI_ERR_BIT(1, 3, IE_RETRY | IE_PRINT,		"Physical Interface Check") \
IPI_ERR_BIT(1, 2, IE_RESET | IE_MSG,		"Slave Initiated Reset") \
IPI_ERR_BIT(1, 1, IE_CMD   | IE_MSG,		"Environmental Error") \
IPI_ERR_BIT(1, 0, IE_MSG,			"Power Fail Alert") \
 \
IPI_ERR_BIT(2, 7, IE_CMD | IE_MSG | IE_UNCORR,	"Data Check (on Raw Data)") \
IPI_ERR_BIT(2, 6, IE_CMD | IE_MSG | IE_UNCORR,	"Uncorrectable Data Check") \
IPI_ERR_BIT(2, 5, IE_CMD | IE_MSG,		"Fatal Error") \
IPI_ERR_BIT(2, 4, IE_CMD | IE_MSG,		"Hardware Write Protected") \
IPI_ERR_BIT(2, 3, IE_QUEUE_FULL | IE_MSG,	"Queue Full") \
IPI_ERR_BIT(2, 2, 0,				"Command Failure") \
 \
IPI_ERR_BIT(3, 6, IE_CMD | IE_MSG,		"Read Access Violation") \
IPI_ERR_BIT(3, 5, IE_CMD | IE_MSG,		"Write Access Violation") \
IPI_ERR_BIT(3, 4, IE_CMD | IE_MSG | IE_RETRY,	"Data Overrun") \
IPI_ERR_BIT(3, 3, IE_CMD | IE_MSG,		"Reallocation space exhausted") \
IPI_ERR_BIT(3, 2, IE_CMD | IE_MSG,		"End of Media Detected") \
IPI_ERR_BIT(3, 1, IE_CMD | IE_MSG,		"End of Extent Detected") \
IPI_ERR_BIT(3, 0, IE_MSG | IE_DUMP,		"Unexpected Master Action") \
 \
IPI_ERR_BIT(4, 7, IE_READ_LOG | IE_MSG,		"Error Log Full") \
IPI_ERR_BIT(4, 6, IE_CMD | IE_MSG,		"Defect Directory Full") \
IPI_ERR_BIT(4, 5, IE_CMD | IE_MSG,		"Logical Link Failure") \
IPI_ERR_BIT(4, 4, IE_CMD | IE_MSG,		"Position Lost") \
 \
 \
IPI_ERR_PARM(IPI_S_CMD_EXC, IPI_F_CMD_EXC, IE_CMD|IE_PRINT|IE_MSG|IE_DUMP, \
			"Command Exception") \
IPI_ERR_BIT(1, 7, 0,	"Invalid Packet Length") \
IPI_ERR_BIT(1, 6, 0,	"Invalid Command Reference Number") \
IPI_ERR_BIT(1, 5, 0,	"Invalid Slave Address") \
IPI_ERR_BIT(1, 4, 0,	"Invalid Facility Address") \
IPI_ERR_BIT(1, 3, 0,	"Invalid Selection Address") \
IPI_ERR_BIT(1, 1, 0,	"Invalid Opcode") \
IPI_ERR_BIT(1, 0, 0,	"Invalid Modifier") \
IPI_ERR_BIT(2, 5, 0,	"Invalid Extent") \
IPI_ERR_BIT(2, 4, 0,	"Out of Context") \
IPI_ERR_BIT(2, 3, 0,	"Invalid Parameter") \
IPI_ERR_BIT(2, 2, 0,	"Missing Parameter") \
IPI_ERR_BIT(2, 1, 0,	"Reserved Value nonzero") \
IPI_ERR_BIT(2, 0, 0,	"Invalid Combination") \
IPI_ERR_BIT(3, 7, 0,	"Not at Initial Position") \
 \
IPI_ERR_PARM(IPI_S_ABORT, IPI_F_ABORT, IE_CMD | IE_MSG, "Command Aborted") \
IPI_ERR_BIT(1, 7, 0,	"Command Aborted") \
IPI_ERR_BIT(1, 6, 0,	"Command Sequence Terminated") \
IPI_ERR_BIT(1, 5, 0,	"Unexecuted Command from Terminated Sequence") \
IPI_ERR_BIT(1, 4, 0,	"Command Chain Terminated") \
IPI_ERR_BIT(1, 3, 0,	"Unexecuted Command from Terminated Chain") \
IPI_ERR_BIT(1, 2, 0,	"Command Order Terminated") \
IPI_ERR_BIT(1, 1, 0,	"Unexecuted Command from Terminated Order") \
 \
IPI_ERR_PARM(IPI_S_COND_SUCC, IPI_F_COND_SUCC, IE_DBG, "Conditional Success") \
IPI_ERR_BIT(1, 7, IE_PRINT,	"Logging Data Appended") \
IPI_ERR_BIT(1, 6, IE_MSG,	"Abort Received - no Command Active") \
IPI_ERR_BIT(1, 5, IE_MSG,	"Abort Received - Status Pending") \
IPI_ERR_BIT(1, 4, IE_MSG,	"Abort Received - Not Operational") \
IPI_ERR_BIT(1, 3, 0,		"Anticipated Error") \
IPI_ERR_BIT(1, 2, 0,		"Anticipated Data Error") \
IPI_ERR_BIT(1, 1, 0,		"Re-allocation Required") \
IPI_ERR_BIT(1, 0, 0,		"Re-allocation Discontinuity") \
IPI_ERR_BIT(2, 7, 0,		"Defect Directory Threshold Exceeded") \
IPI_ERR_BIT(2, 6, 0,		"Error Retry Performed") \
IPI_ERR_BIT(2, 5, IE_CORR,	"Data Retry Performed") \
IPI_ERR_BIT(2, 4, 0,		"Motion Retry Performed") \
IPI_ERR_BIT(2, 3, IE_CORR,	"Data Correction Performed") \
IPI_ERR_BIT(2, 2, IE_CORR,	"Soft Error") \
IPI_ERR_BIT(2, 1, 0,		"Release of Unreserved Addressee") \
IPI_ERR_BIT(2, 0, 0,		"Request Diagnostic Control Command") \
IPI_ERR_BIT(3, 7, 0,		"Error Log Request") \
IPI_ERR_BIT(3, 6, 0,		"Non-Interchange Volume") \
IPI_ERR_BIT(3, 5, 0,		"Retension Required") \
IPI_ERR_BIT(3, 4, 0,		"End of Media Warning") \
IPI_ERR_BIT(3, 3, 0,		"Statistics Update Requested") \
IPI_ERR_BIT(3, 2, 0,		"Parameter Update Requested") \
IPI_ERR_BIT(3, 1, 0,		"Asynchronous Event Occurrence") \
IPI_ERR_BIT(3, 0, 0,		"Master Terminated Transfer") \
 \
IPI_ERR_PARM(IPI_S_INCOMP, IPI_F_INCOMP,  IE_MSG, "Incomplete") \
IPI_ERR_BIT(1, 7, 0,		"Command May be Resumed") \
IPI_ERR_BIT(1, 5, 0,		"COPY Source Space Empty") \
IPI_ERR_BIT(1, 4, 0,		"Response Packet Truncated") \
IPI_ERR_BIT(1, 3, 0,		"Select Subservient Slave") \
IPI_ERR_BIT(2, 7, 0,		"Connect Unsuccessful") \
IPI_ERR_BIT(2, 6, 0,		"Disconnect Unsuccessful") \
IPI_ERR_BIT(2, 5, 0,		"Connect ID Already Assigned") \
IPI_ERR_BIT(2, 4, 0,		"Link Not Connected") \
IPI_ERR_BIT(3, 7, 0,		"Beginning of Media Detected") \
IPI_ERR_BIT(3, 6, 0,		"End of Media Warning") \
IPI_ERR_BIT(3, 5, 0,		"End of Extent Detected") \
IPI_ERR_BIT(3, 4, 0,		"Block Length Difference") \
IPI_ERR_BIT(3, 3, 0,		"Unrecorded Media") \
IPI_ERR_BIT(3, 2, 0,		"Data Length Difference") \
IPI_ERR_BIT(3, 1, 0,		"Block not found") \
				/* End of macro */

/*
 * Define Condition parameter returned by Report Addressee Status.
 */
#define IPI_COND_TABLE_INIT() \
	IPI_ERR_PARM(0x51, 0x51, 0,		"Condition Parm") \
	IPI_ERR_BIT(1, 7, IE_STAT_ON  | IE_ONLINE | IE_PAV, "Operational") \
	IPI_ERR_BIT(1, 6, IE_STAT_OFF | IE_ONLINE,	"Not Operational") \
	IPI_ERR_BIT(1, 5, IE_STAT_ON  | IE_READY | IE_PAV, "Ready") \
	IPI_ERR_BIT(1, 4, IE_STAT_OFF | IE_READY,	"Not Ready") \
	IPI_ERR_BIT(1, 3, IE_MSG,	"switched to another port") \
	IPI_ERR_BIT(1, 2, 0,				"Port Neutral") \
	IPI_ERR_BIT(1, 1, IE_STAT_ON | IE_PAV,		"L-Available") \
	IPI_ERR_BIT(1, 0, IE_STAT_ON | IE_BUSY,		"P-Busy") \
	IPI_ERR_BIT(2, 7, 0,				"Status Pending") \
	IPI_ERR_BIT(2, 6, 0,				"Active") \
	IPI_ERR_BIT(2, 5, 0,				"Inactive") \
	IPI_ERR_BIT(2, 4, IE_STAT_ON  | IE_PAV,		"P-Available") \
	IPI_ERR_BIT(2, 3, IE_STAT_OFF | IE_PAV,		"Not P-Available")


#endif /* ! _ipi_error_h */
