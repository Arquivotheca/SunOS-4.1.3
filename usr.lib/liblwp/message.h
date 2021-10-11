/* @(#) message.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#ifndef _lwp_message_h
#define _lwp_message_h

/* message header (fixed-length) */
typedef struct msg_t {
	struct msg_t *msg_next;	/* link to next message */
	thread_t msg_sender;	/* id of sender */
	int msg_argsize;	/* size of argument buffer in bytes */
	int msg_ressize;	/* size of result buffer in bytes */
	caddr_t msg_argbuf;	/* parameters from sender to receiver */
	caddr_t msg_resbuf;	/* results from receiver to sender */
} msg_t;
#define	MESSAGENULL	((msg_t *)0)
#define	lwp_arg		lwp_tag.msg_argbuf
#define	lwp_res		lwp_tag.msg_resbuf
#define	lwp_ressize	lwp_tag.msg_ressize
#define	lwp_argsize	lwp_tag.msg_argsize

extern qheader_t __SendQ;		/* lwp's blocked on a send */
extern qheader_t __ReceiveQ;		/* lwp's blocked on a receive */
extern int __do_send(/* sender, dest, tag */);
extern void __lwp_rwakeup(/* pid */);
extern void __init_msg();
extern void __msgcleanup(/* corpse */);

#endif /*!_lwp_message_h*/
