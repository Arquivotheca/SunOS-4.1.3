/*	@(#)conf.h 1.1 92/07/30 SMI; from UCB 7.1 6/4/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */

#ifndef _sys_conf_h
#define _sys_conf_h

struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_dump)();
	int	(*d_psize)();
	int	d_flags;
};
#ifdef KERNEL
extern struct bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_reset)();
	int	(*d_select)();
	int	(*d_mmap)();
	struct	streamtab *d_str;
	int	(*d_segmap)();
};
#ifdef KERNEL
extern struct cdevsw cdevsw[];
#endif

/*
 * Streams module information
 */
#define	FMNAMESZ	8

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;
};
#ifdef KERNEL
extern struct fmodsw fmodsw[];
extern int fmodcnt;
#endif

#endif /*!_sys_conf_h*/
