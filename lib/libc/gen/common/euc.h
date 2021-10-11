/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	This module is created for NLS on Jan.07.87	*/

/* static  char *sccsid = "@(#)euc.h 1.1 92/07/30 SMI"; */

#define	SS2	0x008e
#define	SS3	0x008f

typedef struct {
	short int _eucw1, _eucw2, _eucw3;	/*	EUC width	*/
} eucwidth_t;

#define csetno(c) (((c)&0x80)?((c)==SS2)?2:(((c)==SS3)?3:1):0)
	/* Returns code set number for the first byte of an EUC char. */
