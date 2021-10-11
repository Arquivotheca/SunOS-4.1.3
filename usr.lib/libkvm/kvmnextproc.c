#ifndef lint
static	char sccsid[] = "@(#)kvmnextproc.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>

/* number of process structures to cache */
#define NPROC (8192 / sizeof (struct proc))
#define PCACHE (NPROC * sizeof (struct proc))

static int getnth();


struct proc *
kvm_nextproc(kd)
	kvm_t *kd;
{
	register int i;

	while (kd->pnext < kd->nproc) {
		if ((i = getnth(kd, kd->pnext++)) == -1)
			return (NULL);
		if (kd->pbuf[i].p_stat != 0) {
			return (&kd->pbuf[i]);
		}
	}
	return (NULL);
}


struct proc *
kvm_getproc(kd, pid)
	kvm_t *kd;
	int pid;
{
	register int n;
	register int i;

	for (n = 0; n < kd->nproc; ) {
		if ((i = getnth(kd, n++)) == -1)
			return (NULL);
		if ((kd->pbuf[i].p_stat != 0) && (kd->pbuf[i].p_pid == pid)) {
			return (&kd->pbuf[i]);
		}
	}
	return (NULL);
}


/*
 * make sure the n'th process is in the process structure cache
 * and return its position
 */
static int
getnth(kd, n)
	kvm_t *kd;
	int n;
{
	register int poff;

	/* if no proc cache, allocate one */
	if (kd->pbuf == NULL) {
		kd->pbuf = (struct proc *)malloc(PCACHE);
		if (kd->pbuf == NULL) {
#ifdef _KVM_DEBUG
			_kvm_perror("can't allocate proc cache");
#endif _KVM_DEBUG
			return (-1);
		}
	}
	poff = n - kd->pbase;
	if ((kd->pbase < 0) || (poff < 0) || (poff >= NPROC)) {
		kd->pbase = (n / NPROC) * NPROC;
		/*
		 * No attempt is made to limit the read size of the last
		 * segment of proc structures.  As long as the cache
		 * isn't too big, this should be ok.
		 */
		if (kvm_read(kd, kd->proc + (kd->pbase * sizeof (struct proc)),
		    (caddr_t)kd->pbuf, PCACHE) != PCACHE) {
#ifdef _KVM_DEBUG
			_kvm_perror("error reading proc table");
#endif _KVM_DEBUG
			kd->pbase = -1;
			return (-1);
		}
	}
	return (n - kd->pbase);
}
