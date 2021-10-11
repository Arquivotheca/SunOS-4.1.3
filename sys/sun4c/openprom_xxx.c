#ident "@(#)openprom_xxx.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 * Routines for OBP address decoding support.
 *
 * Open Boot Proms report each device as a path of the form
 *	/<dev-spec>/<dev-spec>/<dev-spec>...
 * Each device specification <dev-spec> has the form
 *	<name>@<address-spec>:<options>
 * and <address-spec> takes the form
 *	<addr>,<addr>...
 *
 * These routines interpret and translate the <address-spec> portion of
 * the device path.
 *
 * NOTE: The functional interface in this file WILL change in a future
 * release! Specifically, these decoding routines are driver-specific
 * and belong in the driver ops vector (struct dev_ops), not collected
 * here.  These routines and their interface will move into the dev_ops
 * structure in a future major release, or an equivalent interface will
 * be defined in a future version of the driver interface
 * specification.
 *
 * Nevertheless, drivers which perform address space translations (e.g.,
 * esp) or which need to translate the prom's address representation to
 * something more accessible to the kernel (e.g., sbus) must register
 * their translation routines here in the dev_path_opstab.
 */

#include <sys/param.h>
#define	OPENPROMS
#include <sun/openprom.h>
#include <sun/autoconf.h>

#include <scsi/scsi.h>
#ifndef GENERIC
#include "scsibus.h"
#else
#define	NSCSIBUS	1
#endif

static int path_debug = 0;
#define	dprintf		if (path_debug) printf

int obio_decode_regprop(/* struct dev_info *dip, int nodeid */);
char *obio_encode_reg(/* struct dev_info *dip, char *addrspec */);
int obio_match(/* struct dev_info *dip, char *addrspec */);

static int sbus_decode_regprop(/* struct dev_info *dip, int nodeid */);
static char *sbus_encode_reg(/* struct dev_info *dip, char *addrspec */);
static int sbus_match(/* struct dev_info *dip, char *addrspec */);

static char *esp_encode_reg(/* struct dev_info *dip, char *addrspec */);
static int esp_match(/* struct dev_info *dip, char *addrspec */);

struct dev_path_ops dev_path_opstab[] = {
    {	"obio",	obio_decode_regprop,	obio_encode_reg,	obio_match    },
    {	"sbus",	sbus_decode_regprop,	sbus_encode_reg,	sbus_match    },
    {	"lebuffer",	sbus_decode_regprop, sbus_encode_reg,	sbus_match    },
    {	"dma",	sbus_decode_regprop,	sbus_encode_reg,	sbus_match    },
#if NSCSIBUS > 0
    {	"esp",	0,			esp_encode_reg,		esp_match     },
#endif
    {	0    }
};

/*
 * Decode this node's "reg" property into a struct reg that the kernel
 * can digest. Return the number of reg structs found.
 */
obio_decode_regprop(dip, nodeid)
	struct dev_info *dip;
	int nodeid;
{
	dip->devi_nreg = getproplen(nodeid, "reg")/sizeof (struct dev_reg);
	if (dip->devi_nreg > 0)
		dip->devi_reg =
		    (struct dev_reg *)getlongprop(nodeid, "reg");
	return (dip->devi_nreg);
}

/*
 * Encode a "bustype, addr" structure into a string of the form "%x,%x".
 * The addrspec argument must point to a "big enough" area.
 * Return a pointer to the end of the string.
 */
char *
obio_encode_reg(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	struct dev_reg *regp;

	if (addrspec) {
		*addrspec = '\0';
		if (dip && (regp = dip->devi_reg)) {
			(void)sprintf(addrspec, "%x,%x", regp->reg_bustype,
			    regp->reg_addr);
			while (*addrspec)
				++addrspec;
		}
	}
	return (addrspec);
}

/*
 * Return true if the device in the dev_info node is at the address specified.
 */
obio_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register u_int bustype, offset;

	/*
	 * Assume unit string is "%x,%x", mapping to "bustype, offset".
	 */
	bustype = atou(addrspec);
	while (*addrspec++ != ',')
		;
	offset = atou(addrspec);

	if (dip->devi_reg)
		dprintf("\tbustype %d addr %x; reg %d %x\n", bustype, offset,
		    dip->devi_reg->reg_bustype, dip->devi_reg->reg_addr);
	else
		dprintf("\tbustype %d addr %x; reg ''\n", bustype, offset);

	return (bustype == dip->devi_reg->reg_bustype &&
	    offset == (u_int)dip->devi_reg->reg_addr);
}

/*
 * Decode this node's "reg" property into a struct reg that the kernel
 * can digest. Children of the "sbus" node have reg properties of the
 * form <slot,offset>, where slot = the slot number (0...n) and offset
 * is the offset of the register set within the slot.
 * Return the number of reg structs found.
 */
static int
sbus_decode_regprop(dip, nodeid)
	struct dev_info *dip;
	int nodeid;
{
	register struct dev_reg *regp;
	register u_int slot, offset;
	register int i;

	dip->devi_nreg = getproplen(nodeid, "reg")/sizeof (struct dev_reg);
	if (dip->devi_nreg > 0)
		dip->devi_reg =
		    (struct dev_reg *)getlongprop(nodeid, "reg");
	for (i = 0, regp = dip->devi_reg; i < dip->devi_nreg; ++i, ++regp) {
		slot = regp->reg_bustype;
		offset = (u_int)regp->reg_addr;
		regp->reg_bustype = OBIO;
		regp->reg_addr =
		    (addr_t)(SBUS_BASE + (slot * SBUS_SIZE) + offset);
	}
	return (dip->devi_nreg);
}

/*
 * Encode a "bustype, addr" structure into a string of the form
 * <slot, offset> "%x,%x".
 * The addrspec argument must point to a "big enough" area.
 */
static char *
sbus_encode_reg(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	struct dev_reg *regp;
	register u_int slot, offset;

	if (addrspec == (char *)0)
		return (addrspec);
	*addrspec = '\0';
	if (dip == (struct dev_info *)0)
		return (addrspec);
	if ((regp = dip->devi_reg) == (struct dev_reg *)0)
		return (addrspec);
	if (regp->reg_bustype != OBIO)
		return (addrspec);

	slot = ((u_int)regp->reg_addr - SBUS_BASE)/SBUS_SIZE;
	offset = ((u_int)regp->reg_addr % SBUS_SIZE) & (SBUS_SIZE-1);
	(void)sprintf(addrspec, "%x,%x", slot, offset);
	while (*addrspec)
		++addrspec;
	return (addrspec);
}


/*
 * Return true if the device in the dev_info node is at the address specified.
 */
static int
sbus_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register u_int slot, offset, addr;

	/*
	 * Assume unit string is "%x,%x", mapping to "slot, offset".
	 */
	slot = atou(addrspec);
	while (*addrspec++ != ',')
		;
	offset = atou(addrspec);

	/*
	 * Calculate OBIO offset.
	 */
	addr = SBUS_BASE + (slot * SBUS_SIZE) + offset;

	if (dip->devi_reg) {
		dprintf("\tslot %d offset %x (addr %x); reg %d %x\n",
		    slot, offset, addr, dip->devi_reg->reg_bustype,
		    dip->devi_reg->reg_addr);
	} else {
		dprintf("\tslot %d offset %x (addr %x); reg ''\n",
		    slot, offset, addr);
	}

	return (dip->devi_reg->reg_bustype == OBIO &&
	    addr == (u_int)dip->devi_reg->reg_addr);
}

#if NSCSIBUS > 0

/*
 * Encode <target, lun> into a string of the form "%x,%x".
 * The addrspec argument must point to a "big enough" area.
 */
static char *
esp_encode_reg(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register struct scsi_device *sdev;
	register struct scsi_address *sdeva;

	if (addrspec == (char *)0)
		return (addrspec);
	*addrspec = '\0';
	if (dip == (struct dev_info *)0)
		return (addrspec);

	for (sdev = sd_root; sdev; sdev = sdev->sd_next) {
		if (sdev->sd_dev == dip)
			break;
	}
	if (sdev == (struct scsi_device *)0)
		return (addrspec);

	sdeva = &sdev->sd_address;
	(void)sprintf(addrspec, "%d,%d", sdeva->a_target, sdeva->a_lun);
	while (*addrspec)
		++addrspec;
	return (addrspec);
}

static int
esp_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register int target, lun;
	register struct scsi_device *sdev;
	register struct dev_info *sdevi;
	register struct scsi_address *sdeva;

	/*
	 * Assume unit string is "%x,%x", mapping to "target, lun".
	 */
	target = atou(addrspec);
	while (*addrspec++ != ',')
		;
	lun = atou(addrspec);
	dprintf("Matching target %d lun %d\n", target, lun);

	/*
	 * Scan list of SCSI devices and look for a target/lun match.
	 */
	dprintf("name\tunit\tha\t\ttarget\tlun\tpresent\n");
	for (sdev = sd_root; sdev; sdev = sdev->sd_next) {
		sdevi = sdev->sd_dev;
		sdeva = &sdev->sd_address;

		dprintf("%s\t%d\t%x\t%d\t%d\t%d  ",
		    sdevi->devi_name, sdevi->devi_unit, sdeva->a_cookie,
		    sdeva->a_target, sdeva->a_lun, sdev->sd_present);

		if (sdeva->a_target == target && sdeva->a_lun == lun) {
			if (dip == sdevi) {
				dprintf("HIT!!!\n");
				return (1);
			} else {
				dprintf("target/lun match only\n");
			}
		} else {
			dprintf("no match\n");
		}
	}
	return (0);
}

#endif NSCSIBUS

/*
 * Supposedly, this routine is the only place that enumerates details
 * of the encoding of the "bustype" part of the physical address.
 * This routine depends on details of the physical address map, and
 * will probably be somewhat different for different machines.
 */
decode_address(space, addr)
int space, addr;
{
	char *name;

	/* Here we encode the knowledge that the SBUS is part of OBIO space */
	if (space == OBIO && addr >= SBUS_BASE) {
		printf(" SBus slot %d 0x%x", (addr - SBUS_BASE)/SBUS_SIZE,
			(addr % SBUS_SIZE) & (SBUS_SIZE-1));
		return;
	}

	/* Here we enumerate the set of physical address types */
	switch (space) {
	case	SPO_VIRTUAL:	name = "virtual";	break;
	case	OBMEM:		name = "obmem";		break;
	case	OBIO:		name = "obio";		break;
	default:		name = "unknown";	break;
	}

	printf(" %s 0x%x", name, addr);
}
