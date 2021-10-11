/*       @(#)devaddr.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * This file contains device addresses for the architecture.
 */

#ifndef _sun3x_devaddr_h
#define _sun3x_devaddr_h

/*
 * Physical addresses of nonconfigurable devices.
 */
#define OBIO_P4INTERREG		0x50300000
#define	OBIO_FPA_ADDR		0x5C000000
#define	OBIO_IOMAP_ADDR		0x60000000
#define	OBIO_ENABLEREG		0x61000000
#define	OBIO_BUSERRREG		0x61000400
#define	OBIO_DIAGREG		0x61000800
#define	OBIO_IDPROM_ADDR	0x61000C00
#define	OBIO_MEMREG		0x61001000
#define	OBIO_INTERREG		0x61001400
#define	OBIO_EEPROM_ADDR	0x64000000
#define	OBIO_CLKADDR		0x64002000
#define OBIO_INTEL_ETHER	0x65000000
#define	OBIO_PCACHE_TAGS	0x68000000
#define	OBIO_ECCPARREG		0x6A1E0000
#define OBIO_IOC_TAGS		0x6C000000
#define	OBIO_IOC_FLUSH		0x6D000000

/*
 * Fixed virtual addresses.
 */
#define	V_FPA_ADDR		0xE0000000
#define	V_IOMAP_ADDR		0xFEDF2000
#define	V_L1PT_ADDR		0xFEDF4000
#define	V_IOC_FLUSH		0xFEDF6000
#define	V_ENABLEREG		0xFEDF8000
#define	V_BUSERRREG		0xFEDF8400
#define	V_DIAGREG		0xFEDF8800
#define	V_IDPROM_ADDR		0xFEDF8C00
#define	V_MEMREG		0xFEDF9000
#define	V_INTERREG		0xFEDF9400
#define	V_EEPROM_ADDR		0xFEDFA000
#define	V_CLKADDR		0xFEDFC000
#define	V_CLK1ADDR		0xFEDFA7F8
#define	V_ECCPARREG		0xFEDFE000
#define V_P4INTERREG		0xFEF1E000

#endif /*!_sun3x_devaddr_h*/
