/*	@(#)unctrl.h 1.1 92/07/30 SMI; from S5R3.1 1.3	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * unctrl.h
 *
 */

#ifndef UNCTRL_H
#define	UNCTRL_H

extern char	*_unctrl[];

#if	!defined(NOMACROS) && !defined(lint)

#define	unctrl(ch)	(_unctrl[(unsigned) ch])

#endif	/* NOMACROS && lint */

#endif	/* UNCTRL_H */
