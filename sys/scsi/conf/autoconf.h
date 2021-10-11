/* @(#)autoconf.h	1.1 of 7/30/92 */

/*
 * Copyright (c) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef	_scsi_conf_autoconf_h
#define	_scsi_conf_autoconf_h

/*
 * SCSI subsystem options - global word of options are available
 *
 * bits 0-2 are reserved for debugging/informational level
 * bit  3 reserved for a global disconnect/reconnect switch
 * bit  4 reserved for a global linked command capability switch
 * bit  5 reserved for a global synchronous SCSI capability switch
 *
 * the rest of the bits are reserved for future use
 *
 */

#define	SCSI_DEBUG_TGT	0x1	/* debug statements in target drivers */
#define	SCSI_DEBUG_LIB	0x2	/* debug statements in library */
#define	SCSI_DEBUG_HA	0x4	/* debug statements in host adapters */


#define	SCSI_OPTIONS_DR		0x8	/* Global disconnect/reconnect  */
#define	SCSI_OPTIONS_LINK	0x10	/* Global linked commands */
#define	SCSI_OPTIONS_SYNC	0x20	/* Global synchronous capability */
#define	SCSI_OPTIONS_PARITY	0x40	/* Global parity support */

/*
 * SCSI configuration table- emitted by config(8) when a configuration
 * file specifies a SCSI device located on a SCSI bus and to be found
 * in `arch -k`/{CONFIGURATION-NAME}/ioconf.c.
 */

struct scsi_conf {
#ifdef	OPENPROMS
	struct dev_ops *haops;		/* Host Adapter device driver */
	struct dev_ops *tgtops;		/* Target device driver */
#else	OPENPROMS
	struct	mb_driver *hadriver;	/* Host Adapter device driver */
	struct	mb_driver *tgtdriver;	/* Target device driver */
#endif	OPENPROMS
	char	*tname;			/* Target device name (e.g. "sd") */
	char	*hname;			/* Host Adapter name (e.g. "si") */
	char	hunit;			/* Host Adapter unit # (e.g. "si0") */
	char	target;			/* SCSI Target address */
	char	lun;			/* SCSI Lun on Target */
	char	dunit;			/* UNIX unit # (e.g. "sd0") */
	char	busid;			/* system SCSI bus number */
};

/*
 * SCSI autoconfiguration definitions.
 *
 * The library routine scsi_slave() is provided as a service to target
 * driver to check for bare-bones existence of a SCSI device. It is
 * defined as:
 *
 * 	int scsi_slave(devp, canwait)
 *	struct scsi_device *devp;
 *	int canwait;
 *
 * where devp is the scsi_device structure passed to the target driver
 * at probe time, and where canwait declares whether scsi_slave() can
 * sleep awaiting resources or must return an error if it cannot get
 * resources (canwait == 1 implies that scsi_slave() can sleep- although
 * does not fully guarantee that resources will become available as
 * some are allocated from the iopbmap which may just be completely
 * full). In the process of determining the existence of a SCSI device,
 * scsi_slave will allocate space for the sd_inq field of the scsi_device
 * pointed to by devp (if it is non-zero upon entry).
 *
 * scsi_slave() attempts to follow this sequence in order to determine
 * the existence of a SCSI device:
 *
 *	Attempt to send a TEST UNIT READY command to the device.
 *
 *		If that gets a check condition, run a non-extended
 *		REQUEST SENSE command. Ignore the results of it, as
 *		a the non-extended sense information contains only
 *		Vendor Unique error codes (the idea is that during
 *		probe time the nearly invariant first command to a
 *		device will get a Check Condition, and the real reason
 *		is that the device wants to tell you that a SCSI bus
 *		reset just occurred.
 *
 *	Attempt to allocate an inquiry buffer out of the iopbmap and
 *	run an INQUIRY command (with response data format 0 set).
 *
 *		If that gets a check condition, run another
 *		non-extended REQUEST SENSE command.
 *
 * returns one of the integer values as defined below:
 */

#define	SCSIPROBE_EXISTS	0 /* device exists, inquiry data valid */
#define	SCSIPROBE_NONCCS	1 /* device exists, no inquiry data */
#define	SCSIPROBE_NORESP	2 /* device didn't respond */
#define	SCSIPROBE_NOMEM		3 /* no space available for structures */
#define	SCSIPROBE_FAILURE	4 /* polled command failure- unspecified */

/*
 * Number of commands per device to allocate for.
 */

#define	NCMDS_PER_DEV	3

/*
 * Kernel references
 */

#ifdef	KERNEL

extern struct scsi_conf scsi_conf[];
extern int scsi_options, nscsi_devices;
extern struct scsi_device *sd_root;
extern int scsi_spl;
extern int scsi_ncmds_per_dev, scsi_ncmds;
extern int scsi_reset_delay;

extern void scsi_config();
extern int scsi_slave();
extern void scsi_addcmds();
extern int scsi_add_device();

#endif	KERNEL
#endif _scsi_conf_autoconf_h
