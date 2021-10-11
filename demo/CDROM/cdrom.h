/*************************************************************
 *                                                           *
 *                     file: cdrom.h                         *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

/* @(#)cdrom.h @(#)cdrom.h 1.1 92/07/30 Copyr 1989 Sun Microsystem. */

/* definition for mounted() return value */
#define	STILL_MOUNTED	1
#define	UNMOUNTED	0

extern	int	cdrom_play_track();
extern	int	cdrom_start();
extern	int	cdrom_stop();
extern	int	cdrom_eject();
extern	int	cdrom_read_tocentry();
extern	int	cdrom_read_tochdr();
extern	int	cdrom_lock_unlock();
extern	int	cdrom_pause();
extern	int	cdrom_resume();
extern	int	cdrom_volume();
extern	int	cdrom_get_relmsf();
extern	int	cdrom_play();
extern	int	cdrom_paused();
