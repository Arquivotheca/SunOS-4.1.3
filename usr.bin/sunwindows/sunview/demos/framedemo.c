#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)framedemo.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Frame displayer in windows.  Reads in all the
 *			files of form "frame.xxx" in working directory &
 *			displays them like a movie.
 *			See constants below for limits.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/gfxsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define	MAXFRAMES	1000
#define	FRAMEWIDTH	256
#define	FRAMEHEIGHT	256
#define	USEC_INC	50000
#define	SEC_INC		1

static	struct pixrect *mpr[MAXFRAMES];
static	struct timeval timeout = {SEC_INC,USEC_INC}, timeleft;
static	char s[] = "frame.xxx";
static	struct gfxsubwindow *gfx;
static	int frames, framenum, ximage, yimage;
static	struct rect rect;

char	*sprintf();
void	pw_use_fast_monochrome();

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
framedemo_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	int	fd, framedemo_selected();
	struct	inputmask im;
	fd_set ibits,obits,ebits;

	for (frames = 0; frames < MAXFRAMES; frames++) {
		(void)sprintf(&s[6], "%d", frames + 1);
		fd = open(s, O_RDONLY, 0);
		if (fd == -1) {
			break;
		}
		mpr[frames] = (Pixrect *)(LINT_CAST(
			mem_create(FRAMEWIDTH, FRAMEHEIGHT, 1)));
		(void)read(fd, (char *)(LINT_CAST(mpr_d(mpr[frames])->md_image)),
		    FRAMEWIDTH*FRAMEHEIGHT/8);
		(void)close(fd);
#ifdef i386
		{
		char *mpr_ptr = (char *)(LINT_CAST(mpr_d(mpr[frames])->md_image));
		int i;

		for (i=0 ; i<FRAMEWIDTH*FRAMEHEIGHT/8 ; i++) bitflip8(mpr_ptr++);
		}
#endif
		}

	if (frames == 0) {
	    (void)printf("Couldn't find any 'frame.xx' files in working directory\n");
	    return;
	}

	/*
	 * Initialize gfxsw
	 */
	gfx = gfxsw_init(0, argv);
	if (gfx == (struct gfxsubwindow *)0)
		EXIT(1);
	pw_use_fast_monochrome(gfx->gfx_pixwin);
	/*
	 * Set up input mask
	 */
	(void)input_imnull(&im);
	im.im_flags |= IM_ASCII;
	(void)gfxsw_setinputmask(gfx, &im, &im, WIN_NULLLINK, 0, 1);
	/*
	 * Main loop
	 */
	framedemo_nextframe(1);
	timeleft = timeout;
	FD_ZERO(&ibits);FD_ZERO(&obits);FD_ZERO(&ebits);
	(void)gfxsw_select(gfx, framedemo_selected, ibits, obits, ebits, &timeleft);
	/*
	 * Cleanup
	 */
	(void)gfxsw_done(gfx);
	
	EXIT(0);
}

static
framedemo_selected(gfx_local, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx_local;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	if ((*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)) ||
	    (gfx_local->gfx_flags & GFX_RESTART)) {
		/*
		 * Our timer expired or restart is true so show next frame
		 */
		if (gfx_local->gfx_reps)
			framedemo_nextframe(0);
		else
			(void)gfxsw_selectdone(gfx_local);
	}
	if (*ibits & (1 << gfx_local->gfx_windowfd)) {
		struct	inputevent event;

		/*
		 * Read input from window
		 */
		if (input_readevent(gfx_local->gfx_windowfd, &event)) {
			perror("framedemo");
			return;
		}
		switch (event_action(&event)) {
		case 'f': /* faster usec timeout */
			if (timeout.tv_usec >= USEC_INC)
				timeout.tv_usec -= USEC_INC;
			else {
				if (timeout.tv_sec >= SEC_INC) {
					timeout.tv_sec -= SEC_INC;
					timeout.tv_usec = 1000000-USEC_INC;
				}
			}
			break;
		case 's': /* slower usec timeout */
			if (timeout.tv_usec < 1000000-USEC_INC)
				timeout.tv_usec += USEC_INC;
			else {
				timeout.tv_usec = 0;
				timeout.tv_sec += 1;
			}
			break;
		case 'F': /* faster sec timeout */
			if (timeout.tv_sec >= SEC_INC)
				timeout.tv_sec -= SEC_INC;
			break;
		case 'S': /* slower sec timeout */
			timeout.tv_sec += SEC_INC;
			break;
		case '?': /* Help */
			(void)printf("'s' slower usec timeout\n'f' faster usec timeout\n'S' slower sec timeout\n'F' faster sec timeout\n");
			/*
			 * Don't reset timeout
			 */
			return;
		default:
			(void)gfxsw_inputinterrupts(gfx, &event);
		}
	}
	*ibits = *obits = *ebits = 0;
	timeleft = timeout;
	*timer = &timeleft;
}

static
framedemo_nextframe(firsttime)
	int	firsttime;
{
	int	restarting = gfx->gfx_flags&GFX_RESTART;

	if (firsttime || restarting) {
		gfx->gfx_flags &= ~GFX_RESTART;
		(void)win_getsize(gfx->gfx_windowfd, &rect);
		ximage = rect.r_width/2-FRAMEWIDTH/2;
		yimage = rect.r_height/2-FRAMEHEIGHT/2;
		(void)pw_writebackground(gfx->gfx_pixwin, 0, 0,
		    rect.r_width, rect.r_height, PIX_CLR);
	}
	if (framenum >= frames) {
		framenum = 0;
		gfx->gfx_reps--;
	}
	(void)pw_write(gfx->gfx_pixwin, ximage, yimage, FRAMEWIDTH, FRAMEHEIGHT,
	    PIX_SRC, mpr[framenum], 0, 0);
	if (!restarting)
		framenum++;
}
