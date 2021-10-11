#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)iconedit_canvas.c 1.1 92/07/30";
#endif
#endif

/**************************************************************************/
/*                       iconedit_canvas.c                                */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/**************************************************************************/

#include <sunwindow/sun.h>
#include "iconedit.h"

extern double	 sqrt();
void		 pw_batch();

static undo(), proof_event_proc(), canvas_event_proc();

/**************************************************************************/
/* text mode stuff                                                        */
/**************************************************************************/
 
static CELL_POS        abc_cell_origin,abc_h_cell_origin,abc_feedback_origin;
static char           *abc_string;
static struct pr_size  abc_size;
static int             abc_len;

struct pixfont	      *iced_abc_font;
  
#define abc_home        iced_abc_font->pf_char[*abc_string].pc_home.y;

/************************************************************************/
/* proof subwindow declarations                                         */
/************************************************************************/

Canvas			 iced_proof;
struct pixrect		*iced_fill_sq_pr;
struct pixrect		*iced_fill_ci_pr;
struct pixrect		*iced_proof_pr = &iced_root_gray_patch;
static struct pixwin	*proof_pixwin;

#define	ICON_LEFT_OFFSET	86 
#define	ICON_TOP_OFFSET		14
#define	CURSOR_LEFT_OFFSET	110 
#define	CURSOR_TOP_OFFSET	38

/************************************************************************/
/* canvas subwindow declarations                                        */
/************************************************************************/

Canvas			 iced_canvas;
struct pixwin		*iced_canvas_pw;
struct pixrect		*iced_canvas_pr; /* points to iced_icon_pr or
					  * iced_new_cursor_pr */

CELL_POS iced_dirty_ul_cell, iced_dirty_dr_cell;

int			iced_cell_count;

static short		cell_size;

static short new_cursor_array[16];	
mpr_static(iced_new_cursor_pr, 16, 16, 1, new_cursor_array);
struct cursor   iced_new_cursor  =  { 0, 0, PIX_SRC, &iced_new_cursor_pr };

static short icon_array[256];
mpr_static(iced_icon_pr, 64, 64, 1, icon_array);

static short temp_array[256];
mpr_static(iced_temp_pr, 64, 64, 1, temp_array);

static short undo_array[256];
mpr_static(iced_undo_pr, 64, 64, 1, undo_array);

static void		 nullproc();

static void		 paint_subcanvas();
static void		 paint_cell();

static void		 basereader();
static void		 reset_handlers();
static void		 set_mode();
static void		 tracker();

static void		 point_feedback();
static void		 line_feedback(), line_accept(), line_cancel();
static void		 box_feedback(), box_accept(), box_cancel();
static void		 circle_feedback(), circle_accept(), circle_cancel();
static void		 text_feedback(), text_accept(), text_cancel();

static struct canvas_handlers {
		void    (*reader)();
		void    (*feedback)();
		void    (*accept)();
		void    (*cancel)();
}
	base	= { basereader, nullproc, nullproc, nullproc },
	point	= { tracker, point_feedback, nullproc, nullproc },
	line	= { tracker, line_feedback, line_accept, line_cancel },
	box	= { tracker, box_feedback, box_accept, box_cancel },
	circle	= { tracker, circle_feedback, circle_accept, circle_cancel },
	text	= { tracker, text_feedback, text_accept, text_cancel };

static struct canvas_handlers	*current = &point;

static int		 cur_x, cur_y, cur_op;
static CELL_POS		 pinned;
static SCREEN_POS		 midpoint_of_cell(),
			 paired_points[2] = { {-1, -1}, {-1, -1} };

static u_short		 pin_bits[56] = {
	0xC000, 0x0070, 0xE000, 0x00E0, 0x7000, 0x01C0, 0x3800, 0x0380,
	0x1C00, 0x0700, 0x0E00, 0x0E00, 0x0700, 0x1C00, 0x0380, 0x3800,
	0x01C0, 0x7000, 0x00E0, 0xE000, 0x0071, 0xC000, 0x003B, 0x8000,
	0x001F, 0x0000, 0x000E, 0x0000, 0x000E, 0x0000, 0x001F, 0x0000,
	0x003B, 0x8000, 0x0071, 0xC000, 0x00E0, 0xE000, 0x01C0, 0x7000,
	0x0380, 0x3800, 0x0700, 0x1C00, 0x0E00, 0x0E00, 0x1C00, 0x0700,
	0x3800, 0x0380, 0x7000, 0x01C0, 0xE000, 0x00E0, 0xC000, 0x0070
};
mpr_static(iced_pin_pr, CURSOR_SIZE, CURSOR_SIZE, 1, pin_bits);

/*	The rows of this array of pixrect_ops are "backwards"
 *	because in the simplest case, the definition of paint is 1,
 *	and of erase, 0.
 */
static u_int	ops[2][3] = {
		{ PIX_NOT(PIX_SRC),
		  PIX_NOT(PIX_SRC) & PIX_DST,
		  PIX_NOT(PIX_SRC) ^ PIX_DST },
		{ PIX_SRC, 
		  PIX_SRC | PIX_DST, 
		  PIX_SRC ^ PIX_DST } 
};

static void
nullproc() { return; }

/**************************************************************************/
/* proof section                                                          */
/**************************************************************************/

iced_init_proof() 
{
   iced_proof = window_create(iced_base_frame, CANVAS,
		WIN_RIGHT_OF,		iced_canvas,
		WIN_BELOW,		iced_panel,
		WIN_WIDTH,		iced_proof_width,
		WIN_HEIGHT,	  	iced_proof_height,
		WIN_EVENT_PROC,		proof_event_proc,
                WIN_ERROR_MSG, 		"Unable to create proof canvas\n",
		CANVAS_FAST_MONO,	TRUE,
		0);
   if (iced_proof == NULL) { 
                (void)fprintf(stderr,"Unable to create proof canvas\n"); 
                exit(1); 
   }
   proof_pixwin = canvas_pixwin(iced_proof);
}

/* ARGSUSED */
static
proof_event_proc(win, event)
Window win;
Event *event;
{
   if (iced_state != CURSOR)
      return;

   switch (event_action(event)) {
      case LOC_WINENTER:
	 (void)pw_replrop(proof_pixwin, 0, 0, iced_proof_width, iced_proof_height, 
		    PIX_SRC, iced_proof_pr, 0, 0);
	 break;
      case LOC_WINEXIT:
	 iced_paint_proof_rect();
	 break;
   }
}

iced_paint_proof() {
   (void)pw_replrop(proof_pixwin, 0, 0, iced_proof_width, iced_proof_height, 
	      PIX_SRC, iced_proof_pr, 0, 0);
   iced_paint_proof_rect();
}


iced_paint_proof_rect() {
   if (iced_state == ICONIC) {
      (void)pw_replrop(proof_pixwin, ICON_LEFT_OFFSET, ICON_TOP_OFFSET,
		 64, 64, PIX_SRC, iced_proof_pr, 0, 0);
      (void)pw_write(proof_pixwin, ICON_LEFT_OFFSET, ICON_TOP_OFFSET, 
	       64, 64, iced_new_cursor.cur_function, &iced_icon_pr, 0, 0);
   } else {
      (void)pw_write(proof_pixwin, CURSOR_LEFT_OFFSET, CURSOR_TOP_OFFSET, 
	       16, 16, PIX_SRC, iced_proof_pr, 0, 0);
      (void)pw_write(proof_pixwin, CURSOR_LEFT_OFFSET, CURSOR_TOP_OFFSET, 
	       16, 16, iced_new_cursor.cur_function, &iced_new_cursor_pr, 0, 0);
      (void)window_set(iced_proof, WIN_CURSOR, &iced_new_cursor, 0);
   }
}

/************************************************************************/
/* canvas section                                                       */
/************************************************************************/

iced_init_canvas() {

   iced_canvas = window_create(iced_base_frame, CANVAS,
		WIN_X,                   0,
	 	WIN_BELOW,               iced_mouse_panel,
		WIN_HEIGHT,              CANVAS_SIDE, 
		WIN_WIDTH,               CANVAS_SIDE, 
		WIN_CURSOR,		 &iconedit_main_cursor,
		WIN_CONSUME_PICK_EVENTS, LOC_DRAG, LOC_STILL, 0,
		WIN_EVENT_PROC,          canvas_event_proc,
                WIN_ERROR_MSG, 		 "Unable to create iced canvas\n",
		CANVAS_FAST_MONO,	 TRUE,
		0);
   if (iced_canvas == NULL) {
                (void)fprintf(stderr,"Unable to create iced canvas\n");
                exit(1);
   }
   iced_canvas_pw = canvas_pixwin(iced_canvas);

   current = &base;
   cur_x = cur_y = -1;
}

/************************************************************************/
/* iced_set_state -- either CURSOR or ICON                                   */
/************************************************************************/

iced_set_state(new_state) 
int new_state; 
{

   if (iced_state == new_state)
      return;

   (void)panel_set_value(iced_size_item,new_state);
   if ((iced_state = new_state) == CURSOR)  {
      iced_canvas_pr  = &iced_new_cursor_pr;
      cell_size  = CURSOR_SIZE;
      (void)window_set(iced_proof, WIN_CURSOR, &iced_new_cursor, 0);
   } else {
      iced_canvas_pr  = &iced_icon_pr;
      cell_size  = ICON_SIZE;
      (void)window_set(iced_proof, WIN_CURSOR, &iconedit_main_cursor, 0);
   }
   iced_cell_count = CANVAS_DISPLAY / cell_size;
   (void)backup();
   iced_paint_proof();
   iced_paint_canvas();
}

/************************************************************************/
/* canvas painting routines                                             */
/************************************************************************/

void
iced_paint_canvas()
{
   if (iced_state == ICONIC && iced_icon_canvas_is_clear) {
      iced_clear_proc();
      return;
   }

   if (iced_state == ICONIC) {
      window_set(iced_base_frame, FRAME_LABEL, "painting image...", 0);
      iced_set_cursors_hourglass();
   }

   (void)pw_writebackground(iced_canvas_pw, 0, 0, CANVAS_SIDE, CANVAS_SIDE, PIX_CLR);
   if (panel_get_value(iced_grid_item)) 
      iced_paint_grid(PIX_SET);
   else
      iced_paint_border();
   paint_subcanvas(0,0,iced_cell_count-1,iced_cell_count-1);

   if (iced_state == ICONIC) {
      window_set(iced_base_frame, FRAME_LABEL, "iconedit", 0);
      iced_set_cursors_normal();
   }
}

static void
paint_subcanvas(x0, y0, x1, y1) 
int x0, y0, x1, y1; 
{
   register int	x, y;

   pw_batch(iced_canvas_pw, PW_ALL);
   for (y = y0; y <= y1; y++) {
      for (x = x0; x <= x1; x++) 
	 paint_cell(x, y, pr_get(iced_canvas_pr, x,y));
      pw_show(iced_canvas_pw);
   }
   pw_batch(iced_canvas_pw, PW_NONE);
}

static void
paint_cell(x, y, color)
int x, y, color;
{
   register int	dx, dy, dim;

   if (color) color = 3;
   dx               = CANVAS_MARGIN + cell_size*x + 1;
   dy               = CANVAS_MARGIN + cell_size*y + 1;
   dim              = cell_size - 1;
   (void)pw_rop(iced_canvas_pw, dx, dy, dim, dim, PIX_SRC, iced_patch_prs[color], 1, 1);
}

void
iced_paint_border()
{
	struct rect	 r;

	rect_construct(&r, CANVAS_MARGIN, CANVAS_MARGIN,
			   iced_cell_count * cell_size + 1,
			   iced_cell_count * cell_size + 1);

        pw_batch(iced_canvas_pw, PW_ALL);
	(void)pw_vector(iced_canvas_pw, CANVAS_MARGIN, CANVAS_MARGIN,
				 rect_right(&r), CANVAS_MARGIN,
				 PIX_SET, 1);
	(void)pw_vector(iced_canvas_pw, CANVAS_MARGIN, CANVAS_MARGIN,
				 CANVAS_MARGIN, rect_bottom(&r),
				 PIX_SET, 1);
	(void)pw_vector(iced_canvas_pw, rect_right(&r), CANVAS_MARGIN,
				 rect_right(&r), rect_bottom(&r),
				 PIX_SET, 1);
	(void)pw_vector(iced_canvas_pw, CANVAS_MARGIN, rect_bottom(&r),
				 rect_right(&r), rect_bottom(&r),
				 PIX_SET, 1);
	pw_show(iced_canvas_pw);
        pw_batch(iced_canvas_pw, PW_NONE);
}

void
iced_paint_grid(op)
register u_int op;
{
   register int	i, beg, end, center, count, stub;
   int c_count = CANVAS_DISPLAY / CURSOR_SIZE;

   beg    = CANVAS_MARGIN;
   end    = CANVAS_MARGIN + c_count * CURSOR_SIZE;
   center = ((end-beg)/2) + 12;
   count  = 0;

   pw_batch(iced_canvas_pw, PW_ALL);
   for (i = beg; i <= end; i += CURSOR_SIZE)  {
      if (!(count % 4))
	 stub = 7;
      else if (!(count % 2))
	 stub = 3;
      else
	 stub = 0;
      if (count == 16 || count == 0) stub = 0;
      if (count == 8) {
	 (void)pw_vector(iced_canvas_pw, i, beg-stub, i, center-2, (int)op, NULL);
	 (void)pw_vector(iced_canvas_pw, i, center+2, i, end+stub, (int)op, NULL);
	 (void)pw_vector(iced_canvas_pw, beg-stub, i, center-2, i, (int)op, NULL);
	 (void)pw_vector(iced_canvas_pw, center+2, i, end+stub, i, (int)op, NULL);
      } else {
	 (void)pw_vector(iced_canvas_pw, i, beg-stub, i, end+stub, (int)op, NULL);
	 (void)pw_vector(iced_canvas_pw, beg-stub, i, end+stub, i, (int)op, NULL);
      }
      count++;
   }
   pw_show(iced_canvas_pw);
   pw_batch(iced_canvas_pw, PW_NONE);
}

static
undo()
{
   (void)pr_rop(&iced_temp_pr,0,0,iced_cell_count,iced_cell_count,PIX_SRC,iced_canvas_pr,0,0);
   (void)pr_rop(iced_canvas_pr,0,0,iced_cell_count,iced_cell_count,PIX_SRC,&iced_undo_pr,0,0);
   (void)pr_rop(&iced_undo_pr,0,0,iced_cell_count,iced_cell_count,PIX_SRC,&iced_temp_pr,0,0);
   iced_paint_proof_rect();
   if (iced_dirty_ul_cell.x < 0) iced_dirty_ul_cell.x = 0;
   if (iced_dirty_ul_cell.y < 0) iced_dirty_ul_cell.y = 0;
   if (iced_dirty_dr_cell.x > iced_cell_count-1) iced_dirty_dr_cell.x = iced_cell_count-1;
   if (iced_dirty_dr_cell.y >= iced_cell_count-1) iced_dirty_dr_cell.y = iced_cell_count-1;
   paint_subcanvas(iced_dirty_ul_cell.x, iced_dirty_ul_cell.y,
		   iced_dirty_dr_cell.x, iced_dirty_dr_cell.y);
}

/************************************************************************/
/* canvas_event_proc                                                    */
/************************************************************************/

/* ARGSUSED */
static
canvas_event_proc(win, event)
Window win;
Event  *event;
{
   if (event_action(event) == LOC_RGNENTER) {
     abc_get_info();
     return;
   }
   (*current).reader(event);
}

static void
basereader(event)
Event  *event;
{
   if (win_inputnegevent(event))
      return;

   switch (event_action(event)) {
      case MS_LEFT:
	 cur_op = 1;
	 if (iced_state == ICONIC) 
	  iced_icon_canvas_is_clear = FALSE;
	 break;
      case MS_MIDDLE:
	 cur_op = 0;
	 break;
      case MS_RIGHT:
	 undo();
	 return;
      default:
	 return;		/* ignore all other input  */
   }
   cur_x = cur_y = -1;
   set_mode();
   (*current).feedback(event);
}

static void
tracker(event)
Event  *event;
{
   if (win_inputnegevent(event))  {
      /*  mouse button up */
      switch (event_action(event)) {
	 case MS_LEFT:		
	 case MS_MIDDLE:
	    (*current).accept();
	    reset_handlers();
	    iced_paint_proof_rect();
	    paired_points[0].x = -1;
      }
      return;
   }

   switch (event_action(event)) {

      case LOC_WINEXIT:	
	 if (current == &point)
	    /* paint the proof rect to get any points not yet painted */
	    iced_paint_proof_rect();
	 (*current).cancel();
	 reset_handlers();
	 return;

      case MS_LEFT:
      case MS_MIDDLE:
	 /* two buttons are down, so cancel */
	 (*current).cancel();
	 reset_handlers();
	 return;

      case LOC_STILL:
      case LOC_DRAG:
	 (*current).feedback(event);
	 return;
      }
}

static void
set_mode()
{
   switch (panel_get_value(iced_mode_item))  {
      case MODE_POINT: 
	  current = &point;
	  break;
      case MODE_LINE: 
	  current = &line;
	  break;
      case MODE_RECTANGLE: 
	  current = &box;
	  break;
      case MODE_CIRCLE: 
	  current = &circle;
	  break;
      case MODE_TEXT: 
	  current = &text;
	  break;
   }
}

static void
reset_handlers()
{
   current = &base;
   cur_op  = -1;
}

static SCREEN_POS
midpoint_of_cell(src)
	CELL_POS	*src;
{
	SCREEN_POS	 result;

	result.x = src->x*cell_size + CANVAS_MARGIN + cell_size/2;
	result.y = src->y*cell_size + CANVAS_MARGIN + cell_size/2;
	return result;
}

static SCREEN_POS
origin_of_cell(src)
	CELL_POS	*src;
{
	SCREEN_POS	 result;

	result.x = src->x*cell_size + CANVAS_MARGIN;
	result.y = src->y*cell_size + CANVAS_MARGIN;
	return result;
}

/* ARGSUSED */
static CELL_POS
cell_of_point(point_local)
	SCREEN_POS	*point_local;
{
	CELL_POS	 result;

	result.x = (point_local->x - CANVAS_MARGIN) / cell_size;
	result.y = (point_local->y - CANVAS_MARGIN) / cell_size;
	return result;
}

static
cell_of_event(ie, dest_cell)
	Event		*ie;
	CELL_POS	*dest_cell;
{
	if (ie->ie_locx < CANVAS_MARGIN || ie->ie_locy < CANVAS_MARGIN) {
	    return FALSE;
	}
	dest_cell->x = (ie->ie_locx - CANVAS_MARGIN) / cell_size;
	dest_cell->y = (ie->ie_locy - CANVAS_MARGIN) / cell_size;
	if (dest_cell->x >= iced_cell_count || dest_cell->y >= iced_cell_count) {
		dest_cell->x = dest_cell->y = -1;
		return FALSE;
	}
	if (dest_cell->x == cur_x && dest_cell->y == cur_y) 
		return FALSE;
	cur_x = dest_cell->x;
	cur_y = dest_cell->y;
	return TRUE;
}

static void
pin_cell(cell)
CELL_POS	*cell;
{
   SCREEN_POS	 origin;

   origin = origin_of_cell(cell);
   (void)pw_write(iced_canvas_pw, origin.x+1, origin.y+1,
            cell_size - 2, cell_size - 2,
	    PIX_SRC^PIX_DST, 
	    &iced_pin_pr, 
	    (CURSOR_SIZE - cell_size)/2 + 1,
	    (CURSOR_SIZE - cell_size)/2 + 1  );
}

#define LOOKING_FOR_FIRST  (paired_points[0].x == -1)
static 
get_first_cell(ie)
Event  *ie;
{
   CELL_POS new_cell;
   int		action = event_action(ie);

   if (win_inputposevent(ie) && 
       (action == MS_LEFT || 
	action == MS_MIDDLE || 
	action == LOC_DRAG
       ) && 
       cell_of_event(ie, &new_cell)
      )  
   {
      paired_points[0] = midpoint_of_cell(&new_cell);
      paired_points[1] = paired_points[0];
      pinned = new_cell;
      pin_cell(&new_cell);
      return TRUE;			/* set it if you can	*/
   }		    /*    if not, it will come round again	*/
   return FALSE;
}

static void
point_feedback(ie)
struct	inputevent  *ie;
{
   CELL_POS	     new;

   if (event_action(ie) == LOC_STILL)  {
      iced_paint_proof_rect();
      return;
   }
   if (cell_of_event(ie, &new))  {
      if (event_action(ie) != LOC_DRAG) {
	 (void)backup();
	 iced_dirty_ul_cell.x = new.x;
	 iced_dirty_ul_cell.y = new.y;
	 iced_dirty_dr_cell.x = new.x;
	 iced_dirty_dr_cell.y = new.y;
      } else {
	 iced_dirty_ul_cell.x = min(iced_dirty_ul_cell.x,new.x);
	 iced_dirty_ul_cell.y = min(iced_dirty_ul_cell.y,new.y);
	 iced_dirty_dr_cell.x = max(iced_dirty_dr_cell.x,new.x);
	 iced_dirty_dr_cell.y = max(iced_dirty_dr_cell.y,new.y);
      }
      paint_cell(new.x, new.y, cur_op);
      (void)pr_put(iced_canvas_pr, new.x, new.y, cur_op);
   }
}

static void
line_xor(ends)
register SCREEN_POS ends[] ;
{
   (void)pw_vector(iced_canvas_pw, 
	     ends[0].x, ends[0].y, ends[1].x, ends[1].y,
	     PIX_SRC^PIX_DST, 1);
}

static void
line_feedback(ie)
Event  *ie;
{
   CELL_POS     new_cell;

   if (LOOKING_FOR_FIRST)  {
      (void)get_first_cell(ie);
      return;
   }
   if (cell_of_event(ie, &new_cell) ) {
      line_xor(paired_points);
      paired_points[1] = midpoint_of_cell(&new_cell);
      line_xor(paired_points);
   }
}

static void
line_accept()
{
   line_xor(paired_points);
   paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));
   draw_line_of_cells(pinned.x, pinned.y, cur_x, cur_y);
   iced_dirty_ul_cell.x = min(pinned.x,cur_x);
   iced_dirty_ul_cell.y = min(pinned.y,cur_y);
   iced_dirty_dr_cell.x = max(pinned.x,cur_x);
   iced_dirty_dr_cell.y = max(pinned.y,cur_y);
}

static void
line_cancel()
{
   line_xor(paired_points);
   paired_points[0].x = -1;
   paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));
}

/*
 * This function is the preprocessor to the Bresenham line drawing routine.
 * If the slope of the line is < 1, then x is passed as the independent
 *   variable; otherwise y is the independent variable.
 */
static
draw_line_of_cells(x1, y1, x2, y2)
	register int	x1, y1,	/* coordinates of the first cell  */
			x2, y2;	/* coordinates of the second cell */
{
	register int	dx, dy;

	(void)backup();
	dx = x2 - x1;
	if (dx < 0) dx = -dx;
	dy = y2 - y1;
	if (dy < 0) dy = -dy;

	if (dy < dx) {			/* slope is < 1 */
	    bresenham(x1, y1, x2, y2, dx, dy, 1);
	} else {			/* slope is >= 1 */
	    bresenham(y1, x1, y2, x2, dy, dx, 0);
	}
}

/*
 * This function draws a line in the bitmap by using a generalized
 *   Bresenham's method.  This method determines for each point between the
 *   independent endpoints a dependent value.  Thus for slopes >=1 y is
 *   the independent variable and x values are calculated; for slopes < 1
 *   x is the independent variable. (NOTE: the determination of the independent
 *   and dependent variable is done by draw_line. )
 */

static
bresenham (ind1, dep1, ind2, dep2, d_ind, d_dep, is_x)
int ind1, dep1,		  /* the 1st pt   */
    ind2, dep2,		  /* the 2nd pt   */
    d_ind,		  /* difference between the indpendent variable */
    d_dep,		  /* difference between the dependent variable */
    is_x;		  /* = 1 if x is the indpendent variable */
{
	register int
		incr1,		/* increment if d < 0   */
		incr2,		/* increment if d >= 0  */
		incr_dep,	/* increment the dependent variable;
				   either 1 (positive slope) or -1 (neg.) */
		d,		/* determines if the dependent variable
				   should be increment */
		ind,		/* the current value of the independent
				   variable  */
		dep,		/* the current value of the dependent
				   variable	 */
		ind_end,	/* the stopping point of the independent
				   variable  */
		y,		/* the y value of the pixel */
		x;		/* the x value of the pixel */

	d = 2 * d_dep - d_ind;	/* calulate the initial d value */
	incr1 = 2 * d_dep;
	incr2 = 2 * (d_dep - d_ind);
	/*
	 * Find which point has the lowest independent variable and use it
	 *   as the starting point.
	 */
	if (ind1 > ind2) {
	    ind = ind2;
	    dep = dep2;
	    ind_end = ind1;
	    incr_dep = (dep2 > dep1) ? -1 : 1;
	} else {
	    ind = ind1;
	    dep = dep1;
	    ind_end = ind2;
	    incr_dep = (dep1 > dep2) ? -1 : 1;
	}
	do {
	    /*
	     * x and y are assigned depending on whether x is the dependent or
	     *   independent variable.
	     */
	    if (is_x) {
		x = ind;
		y = dep;
	    } else {
		x = dep;
		y = ind;
	    }
	    paint_cell(x, y, cur_op);
	    (void)pr_put(iced_canvas_pr, x, y, cur_op);
	    if (d < 0)
		d += incr1;
	    else {
		dep += incr_dep;
		d += incr2;
	    }
	} while (ind++ < ind_end);
}

static void
box_solid(cells) CELL_POS cells[]; {

   u_int		op, result;

   result = (u_int)panel_get_value(iced_fill_op_item);
   op     = ops[cur_op][result];
   (void)pr_replrop(iced_canvas_pr, cells[0].x, cells[0].y,
	      cells[1].x - cells[0].x + 1,
	      cells[1].y - cells[0].y + 1,
	      (int)op, iced_fill_sq_pr, cells[0].x, cells[0].y);
   paint_subcanvas(cells[0].x, cells[0].y, cells[1].x, cells[1].y);
}

static void
box_border(cells)
CELL_POS cells[];
{
   register u_int i, j;

   for (i = cells[0].x; i <= cells[1].x; i++) {
      paint_cell((int)i, (int)cells[0].y, cur_op);
      (void)pr_put(iced_canvas_pr, (int)i, cells[0].y, cur_op);
      if (i == cells[0].x || i == cells[1].x) {
	 for (j = cells[0].y+1; j < cells[1].y; j++)  {
	    paint_cell((int)i, (int)j, cur_op);
	    (void)pr_put(iced_canvas_pr, (int)i, (int)j, cur_op);
	 }
      }
      paint_cell((int)i, cells[1].y, cur_op);
      (void)pr_put(iced_canvas_pr, (int)i, cells[1].y, cur_op);
   }
}

static void 
box_xor(cor) 
CELL_POS cor[]; 
{
   register int x0 = cor[0].x;
   register int y0 = cor[0].y;
   register int x1 = cor[1].x;
   register int y1 = cor[1].y;

   /*
   if (x1>x0 && y1>y0) {
      rect_construct(&r, x0, y0, x1-x0+1, y1-y0+1);
   } else if (x1>x0 && y1<=y0) {
      rect_construct(&r, x0, y0, x1-x0+1, y0-y1+1);
   } else if (x1<=x0 && y1>y0) {
      rect_construct(&r, x0, y0, x0-x1+1, y1-y0+1);
   } else if (x1<=x0 && y1<=y0) {
      rect_construct(&r, x0, y0, x0-x1+1, y0-y1+1);
   }
   */

   /*
   pw_lock(iced_canvas_pw, &r);
   */
   (void)pw_vector(iced_canvas_pw, x0, y0, x0, y1, PIX_SRC^PIX_DST, 1);
   (void)pw_vector(iced_canvas_pw, x0, y0, x1, y0, PIX_SRC^PIX_DST, 1);
   (void)pw_vector(iced_canvas_pw, x0, y1, x1, y1, PIX_SRC^PIX_DST, 1);
   (void)pw_vector(iced_canvas_pw, x1, y0, x1, y1, PIX_SRC^PIX_DST, 1);
   /*
   pw_unlock(iced_canvas_pw);
   */
}

static void
box_feedback(event) 
Event  *event; 
{
	CELL_POS     new_cell;

	if (LOOKING_FOR_FIRST)  {
		(void)get_first_cell(event);
		return;
	}
	if (!cell_of_event(event, &new_cell) )
		return;
	box_xor(paired_points);
	paired_points[1] = midpoint_of_cell(&new_cell);
	box_xor(paired_points);
}

static void
box_accept() {

   CELL_POS  corner[2];

   box_xor(paired_points);
   paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));

   if (pinned.x > cur_x) {
	  corner[0].x = cur_x;
	  corner[1].x = pinned.x;
   } else {
	  corner[0].x = pinned.x;
	  corner[1].x = cur_x;
   }
   if (pinned.y > cur_y) {
	  corner[0].y = cur_y;
	  corner[1].y = pinned.y;
   } else {
	  corner[0].y = pinned.y;
	  corner[1].y = cur_y;
   }

   (void)backup();
   if ((int)(panel_get_value(iced_fill_square_item)) == NONE ) {
      box_border(corner);
   } else {
      box_solid(corner);
   }
   iced_dirty_ul_cell.x = corner[0].x;
   iced_dirty_ul_cell.y = corner[0].y;
   iced_dirty_dr_cell.x = corner[1].x;
   iced_dirty_dr_cell.y = corner[1].y;
}

static void
box_cancel()
{
   box_xor(paired_points);
   paired_points[0].x = -1;
   paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));
}

/**************************************************************************/
/* text stuff                                                             */
/**************************************************************************/

static void
text_feedback(event)
Event *event;
{
   u_int            op;

   if (event_action(event) == LOC_STILL) {
      op = ops[cur_op][(u_int)panel_get_value(iced_fill_op_item)];
      (void)pr_rop(&iced_temp_pr,0,0,64,64,PIX_SRC,&iced_icon_pr,0,0);
      write_str_to_pr(&iced_temp_pr, abc_cell_origin.x, abc_cell_origin.y,
		      abc_string,iced_abc_font,op);
      if (iced_state == ICONIC) {
	 (void)pw_replrop(proof_pixwin, ICON_LEFT_OFFSET, ICON_TOP_OFFSET,
		    64, 64, PIX_SRC, iced_proof_pr, 0, 0);
	 (void)pw_write(proof_pixwin, ICON_LEFT_OFFSET, ICON_TOP_OFFSET, 
		  64, 64, iced_new_cursor.cur_function, &iced_temp_pr, 0, 0);
      } else {
	 (void)pw_write(proof_pixwin, CURSOR_LEFT_OFFSET, CURSOR_TOP_OFFSET, 
		  16, 16, PIX_SRC, iced_proof_pr, 0, 0);
	 (void)pw_write(proof_pixwin, CURSOR_LEFT_OFFSET, CURSOR_TOP_OFFSET, 
		  16, 16, iced_new_cursor.cur_function, &iced_temp_pr, 0, 0);
	 (void)window_set(iced_proof, WIN_CURSOR, &iced_new_cursor, 0);
      }
      return;
   }

   if (LOOKING_FOR_FIRST)  {
      int	action = event_action(event);
      if (win_inputposevent(event) && 
	  (action == MS_LEFT   || 
	   action == MS_MIDDLE || 
	   action == LOC_DRAG
	  ) && 
	  cell_of_event(event, &abc_cell_origin)
	 )  
      {
	 if (!strlen(abc_string))
	    iced_panelmsg("No string to insert;","please enter desired text.");
	 compute_abc_feedback();
         box_xor(paired_points);
      }
   } else if (cell_of_event(event, &abc_cell_origin)) {
      box_xor(paired_points);
      compute_abc_feedback();
      box_xor(paired_points);
   }
}

static
compute_abc_feedback() {
   abc_get_info();
   if (iced_abc_font == iced_screenr7  ||
       iced_abc_font == iced_screenr11 ||
       iced_abc_font == iced_gallantr19
      )
      abc_cell_origin.y++;
   abc_h_cell_origin.x   = abc_cell_origin.x;
   abc_h_cell_origin.y   = abc_cell_origin.y + abc_home;
   abc_feedback_origin.x = abc_h_cell_origin.x;
   abc_feedback_origin.y = abc_h_cell_origin.y < CANVAS_MARGIN ?
      0 : abc_h_cell_origin.y;
   paired_points[0]   = midpoint_of_cell(&abc_h_cell_origin);
   paired_points[1].x = paired_points[0].x + ((abc_size.x-1)*cell_size);
   paired_points[1].y = paired_points[0].y + ((abc_size.y-1)*cell_size);
}

static
abc_get_info() {
   abc_string = (char *)panel_get(iced_abc_item,PANEL_VALUE);
   abc_len    = strlen(abc_string);
   abc_size   = pf_textwidth(abc_len,iced_abc_font,abc_string);
}

static void
text_accept()
{
   struct pr_pos extent;
   u_int         op;

   box_xor(paired_points);
   op       = ops[cur_op][(u_int)panel_get_value(iced_fill_op_item)];
   (void)backup();
   write_str_to_pr(iced_canvas_pr,abc_cell_origin.x,abc_cell_origin.y,
		   abc_string,iced_abc_font,op);
   extent.x = abc_h_cell_origin.x + abc_size.x;
   extent.y = abc_h_cell_origin.y + abc_size.y;
   if (extent.x > iced_cell_count-1)
      extent.x = iced_cell_count-1;
   if (extent.y >= iced_cell_count-1)
      extent.y = iced_cell_count-1;
   paint_subcanvas(abc_feedback_origin.x, abc_feedback_origin.y,
		   extent.x, extent.y);
   iced_dirty_ul_cell.x = abc_feedback_origin.x;
   iced_dirty_ul_cell.y = abc_feedback_origin.y;
   iced_dirty_dr_cell.x = extent.x;
   iced_dirty_dr_cell.y = extent.y;
}

static 
write_str_to_pr(pr,left_offset,vertical_offset,string,font,op)
struct pixrect *pr;
int             left_offset;
int             vertical_offset;
char           *string;
struct pixfont *font;
u_int           op;
{
   struct pr_prpos loc;	
   loc.pr    = pr;
   loc.pos.x = left_offset;
   loc.pos.y = vertical_offset; 
   (void)pf_text(loc,(int)op,font,string);
}

static void
text_cancel()
{
   box_xor(paired_points);
   paired_points[0].x = -1;
}

/**************************************************************************/
/* circle stuff                                                           */
/**************************************************************************/

static int
circle_radius(cell0, cell1)
CELL_POS *cell0, *cell1;
{
   register int	dx, dy;

   dx = cell1->x - cell0->x;
   dy = cell1->y - cell0->y;
   return (int) (sqrt ( (double) (dx*dx + dy*dy) ) + 0.5 );
}

static void
circle_xor(points)
SCREEN_POS points[];
{
   line_xor(points);
}

static void
circle_feedback(event)
Event  *event;
{
   CELL_POS	new_cell;

   if (LOOKING_FOR_FIRST)  {
      (void)get_first_cell(event);
      return;
   }

   if (cell_of_event(event, &new_cell) )  {
      circle_xor(paired_points);
      paired_points[1] = midpoint_of_cell(&new_cell);
      circle_xor(paired_points);
   }
}

static void
circle_cancel()
{
   circle_xor(paired_points);
   paired_points[0].x = -1;
   paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));
}

static void 
circle_edge(x0, x1, y)
register int	x0, x1, y;
{
   if ( y >= 0 && y < iced_cell_count ) {
      if (x0 >= 0) {
	 (void)pr_put(iced_canvas_pr, x0, y, cur_op);
	 paint_cell(x0, y, cur_op);
      }
      if (x1 < iced_cell_count ) {
	 (void)pr_put(iced_canvas_pr, x1, y, cur_op);
	 paint_cell(x1, y, cur_op);
      }
   }
}

static void
circle_edge_pair(x,y)
register int	x, y;
{
   register int	mid_x = pinned.x, mid_y = pinned.y;

   circle_edge(mid_x - x, mid_x + x, mid_y - y);
   circle_edge(mid_x - y, mid_x + y, mid_y - x);
   circle_edge(mid_x - x, mid_x + x, mid_y + y);
   circle_edge(mid_x - y, mid_x + y, mid_y + x);
}

#define MAX_LINES	((CANVAS_DISPLAY / ICON_SIZE) * 4)
static struct fill_line  {
   int	left;
   int	right;
   int	y;
}		 scan_lines[MAX_LINES],
		*free_line = scan_lines;

#define CLAMP(n) ((n) < 0) ? 0 :	\
			((n) >= iced_cell_count) ? iced_cell_count-1 : (n)
static void
circle_fill_pair(x, y)
	register int	x, y;
{
	register struct fill_line *line_ptr = free_line;

	free_line += 4;

	line_ptr->left	= CLAMP(pinned.x - x);
	line_ptr->right = CLAMP(pinned.x + x);
	line_ptr->y	= CLAMP(pinned.y - y);

	++line_ptr;
	line_ptr->left  = CLAMP(pinned.x - y); 
	line_ptr->right = CLAMP(pinned.x + y); 
	line_ptr->y     = CLAMP(pinned.y - x);

	++line_ptr;
	line_ptr->left  = CLAMP(pinned.x - y);
	line_ptr->right = CLAMP(pinned.x + y);
	line_ptr->y     = CLAMP(pinned.y + x);

	++line_ptr;
	line_ptr->left  = CLAMP(pinned.x - x);
	line_ptr->right = CLAMP(pinned.x + x);
	line_ptr->y     = CLAMP(pinned.y + y);
}

static int
compare_lines(l1, l2)
	register struct fill_line  *l1, *l2;
{
	if (l1->y < l2->y)
		return -1;
	else if (l1->y > l2->y)
		return 1;
	else if (l1->left < l2->left 
		|| l1->right > l2->right)
		return -1;
	else if (l1->left > l2->left
		|| l1->right  <  l2->right)
		return 1;
	else return 0;
}

static void
circle_fill()
{
	register struct fill_line *line_ptr = scan_lines;
	register int		   len, op, y = -1;

	op = ops[cur_op][(int)panel_get_value(iced_fill_op_item)];

	qsort((char *)(LINT_CAST(scan_lines)), free_line - scan_lines,
		    sizeof(struct fill_line), compare_lines);
	while (line_ptr < free_line)  {
		register int	x0;

		if (line_ptr->y != y) {
			x0 = line_ptr->left;
			y = line_ptr->y;
			len = line_ptr->right - x0 + 1;
			(void)pr_rop(iced_canvas_pr, x0, y, len, 1, op, iced_fill_ci_pr, x0, y);
			paint_subcanvas(x0, y, line_ptr->right, y);
		}
		line_ptr++;
	}
}

static void
circle_accept()
{
	int		d, x, y, filled;
	CELL_POS	cell2;

	circle_xor(paired_points);
	paint_cell(pinned.x, pinned.y, pr_get(iced_canvas_pr, pinned.x, pinned.y));

	if (filled = (int)panel_get_value(iced_fill_circle_item) != NONE)  {
		free_line = scan_lines;
	}

/*	Basic Bresenham circle starts at pi/2 (x == 0, y == radius), and
 *	moves x and y toward each other until they meet at pi/4.  Swapping
 *	x and y gives the symmetric point within a quadrant; symmetric
 *	points in other quadrants are derived by twiddling signs of x and y.
 */
	cell2 = cell_of_point(&paired_points[1]);
	x = 0;
	y = circle_radius(&pinned, &cell2);
	d = 3 - 2*y;

        iced_dirty_ul_cell.x = pinned.x - y;
        iced_dirty_ul_cell.y = pinned.y - y;
        iced_dirty_dr_cell.x = pinned.x + y;
        iced_dirty_dr_cell.y = pinned.y + y;
	(void)backup();
	while ( x < y ) {
		if (filled) {
			circle_fill_pair(x, y);
		}  else  {
			circle_edge_pair(x, y);
		}
		if (d < 0)
			d += 4*x + 6;
		else {
			d += 4*(x-y) +10;
			y -= 1;
		}
		x++;
	}
	if (x == y) {
		if (filled) {
			circle_fill_pair(x, y);
		}  else  {
			circle_edge_pair(x, y);
		}
	}
	if (filled) {
		circle_fill();
	}
}


