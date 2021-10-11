/*	@(#)vdmodsw.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifdef VDDRV
#include <sun/vddrv.h>

extern struct vd_moduleops vd_driverops;
extern struct vd_moduleops vd_netops;
extern struct vd_moduleops vd_syscallops;
extern struct vd_moduleops vd_nullops;

/*
 * Define module ops for each module type
 */
struct vd_modsw vd_modsw[] = {
{	VDMAGIC_DRV,	&vd_driverops	},
{	VDMAGIC_NET,	&vd_netops	},
{	VDMAGIC_SYS,	&vd_syscallops	},
{	VDMAGIC_USER,	&vd_nullops	},
{	VDMAGIC_PSEUDO,	&vd_driverops	},
#ifdef sun4m
{	VDMAGIC_MBDRV,	&vd_driverops	},
{	VDMAGIC_MBNET,	&vd_netops	},
#endif /* sun4m */
{	0,		0		}};

#endif /* VDDRV */
