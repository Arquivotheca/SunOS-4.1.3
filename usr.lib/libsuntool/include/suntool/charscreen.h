/*	@(#)charscreen.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Definitions relating to physical screen image.
 */

/*
 * Macros to convert character coordinates to pixel coordinates.
 */
#define row_to_y(row)	((row)*chrheight)
#define col_to_x(col)	(((col)*chrwidth) + chrleftmargin)
#define y_to_row(y)	((y)/chrheight)
#define x_to_col(x)	((((x) >= chrleftmargin) ? \
			  ((x) - chrleftmargin) : 0)/chrwidth)

/*
 * Character dimensions (fixed width fonts only!)
 * and of screen in pixels.
 */
int	chrheight, chrwidth, chrbase;
int	winheightp, winwidthp;
int	chrleftmargin;

struct	pixfont *pixfont;

/*
 * If delaypainting, delay painting.  Set when clear screen.
 * When input will block then paint characters (! white space) of entire image
 * and turn delaypainting off.
 */
int	delaypainting;

#ifdef cplus
void	pstring(char *s, int col, int row);
void	pclearline(int fromcol, int tocol, int row);
void	pcopyline(int fromcol, int tocol, int count, int row);
void	pclearscreen(int fromrow, int torow, int count);
void	pcopyscreen(int fromrow, int torow, int count);
#endif
