/*	@(#)ucred.h	1.1 92/07/30 SMI	*/

#ifndef _sys_ucred_h
#define _sys_ucred_h

#include <sys/label.h>
#include <sys/audit.h>

/*
 * User credential structure
 */
struct ucred {
 	u_short		cr_ref;			/* reference count */
 	u_short  	cr_uid;			/* effective user id */
 	u_short 	cr_gid;			/* effective group id */
 	u_short		cr_auid;		/* user id, for auditing */
 	u_short  	cr_ruid;		/* real user id */
 	u_short		cr_rgid;		/* real group id */
 	int    		cr_groups[NGROUPS];	/* groups, 0 terminated */
 	blabel_t	cr_label;		/* Mandatory Access Control */
 	audit_state_t	cr_audit;		/* audit_state */
};

#ifdef KERNEL
#define	crhold(cr)	(cr)->cr_ref++
void crfree();
struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();
struct ucred *crgetcred();
#endif KERNEL

#endif /*!_sys_ucred_h*/
