/*	@(#)clockhands.h 1.1 92/07/30	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Structure defining data points for hands of clocktool.
 */
struct	hands {
	char	x1, y1;		/* first origin for this position */
	char	x2, y2;		/* second origin for this position */
	char	sec_x, sec_y;	/* second-hand origin for this minute */
	char	min_x, min_y;	/* end of minute and second hands */
	char	hour_x, hour_y; /* end of hour hand */
};

extern	struct hands hand_points[];

/*
 * The following hands stuff is for the roman face.
 */
struct	endpoints {
	char	x;
	char	y;
};

extern	struct endpoints ep_min[];
extern	struct endpoints ep_hr[];
extern	struct endpoints ep_sec[];
extern	struct endpoints ep_secorg[];
