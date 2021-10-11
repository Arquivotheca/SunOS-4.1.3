/*	@(#)session.h 1.1 92/07/30 SMI	*/

#ifndef __sys_session_h
#define __sys_session_h
#include <sys/types.h>	/* for pid_t */

/*
 * per session information (controlling tty)
 *
 * The session leader does the only VN_HOLD() on the ctty.
 * The session leader marks the tty as available when he exits;
 * The tty gets taken from the session on last close or when the
 * session leader exits.
 */
struct	sess {
	pid_t		s_sid;		/* Session id */
	short		s_members;	/* When 0, free the session */
	struct vnode	*s_vp;		/* Controlling tty of this session */
	dev_t		s_ttyd;		/* Compat with u.u_ttyd which is really
					 * v_rdev in the vnode of the ctty */
	short		*s_ttyp;	/* Pointer to sd_pgrp in stream tab */
	int		s_flags;	/* See below */
	struct sess	*s_next;	/* Linked list of all sessions */
};

extern struct sess *allsess;		/* All sessions via s_next */
extern struct sess sess0;		/* Init & friends */

#define	SESS_SYS	0		/* Default session id (no tty) */
					/* Also: goto old sess0, a la BSD */
#define	SESS_NEW	1		/* New session wanted, a la posix */

/*
 * Let go of the vnode.  Called by freesession on last member exit and
 * by strclose() on last close of the session vnode.
 */
#define	SESS_VN_RELE(s)	{	if ((s)->s_vp) { \
					VN_RELE((s)->s_vp); \
					(s)->s_vp = NULL; \
					(s)->s_ttyd = 0; \
					*(s)->s_ttyp = NULL; \
					(s)->s_ttyp = NULL; \
				} \
			}
/*
 * Leave the session,
 * freeing the session if last member.
 *
 * MUST be followed by a SESS_ENTER unless the process is being reaped.
 */
#define	SESS_EXIT(p)	{	if (!--((p)->p_sessp->s_members)) \
					freesess((p)->p_sessp); \
				(p)->p_sessp = NULL; \
			}
/*
 * Add a process to an existing session
 */
#define	SESS_ENTER(p, sp){	(p)->p_sessp = (sp); \
				(sp)->s_members++; \
			}
/*
 * Orphan the specified process and
 * check and maybe orphan all its' children
 */
#define	SESS_ORPH(p)	{	register struct proc *pp; \
				(p)->p_flag |= SORPHAN; \
				for (pp=(p)->p_cptr; pp; pp=pp->p_osptr) \
					(void)orphan_chk(pp->p_pgrp); \
			}

#endif /* __sys_session_h */
