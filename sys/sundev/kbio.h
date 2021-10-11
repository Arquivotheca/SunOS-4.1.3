/*	@(#)kbio.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Keyboard related ioctls
 */

#ifndef _sundev_kbio_h
#define _sundev_kbio_h

#include <sys/ioccom.h>

/*
 * See sundev/kbd.h for TR_NONE (don't translate) and TR_ASCII
 * (translate to ASCII) TR_EVENT (translate to virtual input
 * device codes)
 */
#define	KIOCTRANS	_IOW(k, 0, int)	/* set keyboard translation */
#define	KIOCGTRANS	_IOR(k, 5, int)	/* get keyboard translation */

#define	KIOCTRANSABLE	_IOW(k, 6, int)	/* set keyboard translatability */
#define	KIOCGTRANSABLE	_IOR(k, 7, int)	/* get keyboard translatability */
#define	TR_CANNOT	0	/* Cannot translate keyboard using tables */
#define	TR_CAN		1	/* Can translate keyboard using tables */

/*
 * Old-style keymap entry, for backwards compatibility only.
 */
struct	kiockey {
	int	kio_tablemask;	/* Translation table (one of: 0, CAPSMASK,
				   SHIFTMASK, CTRLMASK, UPMASK,
				   ALTGRAPHMASK, NUMLOCKMASK) */
#define	KIOCABORT1	-1	/* Special "mask": abort1 keystation */
#define	KIOCABORT2	-2	/* Special "mask": abort2 keystation */
	u_char	kio_station;	/* Physical keyboard key station (0-127) */
	u_char	kio_entry;	/* Translation table station's entry */
	char	kio_string[10];	/* Value for STRING entries (null terminated) */
};

/*
 * Set kio_tablemask table's kio_station to kio_entry.
 * Copy kio_string to string table if kio_entry is between STRING and
 * STRING+15.  EINVAL is possible if there are invalid arguments.
 */
#define	KIOCSETKEY	_IOW(k, 1, struct kiockey)

/*
 * Get kio_tablemask table's kio_station to kio_entry.
 * Get kio_string from string table if kio_entry is between STRING and
 * STRING+15.  EINVAL is possible if there are invalid arguments.
 */
#define	KIOCGETKEY	_IOWR(k, 2, struct kiockey)

/*
 * Send the keyboard device a control command.  sundev/kbd.h contains
 * the constants that define the commands.  Normal values are:
 * KBD_CMD_BELL, KBD_CMD_NOBELL, KBD_CMD_CLICK, KBD_CMD_NOCLICK.
 * Inappropriate commands for particular keyboard types are ignored.
 *
 * Since there is no reliable way to get the state of the bell or click
 * or LED (because we can't query the kdb, and also one could do writes
 * to the appropriate serial driver--thus going around this ioctl)
 * we don't provide an equivalent state querying ioctl. 
 */
#define	KIOCCMD		_IOW(k, 8, int)	/* Send keyboard command */

/*
 * Get keyboard type.  Return values are one of KB_* from sundev/kbd.h,
 * e.g., KB_KLUNK, KB_VT100, KB_SUN2, KB_SUN3, KB_SUN4, KB_ASCII.
 * -1 means that the type is not known.
 */
#define	KIOCTYPE	_IOR(k, 9, int)	/* get keyboard type */

/*
 * Set flag indicating whether keystrokes get routed to /dev/console.
 */
#define	KIOCSDIRECT	_IOW(k, 10, int)

/*
 * Get flag indicating whether keystrokes get routed to /dev/console.
 */
#define	KIOCGDIRECT	_IOR(k, 11, int)

/*
 * New-style key map entry.
 */
struct	kiockeymap {
	int	kio_tablemask;	/* Translation table (one of: 0, CAPSMASK,
				   SHIFTMASK, CTRLMASK, UPMASK,
				   ALTGRAPHMASK) */
	u_char	kio_station;	/* Physical keyboard key station (0-127) */
	u_short	kio_entry;	/* Translation table station's entry */
	char	kio_string[10];	/* Value for STRING entries (null terminated) */
};

/*
 * Set kio_tablemask table's kio_station to kio_entry.
 * Copy kio_string to string table if kio_entry is between STRING and
 * STRING+15.  EINVAL is possible if there are invalid arguments.
 */
#define	KIOCSKEY	_IOW(k, 12, struct kiockeymap)

/*
 * Get kio_tablemask table's kio_station to kio_entry.
 * Get kio_string from string table if kio_entry is between STRING and
 * STRING+15.  EINVAL is possible if there are invalid arguments.
 */
#define	KIOCGKEY	_IOWR(k, 13, struct kiockeymap)

/*
 * Set and get LED state.
 */
#define KIOCSLED	_IOW(k, 14, char)
#define KIOCGLED	_IOR(k, 15, char)

/*
 * Set and get compatibility mode.
 */
#define	KIOCSCOMPAT	_IOW(k, 16, int)
#define	KIOCGCOMPAT	_IOR(k, 17, int)

/*
 * Get keyboard layout.
 */
#define	KIOCLAYOUT	_IOR(k, 20, int)

#endif /*!_sundev_kbio_h*/
