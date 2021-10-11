/*	@(#)device.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef	_scsi_conf_device_h
#define	_scsi_conf_device_h

#include <scsi/scsi_types.h>

/*
 * SCSI device structure.
 *
 *	The SCSI subsystem will maintain a chain of devices either found
 *	or asked for during system configuration. The external global
 *	variable sd_root will link all devices found through the sd_next
 *	field. The utility routine find_dev() will look for (and
 *	possibly allocate and add to this chain) the device identified
 *	by the scsi_address sd_address.
 *
 *	By 'device', we mean a Target/Lun out on some SCSI bus somewhere.
 *
 */
 
struct scsi_device {

	struct scsi_device	*sd_next;	/* next in global chain */

	/*
	 * routing info for this device
	 */

	struct scsi_address	sd_address;

	/*
	 * Autoconfiguration information
	 */

#ifdef	OPENPROMS
	struct dev_info		*sd_dev;
#else	OPENPROMS
	struct mb_device	*sd_dev;
#endif	OPENPROMS

	/*
	 * If this SCSI_DEVICE is a CCS compatible device, this
	 * field will point to the retrieved inquiry data.
	 *
	 */

	struct scsi_inquiry	*sd_inq;


	/*
	 * Likewise, if this is a CCS compatible device, we'll
	 * put a common pointer to the sense buffer here.
	 */

	struct scsi_extended_sense	*sd_sense;

	/*
	 * More detailed information is 'private' information, i.e., is
	 * only pertinent to Target drivers.
	 *
	 */

	caddr_t			sd_private;

	/*
	 * When a driver has found the device present (i.e., it responded
	 * to selection), it sets this flag to indicate this is so.
	 */

	u_char			sd_present;
};

#ifdef	KERNEL
extern struct scsi_device *sd_root;
extern int nscsi_devices;
#endif
#endif	_scsi_conf_device_h
