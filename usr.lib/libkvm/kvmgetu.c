#ifndef lint
static	char sccsid[] = "@(#)kvmgetu.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>

struct user *
kvm_getu(kd, proc)
	kvm_t *kd;
	struct proc *proc;
{

	if (proc->p_stat == SZOMB)	/* zombies don't have u-areas */
		return (NULL);

	/* first time through, allocate holding buffer */
	if ((kd->uarea == NULL) &&
	    ((kd->uarea = (char *)malloc(sizeof(struct user))) == NULL)) {
#ifdef _KVM_DEBUG
		_kvm_perror("no memory for u-area buffer");
#endif _KVM_DEBUG
		return (NULL);
	}

	/*
	 * Just read from the virtual address corresponding to the process's
	 * u-area. On a running system references to these address will
	 * automatically fault the u-area pages back in as needed. On a 
	 * crashed system, the code eventually calls the _kvmphysaddr
	 * routine which finds the in core address (if any) to read for
	 * the u-area page (only it knows the implementation of the various 
	 * segment types).
	 */

	if (kvm_read(kd, (u_long)proc->p_uarea, kd->uarea, sizeof(struct user))
		!= sizeof(struct user)) return(NULL);

/* this is like the getkvm() macro, but returns NULL on error instead of -1 */
#ifdef _KVM_DEBUG
#define getkvmnull(a,b,m)						\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b)) 	\
						!= sizeof (*b)) {	\
		_kvm_error("error reading %s", m);			\
		return (NULL);						\
	}
#else !_KVM_DEBUG
#define getkvmnull(a,b,m)						\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b)) 	\
						!= sizeof (*b)) {	\
		return (NULL);						\
	}
#endif _KVM_DEBUG

	/*
	 * As a side-effect for adb -k, initialize the user address space
	 * description (if there is one; proc 0 and proc 2 don't have
	 * address spaces).
	 */
	if (proc->p_as) {
		getkvmnull(proc->p_as, &kd->Uas,
			"user address space descriptor");
	}

	return ((struct user *)kd->uarea);
}

/*
 * XXX - do we also need a kvm_getkernelstack now that it is not part of the
 * u-area proper any more?
 */

