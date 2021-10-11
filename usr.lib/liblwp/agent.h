/* @(#) agent.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
#ifndef _lwp_agent_h
#define _lwp_agent_h

/* info about each agent */
typedef struct agent_t {
	struct agent_t *agt_next;	/* agents queued on signal (MUST BE 1st) */
	object_type_t agt_type;		/* type(lwp/agt) -- MUST BE 2nd */
	bool_t agt_replied;		/* TRUE iff prev. agt. replied to */
	int agt_signum;			/* which signal is to be caught */
	struct lwp_t *agt_dest;		/* who gets the message (agent creator) */
	caddr_t agt_memory;		/* client space for agent message */
	int agt_lock;			/* lock to check stale references */
	event_t *agt_event;		/* info particular to the event */
	qheader_t agt_msgs;		/* FIFO of unreceived msgs */
} agent_t;
#define	AGENTNULL ((agent_t *) 0)

extern qheader_t __Agents[];	/* per-signal list of agents (agent_t's) */
extern int __reply_agent(/*agt*/);
extern void __agentcleanup(/*corpse*/);
extern void __init_agent();

#endif /*!_lwp_agent_h*/
