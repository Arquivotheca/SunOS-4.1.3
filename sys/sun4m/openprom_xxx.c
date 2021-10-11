#ident "@(#)openprom_xxx.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 * Routines and tables for OBP address encoding/matching support.
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

#define	ENCODE_ERROR 	((char *)0)
#define	ENCODE_SKIP_ME	((char *)-1)
#define	ENCODE_FULLNAME	((char *)-2)
#define	ENCODE_OK	((char *)1)

/*
 *
 * A word about the `encode' functions:  Encode functions are used to
 * determine the OBP device name of bootable devices for automatic
 * reboot -- ususally by munix during installation.  The program
 * /usr/etc/unixname2bootname is currently the only user of this stuff.
 * Thus, you need to supply a function, if the supplied, generic, default
 * functions do not provide the proper OBP name generation for installation
 * to work properly on your device.
 *
 * They are called with two arguments:
 *	A struct dev_info pointer and pointer to a receiving character buffer.
 *
 *	They return a char * which has the following significance:
 *
 *	(The return of a char * and the specific return values chosen are
 *	for historical reasons, only and have no significance, except
 *	for compatability with similar functions in other architectures.
 *	that did not provide this flexibility.)
 *
 *		A value of -1 (ENCODE_SKIP_ME) means that the encoding
 *		function signifies that the current node should not be
 *		represented in the output pathname.  This option should
 *		be used with software nexus nodes that are not represented
 *		in the OBP device tree.
 *
 *		A value of zero (ENCODE_ERROR) signifies an error,
 *		and for some reason, the pathname cannot be determined.
 *		The error is returned back up the chain to the user level
 *		program to deal with -- currently, it spits out an old style
 *		PROM device name.
 *
 *		A value of -2 (ENCODE_FULLNAME) signifies that the
 *		encoding function has returned the device name and
 *		the optional address qualifer (for example "foo@x,y",
 *		and the calling code should not prepend the devi->devi_name.
 *		The calling code prepends a slash character and includes
 *		the returned string. In this case, the output buffer should
 *		contain a non-NULL string.
 *
 *		Other values (ENCODE_OK) :  Again there are
 *		two more possibilites:
 *
 *			The receiving buffer contains a non-NULL string.
 *			In this case, the calling utilities insert
 *			/%s@%s where the first %s is the device name
 *			from devi->devi_name and the second %s is
 *			the qualifier returned by the `encode' function.
 *			The qualifer is usually %x,%x which is usually
 *			bustype,offset or target,lun or slave,facility...
 *
 *			The receiving buffer contains a NULL string
 *			which means there is no address qualifier for
 *			this node.  The calling utilities insert
 *			/%s where %s is devi->devi_name.
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

/*
 * To enable debugging output, define DPRINTF and set path_debug to 1
 */

#define	DPRINTF	1
#ifdef	DPRINTF
static int xxx_path_debug = 0;
#define	dprintf		if (xxx_path_debug) printf
#endif	DPRINTF

/*
 * On sun4m, all decoding is handled generically since we have
 * range properties available to us.
 *
 * Also, all encoding is handled generically, except for
 * bootable disk devices such as IPI and SCSI which need
 * special handling for things like target and lun.
 * (the function names obio_* are misleading - they are generic).
 */

char *obio_encode_reg(/* struct dev_info *dip, char *addrspec */);
int obio_match(/* struct dev_info *dip, char *addrspec */);

#if	NSCSIBUS > 0
static char *esp_encode_reg(/* struct dev_info *dip, char *addrspec */);
static int esp_match(/* struct dev_info *dip, char *addrspec */);
#endif	NSCIBUS > 0

#ifdef	VME
#include "id.h"
#if	NID > 0
static char *ipi_encode_reg(/* struct dev_info *dip, char *addrspec */);
static int ipi_match(/* struct dev_info *dip, char *addrspec */);
#endif	NID > 0
#endif	VME

struct dev_path_ops dev_path_opstab[] = {
	{	"obio",	    0,	obio_encode_reg, obio_match  },
	{	"sbus",	    0,	obio_encode_reg, obio_match  },
	{	"lebuffer", 0,	obio_encode_reg, obio_match  },
	{	"dma",	    0,	obio_encode_reg, obio_match  },
	{	"ledma",    0,	obio_encode_reg, obio_match  },
	{	"espdma",   0,	obio_encode_reg, obio_match  },
#if	NSCSIBUS > 0
	{	"esp",	    0,	esp_encode_reg,  esp_match   },
#endif
#ifdef	VME
	{	"vme",	    0,	obio_encode_reg, obio_match  },
#if	NID > 0
#ifndef	NO_IDC_COMPATIBILITY
	{	"idc",	    0,	ipi_encode_reg, ipi_match   },
#endif	!NO_IDC_COMPATIBILITY
	{	"ipi3sc",   0,	ipi_encode_reg, ipi_match   },
#endif	NID > 0
#endif	VME
	{	0    }
};

/*
 * NULL terminated List of device names whose unit numbers are
 * represented as three hex digits, i.e.: IPI idXXXp.
 */

char *hex_unit_devices[] = {
#ifdef	VME
#if	NID > 0
	"id",
#endif	NID > 0
#endif	VME
	(char *)0
};

extern int	cpu;
extern addr_t getlongprop();

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
	register u_int bus_type, addr, len;

#ifdef	DPRINTF
	dprintf("obio_encode_reg(): encoding <%s> ", dip->devi_name);
#endif	DPRINTF

	if (addrspec == (char *)0)
		return (ENCODE_ERROR);
	*addrspec = '\0';
	if (dip == (struct dev_info *)0)
		return (ENCODE_ERROR);

	/*
	 * If I'm the root node, skip me...
	 */
	if (dip->devi_parent == 0)
		return (ENCODE_SKIP_ME);

	/*
	 * If I'm not a real nodeid, error exit...
	 */

	if ((dip->devi_nodeid == 0) || (dip->devi_nodeid == -1))  {
#ifdef	DPRINTF
		dprintf("obio_encode_reg(): Invalid nodeid in <%s>\n",
			dip->devi_name);
#endif	DPRINTF
		return (ENCODE_ERROR);
	}

	/*
	 * Get the original (unmodified) reg property
	 * from the PROM, use the first reg in the property
	 * as the bustype,offset fields, and free the memory
	 * allocated by getlongprop().  If there is no reg
	 * property, just return OK with a NULL buffer,
	 * because this means there is no address qualifier.
	 */

	len = (u_int)getproplen(dip->devi_nodeid, "reg");
	regp = (struct dev_reg *)getlongprop(dip->devi_nodeid, "reg");
	if (regp == 0)  {
#ifdef	DPRINTF
		dprintf("no addr qualifier (reg) for nodeid <%x>\n",
		    dip->devi_nodeid);
#endif	DPRINTF
		return (ENCODE_OK);
	}

	bus_type = regp->reg_bustype;
	addr = (u_int)regp->reg_addr;
	kmem_free((caddr_t)regp, len);

	(void)sprintf(addrspec, "%x,%x", bus_type, addr);
#ifdef	DPRINTF
	dprintf("addr %x, reg_addr %x returning <%s>\n",
	    addr, regp->reg_addr, addrspec);
#endif	DPRINTF
	return (ENCODE_OK);
}

#ifdef	VME
#if	NID > 0

/*
 * Return true if the device in the dev_info node is at the ipi disk addr
 * specified.  XXX: Channel is always zero, for now.
 */

static int
ipi_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register u_int facility, channel;

	/*
	 * Assume unit string is "%x,%x", mapping to "facility, channel".
	 */
	facility = atou(addrspec);
	while (*addrspec != 0)  {
		if (*addrspec++ == ',')
			break;
	}
	channel = atou(addrspec);

#ifdef	DPRINTF
	dprintf("\tipi <facility>,<channel> <%d>,<%d>;", facility, channel);
	dprintf(" found <%d>,<%d>\n", dip->devi_unit, 0);
#endif	DPRINTF

	return (facility == ((u_int)dip->devi_unit & 0xf));
}
#endif	NID > 0
#endif	VME

/*
 * Return true if the device in the dev_info node is at the address specified.
 */
obio_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register u_int bus_type, offset;
	register struct dev_reg *regp;
	register int len, retval;

	/*
	 * Assume unit string is "%x,%x", mapping to "bustype, offset".
	 */
	bus_type = atou(addrspec);
	while (*addrspec != 0)  {
		if (*addrspec++ == ',')
			break;
	}
	offset = atou(addrspec);

	len = getproplen(dip->devi_nodeid, "reg");
	regp = (struct dev_reg *)getlongprop(dip->devi_nodeid, "reg");
	if (regp == 0)  {
#ifdef	DPRINTF
		dprintf("no addr qualifier (reg) for nodeid <%x>\n",
		    dip->devi_nodeid);
#endif	DPRINTF
		return (0);
	}
#ifdef	DPRINTF
	dprintf("\tbustype %d addr %x; reg %d %x\n", bus_type, offset,
	    dip->devi_reg->reg_bustype, dip->devi_reg->reg_addr);
#endif	DPRINTF

	retval = (bus_type == (u_int)regp->reg_bustype &&
		offset == ((u_int)regp->reg_addr));
	kmem_free((caddr_t)regp, (u_int) len);
	return (retval);
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
		return (ENCODE_ERROR);
	*addrspec = '\0';
	if (dip == (struct dev_info *)0)
		return (ENCODE_ERROR);

	for (sdev = sd_root; sdev; sdev = sdev->sd_next) {
		if (sdev->sd_dev == dip)
			break;
	}
	if (sdev == (struct scsi_device *)0)
		return (ENCODE_ERROR);

	sdeva = &sdev->sd_address;
	(void)sprintf(addrspec, "%d,%d", sdeva->a_target, sdeva->a_lun);
	return (ENCODE_OK);
}

static int
esp_match(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	register int target = 0;
	register int lun = 0;
	register struct scsi_device *sdev;
	register struct dev_info *sdevi;
	register struct scsi_address *sdeva;

	/*
	 * Assume unit string is "%x,%x", mapping to "target, lun".
	 */
	target = atou(addrspec);
	while (*addrspec != 0)  {
		if (*addrspec++ == ',')
			break;
	}
	lun = atou(addrspec);
#ifdef	DPRINTF
	dprintf("Matching target %d lun %d\n", target, lun);
#endif	DPRINTF

	/*
	 * Scan list of SCSI devices and look for a target/lun match.
	 */
#ifdef	DPRINTF
	dprintf("name\tunit\tha\t\ttarget\tlun\tpresent\n");
#endif	DPRINTF
	for (sdev = sd_root; sdev; sdev = sdev->sd_next) {
		sdevi = sdev->sd_dev;
		sdeva = &sdev->sd_address;

#ifdef	DPRINTF
		dprintf("%s\t%d\t%x\t%d\t%d\t%d  ",
		    sdevi->devi_name, sdevi->devi_unit, sdeva->a_cookie,
		    sdeva->a_target, sdeva->a_lun, sdev->sd_present);
#endif	DPRINTF

		if (sdeva->a_target == target && sdeva->a_lun == lun) {
#ifndef	DPRINTF
			if (dip == sdevi)
				return (1);
#else	! DPRINTF
			if (dip == sdevi)  {
				dprintf("HIT!!!\n");
				return (1);
			} else
				dprintf("target/lun match only\n");
		} else {
			dprintf("no match\n");
#endif	! DPRINTF
		}
	}
	return (0);
}
#endif NSCSIBUS > 0

#ifdef	VME
#if NID > 0
/*
 * XXX: For IPI Devices
 * Encode <facility, 0> into a string of the form "%x,%x".
 * Best I can tell, is devi_unit low order 4 bits are the disk number.
 */
static char *
ipi_encode_reg(dip, addrspec)
	struct dev_info *dip;
	char *addrspec;
{
	if (addrspec == (char *)0)
		return (ENCODE_ERROR);
	*addrspec = '\0';
	if (dip == (struct dev_info *)0)
		return (ENCODE_ERROR);

	(void)sprintf(addrspec, "%x,0", dip->devi_unit&0xf);
	return (ENCODE_OK);
}
#endif	NID > 0
#endif	VME
