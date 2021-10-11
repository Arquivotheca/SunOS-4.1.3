/*	@(#)charimage.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Definitions relating to maintenance of virtual screen image.
 */

/*
 * Screen is maintained as an array of characters.
 * Screen is bottom lines and right columns.
 * Each line has length and array of characters.
 * Characters past length position are undefined.
 * Line is otherwise null terminated.
 */
extern unsigned char	**image;
extern char	**screenmode;
extern int	top, bottom, left, right;
extern int	cursrow, curscol;

#define length(line)	((line)[-1])

#define MODE_CLEAR	0
#define MODE_INVERT	1
#define MODE_UNDERSCORE	2
#define MODE_BOLD	4

#define	setlinelength(line, column) \
     {  unsigned char *cptr = (unsigned char *)(&((line)[-1]));\
        int _col = ((column)>right)?right:(column); \
        (line)[(_col)] = '\0'; \
        *cptr = (_col); \
     }   
