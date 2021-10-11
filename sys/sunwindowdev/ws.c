#ifndef lint
static  char sccsid[] = "@(#)ws.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Workstation/Wsindev open and close code.  Includes tuning
 * parameters.
 */

#include <sunwindowdev/wintree.h>
#include <sys/errno.h>
#include <sys/file.h>		/* For f_data */
#include <sys/kmem_alloc.h>
#include <sundev/kbio.h>	/* For keyboard ioctls */

#define	WS_VQ_NODE_BYTES	8192
u_int	ws_vq_node_bytes = WS_VQ_NODE_BYTES;
/* Number of bytes to use for input q nodes */
int	ws_vq_expand_times = 6;
/* Number of times can expand q by ws_vq_node_bytes */
#define	WS_QUEUE_COLLAPSE_FACTOR 0	/* Try to collapse q to 1/nth */
					/* Zero mean collapse all the time */
int	ws_q_collapse_factor = WS_QUEUE_COLLAPSE_FACTOR;

extern	void ws_break_handler();
extern	void ws_stop_handler();
extern	void ws_flush_handler();
Ws_usr_async ws_break_default =
	    {0, SHIFT_TOP, 1, TOP_FIRST + 'i', 1, ws_break_handler};
Ws_usr_async ws_stop_default =
	    {0, SHIFT_TOP, 1, SHIFT_TOP, 0, ws_stop_handler};
Ws_usr_async ws_flush_default =
	    {0, SHIFT_TOP, 1, TOP_FIRST + 'f', 1, ws_flush_handler};

Ws_focus_set ws_kbd_focus_pt_default = {LOC_WINENTER, 1, 0};
Ws_focus_set ws_kbd_focus_sw_default = {LOC_WINENTER, 1, 0};
struct	timeval ws_event_timeout_default = {2, 0};
	/* 2 secs give micro synchronization, thus collapsing when tracking */

/*
 * Tuning parameters that are set if ws_tuning_done is 0 when start SunWindows.
 */
int	ws_tuning_done;		/* Controls whether the following should be */
				/* initialized. */
int	ws_fast_timeout;	/* Fast polling rate in hz */
int	ws_slow_timeout;	/* Slow polling rate in hz */
int	ws_fast_poll_duration;	/* Stop fast polling after this # hz */
int	ws_loc_still;		/* Locator is still after this number of hz */
int	ws_check_lock;		/* Check locks every this # of hz if pid */
int	ws_no_pid_check_lock;	/* Check locks every this # of hz if no pid */
struct	timeval ws_check_time;	/* Check locks every this # usec */
int	winclistcharsmax;	/* Limit the amount of clist space can use */
int	ws_q_collapse_trigger;	/* # q items that trigger collapse */

ws_init_tuning()
{
#define	WS_FAST_TIMEOUT		(hz/40)	/* Fast polling rate */
#define	WS_SLOW_TIMEOUT		(hz/8)	/* Slow polling rate */
#define	WS_FAST_POLL_DURATION	(hz/5)	/* Stop fast polling after this # hz */
#define	WS_MOUSE_STILL		(hz/5)	/* Locator is still after this # of hz*/
#define	WS_CHECK_LOCK		(hz/2)	/* Check locks every #of hz if pid */
#define	WS_NO_PID_CHECK_LOCK	(hz*20)	/* Check locks every #of hz if no pid */

	if (!ws_tuning_done) {
		extern int hz;

		/* Init things can't do statically */
		if (!ws_fast_timeout)
			ws_fast_timeout = WS_FAST_TIMEOUT;
		if (!ws_slow_timeout)
			ws_slow_timeout = WS_SLOW_TIMEOUT;
		if (!ws_fast_poll_duration)
			ws_fast_poll_duration = WS_FAST_POLL_DURATION;
		if (!ws_loc_still)
			ws_loc_still = WS_MOUSE_STILL;
		if (!ws_check_lock)
			ws_check_lock = WS_CHECK_LOCK;
		if (!ws_no_pid_check_lock)
			ws_no_pid_check_lock = WS_NO_PID_CHECK_LOCK;
		if (!ws_check_time.tv_usec)
			ws_check_time.tv_usec = 1000000 / hz * ws_check_lock;
		if (!winclistcharsmax)
			/* Use half of all clist space */
			winclistcharsmax = nclist * CBSIZE / 2;
		if ((!ws_q_collapse_trigger) && ws_q_collapse_factor)
			ws_q_collapse_trigger =
			    (ws_vq_node_bytes / sizeof (Vuid_q_node)) /
			    ws_q_collapse_factor;
		ws_tuning_done = 1;
	}
}

/* Find workstation using input_device_name */
Workstation *
ws_indev_match_name(input_device_name, indev_ptr)
	char *input_device_name;
	Wsindev **indev_ptr;
{
	register Workstation *ws;
	register Wsindev *indev;

	if (input_device_name[0] == '\0')
		return (WORKSTATION_NULL);
	for (ws = workstations; ws < &workstations[nworkstations]; ws++) {
		for (indev = ws->ws_indev; indev != WSINDEV_NULL;
		    indev = indev->wsid_next) {
			if (indev->wsid_name[0] != '\0' &&
			    (strncmp(input_device_name,
			    indev->wsid_name, SCR_NAMESIZE) == 0)) {
				if (indev_ptr)
					*indev_ptr = indev;
				return (ws);
			}
		}
	}
	return (WORKSTATION_NULL);
}

/* Find workstation using dev */
Workstation *
ws_indev_match_dev(dev)
	dev_t dev;
{
	register Workstation *ws;
	register Wsindev *indev;

	for (ws = workstations; ws < &workstations[nworkstations]; ws++) {
		for (indev = ws->ws_indev; indev != WSINDEV_NULL;
		    indev = indev->wsid_next) {
			if (indev->wsid_dev == dev)
				return (ws);
		}
	}
	return (WORKSTATION_NULL);
}

/*
 * A workstation is opened by a window that trying to establish itself
 * as the root window of a first desktop at a workstation.  The list of
 * desktops is searched for a conflict of names of framebuffers to avoid
 * blitzing an existing screen.  The list of workstations is searched for
 * a conflict of names of input devices that the window has opened for
 * itself.
 */
Workstation *
ws_open()
{
	register Workstation *ws = WORKSTATION_NULL;
	register Workstation *ws_probe = WORKSTATION_NULL;
	extern	int ws_poll_rate;

	/* Initialize tuning parameters */
	ws_init_tuning();
	/* Allocate an unused workstation */
	for (ws_probe = workstations; ws_probe < &workstations[nworkstations];
	    ws_probe++) {
		if (!(ws_probe->ws_flags & WSF_PRESENT)) {
			ws = ws_probe;
			break;
		}
	}
	if ((ws == WORKSTATION_NULL) || (ws->ws_dtop != DESKTOP_NULL)) {
		win_errno = EBUSY;
		return (WORKSTATION_NULL);
	}
	/* Allocate the input q */
	ws->ws_qbytes = ws_vq_node_bytes;
	ws->ws_qdata = new_kmem_zalloc((u_int)ws->ws_qbytes, KMEM_SLEEP);
	/*
	 * There is no need to check for kmem_zalloc here because
	 * kmem_zalloc will wait until there is space before it returns
	if (ws->ws_qdata == NULL) {
#ifdef WINDEVDEBUG
		printf("Couldn't allocate %D byte event buffer bytes\n",
		    ws->ws_qbytes);
#endif
		win_errno = ENOMEM;
		return (WORKSTATION_NULL);
	}
	 */
	vq_initialize(&ws->ws_q, ws->ws_qdata, ws->ws_qbytes);

	/* Make present so that need ws_close to clean up after this point */
	ws->ws_flags |= WSF_PRESENT;
	/* Initialize input queue synchronization parameters */
	ws->ws_break = ws_break_default;
	ws->ws_stop = ws_stop_default;
	ws->ws_eventtimeout = ws_event_timeout_default;
	/* Initialize kbd focus changing events */
	ws->ws_kbd_focus_pt = ws_kbd_focus_pt_default;
	ws->ws_kbd_focus_sw = ws_kbd_focus_sw_default;
	/*
	 * Start device driver looking for input and doing deadlock
	 * resolution if haven't started yet.
	 */
	if (ws_poll_rate == 0) {
		int ws_interrupt();

		ws_poll_rate = ws_fast_timeout;
		timeout(ws_interrupt, (caddr_t)0, ws_poll_rate);
	}
	return (ws);
}

/*
 * A workstation is told by its last desktop to close, as is a desktop
 * told by its last window to close.
 */
ws_close(ws)
	register Workstation *ws;
{
#ifdef WINSVJ
	void    ws_dealloc_rec_q();	/* Recording q dealloc routine */
#endif

	/*
	 * ws_close_indev can sleep (see ws_close_indev) which can
	 * allow ws_interrupt to be called as a timeout.  Make this
	 * workstation invisible to ws_interrupt now.
	 */
	ws->ws_flags |= WSF_EXITING;
	/* Close input devices used by this workstation */
	while (ws->ws_indev != WSINDEV_NULL)
		/* ws_close_indev deletes ws_indev from list */
		if (ws_close_indev(ws, ws->ws_indev) == -1)
			break;

	/* All desktops should be removed by now */
#ifdef WINDEVDEBUG
	if (ws->ws_dtop != DESKTOP_NULL)
		printf("ws_close: desktop still opened\n");
#endif

	/* Deallocate input queue */
	if (ws->ws_qdata != NULL)
		kmem_free(ws->ws_qdata, (u_int)ws->ws_qbytes);

	/* Deallocate input state descriptions */
	vuid_destroy_state(ws->ws_instate);
	vuid_destroy_state(ws->ws_rtstate);

	/* Deallocate any event recording queue */
#ifdef WINSVJ
	(void) ws_dealloc_rec_q(ws);
#endif
	bzero((caddr_t)ws, sizeof (*ws));
	/* ws_interrupt turns self off after last workstation goes away */
}

Wsindev *
ws_open_indev(ws, name, fd)
	Workstation *ws;
	char *name;
	int fd;
{
	register Wsindev *wsid;
	register int	i;
	int	mode;
	struct	file *fp = 0;
	struct vnode *vp;
	Wsindev *wsid_tmp;
	dev_t	dev;
	struct	termios tios;

	/* Dup file descriptor for device */
	if ((u.u_error = kern_dupfd(fd, &fp)))
		goto SilentError;
	vp = (struct vnode *)fp->f_data;
	if (vp->v_stream == NULL) {
		u.u_error = ENOSTR;	/* not a streams device */
		goto SilentError;
	}
	dev = vp->v_rdev;
	/* See if can find this device open already */
	if (ws_indev_match_dev(dev) != WORKSTATION_NULL) {
		u.u_error = EBUSY;
		goto SilentError;
	}
	/* Allocate new input record */
	wsid = (Wsindev *)new_kmem_zalloc(sizeof (*wsid), KMEM_SLEEP);
	if (wsid == WSINDEV_NULL) {
#ifdef WINDEVDEBUG
		printf("ws_open_indev: heap alloc failed\n");
#endif
		u.u_error = ENOMEM;
		goto SilentError;
	}
	bzero((caddr_t)wsid, sizeof (*wsid));
	/* Initialize record */
	wsid->wsid_fp = fp;
	wsid->wsid_dev = dev;
	wsid->wsid_vp = vp;
	wsid->wsid_flags = 0;
	/* Copy name */
	for (i = 0; i < SCR_NAMESIZE; i++)
		wsid->wsid_name[i] = name[i];
	/* Determine current byte stream mode */
	strioctl(vp, VUIDGFORMAT, (caddr_t)&wsid->wsid_previous_mode, 0);
	if (u.u_error) {
		/* ioctl err VUIDGFORMAT */
		if ((u.u_error == ENOTTY) || (u.u_error == EINVAL)) {
			wsid->wsid_flags |= WSID_TREAT_AS_ASCII;
			wsid->wsid_previous_mode = VUID_NATIVE;
		} else
			goto Error;
	}
	/* Set up to send firm events */
	mode = VUID_FIRM_EVENT;
	strioctl(vp, VUIDSFORMAT, (caddr_t)&mode, 0);
	if (u.u_error) {
		/* ioctl err VUIDSFORMAT */
		if ((u.u_error == ENOTTY) || (u.u_error == EINVAL)) {
			wsid->wsid_flags |= WSID_TREAT_AS_ASCII;
			wsid->wsid_previous_mode = VUID_NATIVE;
		} else
			goto Error;
	}
	/* Get sgttyb flags */
	strioctl(vp, TCGETS, (caddr_t)&tios, 0);
	if (u.u_error) {
		/* ioctl err TCGETS */
		goto Error;
	}
	wsid->wsid_iflag = tios.c_iflag;
	wsid->wsid_oflag = tios.c_oflag;
	wsid->wsid_cflag = tios.c_cflag;
	wsid->wsid_lflag = tios.c_lflag;
	/* Set raw */
	tios.c_iflag = 0;
	tios.c_oflag = 0;
	tios.c_cflag &= ~(CSIZE|PARENB);
	tios.c_cflag |= CS8;
	tios.c_lflag = 0;
	strioctl(vp, TCSETSF, (caddr_t)&tios, 0);
	if (u.u_error) {
		/* ioctl err TCSETSF */
		goto Error;
	}
	/*
	 * Get/set direct I/O flag.  This is a hack to avoid /dev/console
	 * interference when using /dev/kbd.  Silent about any error
	 * on KIOCGDIRECT because only /dev/kbd will respond.
	 */
	strioctl(vp, KIOCGDIRECT, (caddr_t)&wsid->wsid_direct_flag, 0);
	if (u.u_error == 0) {
		int directio = 1;

		strioctl(vp, KIOCSDIRECT, (caddr_t)&directio, 0);
		if (u.u_error) {
			/* ioctl err KIOCSDIRECT */
			goto Error;
		}
		wsid->wsid_flags |= WSID_DIRECT_VALID;
		/*
		 * It's a keyboard; set up not to OR in bucky bits.
		 */
		mode = 0;
		strioctl(vp, KIOCSCOMPAT, (caddr_t)&mode, 0);
		if (u.u_error) {
			/* ioctl err KIOCSCOMPAT */
			goto Error;
		}
	}
	/* Put new record in list */
	wsid_tmp = ws->ws_indev;
	ws->ws_indev = wsid;
	wsid->wsid_next = wsid_tmp;
	return (wsid);
Error:
#ifdef WINDEVDEBUG
	printf("err = %D\n", u.u_error);
#endif
SilentError:
	return (WSINDEV_NULL);
}

int
ws_close_indev(ws, wsid)
	Workstation *ws;
	Wsindev *wsid;
{
	struct termios tios;
	register struct vnode *vp = (struct vnode *)wsid->wsid_fp->f_data;
	register Wsindev *indev, **indev_ptr;
	int on = 1;

	/*
	 * Remove wsid from list associated with workstation.
	 * This will avoid any further read of this device because zsclose
	 * (called from closef below) might sleep.
	 * In other words, when zsclose sleeps, the ws_interrupt timeout
	 * can occur.  By closef the device's original blocking state
	 * has been restored.  Thus, when ws_interrupt does a read of this
	 * device, it might block (sleep).  Blocking from a timeout notification
	 * will lead to a panic: sleep.
	 */
	indev_ptr = &ws->ws_indev;
	for (indev = ws->ws_indev; indev != WSINDEV_NULL;
	    indev = indev->wsid_next) {
		if (indev == wsid) {
			*indev_ptr = indev->wsid_next;
			indev->wsid_next = WSINDEV_NULL;
			goto Found;
		}
		indev_ptr = &indev->wsid_next;
	}
	/* ws_close_indev: device not found */
	return (-1);
Found:
	u.u_error = 0;
	/* Reset sgttyb flags */
	strioctl(vp, TCGETS, (caddr_t)&tios, 0);
#ifdef WINDEVDEBUG
	if (u.u_error)
		printf("ioctl err %D TCGETS\n", u.u_error);
#endif
	tios.c_iflag = wsid->wsid_iflag;
	tios.c_oflag = wsid->wsid_oflag;
	tios.c_cflag = wsid->wsid_cflag;
	tios.c_lflag = wsid->wsid_lflag;
	strioctl(vp, TCSETSF, (caddr_t)&tios, 0);
#ifdef WINDEVDEBUG
	if (u.u_error)
		printf("ioctl err %D TCSETSF\n", u.u_error);
#endif
	/* Reset vuid format */
	strioctl(vp, VUIDSFORMAT, (caddr_t)&wsid->wsid_previous_mode, 0);
#ifdef WINDEVDEBUG
	if (u.u_error)
		printf("ioctl err %D VUIDSFORMAT\n", u.u_error);
#endif
	/* Reset direct flag */
	if (wsid->wsid_flags & WSID_DIRECT_VALID) {
		strioctl(vp, KIOCSDIRECT, (caddr_t)&wsid->wsid_direct_flag, 0);
#ifdef WINDEVDEBUG
		if (u.u_error)
			printf("ioctl err %d KIOCSDIRECT\n", u.u_error);
#endif
		/*
		 * It's a keyboard; set up to OR in bucky bits.
		 */
		strioctl(vp, KIOCSCOMPAT, (caddr_t)&on, 0);
#ifdef WINDEVDEBUG
		if (u.u_error)
			printf("ioctl err %D KIOCSCOMPAT\n", u.u_error);
#endif
	}
	/* Cleanup file pointer */
	if (wsid->wsid_fp)
		closef(wsid->wsid_fp);
	/* Release memory */
	kmem_free((caddr_t)wsid, sizeof (*wsid));
	return (0);
}

void
ws_shrink_queue(ws)
	register Workstation *ws;
{
	Vuid_queue q;
	u_int qbytes;
	caddr_t qdata;

	/* Allocate new input q */
	qbytes = ws_vq_node_bytes;
	qdata = new_kmem_zalloc(qbytes, KMEM_NOSLEEP);
	if (qdata == NULL)
		return;
	vq_initialize(&q, qdata, (u_int)qbytes);
	/* Deallocate old input queue */
	if (ws->ws_qdata != NULL)
		kmem_free(ws->ws_qdata, ws->ws_qbytes);
	/* Update ws */
	ws->ws_qbytes = qbytes;
	ws->ws_qdata = qdata;
	ws->ws_q = q;
}

/* Deal with input queue overflow */
void
ws_handle_overflow(ws)
	register Workstation *ws;
{
	Vuid_queue q;
	u_int qbytes;
	caddr_t qdata;
	Firm_event firm_event;

	if (ws->ws_qbytes >= ws_vq_node_bytes * ws_vq_expand_times)
		goto Flush;
	/* Allocate new input q */
	qbytes = ws->ws_qbytes + ws_vq_node_bytes;
	qdata = new_kmem_zalloc(qbytes, KMEM_NOSLEEP);
	if (qdata == NULL)
		goto Flush;
	vq_initialize(&q, qdata, (u_int)qbytes);
	/* Copy contents of old q onto new one */
	while (vq_get(&ws->ws_q, &firm_event) != VUID_Q_EMPTY)
		if (vq_put(&q, &firm_event) == VUID_Q_OVERFLOW)
#ifdef WINDEVDEBUG
			printf("Window input queue unexpectedly full!\n");
#else
			;
#endif
	/* Deallocate old input queue */
	if (ws->ws_qdata != NULL)
		kmem_free(ws->ws_qdata, ws->ws_qbytes);
	/* Update ws */
	ws->ws_qbytes = qbytes;
	ws->ws_qdata = qdata;
	ws->ws_q = q;
	return;
Flush:
	/* Punt and flush queue */
	printf("Window input queue overflow!\n");
	ws_flush_input(ws);
	printf("Window input queue flushed!\n");
}
