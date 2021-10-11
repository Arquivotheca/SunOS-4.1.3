#ident	"@(#)types.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_impl_types_h
#define	_scsi_impl_types_h

/*
 * Local Types for SCSI subsystems
 */

#include <sys/param.h>
#ifdef	KERNEL
#include <sys/systm.h>
#endif	KERNEL
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/fcntl.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#ifdef	KERNEL
#include <sys/kernel.h>
#endif	KERNEL
#include <sys/dkbad.h>
#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#ifdef KERNEL
#include <machine/cpu.h>
#endif
#include <machine/scb.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#ifdef	sun4c
#include <sundev/mbvar.h>
#else	sun4c
#include <sys/buf.h>
#include <sundev/mbvar.h>
#endif	sun4c
#include <sys/debug.h>


#include <scsi/impl/services.h>
#include <scsi/impl/transport.h>
#include <scsi/impl/pkt_wrapper.h>

#include <scsi/conf/autoconf.h>
#include <scsi/conf/device.h>

#endif	/* !_scsi_impl_types_h */
