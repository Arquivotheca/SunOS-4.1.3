#ident	"%Z%%M% %I% %E% SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

#ifdef	KERNEL

#include <scsi/conf/autoconf.h>

/*
 * Autoconfiguration Dependent Data
 */

/*
 * Total number of scsi devices found so far (Targets && Luns).
 */

int	nscsi_devices;

/*
 * Global SCSI device chain- nscsi_devices long, null terminated.
 * See <scsi/conf/device.h> for structure definition.
 */

struct scsi_device *sd_root;

/*
 * Number of commands to allocate per device (if standard implementation
 * allocation routines are used). This doesn't mean each device is
 * limited to this amount- it just means how much the attachment of
 * a device causes to be added to the pool of available commands.
 */

int scsi_ncmds_per_dev = NCMDS_PER_DEV;

/*
 * delay after scsi bus reset in usec
 */

int scsi_reset_delay = 2*1000000;

/*
 * SCSI options word- defines are kept in <scsi/conf/autoconf.h>
 *
 * All this options word does is to enable such capabilities. Each
 * implementation may disable this word, or ignore it entirely.
 * Changing this word after system autoconfiguration is not guaranteed
 * to cause any change in the operation of the system.
 */

int scsi_options =
	SCSI_OPTIONS_PARITY	|
	SCSI_OPTIONS_SYNC	|
	SCSI_OPTIONS_LINK	|
	SCSI_OPTIONS_DR;

#endif	KERNEL
