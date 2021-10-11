
/*
 * Lego register definitions, common to all environments.
 * Brooktree Color Table
 */

#ifndef lint
static char	sccsid_lego_dac_reg_h[] = { "@(#)lego-dac-reg.h 3.7 88/10/27" };
#endif lint

/*
 * DAC register offsets from base address. These offsets are
 * intended to be added to a pointer-to-int whose value is the
 * base address of the Lego memory mapped register area.
 *
 * When writing to the DACs, we write a word (to the word address) with
 * the value to be written replicated in the high and low bytes. The
 * h/w reason(s) for this incantation will settle down in the future.
 * An early h/w version used the low byte but future use is the
 * high byte, for compatability with other h/w.
 */

/* status and command registers */
/* chip input line C0 is address bit 2, C1 is address bit 3 */

/* specify color table palette index or chip register index */
#define L_DAC_INDEX		(0x000 / sizeof(int))

/*
 * access palette color component, use three successive accesses
 * in the order <r, g, b>. The blue access will increment the
 * index and the next color triplet could be transferred
 * without explicitly loading it's address index.
 */
#define L_DAC_PALETTE		(0x004 / sizeof(int))

/*
 * access chip register (see indices below).
 */
#define L_DAC_REGISTER		(0x008 / sizeof(int))

/*
 * access overlay color component, use three successive accesses
 * in the order <r, g, b>. The blue access will increment the
 * index and the next color triplet could be transferred
 * without having to load it's address.
 */
#define L_DAC_OVERLAY		(0x00C / sizeof(int))

/*
 * chip register indices.
 */
#define L_DAC_OVERLAY_C0	0
#define L_DAC_OVERLAY_C1	1
#define L_DAC_OVERLAY_C2	2
#define L_DAC_OVERLAY_C3	3
#define L_DAC_READMASK		4
#define L_DAC_BLINKMASK		5
#define L_DAC_CMD		6
#define L_DAC_TEST		7

/*
 * DAC registers as a structure.
 */
struct l_dac {
	int	l_dac_index;
	int	l_dac_palette;
	int	l_dac_register;
	int	l_dac_overlay;
	};
