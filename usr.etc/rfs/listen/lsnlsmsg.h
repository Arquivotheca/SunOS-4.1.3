/*	@(#)lsnlsmsg.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lsnlsmsg.h	1.4"

/*
 * The Network Listener Process Service (NLPS) 
 * protocol message sent by client machines to
 * a listener process to request a service on the listener's
 * machine. The message is sent to "netnodename(r_nodename)"
 * where r_nodename is the nodename (see uname(2)) of the
 * remote host. Note that client's need not know (or care)
 * about the details of this message.  They use the "att_service(3)"
 * library routine which uses this message.
 *
 * Message is in ASCII and has the following format:
 *
 * iiii:lll:hhh:sssssssssssssss
 *
 * where:
 *	iiii = 4 character ID == "NLPS"
 *	lll = 3 character low version number (ASCII decimal digits)
 *	hhh = 3 character hi  version number (ASCII decimal digits)
 *	sssssssssssssss = 15 character service_code (ASCII letters, digits or '_')
 *	: = field separator char == ':'
 */

#define NLPSIDSZ	4
#define NLPSIDCHAR	{'N','L','P','S'}
#define NLPSIDSTR	"NLPS"
#define NLPSVSZ		3
#define	NLPSSSZ		16		/* 15 chars + NULL */
#define NLPSSEPCHAR	':'		/* field separator character */

