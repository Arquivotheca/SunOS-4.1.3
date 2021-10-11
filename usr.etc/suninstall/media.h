/* @(#)media.h 1.1 92/07/30 SMI */

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 *	Block size definitions
 */

#define	BS_HALF			20	/* Block size for 1/2" tapes */
#define	BS_QUARTER		126	/* Block size for 1/4" tapes */
#define	BS_NQUARTER		40	/* Block size for new 1/4" tapes */
#define	BS_DISKETTE		18	/* Block size for 31/2" diskettes */
#define	BS_DEFAULT		BS_NQUARTER /* Block size for unknown tapes */

#define	BS_CD_ROM		32	/*
					** This will be converted to
					** kilobytes in
					** the media_extract() function in
					** lib/media_io.c
					*/


/*
 * media type defines - these are an enumerated list of generic
 * types of media.  It is unclear weather a generic type list or a bit mask
 * flags of attibutes is the best thing to do, for now it's a list.
 * acutally, suninstall ought to have a better way of getting devices and
 * info about them - a config file perhaps?
 */
/* XXX should their be TAPE_QTR, TAPE_HALF...; or use flags? */
#define	MEDIAT_TAPE		1
#define	MEDIAT_CD_ROM		2
#define	MEDIAT_FLOPPY		3

/*
 * (possible) media flags XXX
 */
/* XXX #define MEDIA_F_ASK_TAPE_BLKSIZE ** do MTIOCxxx to get blocksize */

#define SPECIAL_ST_FORM		1
#define SPECIAL_ST_NULL		"st_"


/*
 *	External declarations for media functions
 */
extern	int	get_media_flag();

