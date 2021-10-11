#ifndef lint
static	char sccsid[] = "@(#)kvmnlist.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <nlist.h>

/*
 * Look up a set of symbols in a kernel namelist.
 */
int
kvm_nlist(kd, nl)
	kvm_t *kd;
	struct nlist nl[];
{
	register int e;

	if (kd->namefd == -1) {
#ifdef _KVM_DEBUG
		_kvm_error("kvm_nlist: no namelist descriptor");
#endif _KVM_DEBUG
		return (-1);
	}
	e = _nlist(kd->namefd, nl);

#ifdef _KVM_DEBUG
	if (e == -1)
		_kvm_error("bad namelist file %s", kd->name);
#endif _KVM_DEBUG

	return (e);
}
