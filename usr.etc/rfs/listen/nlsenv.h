/*	@(#)nlsenv.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsenv.h	1.2"

/*
 * these are names of environment variables that the listener
 * adds to the servers environment before the exec(2).
 *
 * the variables should be accessed via library routines.
 *
 * see nlsgetcall(3X) and nlsprovider(3X).
 */

#define	NLSADDR		"NLSADDR"
#define NLSOPT		"NLSOPT"
#define NLSUDATA	"NLSUDATA"
#define NLSPROVIDER	"NLSPROVIDER"

/*
 * the following variables can be accessed "normally"
 */

#define	HOME		"HOME"
#define PATH		"PATH"


