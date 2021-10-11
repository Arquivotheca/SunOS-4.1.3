/*	@(#)win_screen.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Window system screen (sometimes called a desktop) structure.
 */

struct	singlecolor {
	u_char	red, green, blue;
};

#define	SCR_NAMESIZE	20

struct screen {
	char	scr_rootname[SCR_NAMESIZE];/* root window device name */
	char	scr_kbdname[SCR_NAMESIZE];/* keyboard device name */
	char	scr_msname[SCR_NAMESIZE]; /* mouse device name */
	char	scr_fbname[SCR_NAMESIZE]; /* frame buffer device name */
	struct	singlecolor scr_foreground;/* Color of foreground */
	struct	singlecolor scr_background;/* Color of background */
	int	scr_flags;
#define	SCR_SWITCHBKGRDFRGRD	0x1	/* switch display bkgrd and frgrd */
#define	SCR_TOGGLEENABLE	0x2	/* when enter screen with mouse set
					 * enable plane to 0, when exit then
					 * set to 1 (only works with certain
					 * frame buffer set ups) */
#define	SCR_8BITCOLORONLY	0x4	/* only utilize 8bit color plane group*/
#define	SCR_OVERLAYONLY		0x8	/* only utilize overlay plane group */
	struct	rect scr_rect;		/* position/size in device coords */
};

/*
 * Multiple display devices can be used at once.  The following constants
 * are used to identify where the devices are relative to each other so
 * that cursor motions off one screen onto another can be achieved.
 */
#define	SCR_NORTH		0
#define	SCR_EAST		1
#define	SCR_SOUTH		2
#define	SCR_WEST		3

#define	SCR_POSITIONS 		4

