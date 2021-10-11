#ident "@(#)sunergy_tree.c	1.1	1.1 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */


/*
 * fake device tree for Sunergy 
 *
 * This contains the system dependent nodes.
 * Module dependent nodes are in <modname>_tree.c
 * mod_info is the link in the device tree.
 */

#include "fakeprom.h"
/************************************************************************
 *	data parameter values
 */
static int	pagesize = 4096;	/* iommu.page-size */

/************************************************************************
 *	device register layouts
 */

static struct dev_reg memory_reg[] =		{ { 0x0, (addr_t)0x00000000, 0x00700000 },};
static struct dev_reg sbus_range[] =		{ { 0x0, (addr_t)0x30000000, 0x4FFFFFFF },};
static struct dev_reg iommu_reg[] =		{ { 0x0, (addr_t)0x10000000, 0x300 },};
static struct dev_reg sbus_reg[] =		{ { 0x0, (addr_t)0x10001000, 32 },};
static struct dev_reg module_id_reg[] =		{ { 0x0, (addr_t)0x10002000, 4 },};
static struct dev_reg obio_range[] =		{ { 0x0, (addr_t)0x70000000, 0x0FFFFFFF},};
static struct dev_reg kbd_reg[] =		{ { 0x0, (addr_t)0x01000000, 8 },};
static struct dev_reg zs_reg[] =		{ { 0x0, (addr_t)0x01100000, 8 },};
static struct dev_reg floppy_reg[] =		{ { 0x0, (addr_t)0x01200000, 8 },};
static struct dev_reg auxio_reg[] =		{ { 0x0, (addr_t)0x01300000, 4 },};
static struct dev_reg slavio_reg[] =		{ { 0x0, (addr_t)0x01400000, 4 },};
static struct dev_reg crt_reg[] =		{ { 0x0, (addr_t)0x01500000, 4096 },};
static struct dev_reg eeprom_reg[] =		{ { 0x0, (addr_t)0x01900000, 8192 },};
static struct dev_reg codec_reg[] =		{ { 0x0, (addr_t)0x01A00000, 4096 },};
static struct dev_reg gpdec_reg[] =		{ { 0x0, (addr_t)0x01B00000, 4 },};
static struct dev_reg vsr_reg[] =		{ { 0x0, (addr_t)0x01C00000, 4 },};
static struct dev_reg counter_reg[] =		{ { 0x0, (addr_t)0x09300000, 16 },
						  { 0x0, (addr_t)0x09301000, 16 },
						  { 0x0, (addr_t)0x09302000, 16 },
						  { 0x0, (addr_t)0x09303000, 16 },
						  { 0x0, (addr_t)0x09310000, 20 },};
static struct dev_reg interrupt_reg[] =		{ { 0x0, (addr_t)0x09400000, 16 },
						  { 0x0, (addr_t)0x09401000, 16 },
						  { 0x0, (addr_t)0x09402000, 16 },
						  { 0x0, (addr_t)0x09403000, 16 },
						  { 0x0, (addr_t)0x09410000, 20 },};
static struct dev_reg simdisk_reg[] =		{ { 0x0, (addr_t)0x01F00000, 32 },};

/**********************************************************************************
 *	device property lists
 */

static struct property counter_props[] =	{ {psname, 8, "counter"},
						  {psreg, RARRAY(counter_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property eeprom_props[] =		{ {psname, 7, "eeprom"},
						  {psreg, RARRAY(eeprom_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property interrupt_props[] =	{ {psname, 10, "interrupt"},
						  {psreg, RARRAY(interrupt_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property iommu_props[] =		{ {psname, 6, "iommu"},
						  {psreg, RARRAY(iommu_reg)},
						  {"page-size", ARRAY(pagesize)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property kbd_props[] =		{ {psname, 3, "zs"},
						  {psreg, RARRAY(kbd_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property memory_props[] =		{ {psname, 7, "memory"},
						  {psreg, RARRAY(memory_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property module_id_props[] =	{ {psname, 10, "module-id"},
						  {psreg, RARRAY(module_id_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property obio_props[] =		{ {psname, 5, "obio"},
						  {psrange, RARRAY(obio_range)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property sbus_props[] =		{ {psname, 5, "sbus"},
						  {psreg, RARRAY(sbus_reg)},
						  {psrange, RARRAY(sbus_range)},
						  {0, 0, 0,},
						  {NULL}, };

static struct property root_props[] =		{ {psname, 14, "Sun4M/sunergy"},
						  {0, 0, 0,},
						  {NULL}, };
static struct property zs_props[] =		{ {psname, 3, "zs"},
						  {psreg, RARRAY(zs_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property option_props[] =		{ {psname, 8, "options"},
						  {"auto-boot", BOOLEAN },
						  {0, 0, 0,},
						  {NULL}, };
static struct property floppy_props[] =		{ {psname, 7, "floppy"},
						  {psreg, RARRAY(floppy_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property auxio_props[] =		{ {psname, 10, "auxio-reg"},
						  {psreg, RARRAY(auxio_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property slavio_props[] =		{ {psname, 14, "slavioctl-reg"},
						  {psreg, RARRAY(slavio_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property crt_props[] =		{ {psname, 8, "crt-reg"},
						  {psreg, RARRAY(crt_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property codec_props[] =		{ {psname, 6, "codec"},
						  {psreg, RARRAY(codec_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property gpdec_props[] =		{ {psname, 10, "gpdec-reg"},
						  {psreg, RARRAY(gpdec_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property vsr_props[] =		{ {psname, 7, "vs-reg"},
						  {psreg, RARRAY(vsr_reg)},
						  {0, 0, 0,},
						  {NULL}, };
static struct property simdisk_props[] =	{ {psname, 4, "sd0"},
						  {psreg, RARRAY(simdisk_reg)},
						  {psdevtype, 6, "block"},
						  {0, 0, 0,},
						  {NULL}, };

/************************************************************************
 *	device nodes lacing it all into a nice tree
 */

/* lance, scsi, pp, lsc, dbri */
static devnode_t sbus_info =		{ NULL, NULL, sbus_props, };

/* scsr missing */
static devnode_t simdisk_info =		{ NULL, NULL, simdisk_props, };
static devnode_t interrupt_info =	{ &simdisk_info, NULL, interrupt_props, };
static devnode_t counter_info =		{ &interrupt_info, NULL, counter_props, };
static devnode_t vsr_info =		{ &counter_info, NULL, vsr_props, };
static devnode_t gpdec_info =		{ &vsr_info, NULL, gpdec_props, };
static devnode_t codec_info =		{ &gpdec_info, NULL, codec_props, };
static devnode_t eeprom_info =		{ &codec_info, NULL, eeprom_props, };
static devnode_t crt_info =		{ &eeprom_info, NULL, crt_props, };
static devnode_t slavio_info =		{ &crt_info, NULL, slavio_props, };
static devnode_t auxio_info =		{ &slavio_info, NULL, auxio_props, };
static devnode_t floppy_info =		{ &auxio_info, NULL, floppy_props, };
static devnode_t zs_info =		{ &floppy_info, NULL, zs_props, };
static devnode_t kbd_info =		{ &zs_info, NULL, kbd_props, };

extern devnode_t mod0_info;	/* pick up M-Bus Modules */

static devnode_t obio_info =		{ &mod0_info, &kbd_info, obio_props, };
static devnode_t iommu_info =		{ &obio_info, &sbus_info, iommu_props, };
static devnode_t module_id_info =	{ &iommu_info, NULL, module_id_props, };
static devnode_t memory_info =		{ &module_id_info, NULL, memory_props, };
static devnode_t option_info =		{ &memory_info, NULL, option_props, };

devnode_t       root_info =		{ NULL, &memory_info, root_props, };
