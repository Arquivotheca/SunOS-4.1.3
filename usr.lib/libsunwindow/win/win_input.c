#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_input.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Win_input.c: Implement the input functions of the win_struct.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#ifdef KEYMAP_DEBUG
#include <sunwindow/win_input.h>
#else
#include <sunwindow/win_input.h>
#endif
#include <sunwindow/win_struct.h>
#include <sunwindow/win_ioctl.h>

extern	errno;

/* 
 * Most of the functions below return the value
 * returned by ioctl. Zero (0) denotes success.
 * Currently, the return values are being ignored 
 * (in most cases).
 */

/*
 * Input control.
 */

win_getinputmask(windowfd, im, nextwindownumber)
	int 	windowfd, *nextwindownumber;
	struct	inputmask *im;
{
	int err;
	struct	inputmaskdata ims;

	(void)werror(err = ioctl(windowfd, WINGETINPUTMASK, &ims), WINGETINPUTMASK);
	if (!err) {
	    if (im)
		*im = ims.ims_set;
	    if (nextwindownumber)
		*nextwindownumber = ims.ims_nextwindow;
	}
	return (err);
}

win_setinputmask(windowfd, im_set, im_flush, nextwindownumber)
	int 	windowfd, nextwindownumber;
	struct	inputmask *im_set, *im_flush;
{
	int err;
	struct	inputmaskdata ims;


	if (im_set)
		ims.ims_set = *im_set;
	else
		(void)input_imnull(&ims.ims_set);
	if (im_flush)
		ims.ims_flush = *im_flush;
	else
		(void)input_imnull(&ims.ims_flush);
	ims.ims_nextwindow = nextwindownumber;
	(void)werror(err = ioctl(windowfd, WINSETINPUTMASK, &ims), WINSETINPUTMASK);
	return (err);
}

/*
 * Utilities
 */
input_imnull(im)
	struct	inputmask *im;
{
	int	i;

	im->im_flags = 0;
	im->im_shifts = 0;
	for (i=0;i<IM_CODEARRAYSIZE;i++)
		im->im_inputcode[i] = 0;
	for (i=0;i<IM_SHIFTARRAYSIZE;i++)
		im->im_shiftcodes[i] = 0;
}

input_imall(im)
	struct	inputmask *im;
{
	int	i;

	(void)input_imnull(im);
	im->im_flags = IM_ASCII|IM_META;
	for (i=0;i<IM_CODEARRAYSIZE;i++)
		im->im_inputcode[i] = 1;
}

/* Keymap debugging variable; can be removed after debugging is done */

extern int keymap_blockio;

input_readevent(windowfd, event)
	int	windowfd;
	struct	inputevent *event;
{
	int	n;
	int	bufsize = sizeof(struct inputevent);
	char str[32];

	n = read(windowfd, (caddr_t)event, bufsize);
	if (n == -1)
		return(-1);
	if (n != bufsize) {
		errno = EIO;
		return(-1);
	}
		
#ifdef KEYMAP_DEBUG
		if ((keymap_debug_mask & KEYMAP_SHOW_EVENT_STREAM) && !keymap_blockio)
			printf("input_readevent: read %d '%s'\n", windowfd,
					win_keymap_event_decode(event, str));
#endif
	(void)win_keymap_map(windowfd, event);

#ifdef KEYMAP_DEBUG
	if ((keymap_debug_mask & KEYMAP_SHOW_EVENT_STREAM) && !keymap_blockio)
		printf("input_readevent: posted '%s'\n",
				win_keymap_event_decode(event,str));
#endif

	return(0);
}

/*
 * Input focus and synchronization routines.
 */
win_get_kbd_mask(windowfd, im)
	int 	windowfd;
	struct	inputmask *im;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINGETKBDMASK, im), WINGETKBDMASK);
	return (err);
}

win_set_kbd_mask(windowfd, im)
	int 	windowfd;
	struct	inputmask *im;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINSETKBDMASK, im), WINSETKBDMASK);
	return (err);
}

win_get_pick_mask(windowfd, im)
	int 	windowfd;
	struct	inputmask *im;
{
	int err; 
	(void)werror(err = ioctl(windowfd, WINGETPICKMASK, im), WINGETPICKMASK);
	return (err);
}

win_set_pick_mask(windowfd, im)
	int 	windowfd;
	struct	inputmask *im;
{
	int err;
	(void)werror(err = ioctl(windowfd, WINSETPICKMASK, im), WINSETPICKMASK);
	return (err);
}

win_get_designee(windowfd, nextwindownumber)
	int 	windowfd, *nextwindownumber;
{
	struct	inputmask im;

	(void)win_getinputmask(windowfd, &im, nextwindownumber);
}

win_set_designee(windowfd, nextwindownumber)
	int 	windowfd, nextwindownumber;
{
	struct	inputmask im;

	(void)win_getinputmask(windowfd, &im, (int *)0);
	(void)win_setinputmask(windowfd, &im, (struct inputmask *)0, nextwindownumber);
}

win_get_focus_event(windowfd, fe, shifts)
	int 	windowfd;
	Firm_event *fe;
	int	*shifts;
{
	Focus_event foe;
	int err;

	(void)werror(err = ioctl(windowfd, WINGETFOCUSEVENT, &foe), WINGETFOCUSEVENT);
	 if (!err) {
	     if (fe) {
	         fe->id = foe.id;
	         fe->value = foe.value;
	     }
	     if (shifts)
	         *shifts = foe.shifts;
	}
	return (err); 
}

win_set_focus_event(windowfd, fe, shifts)
	int 	windowfd;
	Firm_event *fe;
	int	shifts;
	
{
	int err;
	Focus_event foe;

	foe.id = fe->id;
	foe.value = fe->value;
	foe.shifts = shifts;
	(void)werror(err = ioctl(windowfd, WINSETFOCUSEVENT, &foe), WINSETFOCUSEVENT);
	return (err);
}

win_get_swallow_event(windowfd, fe, shifts)
	int 	windowfd;
	Firm_event *fe;
	int	*shifts;
{
	int err;
	Focus_event foe;

	(void)werror(err = ioctl(windowfd, WINGETSWALLOWEVENT, &foe), WINGETSWALLOWEVENT);
	if (!err) {
	    if (fe) {
		fe->id = foe.id;
		fe->value = foe.value;
	    }
	    if (shifts)
		*shifts = foe.shifts;
	}
	return (err);  
}

win_set_swallow_event(windowfd, fe, shifts)
	int 	windowfd;
	Firm_event *fe;
	int	shifts;
	
{
	int err;
	Focus_event foe;

	foe.id = fe->id;
	foe.value = fe->value;
	foe.shifts = shifts;
	(void)werror(err = ioctl(windowfd, WINSETSWALLOWEVENT, &foe), WINSETSWALLOWEVENT);
	return (err);
}

win_get_event_timeout(windowfd, tv)
	int 	windowfd;
	struct	timeval *tv;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINGETEVENTTIMEOUT, tv), WINGETEVENTTIMEOUT);
	return (err);
}

win_set_event_timeout(windowfd, tv)
	int 	windowfd;
	struct	timeval *tv;
{
	int err; 

	(void)werror(err = ioctl(windowfd, WINSETEVENTTIMEOUT, tv), WINSETEVENTTIMEOUT);
	return (err);
}

win_get_vuid_value(windowfd, id)
	unsigned short 	id;
{
	int err;
	Firm_event fe;
	
	fe.id = fe.pair_type = fe.pair = fe.value = fe.time.tv_sec = fe.time.tv_usec = 0;

	fe.id = id;
	(void)werror(err = ioctl(windowfd, WINGETVUIDVALUE, &fe), WINGETVUIDVALUE);
	if (!err) 
            return (fe.value);
	else 
	    return (err);
}

win_refuse_kbd_focus(windowfd)
	int 	windowfd;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINREFUSEKBDFOCUS, 0), WINREFUSEKBDFOCUS);
	return (err);
}

win_release_event_lock(windowfd)
	int 	windowfd;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINUNLOCKEVENT, 0), WINUNLOCKEVENT);
	return (err);
}

int
win_set_kbd_focus(windowfd, number)
	int 	windowfd;
	int	number;
{
	int err;

	(void)werror(err = ioctl(windowfd, WINSETKBDFOCUS, &number), WINSETKBDFOCUS);
	return (err);
}

int
win_get_kbd_focus(windowfd)
	int 	windowfd;
{
	int number;

	(void)werror(ioctl(windowfd, WINGETKBDFOCUS, &number), WINGETKBDFOCUS);
	return (number);
}

/*
 *	Mouse control functions -- button order & scaling
 */

int
win_get_button_order(windowfd)
	int 	windowfd;
{
	int number;

	(void)werror(ioctl(windowfd, WINGETBUTTONORDER, &number), WINGETBUTTONORDER);
	return (number);
}

int
win_set_button_order(windowfd, number)
	int 	windowfd;
	int	number;
{
	int err;

	err = ioctl(windowfd, WINSETBUTTONORDER, &number);
	(void)werror(err, WINSETBUTTONORDER);
	return (err);
}

int
win_get_scaling(windowfd, buffer)
	int 	windowfd;
	Ws_scale_list *buffer;
{
	int err;

	err = ioctl(windowfd, WINGETSCALING, buffer);
	(void)werror(err, WINGETSCALING);
	return (err);
}

int
win_set_scaling(windowfd, buffer)
	int 	windowfd;
	Ws_scale_list *buffer;
{
	int err;

	err = ioctl(windowfd, WINSETSCALING, buffer);
	(void)werror(err, WINSETSCALING);
	return (err);
}
