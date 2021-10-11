/*      @(#)eeprom.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sun3_eeprom_h
#define _sun3_eeprom_h

/*
 * The EEPROM consists of one 2816 type EEPROM providing 2K bytes
 * of electically erasable storage.  To modify the EEPROM, each
 * byte must be written separately.  After writing each byte
 * a 10 millisecond pause must be observed before the EEPROM can
 * read or written again.  The majority of the EEPROM is diagnostic
 * and is defined in ../mon/eeprom.h.  The software specific
 * information (struct ee_soft) is defined here.  This structure
 * is 0x100 bytes big.
 */
struct ee_soft {
	u_short	ees_wrcnt[3];		/* write count (3 copies) */
	u_short	ees_nu1;		/* not used */
	u_char	ees_chksum[3];		/* software area checksum (3 copies) */
	u_char	ees_nu2;		/* not used */
	u_char	ees_resv[0x100-0xc];	/* XXX - figure this out sometime */
};

#define EE_SOFT_DEFINED		/* tells ../mon/eeprom.h to use this ee_soft */

#include <mon/eeprom.h>

#define	EEPROM_SIZE		0x800	/* size of eeprom in bytes */
#define	OBIO_EEPROM_ADDR	0x40000	/* address of eeprom in obio space */

#define	EEPROM_ADDR	0x0FFE0000	/* virtual address we map eeprom to */

#define	EEPROM		((struct eeprom *)EEPROM_ADDR)

#endif /*!_sun3_eeprom_h*/
