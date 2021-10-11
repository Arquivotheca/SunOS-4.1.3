#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)cursor.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * cursor.c: Routines for creating & modifying a cursor.
 *
 */

#include <varargs.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/cursor_impl.h>

static 	int	cursor_set_attr();

/* VARARGS0 */
Cursor
cursor_create(va_alist)
va_dcl
{
    caddr_t 		avlist[ATTR_STANDARD_SIZE];
    struct cursor	*cursor;
    char *calloc();
    va_list		valist;

    va_start(valist);
    (void)attr_make(avlist, ATTR_STANDARD_SIZE, valist);
    va_end(valist)

    if (!(cursor = (struct cursor *) (LINT_CAST(calloc(1, sizeof(struct cursor))))))
	return(0);
 
    cursor->cur_function = PIX_SRC | PIX_DST;
    cursor->cur_shape = 
#ifdef ecd.cursor
        mem_create(CURSOR_MIN_SIZE, CURSOR_MIN_SIZE, 1);
#else
	mem_create(CURSOR_MAX_IMAGE_WORDS, CURSOR_MAX_IMAGE_WORDS, 1);
#endif ecd.cursor
    cursor->flags = FREE_SHAPE;

    cursor->horiz_hair_op = PIX_SRC;
    cursor->horiz_hair_color = -1;
    cursor->horiz_hair_thickness = 1;
    cursor->horiz_hair_length = CURSOR_TO_EDGE;
    cursor->horiz_hair_gap = 0;

    cursor->vert_hair_op = PIX_SRC;
    cursor->vert_hair_color = -1;
    cursor->vert_hair_thickness = 1;
    cursor->vert_hair_length = CURSOR_TO_EDGE;
    cursor->vert_hair_gap = 0;

    (void)cursor_set_attr(cursor, avlist);

    return (Cursor) cursor;
}


void
cursor_destroy(cursor_client)
Cursor	cursor_client;
{
    struct cursor 	*cursor;
    
    cursor = (struct cursor *)(LINT_CAST(cursor_client));
    if (free_shape(cursor))
	(void)pr_destroy(cursor->cur_shape);

    free((caddr_t)cursor);
}


/* cursor_get returns the current value of which_attr. */
caddr_t
cursor_get(cursor_client, which_attr)
register Cursor			cursor_client;
Cursor_attribute		which_attr;
{
    struct cursor	*cursor;
    
    cursor = (struct cursor *)(LINT_CAST(cursor_client));
    switch (which_attr) {
	case CURSOR_SHOW_CURSOR:
	    return (caddr_t) show_cursor(cursor);

	case CURSOR_SHOW_CROSSHAIRS:
	    return (caddr_t) show_crosshairs(cursor);

	case CURSOR_SHOW_HORIZ_HAIR:
	    return (caddr_t) show_horiz_hair(cursor);

	case CURSOR_SHOW_VERT_HAIR:
	    return (caddr_t) show_vert_hair(cursor);

	case CURSOR_CROSSHAIR_BORDER_GRAVITY:
	case CURSOR_HORIZ_HAIR_BORDER_GRAVITY:
	    return (caddr_t) horiz_border_gravity(cursor);

	case CURSOR_VERT_HAIR_BORDER_GRAVITY:
	    return (caddr_t) vert_border_gravity(cursor);

	case CURSOR_FULLSCREEN:
	    return (caddr_t) fullscreen(cursor);

	case CURSOR_XHOT:
	    return (caddr_t) cursor->cur_xhot;

	case CURSOR_YHOT:
	    return (caddr_t) cursor->cur_yhot;

	case CURSOR_OP:
	    return (caddr_t) cursor->cur_function;

	case CURSOR_IMAGE:
	    return (caddr_t) cursor->cur_shape;

	case CURSOR_CROSSHAIR_OP:
	case CURSOR_HORIZ_HAIR_OP:
	    return (caddr_t) cursor->horiz_hair_op;

	case CURSOR_VERT_HAIR_OP:
	    return (caddr_t) cursor->vert_hair_op;

	case CURSOR_CROSSHAIR_COLOR:
	case CURSOR_HORIZ_HAIR_COLOR:
	    return (caddr_t) cursor->horiz_hair_color;

	case CURSOR_CROSSHAIR_THICKNESS:
	case CURSOR_HORIZ_HAIR_THICKNESS:
	    return (caddr_t) cursor->horiz_hair_thickness;

	case CURSOR_VERT_HAIR_COLOR:
	    return (caddr_t) cursor->vert_hair_color;

	case CURSOR_VERT_HAIR_THICKNESS:
	    return (caddr_t) cursor->vert_hair_thickness;

	case CURSOR_CROSSHAIR_GAP:
	case CURSOR_HORIZ_HAIR_GAP:
	    return (caddr_t) cursor->horiz_hair_gap;

	case CURSOR_VERT_HAIR_GAP:
	    return (caddr_t) cursor->vert_hair_gap;

	case CURSOR_CROSSHAIR_LENGTH:
	case CURSOR_HORIZ_HAIR_LENGTH:
	    return (caddr_t) cursor->horiz_hair_length;

	case CURSOR_VERT_HAIR_LENGTH:
	    return (caddr_t) cursor->vert_hair_length;

      default:
	 return (caddr_t) 0;
   }

} /* cursor_get */


/* VARARGS1 */
int
cursor_set(cursor_client, va_alist)
Cursor		cursor_client;
va_dcl
{
    caddr_t 		avlist[ATTR_STANDARD_SIZE];
    va_list		valist;
    struct cursor	*cursor;

    va_start(valist);
    (void)attr_make(avlist, ATTR_STANDARD_SIZE, valist);
    va_end(valist);
 
    cursor = (struct cursor *)(LINT_CAST(cursor_client));
    return cursor_set_attr(cursor, avlist);
}


/* cursor_set_attr sets the attributes mentioned in avlist. */
static int
cursor_set_attr(cursor, avlist)
register struct cursor	*cursor;
register Attr_avlist	avlist;
{
    register Cursor_attribute	which_attr;

    while (which_attr = (Cursor_attribute) *avlist++) {
	switch (which_attr) {
	    case CURSOR_SHOW_CURSOR:
		if (*avlist++)
		    cursor->flags &= ~DONT_SHOW_CURSOR;
		else
		    cursor->flags |= DONT_SHOW_CURSOR;
		break;

	    case CURSOR_SHOW_CROSSHAIRS:
		if (*avlist++)
		    cursor->flags |= SHOW_HORIZ_HAIR | SHOW_VERT_HAIR;
		else
		    cursor->flags &= ~(SHOW_HORIZ_HAIR | SHOW_VERT_HAIR);
		break;

	    case CURSOR_SHOW_HORIZ_HAIR:
		if (*avlist++)
		    cursor->flags |= SHOW_HORIZ_HAIR;
		else
		    cursor->flags &= ~SHOW_HORIZ_HAIR;
		break;

	    case CURSOR_SHOW_VERT_HAIR:
		if (*avlist++)
		    cursor->flags |= SHOW_VERT_HAIR;
		else
		    cursor->flags &= ~SHOW_VERT_HAIR;
		break;

	    case CURSOR_CROSSHAIR_BORDER_GRAVITY:
		if (*avlist++)
		    cursor->flags |= 
			HORIZ_BORDER_GRAVITY | VERT_BORDER_GRAVITY;
		else
		    cursor->flags &=
			~(HORIZ_BORDER_GRAVITY | VERT_BORDER_GRAVITY);
		break;

	    case CURSOR_HORIZ_HAIR_BORDER_GRAVITY:
		if (*avlist++)
		    cursor->flags |= HORIZ_BORDER_GRAVITY;
		else
		    cursor->flags &= ~HORIZ_BORDER_GRAVITY;
		break;

	    case CURSOR_VERT_HAIR_BORDER_GRAVITY:
		if (*avlist++)
		    cursor->flags |= VERT_BORDER_GRAVITY;
		else
		    cursor->flags &= ~VERT_BORDER_GRAVITY;
		break;

	    case CURSOR_FULLSCREEN:
		if (*avlist++)
		    cursor->flags |= FULLSCREEN;
		else
		    cursor->flags &= ~FULLSCREEN;
		break;

	    case CURSOR_XHOT:
		cursor->cur_xhot = (int) *avlist++;
		break;

	    case CURSOR_YHOT:
		cursor->cur_yhot = (int) *avlist++;
		break;

	    case CURSOR_OP:
		cursor->cur_function = (int) *avlist++;
		break;

	    case CURSOR_IMAGE:
		/* free the storage for the image if mem_create() was used */
		if (free_shape(cursor)) {
		    (void)pr_destroy(cursor->cur_shape);
		    cursor->flags &= ~FREE_SHAPE;
		}

		cursor->cur_shape = (struct pixrect *) (LINT_CAST(*avlist++));
		break;

	    case CURSOR_CROSSHAIR_COLOR:
		cursor->horiz_hair_color = 
		    cursor->vert_hair_color = (int) *avlist++;
		break;

	    case CURSOR_CROSSHAIR_OP:
		cursor->horiz_hair_op = 
		    cursor->vert_hair_op = (int) *avlist++;
		break;

	    case CURSOR_HORIZ_HAIR_OP:
		cursor->horiz_hair_op = (int) *avlist++;
		break;

	    case CURSOR_VERT_HAIR_OP:
		cursor->vert_hair_op = (int) *avlist++;
		break;

	    case CURSOR_HORIZ_HAIR_COLOR:
		cursor->horiz_hair_color = (int) *avlist++;
		break;

	    case CURSOR_CROSSHAIR_THICKNESS:
		if ((int) *avlist > CURSOR_MAX_HAIR_THICKNESS)
		    return 1;
		cursor->horiz_hair_thickness = 
		    cursor->vert_hair_thickness = (int) *avlist++;
		break;

	    case CURSOR_HORIZ_HAIR_THICKNESS:
		if ((int) *avlist > CURSOR_MAX_HAIR_THICKNESS)
		    return 1;
		cursor->horiz_hair_thickness = (int) *avlist++;
		break;

	    case CURSOR_VERT_HAIR_COLOR:
		cursor->vert_hair_color = (int) *avlist++;
		break;

	    case CURSOR_VERT_HAIR_THICKNESS:
		if ((int) *avlist > CURSOR_MAX_HAIR_THICKNESS)
		    return 1;
		cursor->vert_hair_thickness = (int) *avlist++;
		break;

	    case CURSOR_CROSSHAIR_LENGTH:
		cursor->horiz_hair_length = 
		    cursor->vert_hair_length = (int) *avlist++;
		break;

	    case CURSOR_HORIZ_HAIR_LENGTH:
		cursor->horiz_hair_length = (int) *avlist++;
		break;

	    case CURSOR_VERT_HAIR_LENGTH:
		cursor->vert_hair_length = (int) *avlist++;
		break;

	    case CURSOR_CROSSHAIR_GAP:
		cursor->horiz_hair_gap = 
		    cursor->vert_hair_gap = (int) *avlist++;
		break;

	    case CURSOR_HORIZ_HAIR_GAP:
		cursor->horiz_hair_gap = (int) *avlist++;
		break;

	    case CURSOR_VERT_HAIR_GAP:
		cursor->vert_hair_gap = (int) *avlist++;
		break;

	  default:
	     /* Here we should complain about something */
	     return 1;
       }
    }
    return 0;
} /* cursor_set */


Cursor
cursor_copy(cursor_client)
register Cursor cursor_client;
{
    register struct cursor	*new_cursor;
    struct cursor		*cursor;

    cursor = (struct cursor *)(LINT_CAST(cursor_client));
    if (!(new_cursor = (struct cursor *) 
    		(LINT_CAST(calloc(1, sizeof(struct cursor))))))
	return 0;
 
    *new_cursor = *cursor;
    new_cursor->cur_shape = 
	mem_create(cursor->cur_shape->pr_width, cursor->cur_shape->pr_height, 
		   cursor->cur_shape->pr_depth);
    if (!new_cursor->cur_shape)
	return 0;

    /* we created this, so we free it later */
    new_cursor->flags |= FREE_SHAPE;

    /* copy the old image to the new image */
    (void)pr_rop(new_cursor->cur_shape, 0, 0, cursor->cur_shape->pr_width, 
	   cursor->cur_shape->pr_height, PIX_SRC, cursor->cur_shape, 0, 0);

    return (Cursor) new_cursor;
}
