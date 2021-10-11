/*	@(#)lssmbmsg.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lssmbmsg.h	1.2"

/*
 * lssmbmsg.h:	The listener checks that a message is an SMB
 *		(MS-NET) message by checking that the first byte
 *		of the message contains 0xff and the next three
 *		contain "SMB".
 */

#define	SMBIDSTR	"SMB"
#define	SMBIDSZ		3

/*
 * SMB command codes 
 */

#define FSPnil		-1		/* no command currently in progress */
#define FSPillegal	-2		/* illegal command being handled */
#define FSPnop		0xff		/* no-op for testing */
#define FSPmkdir	0x00		/* create directory */
#define FSPrmdir	0x01		/* delete directory */
#define FSPopen		0x02		/* open file */
#define FSPcreate	0x03		/* create file */
#define FSPclose	0x04		/* close file */
#define FSPflush	0x05		/* flush file */
#define FSPdelete	0x06		/* delete file */
#define FSPmv		0x07		/* rename file */
#define FSPgetatr	0x08		/* get file attributes */
#define FSPsetatr	0x09		/* set file attributes */
#define FSPread		0x0a		/* read from file */
#define FSPwrite	0x0b		/* write to file */
#define FSPlock		0x0c		/* lock byte range */
#define FSPunlock	0x0d		/* unlock byte range */
#define FSPmktmp	0x0e		/* make temporary file */
#define FSPmknew	0x0f		/* make new file */
#define FSPchkpth	0x10		/* check directory path */
#define FSPexit		0x11		/* process exit */
#define FSPlseek	0x12		/* seek in file; return position */
#define FSPtcon		0x70		/* tree connect */
#define FSPtdis		0x71		/* tree disconnect */
#define FSPnegprot	0x72		/* negotiate protocol */
#define FSPdskattr	0x80		/* get disk attributes */
#define FSPsearch	0x81		/* search directory */
#define FSPsplopen	0xc0		/* open print spool file */
#define FSPsplclose	0xc2		/* close print spool file */
#define FSPsplwr	0xc1		/* write to print spool file */
#define FSPsplretq	0xc3		/* return print queue */

/*
 * Indexes into SMB message
 */

#define FSP_IDF		0
#define FSP_COM		4
#define FSP_RCLS	5
#define FSP_REH		6
#define FSP_ERR		7
#define FSP_REB		9
#define FSP_RES		10
#define FSP_PID		26
#define FSP_UID		28
#define FSP_TID		24
#define FSP_MID		30
#define FSP_WCNT	32
#define FSP_PARMS	33
#define FSP_BCNT	FSP_PARMS + 4
#define FSP_DATA	FSP_BCNT + 4

