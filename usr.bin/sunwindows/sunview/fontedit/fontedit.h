/*	@(#)fontedit.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* for boundry values which cannot be determined from the pixfont description */
#define FT$UNKNOWN	(-1)

/* length of a path name */
#define FT$PATHLEN      1024

#define PIX_XOR		(PIX_SRC ^ PIX_DST)

#define BACKGROUND	0
#define FOREGROUND	1
#define HI_LIGHT	2
#define MARKER		3


/* the heights for the subwindow */
#define MSG_WINDOW_HEIGHT	24
#define OPTION_WINDOW_HEIGHT    90
#define CANVAS_WINDOW_HEIGHT    1024
#define ECHO_WINDOW_HEIGHT	30


/* Limits: */
#define MAX_NUM_CHARS_IN_FONT	256
#define MAX_FONT_CHAR_SIZE_WIDTH 48
#define MAX_FONT_CHAR_SIZE_HEIGHT 24


/* size of cell to represent a pixel in a font */
#define CELL_SIZE	16

typedef struct {
	int			open;	/* true if the char is being edited */
} a_fted_char_info;


typedef struct {
    int		but_id;
    int		org_x, org_y;
    int		cell_x, cell_y;
    int		old_color;
} marker;


/* a useful macro for checking the return from */
/* routines which create windows */
#define CHECK_RETURN(VAL,TYPE,MSG)\
	if ( VAL == TYPE NULL) {	\
		fprintf(stderr,"Return value is 0 :%s\n",MSG);	\
		exit(-1);	\
	}
