/*************************************************************
 *                                                           *
 *                     file: toc.c                           *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

/* @(#)toc.h @(#)toc.h 1.1 92/07/30 Copyr 1989 Sun Microsystem. */

/*
 * This file contains the typedef's of toc data structure
 */

typedef struct track_entry {
	int	track_number;		
	int	control;		/* Bit 0-3 are valid bits */
	Msf	msf;
	Msf	duration;
} *Track_entry;

typedef struct toc {
	int	start_track;		/* starting track number */
	int	end_track;		/* ending track number */
	Track_entry	*toc_list;	/* list of track entries */
	int	size;			/* size of the list */
	Msf	total_msf;		/* the length of the disc */
} *Toc;

extern 	Toc	init_toc();
extern	void	add_track_entry();
extern	void	destroy_toc();
extern	Msf	get_track_duration();
extern	int	get_track_control();
