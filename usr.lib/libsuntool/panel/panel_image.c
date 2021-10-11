#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_image.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*               Copyright (c) 1986 by Sun Microsystems, Inc.                */

#include <suntool/panel_impl.h>

static void 	 outline_button();
static void 	 special_outline_button();
static Pixrect	*panel_button_image_real();


/*****************************************************************************/
/* panel_button_image                                                        */
/* panel_button_image creates a button pixrect from the characters in        */
/* 'string'.  'width' is the desired width of the button (in characters).    */
/* If 'font' is NULL, the font in 'panel' is used.                           */
/*****************************************************************************/

/* this NEW proc below creates images which are double outlined, rather
 * than just bolded.  Currently used in alerts only.
 */
extern Pixrect *
panel_button_image2(client_object, string, width, font)
Panel		  client_object;
char		  *string;
int		   width;
Pixfont	  	  *font;
{
    return ((Pixrect *) panel_button_image_real(
	client_object, string, width, font, 1));
}

Pixrect *
panel_button_image(client_object, string, width, font)
Panel		   client_object;
char		  *string;
int		   width;
Pixfont	  	  *font;
{
    return ((Pixrect *) panel_button_image_real(
	client_object, string, width, font, 0));
}

static Pixrect *
panel_button_image_real(client_object, string, width, font, special)
Panel		   client_object;
char		  *string;
int		   width;
Pixfont	  	  *font;
int		   special;
{
   panel_handle    object = PANEL_CAST(client_object);
   struct pr_prpos where;	/* where to write the string */
   struct pr_size  size;	/* size of the pixrect */
   panel_handle    panel;

   /* make sure we were really passed a panel, not an item */
   if (is_panel(object)) 
      panel = object;
   else if (is_item(object)) 
      panel = ((panel_item_handle) object)->panel;
   else 
      return NULL;

   if (!font)
      font = panel->font;
      
   size = pf_textwidth(strlen(string), font, string);

   width = panel_col_to_x(font, width);

   if (width < size.x)
      width = size.x;

   where.pr = (special) ?
	mem_create(width + 16, size.y + 8, 1)
      : mem_create(width + 12, size.y + 4, 1);
   if (!where.pr)
      return (NULL);

   where.pos.x = (special) ?
	8 + (width - size.x) / 2
      : 6 + (width - size.x) / 2;
   where.pos.y = (special) ?
	4 + panel_fonthome(font)
      : 2 + panel_fonthome(font);

   (void)pf_text(where, PIX_SRC, font, string);
   
   if (special) {
	special_outline_button(where.pr);
    } else outline_button(where.pr);

   return (where.pr);
}


static void
outline_button(pr)
register Pixrect *pr;
/* outline_button draws an outline of a button in pr.
*/
{
   int	x_left		= 0;
   int	x_right		= pr->pr_width - 1;
   int	y_top		= 0;
   int	y_bottom	= pr->pr_height - 1;
   int	x1		= 3;
   int	x2		= x_right - 3;
   int	y1		= 3;
   int	y2		= y_bottom - 3;

   /* horizontal lines */
   (void)pr_vector(pr, x1, y_top, x2, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_top + 1, x2, y_top + 1, PIX_SRC, 1);

   (void)pr_vector(pr, x1, y_bottom, x2, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x1, y_bottom - 1, x2, y_bottom - 1, PIX_SRC, 1);

   /* vertical lines */
   (void)pr_vector(pr, x_left, y1, x_left, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1, x_left + 1, y2, PIX_SRC, 1);

   (void)pr_vector(pr, x_right, y1, x_right, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1, x_right - 1, y2, PIX_SRC, 1);

   /* left corners */
   (void)pr_vector(pr, x_left,     y1,     x1,     y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1,     x1 + 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y1 + 1, x1 + 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_left,     y2,     x1,     y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2,     x1 + 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 1, y2 - 1, x1 + 2, y_bottom, PIX_SRC, 1);

   /* right corners */
   (void)pr_vector(pr, x_right,     y1,     x2,     y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1,     x2 - 1, y_top, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y1 + 1, x2 - 2, y_top, PIX_SRC, 1);

   (void)pr_vector(pr, x_right,     y2,     x2,     y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2,     x2 - 1, y_bottom, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 1, y2 - 1, x2 - 2, y_bottom, PIX_SRC, 1);
}

/* below draws a double, then single outlined button image */
static void
special_outline_button(pr)
register Pixrect *pr;
{
   int	x_left		= 0;
   int	x_right		= pr->pr_width - 1;
   int	y_top		= 0;
   int	y_bottom	= pr->pr_height - 1;
   int	x1		= 3;
   int	x2		= x_right - 3;
   int	y1		= 3;
   int	y2		= y_bottom - 3;

   /* top horizontal lines */
   (void)pr_vector(
	pr, x1, y_top, x2, y_top, PIX_SRC, 1); /*top, single*/
   (void)pr_vector(
	pr, x1+1, y_top + 2, x2-1, y_top + 2, PIX_SRC, 1);/*1st dbl line*/
   (void)pr_vector(
	pr, x1+1, y_top + 3, x2-1, y_top + 3, PIX_SRC, 1);/*2nd dbl line*/

   /* bottom horizontal lines */
   (void)pr_vector(
	pr, x1, y_bottom, x2, y_bottom, PIX_SRC, 1); /*bottom single*/
   (void)pr_vector(	
	pr, x1+1, y_bottom - 2, x2-1, y_bottom - 2, PIX_SRC, 1);/*1st above*/
   (void)pr_vector(
	pr, x1+1, y_bottom - 3, x2-1, y_bottom - 3, PIX_SRC, 1);/*2nd above*/

   /* left vertical lines (left to right)*/
   (void)pr_vector(pr, x_left, y1, x_left, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 2, y1+1, x_left + 2, y2-1, PIX_SRC, 1);
   (void)pr_vector(pr, x_left + 3, y1+1, x_left + 3, y2-1, PIX_SRC, 1);

   /* right vertical lines (right to left)*/
   (void)pr_vector(pr, x_right, y1, x_right, y2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 2, y1+1, x_right - 2, y2-1, PIX_SRC, 1);
   (void)pr_vector(pr, x_right - 3, y1+1, x_right - 3, y2-1, PIX_SRC, 1);

   /* left top corners */
   (void)pr_vector(
	pr, x_left, y1, x1, y_top, PIX_SRC, 1); /*single line*/
    /* now dbl line */
   (void)pr_vector(pr, x_left+2, y1+1, x1+1, y_top+2, PIX_SRC, 1); 
   (void)pr_vector(pr, x_left+3, y1+1, x1+2, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y1+2, x1+3, y_top+2, PIX_SRC, 1);

   /* left bottom corners */
   (void)pr_vector(
	pr, x_left, y2, x1, y_bottom, PIX_SRC, 1); /*single line*/
    /* now dbl line */
   (void)pr_vector(pr, x_left+2, y2-1, x1+1, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y2-1, x1+2, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_left+3, y2-2, x1+3, y_bottom-2, PIX_SRC, 1);

   /* right top corners */
   (void)pr_vector(
	pr, x_right, y1, x2, y_top, PIX_SRC, 1); /*single line*/
   (void)pr_vector(pr, x_right-2, y1+1, x2-1, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y1+1, x2-2, y_top+2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y1+2, x2-3, y_top+2, PIX_SRC, 1);

   /* right bottom corners */
   (void)pr_vector(
	pr, x_right, y2, x2, y_bottom, PIX_SRC, 1); /*single line*/
   (void)pr_vector(pr, x_right-2, y2-1, x2-1, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y2-1, x2-2, y_bottom-2, PIX_SRC, 1);
   (void)pr_vector(pr, x_right-3, y2-2, x2-3, y_bottom-2, PIX_SRC, 1);
}
