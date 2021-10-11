/* @(#)idprom.h	1.1 7/30/92 SMI */	

/*
 * Copyright (c) 1986,1990 by Sun Microsystems, Inc.
 */

#ifndef _mon_idprom_h
#define _mon_idprom_h

#ifndef LOCORE
/*
 * Structure declaration for ID prom in CPU and Ethernet boards
 */

struct idprom {
	unsigned char	id_format;	/* format identifier */
	/* The following fields are valid only in format IDFORM_1. */
	unsigned char	id_machine;	/* machine type */
	unsigned char	id_ether[6];	/* ethernet address */
	long		id_date;	/* date of manufacture */
	unsigned	id_serial:24;	/* serial number */
	unsigned char	id_xsum;	/* xor checksum */
	unsigned char	id_undef[16];	/* undefined */
};
#endif LOCORE

#define IDFORM_1	1	/* Format number for first ID proms */

/*
 * The machine type field assignments are constrained such that the
 * IDM_ARCH_MASK bits define the CPU architecture and the remaining bits
 * identify the individual implementation of that architecture.
 */
#define	IDM_ARCH_MASK	0xf0	/* mask for architecture bits */
#define	IDM_ARCH_SUN2	0x00	/* arch value for Sun-2 */
#define	IDM_ARCH_SUN3	0x10	/* arch value for Sun-3 */
#define IDM_ARCH_SUN4   0x20    /* arch value for Sun-4 */
#define IDM_ARCH_SUN3X	0x40    /* arch value for Sun-3x */
#define	IDM_ARCH_SUN4C	0x50	/* arch value for Sun-4c */
#define	IDM_ARCH_SUN4M	0x70	/* arch value for Sun-4m */

/* 
 * All possible values of the id_machine field (so far): 
 */
#define	IDM_SUN2_MULTI		1	/* Machine type for Multibus CPU brd */
#define	IDM_SUN2_VME		2	/* Machine type for VME CPU board    */
#define	IDM_SUN3_CARRERA	0x11	/* Carrera CPU	*/
#define	IDM_SUN3_M25		0x12	/* M25 CPU	*/
#define	IDM_SUN3_SIRIUS		0x13	/* Sirius CPU	*/
#define IDM_SUN3_PRISM		0x14    /* Prism CPU	*/
#define IDM_SUN3_F		0x17    /* Sun3F CPU	*/
#define IDM_SUN3_E		0x18    /* Sun3E CPU	*/
#define IDM_SUN4		0x21    /* Sparc CPU	*/
#define IDM_SUN4_COBRA		0x22    /* Cobra CPU	*/
#define IDM_SUN4_STINGRAY	0x23    /* Stingray CPU	*/
#define IDM_SUN4_SUNRAY		0x24	/* Sunray CPU   */
#define IDM_SUN3X_PEGASUS	0x41    /* Pegasus CPU	*/
#define IDM_SUN3X_HYDRA         0x42    /* Hydra CPU    */
#define IDM_SUN4C               0x51    /* Campus CPU   */
#define IDM_SUN4C_60		0x51	/* Campus-1 CPU */
#define IDM_SUN4C_40		0x52	/* Reserve some names */
#define IDM_SUN4C_65		0x53	/* That we might do */
#define IDM_SUN4C_20		0x54	/* It might be bigger */
#define IDM_SUN4C_75		0x55	/* It might be smaller */
#define IDM_SUN4C_30		0x56	/* It might be faster */
#define IDM_SUN4C_50		0x57	/* It might be slower */
#define IDM_SUN4C_70		0x58	/* It might cost more */
#define IDM_SUN4C_80		0x59	/* It might cost less */
#define IDM_SUN4C_10		0x5a	/* It might sell well */
#define IDM_SUN4C_45		0x5b	/* And then it might not */
#define IDM_SUN4C_05		0x5c	/* It might be pink */
#define IDM_SUN4C_85		0x5d	/* It might be blue */
#define IDM_SUN4C_32		0x5e	/* I certainly don't know */
#define IDM_SUN4C_HIKE		0x5f	/* Do you? */


#define IDM_SUN4M_690		0x71	/* SPARCsystem 600 series */
#define IDM_SUN4M_50		0x72	/* Campus 2 */

#endif /*!_mon_idprom_h*/
