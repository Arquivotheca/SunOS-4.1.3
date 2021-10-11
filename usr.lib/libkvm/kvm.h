/*	@(#)kvm.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* define a 'cookie' to pass around between user code and the library */
typedef struct _kvmd kvm_t;

/* libkvm routine definitions */
extern kvm_t		*kvm_open();
extern int		 kvm_close();
extern int		 kvm_nlist();
extern int		 kvm_read();
extern int		 kvm_write();
extern struct proc	*kvm_getproc();
extern struct proc	*kvm_nextproc();
extern int		 kvm_setproc();
extern struct user	*kvm_getu();
extern int		 kvm_getcmd();
