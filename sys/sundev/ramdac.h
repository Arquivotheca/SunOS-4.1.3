/* @(#)ramdac.h	1.1 92/07/30 */
#ifndef	_ramdac_DEFINED_
#define _ramdac_DEFINED_

#include <pixrect/pixrect.h>

#define RAMDAC_READMASK		04
#define RAMDAC_BLINKMASK	05
#define RAMDAC_COMMAND		06
#define RAMDAC_CTRLTEST		07

/* 3 Brooktree ramdac 457 or 458 packed in a 32-bit register */
/* fbunit defined in <pixrect/pixrect.h> */
struct ramdac {
    union fbunit    addr_reg,	       /* address register */
                    lut_data,	       /* lut data port */
                    command,	       /* command/control port */
                    overlay;	       /* overlay lut port */
};

#define ASSIGN_LUT(lut, value) (lut).packed = (value & 0xff) | \
	((value & 0xff) << 8) | ((value & 0xff) << 16)
/*
 * when "ctrl/test" is selected, the least significant 3 bits control
 * which channel this ramdac is controlling.
 *
 * select 4:1 multiplexing to make enable/overlay possible, also makes
 * the overlay transparent and display the overlay plane only.
 *
 * enable read and write to all planes
 *
 * no blinking
 */

#define INIT_BT458(lut)   {                                      \
	ASSIGN_LUT((lut)->addr_reg, 0);                          \
	ASSIGN_LUT((lut)->addr_reg, RAMDAC_CTRLTEST);            \
	ASSIGN_LUT((lut)->command, 04);                          \
	ASSIGN_LUT((lut)->addr_reg, RAMDAC_COMMAND);             \
	ASSIGN_LUT((lut)->command, 0x43);                        \
	ASSIGN_LUT((lut)->addr_reg, RAMDAC_READMASK);            \
	ASSIGN_LUT((lut)->command, 0xff);                        \
	ASSIGN_LUT((lut)->addr_reg, RAMDAC_BLINKMASK);           \
	ASSIGN_LUT((lut)->command, 0);                           \
    }

#define INIT_OCMAP(omap) {                                       \
	(omap)[0].packed =  0x000000;                            \
	(omap)[1].packed =  0xffffff;                            \
	(omap)[2].packed =  0x00ff00;                            \
	(omap)[3].packed =  0x000000;                            \
    }

#define INIT_CMAP(cmap, size)   {                                \
	register int    idx;                                     \
	for (idx = 0; idx < size; idx++)                         \
	    ASSIGN_LUT((cmap)[idx], idx);                        \
    }


#endif	_ramdac_DEFINED_

