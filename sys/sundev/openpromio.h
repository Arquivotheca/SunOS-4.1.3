/*
 * @(#)openpromio.h 1.1 92/07/30 SMI
 */

/*
 * Copyright (c) 1989, Sun Microsystems, Inc.
 */

#ifndef	_sundev_openpromio_h
#define	_sundev_openpromio_h

#include <sys/ioccom.h>

/*
 * XXX HACK ALERT
 *
 * You might think that this interface could support setting non-ASCII
 * property values.  Unfortunately the 4.0.3c openprom driver SETOPT
 * code ignores oprom_size and uses strlen() to compute the length of
 * the value.  The 4.0.3c openprom eeprom command makes its contribution
 * by not setting oprom_size to anything meaningful.  So, if we want the
 * driver to trust oprom_size we have to use SETOPT2.  XXX.
 */

struct openpromio {
	u_int	oprom_size;		/* real size of following array */
	char	oprom_array[1];		/* For property names and values */
					/* NB: Adjacent, Null terminated */
};
#define	OPROMMAXPARAM	1024		/* max size of array */

/*
 * Note that all OPROM ioctl codes are type void. Since the amount
 * of data copied in/out may (and does) vary, the openprom driver
 * handles the copyin/copyout itself.
 */
#define	OPROMGETOPT	_IO(O, 1)
#define	OPROMSETOPT	_IO(O, 2)
#define	OPROMNXTOPT	_IO(O, 3)
#define	OPROMSETOPT2	_IO(O, 4)	/* working OPROMSETOPT */
#define	OPROMNEXT	_IO(O, 5)	/* interface to raw config_ops */
#define	OPROMCHILD	_IO(O, 6)	/* interface to raw config_ops */
#define	OPROMGETPROP	_IO(O, 7)	/* interface to raw config_ops */
#define	OPROMNXTPROP	_IO(O, 8)	/* interface to raw config_ops */
#define	OPROMU2P	_IO(O, 9)	/* unix path to OBP prom path */
#define	OPROMGETCONS	_IO(O, 10)	/* find out console type */

#endif	/* !_sundev_openpromio_h */
