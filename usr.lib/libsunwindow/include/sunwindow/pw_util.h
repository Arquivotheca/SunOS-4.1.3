/*	@(#)pw_util.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Pw_util.h is a collection of utilities that implementors of
 * pixwin.h specified routines should use.
 */

/*
 * Loop on clipping and call rect clipping routines.
 * Users of this macro operate upon rintersect.
 */
#define	pw_begincliploop_internal(pw, rdest, rintersect, rn) \
	{ for ((rn) = (pw)->pw_clipdata->pwcd_clipping.rl_head; \
	  (rn); (rn) = (rn)->rn_next) { \
		/* \
		 * Not doing: \
		 * rl_rectoffset(&(pw)->pw_clipping, &(rn)->rn_rect, &rtemp); \
		 * cause assuming all clipping is normalized. \
		 */ \
		if (rect_intersectsrect((rdest), &(rn)->rn_rect)) { \
			(void)rect_intersection((rdest), &(rn)->rn_rect,(rintersect));

#define	pw_begincliploop(pw, rdest, rintersect) \
    { register struct rectnode *rn; \
      pw_begincliploop_internal(pw, rdest, rintersect, rn);

#define	pw_begincliploop2(pw, rdest, rintersect) \
    { register struct rectnode *rn2; \
      pw_begincliploop_internal(pw, rdest, rintersect, rn2);

/*
 * Terminates pw_begincliploop
 */
#define	pw_endcliploop()\
		}\
	}\
	}\
    }

/*
 * Check for batching and calls batch handler
 * Setup rdest; left, top, width, height are in window coordinates.
 * Lock rdest.
 */
#define	PW_SETUP(pw, rdest, label, left, top, width, height) \
	{ \
	int left_eval = (left); \
	int top_eval = (top); \
	 \
	(rdest).r_width = (width); \
	(rdest).r_height = (height); \
	if ((pw)->pw_clipdata->pwcd_batch_type != PW_NONE) { \
		(rdest).r_left = (left_eval); \
		(rdest).r_top = (top_eval); \
		(void)pw_update_batch((pw), &(rdest)); \
		goto label; \
	} \
	(rdest).r_left = PW_X_FROM_WIN((pw), (left_eval)); \
	(rdest).r_top = PW_Y_FROM_WIN((pw), (top_eval)); \
	(void)pw_lock((pw), &(rdest)); \
	(rdest).r_left = (left_eval); \
	(rdest).r_top = (top_eval); \
	}

/*
 * Go from window coordinate space to pixwin coordinate space.
 */
#define	PW_X_TO_WIN(pw, x) ((x) - (pw)->pw_clipdata->pwcd_x_offset)
#define	PW_Y_TO_WIN(pw, y) ((y) - (pw)->pw_clipdata->pwcd_y_offset)

/*
 * Go from pixwin coordinate space to window coordinate space.
 */
#define	PW_X_FROM_WIN(pw, x) ((x) + (pw)->pw_clipdata->pwcd_x_offset)
#define	PW_Y_FROM_WIN(pw, y) ((y) + (pw)->pw_clipdata->pwcd_y_offset)

/*
 * Go from window coordinate space to pixwin coordinate space.
 * Called when write to retained image that covers pixwin coordinate
 * space.  Called with pixwin destination offsets.
 */
#define	PW_RETAIN_X_OFFSET(pw, x) PW_X_FROM_WIN((pw), (x))
#define	PW_RETAIN_Y_OFFSET(pw, y) PW_Y_FROM_WIN((pw), (y))

/*
 * Go from pixwin coordinate space to window coordinate space.
 * Called with offsets when enter routines that don't go through
 * ops-vector, and thus pw_ops* adjustment.  Called with pixwin
 * destination offsets.
 */
#define	PW_X_OFFSET(pw, x) PW_X_TO_WIN((pw), (x))
#define	PW_Y_OFFSET(pw, y) PW_Y_TO_WIN((pw), (y))

#define	PW_FIXUP_TRANSLATE(pw) \
	rl_passtoparent(PW_X_FROM_WIN((pw), 0), \
	    PW_Y_FROM_WIN((pw), 0), &(pw)->pw_fixup)

#ifdef	cplus
/*
 * C Library routine specifically relating to pixel device function
 * implementation.
 */

/*
 * Make rl_fixup set to the clipped part of the bounding box of pw->pw_clipping
 */
void	pw_initfixup(struct pixwin *pw, struct rectlist *rl_fixup);

#endif

