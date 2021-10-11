/*       @(#)devaddr.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * This file contains device addresses for the architecture.
 */

#ifndef _sun4m_devaddr_h
#define _sun4m_devaddr_h

/*
 * Fixed virtual addresses.
 * Allocated from the top of the
 * available virtual address space
 * and working down. Note that MP
 * systems use 0xFD[0123]..... and
 * 0xFC0...... for per-cpu mappings;
 * it is convenient to place some
 * devices that are per-cpu into this
 * mapping scheme.
 * We may be able to lower COUNTER_PGS
 * and INT_REGS_PGS to zero, if we decide
 * to use the per-cpu mappings to the
 * devices.
 */

#define PERCPU_SHIFT      	20
#define PERCPUSIZE        	0x00100000

/*
 * NOTE: NCPU is actually the number of
 * sets of counter/timer and interrupt
 * registers supported by the Sun-4M
 * archetecture specification. A better
 * name might be nice, since we may want
 * to support fewer "CPUs" while still
 * doing the right thing with the mappings.
 */
/* #define NCPU			4 */ /* moved to sun4m/param.h */

#define V_WKBASE_ADDR		0xFEFE0000

#define V_IOMMU_ADDR		(V_WKBASE_ADDR+0x0000)
#define V_IOMMU_PGS		1

/* page 0x2-0x9 spare */

#define V_SBUSCTL_ADDR		(V_WKBASE_ADDR+0xA000)
#define V_SBUSCTL_PGS		1

/* page 0xC-0xE spare */

#define V_MEMERR_ADDR		(V_WKBASE_ADDR+0xF000)
#define V_MEMERR_PGS		1

#define V_AUXIO_ADDR		(V_WKBASE_ADDR+0x10000)
#define V_AUXIO_PGS		1

#define V_VMECTL_ADDR           (V_WKBASE_ADDR+0x11000)
#define V_VMECTL_PGS		2

#define V_INTERRUPT_ADDR	(V_WKBASE_ADDR+0x14000)
#define V_INTERRUPT_PGS		5

#define V_COUNTER_ADDR		(V_WKBASE_ADDR+0x19000)
#define V_COUNTER_PGS		5

#define V_EEPROM_ADDR		(V_WKBASE_ADDR+0x1E000)
#define V_EEPROM_PGS		2


/* compatibility */
#define	EEPROM_ADDR		0xFEFFE000	/* need EEPROM_PGS pages */
#define	V_CLK1ADDR		0xFEFFFFF8
#define	COUNTER_ADDR		0xFEFF9000	/* need COUNTER_PGS pages */
#define	V_INT_REGS		0xFEFF4000	/* need INT_REGS_PGS pages */
#define	V_SYSCTL_REG		0xFEFF3000
#define	V_VMEBUS_VEC		0xFEFF2000	/* vme interface vec reg */
#define	V_VMEBUS_ICR		0xFEFF1000	/* vme interface ctl reg */
#define	AUXIO_ADDR		0xFEFF0000
#define	MEMERR_ADDR		0xFEFEF000

/*
 * See "genpercpu.c" and various "percpu.h" files
 * for mapping layouts.
 */
#define VA_PERCPU         	0xFE000000 /* base of per-cpu mapping area */
#define VA_PERCPUME       	0xFD000000 /* base of cpu-me mapping area */

#define	EEPROM_PGS		0x2		/* Pages needed for EEPROM */
#define	INT_REGS_PGS		0x5		/* Pages needed for inter. reg */
#define	COUNTER_PGS		0x5		/* Pages needed for timers reg */
#define	SBUS_PGS		0x3		/* Pages needed for sbus control regs */

/* Physical addresses to be used with bypass mode (ASI_MMUPASS or ASI_CTL) */
#define	DIAGREG			0xF1600000

/* VMEbus Interface control register mask defines */
#define	VMEICP_IOC	0x80000000	/* IOCache enable */

#ifndef LOCORE
#define VMEBUS_ICR	(u_int *) V_VMEBUS_ICR
#endif !LOCORE

#endif /*!_sun4m_devaddr_h*/

