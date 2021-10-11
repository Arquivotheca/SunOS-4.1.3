/*      @(#)win_ioctl.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef	sunwindow_win_ioctl_DEFINED
#define	sunwindow_win_ioctl_DEFINED

/*
 * The header file defines the ioctl calls made to the SunWindows system
 * and any data structures used.
 *
 * A window is created by opening a window device named /dev/win#, where
 * # is a decimal number starting at 0, with exclusive access (O_XUSE).
 * The number is gotten by first opening /dev/win and using the returned
 * file descriptor to make a WINNEXTFREE ioctl.  A user process doing this
 * must be prepared to loop thru this procedure on an EACCES error.
 */

/*
 * Tree operations.
 */
#define	WINSETLINK	_IOW(g, 1, struct winlink)
	/* Copy in *winlink and set *wl_which of calling window to wl_link. */
#define	WINGETLINK	_IOWR(g, 2, struct winlink)
	/* Copy in *winlink and set wl_link to wl_which of calling window
	and then copyout *winlink. */
#define	WININSERT	_IO(g, 3)
	/* Insert calling window in the clipping tree in the position
	indicated by all the windows current links. */
#define	WINREMOVE	_IO(g, 4)
	/* Remove calling window from the clipping tree but leave its
	links and subtree intact.  A window is automatically removed
	and destroyed on the last close if not done explicitely */
#define	WINNEXTFREE	_IOR(g, 5, struct winlink)
	/* Copy out *winlink with wl_link set to the next available window
	slot one should use to get a new window via an open system call.
	If WIN_NULLLINK (-1) is returned then there are no slots available.
	The open call should pass in a O_XUSE (exclusive open) flag and be
	be prepared for the slot that he was going for to be taken. */

struct winlink {
	int	wl_link;
	int	wl_which;
};



/*  For a fast window enumerator in user-space.
*/
#define WINGETTREELAYER _IOWR(g, 25, Win_tree_layer)

/*
 * Mouse cursor operations.
 */
#define	WINSETMOUSE	_IOW(g, 6, struct mouseposition)
	/* Copy in *mouseposition and set mouse relative to window.
	WINSETKBDFOCUS is the rough equivalent for the kbd focus. */
/* these will be changed back to use struct cursor
 * when our kernel is released.
 */
#define	WINSETCURSOR	_IOW(g, 7, struct old_cursor)
	/* Copy in *cursor. */
#define	WINGETCURSOR	_IOWR(g, 8, struct old_cursor)
	/* Copy out *cursor and cursor->pm_image (gotten by copyin). */
/* these will go away when 
 * our new kernel is released.
 */
#define	WINSETLOCATOR	_IOW(g, 48, struct cursor)
	/* Copy in *cursor. */
#define	WINGETLOCATOR	_IOWR(g, 49, struct cursor)
	/* Copy out *cursor and cursor->pm_image (gotten by copyin). */

#define	WINFINDINTERSECT _IOWR(g, 9, struct winintersect)
	/* Copy in *winintersect and get x & y which are window relative.
	Copy out *winintersect after set wl_link with topmost window under
	x, y.  WINGETKBDFOCUS is the rough equivalent for the kbd focus. */

struct winintersect {
	short	wi_x, wi_y;
	int	wi_link;
};

struct mouseposition {
	short	msp_x, msp_y;
};

/*
 * Geometry operations.
 */
#define	WINGETRECT	_IOR(g, 10, struct rect)
	/* Copy out *rect which is the calling window's position &
	size relative to its parent. */
#define	WINSETRECT	_IOW(g, 11, struct rect)
	/* Copy in *rect which is the calling window's new position &
	size relative to its parent. */
#define	WINSETSAVEDRECT	_IOW(g, 12, struct rect)
	/* Copy in *rect which is the calling window's saved rect's
	new position & size relative to its parent. */
#define	WINGETSAVEDRECT _IOR(g, 13, struct rect)
	/* Copy out *rect which is the calling window's saved rect's position &
	size relative to its parent. */
/*
 * Misc operations.
 *
 * User flags.  Reserved bits for system and window manager defined.
 */
#define	WUF_SYS1	(0x01)
#define	WUF_BLANKET	(WUF_SYS1)	/* Indicates blanket window */
#define	WUF_SYS2	(0x02)
#define	WUF_SYS3	(0x04)
#define	WUF_SYS4	(0x08)
#define	WUF_WMGR1	(0x10)	/* Window iconic flag */
#define	WUF_WMGR2	(0x20)	/* Do nothing in SIGWINCH handler */
#define	WUF_WMGR3	(0x40)
#define	WUF_WMGR4	(0x80)  /* Keep window on top */
#define	WUF_RESERVEDMAX	(0x80)
#define	WINGETUSERFLAGS	_IOR(g, 14, int)
	/* Copy out user flag value */
#define	WINSETUSERFLAGS	_IOW(g, 15, int)
	/* Copy in user flag value */
#define	WINGETOWNER	_IOR(g, 16, int)
	/* Copy out int which is the pid of who is signalled for SIGWINCH. */
#define	WINSETOWNER	_IOW(g, 17, int)
	/* Copy in int which is the pid of who is signalled for SIGWINCH. */
#ifndef PRE_FLAMINGO
#define WINGETNOTIFYALL _IOR(g, 81, int)
	/* Copy out flag which indicates if signalled for all changes */
#define WINSETNOTIFYALL _IOW(g, 82, int)
	/* Copy in flag which indicates if signalled for all changes */
#endif

/*
 * Input control.
 */
#define	WINGETINPUTMASK	_IOR(g, 19, struct inputmaskdata)
	/* Copy out *inputmaskdata.  ims_flush is undefined. */
#define	WINSETINPUTMASK	_IOW(g, 20, struct inputmaskdata)
	/* Copy in *inputmaskdata and set input mask to ims_set.
	Input that is not matched by ims_set is next redirected to
	ims_nextwindow if not equal to WIN_NULLLINK (-1).
	Flush any existing events specified by ims_flush.
	Note: use of flushing should be very restricted.
	Valuable mouse-ahead and type-ahead may be lost. */

struct	inputmaskdata {
	struct	inputmask ims_set, ims_flush;
	int	ims_nextwindow;
};
/*
 * Multi-mask input control.
 */
#define	WINGETKBDMASK	_IOR(g, 52, struct inputmask)
	/* Copy out *inputmask for kbd mask */
#define	WINGETPICKMASK	_IOR(g, 53, struct inputmask)
	/* Copy out *inputmask for pick mask */
#define	WINSETKBDMASK	_IOW(g, 54, struct inputmask)
	/* Copy in *inputmask and set kbd input mask */
#define	WINSETPICKMASK	_IOW(g, 55, struct inputmask)
	/* Copy in *inputmask and set pick input mask */
#define	WINSETNEXTINPUT	_IOW(g, 56, int)
	/* Copy in *int and set next input window (none is WIN_NULLLINK (-1)) */
#define	WINGETNEXTINPUT	_IOR(g, 57, int)
	/* Copy out *int using next input window (none is WIN_NULLLINK (-1)) */

/*
 * Locator behavior (Scaling & Button order)
 */
#define	WINGETBUTTONORDER _IOR(g, 36, int)
#define	WINSETBUTTONORDER _IOW(g, 37, int)
	/* Get / Set the apparent order of buttons on the mouse */	
#define	WINGETSCALING _IOR(g, 38, Ws_scale_list)
#define	WINSETSCALING _IOW(g, 39, Ws_scale_list)
	/* Get / Set the scaling used on the mouse */	

/*
 * Kbd focus control.
 */
typedef	struct focus_event {
        short   id;             /* id of event that changes kbd focus */
        int     value;          /* value of event that changes kbd focus */
        int     shifts;         /* shift mask required to change kbd focus*/
#define FOCUS_ANY_SHIFT      -1      /* don't care value for shifts */
} Focus_event;
#define	WINSETFOCUSEVENT _IOW(g, 70, struct focus_event)
	/* Copy in *focus_event and use id, shifts and value fields to
	change the event that changes the kbd focus (default LOC_WINENTER).
	This event is passed thru to the application after setting the focus. */
#define	WINGETFOCUSEVENT _IOR(g, 71, struct focus_event)
	/* Copy out *firm_event after setting id, shifts and value fields to
	the event that changes the the kbd focus (default LOC_WINENTER).
	This event is passed thru to the application after setting the focus. */
#define	WINSETSWALLOWEVENT _IOW(g, 72, struct focus_event)
	/* Like WINSETFOCUSEVENT but defines the event that sets the kbd
	focus but is NOT passed thru to the application. */
#define	WINGETSWALLOWEVENT _IOR(g, 73, struct focus_event)
	/* Like WINGETFOCUSEVENT but gets the event that sets the kbd
	focus but is NOT passed thru to the application. */
#define	WINREFUSEKBDFOCUS _IO(g, 58)
	/* After just having received a KBD_REQUEST event, an application
	makes this call if it doesn't want the kbd focus at this time.
	This "refuse" interface, instead of a more direct "give it to me"
	interface, is justified by backwards compatibility with applications
	that don't know about KBD_REQUEST. */
#define	WINSETKBDFOCUS	_IOW(g, 61, int)
	/* Set the window number passed in to the kbd focus.
	WINSETMOUSE is the rough equivalent for the pick focus.
	This action is a hint that applies only when the multiple focus
	mechanism is being used.  If the user changes the kbd focus directly
	then the user takes presidence.  EINVAL is returned if the passed in
	window number is invalid. */
#define	WINGETKBDFOCUS	_IOWR(g, 62, int)
	/* Get the window number which is the kbd focus.
	WINFINDINTERSECT is the rough equivalent for the pick focus.
	If the multiple focus mechanism is not being used then the pick
	focus is returned. */

/*
 * Vuid state probe.
 */
#define	WINGETVUIDVALUE	_IOWR(g, 27, struct firm_event)
	/* Copy in *firm_event to determine id, place value in firm_event
	value field and copy out *firm_event (0 is default result). */

/*
 * Event lock control.
 */
#define	WINSETEVENTTIMEOUT _IOW(g, 59, struct timeval)
	/* Copy in *timeval and use it to globally change the event timeout.
	(0,0) gives unsynchronized behavior (thus making WINREFUSEKBDFOCUS
	and WINUNLOCKEVENT nops), (-1, -1) gives completely synchronized
	behavior (requiring explicit user action to break the lock), and
	(about 10, 0) give good synchronized behavior while only requiring
	the user to wait 10 seconds for an errant programs lock to be broken. */
#define	WINGETEVENTTIMEOUT _IOR(g, 60, struct timeval)
	/* Copy out *timeval after setting to the event timeout. */
#define	WINUNLOCKEVENT	_IO(g, 28)
	/* Unlock window event lock.  Usually done implicitly by doing
	a read or select.  However, if going off to do some time consuming
	time and don't want to lock up the user interface then call this. */

/*
 * Kernel operations applying globally to a screen.
 */
#define	WINLOCKDATA	_IO(g, 21)
	/* Lock window data for updates.  Only pid of calling user will be
	allowed access through ioctl calls.  The lock is reference counted.
	May block.  */
#define	WINCOMPUTECLIPPING	_IO(g, 18)
	/* Will compute new clipping if have data lock.
	Doesn't notify windows via SIGWINCH.
	Used by window managers to find out new clipping of window after
	doing something to the window tree so can (in conjunction with
	WINPARTIALREPAIR) save amount of repairing by moving bits on screen. */
#define	WINPARTIALREPAIR	_IOW(g, 43, struct rect)
	/* Remove this rect (window relative) from the pending damaged
	rectlist for this window and its children.
	Used by window managers (after moving bits on screen) to minimize
	the amount of repainting that the window needs to do.
	Do only when have data lock.
	If some of the pending damage is still pending from a previous
	clipping computation (i.e., a previous window rearrangement)
	then this operation is a nop. */
#define	WINUNLOCKDATA	_IO(g, 22)
	/* Unlock window data from updates.  Causes new clipping to be
	generated (if needed) and windows with damage that needs to be
	repaired to be notified with SIGWINCH.  */
#define	WINGRABIO	_IO(g, 23)
	/* All input actions directed to calling window. 
	Only this window can get lock to write to the display. 
	Used during window management user interface actions. */
#define	WINRELEASEIO	_IO(g, 24)
	/* Undoes WINGRABINPUT. */

/*
 * Display management operations.
 */
#define	WINLOCKSCREEN	_IOWR(g, 29, struct winlock)
	/* Copy in *winlock->wl_rect (Window relative) that is
	used for cursor removal.  Copy out *winlock->clipid which
	is sequence number of exposed RectList.  Lock screen for updates.
	May block. */
#define	WINUNLOCKSCREEN _IO(g, 30)
	/* Unlock screen from updates */
#define	WINGETEXPOSEDRL	_IOWR(g, 31, struct winclip)
	/* Copy in *winclip.wc_blockbytes and if enough room then copy out
	RectList that contains the clipping of the window. 
	If not enough bytes then wc_blockbytes will be altered to specify
	how many needed. */
#define	WINGETDAMAGEDRL	_IOWR(g, 32, struct winclip)
	/* Like WINGETEXPOSEDRL but used during fixup when receive SIGWINCH. */
#define	WINDONEDAMAGED	_IOW(g, 33, int)
	/* Call after finished fixing up for WINGETDAMAGEDRL. 
	Call even if chose to ignore fixup list.  int is users notion of
	his current clipping id. */
#define	WINSHAREQUEUE _IO(g, 80)
	/* Convert the event queue for this window to circular form and
	place in shared memory.  If not already sharing winlock, or if
	another window's queue is already shared, will fail.  For X. */

struct	winlock {
	int	wl_clipid;	/* clipping ID that must match current held */
	struct	rect wl_rect;	/* window coordinates of area that may write */
};

struct winclip {
	int	wc_blockbytes;		/* size of wc_block */
	int	wc_clipid;		/* Current clip id of clipping */
	struct	rect	wc_screenrect;	/* Screen relatived (used when paint) */
	char	*wc_block;		/* Block where RectList is copied. */
};

/*
 * Colormap sharing mechanism
 */
#define	WINSETCMS	_IOWR(g, 34, struct cmschange)
	/* Tell the kernel that window is using cc_cms.cms_name color map
	segment, its a cc_cms.cms_size segment and that cc_map.* are the
	values that you are using.  The kernel returns the offset of the
	segment in cc_cms.cms_addr.  If your named segment exists then
	you will share it with other windows.  Otherwise, your requested
	portion of the color map will be loaded ONLY when input is being
	directed to your window. */
#define	WINGETCMS	_IOWR(g, 35, struct cmschange)
	/* Get the color map data for the calling window. */

struct	cmschange {
	struct	colormapseg cc_cms;
	struct	cms_map cc_map;
};

/*
 * Plane group utilization mechanism
 */
#define WINSETPLANEGROUP      _IOW(g, 63, int)
	/* This ioctl records the plane group uses by the given window */
#define WINGETPLANEGROUP      _IOR(g, 64, int)
	/* This ioctl divulges the plane group uses by the given window */

#define	WIN_MAX_PLANE_GROUPS	20	/* 20 is arbitrary */
struct	win_plane_groups_available {
	char	plane_groups_available[WIN_MAX_PLANE_GROUPS];
};

#define WINSETAVAILPLANEGROUPS _IOW(g, 65, struct win_plane_groups_available)
	/* This ioctl records the plane groups used by the screen */
#define WINGETAVAILPLANEGROUPS _IOR(g, 66, struct win_plane_groups_available)
	/* This ioctl divulges the plane groups used by the screen */

/*
 * Screen creation, inquiry and deletion.
 */
struct	usrdesktop {
	struct	screen udt_scr;
	int	udt_fbfd;	/* fd of frame buffer */
	int	udt_kbdfd;	/* fd of keyboard */
	int	udt_msfd;	/* fd of mouse */
};
#define	WIN_NODEV	-1	/* special fd meaning no device */
#define	WINSCREENNEW	_IOW(g, 40, struct usrdesktop)
	/* Copies in usrdesktop and installs the calling window as its root.
	The root window is opened before making this call (obviously).
	Udt_scr.scr_name is made available to other window processes via
	WINSCREENGET so that they can pass it to pr_open during pixrect
	creation.  The root's rect size is set to the size of scr_rect. */
#define	WINSCREENGET	_IOR(g, 41, struct screen)
	/* Copies out screen of the calling window. */
#define	WINSCREENDESTROY _IO(g, 42)
	/* Deallocates the screen of the calling window and does other
	appropriate cleanup tasks.  This is permitted only
	if the calling window is the root window of the screen. */
struct  winscreenposdummy {int  a, b, c, d};
#define	WINSCREENPOSITIONS _IOW(g, 43, struct winscreenposdummy)
	/* Copy in *neighbors to screen occupied by calling window.
	This call specifies the relationship of the calling window's screen to
	other screens by naming the screens which are above/left/etc
	of this screen.  This allows the mouse to be moved from screen
	to screen.  The values of the array passed to the kernel are window
	numbers.  The screen on which each window sits is the neighbor.
	Use WIN_NULLLINK (-1) if there is no neighbor at that slot. */
#define	WINGETSCREENPOSITIONS _IOR(g, 45, struct winscreenposdummy)
	/* Copy out neighbors of calling window's desktop.
	(See WINSCREENPOSITIONS). */
#define	WINSETKBD	_IOW(g, 46, struct usrdesktop)
	/* Like WINSCREENNEW but only use kbd stuff.
	The keyboard is set to the new device. */
#define	WINSETMS	_IOW(g, 47, struct usrdesktop)
	/* Like WINSCREENNEW but only use ms stuff.
	The mouse is set to the new device. */

struct	input_device {
	int	id;			/* fd (or number) of input device
					   or -1 */
	char	name[SCR_NAMESIZE];	/* name of input device */
};
#define	WINSETINPUTDEV	_IOW(g, 50, struct input_device)
	/* Copies in input_device which identifies the input device.
	The kernel sends an ioctl to make it send firm events.
	The device's unread input is flushed.  SunWindows starts
	reading from the device.  The input device is opened before
	making this call (obviously) and the id field is set to the
	file descriptor.  The name is used to identify the device
	on subsequent calls to SunWindows.  If id is -1 then the
	device indicated by name is reset (to firm events or native
	mode, flush unread input) and no longer read by SunWindows. */
#define	WINGETINPUTDEV	_IOWR(g, 51, struct input_device)
	/* Copies in input_device and (depending on id) returns some
	information about an input device.  If id is -1 then the
	device indicated by name is checked for existence (as far as
	SunWindows is concerned).  If it exists then ioctl returns 0,
	but if it doesn't then errno is set to ENODEV.  If id is some
	small positive decimal then the idth input device's name is
	returned in name.  This is used for enumerated the input devices
	being used.  errno is set to ENODEV if id doesn't correspond
	to an open input device. */

/*
 * Debugging utilities
 */
#define	WINPRINT	_IO(g, 44)
	/* Print the calling window's data structure on the console.*/

#endif	sunwindow_win_ioctl_DEFINED



/* 
* pw_dbl_set struct to be  sent to WINDBLSET call
*/
struct pwset {
	int 	attribute;		/* Attribute PR_DBL_ */
	int	value;			/* Value foreground, background etc. */
};

/*
 *	Double buffering IOCTL's (71 thru 79)
 */
#define  WINDBLACCESS	_IO(g, 71)
	/* Allow Window to be double buffered */

#define WINDBLFLIP	_IO(g, 72)
	/* Flip the double buffer */

#define WINDBLABSORB	_IO(g, 73)

#define WINDBLRLSE	_IO(g, 74)
        /* release the double buffer */

#define WINDBLSET	_IOW(g, 75, struct pwset)

#define WINDBLGET	_IOWR(g, 76, struct pwset)

#define WINDBLCURRENT	_IOR(g, 77, struct rect)
        /* get the rect of the current window doing double buffering
	   in relation to the current window */

#include <sun/fbio.h>
/* Window id information for multiple window double buffering states. */
#define WINWIDSET	_IOWR(g, 78, struct fb_wid_dbl_info)
#define WINWIDGET	_IOWR(g, 79, struct fb_wid_dbl_info)

/* Cursor planes clearing during re-display */
#define WINCLEARCURSORPLANES	_IOWR(g, 83, struct rect)
/*
 *  New Genesis Window Driver Ioctls (150 thru 156)
 */
#define WINSETCPCURSOR  _IOW(g, 150, struct cursor)
	/* Set cursor processor cursor attributes. The cursor data
	   structure has been expanded to accomodate new cursor
	   processor specific attributes. */

#define WINGETCPCURSOR  _IOWR(g, 151, struct cursor)
	/* Get cursor processor cursor attributes. The cursor data
	   structure has been expanded to accomodate new cursor
	   processor specific attributes.*/

#define WINDISCONNECT  _IO(g, 152)
        /* Disconnect locator from cursor position update; this allows the
	   locator to function as a raw input device and not as a cursor
	   positioning device */ 

#define WINRECONNECT  _IO(g, 153)
        /* Reconnect locator to cursor position update; this restores the
	   locator to its usual task of driving the cursor */ 

#define WINGETROOTWIN  _IOWR(g, 154, struct input_device)
        /* Given the name of a frame buffer device, returns the
           window number of the root window for the desktop
           running on this frame buffer.  input_device.name 
           should be set to the frame buffer device name, and
           the root window number is returned in input_device.id.
           This is done to avoid creating a new structure for
           this ioctl, since a structure already exists which
           will do the job */

#define WINGETVALIDVALUES  _IOR(g, 155, struct valid_values_table)
        /* Get list of valid cursor processor parameter values so that
	   the user's attribute values can be range checked */

#define WINTUNINGVALUE  _IOWR(g, 156, struct tuning_pair)
	/* Allow a user to get/set the window driver tuning parameters
	   such as polling rate and queue size */

/*
 * Input event recording and synthesis (playback) functionality.
 */

#define	WINSETRECQUE	_IOW(g, 200, struct winrecq_setup)
	/* Recording Queue setup function */

typedef struct winrecq_setup {
	short	recq_cmd;		/* Setup command request */
#define WIN_RECQ_CRE	1		/* Create recording queue */
#define WIN_RECQ_DEL	0		/* Delete recording queue */
	short	recq_size;		/* Recording queue size in events */
#define WIN_RECQ_DEF	512		/* Recording queue default size */
} Winrecq_setup;

#define	WINSETRECORD	_IOW(g, 201, int)
	/* Recording mode function */
#define	WIN_SETREC_ON	1		/* Turn on recording */
#define WIN_SETREC_OFF	0		/* Turn off recording */

#define	WINREADRECQ	_IOWR(g, 202, struct winrecq_readbuf)
	/* Read an event from the event recording queue */

typedef struct winrecq_readbuf {
	short		trans;		/* Number events to read (transfer) */
	short		total;		/* Number events read */
	Firm_event	*fe;		/* Buffered events read */
} Winrecq_readbuf;

#define	WINSETPLAYBACK	_IOW(g, 203, int)
	/* Playback mode function */
#define	WIN_SETPLAY_ON	1		/* Turn on playback */
#define	WIN_SETPLAY_OFF	0		/* Turn off playback */

#define WINSETPLAYINTR	_IOW(g, 204, struct winplay_intr)
	/* Playback user event input termination event definition */

typedef struct winplay_intr {
	int	intr_flags;		/* Playback interrupt flags */
#define	WIN_PLAY_INTR_NONE	0x00	/* No interrupts allowed */
#define	WIN_PLAY_INTR_DEF	0x01	/* Default interrupts */
#define WIN_PLAY_INTR_USER	0x02	/* User defined interrupt event */
#define WIN_PLAY_INTR_SYNC	0x04	/* On Interrupt Generate Sync Pt */
	u_short	intr_feid;		/* User defined interrupt event id */
} Winplay_intr;

#define	WINSYNCIDDEF	0xff
	/* Default Sync Event Id Number */

#define	WINSETSYNCPT _IOW(g, 205, struct winrecplay_syncbuf)
	/* Record/Playback a user sync point */

typedef struct winrecplay_syncbuf {
	int	sync_cmd;		/* Set Sync Command */
#define	WINSETSYNCID	1		/* Set The Global Sync Event Id */
	/* Next are for setting a sync point/event */
#define WINSYNCDEFAULT	2		/* Use defaults type, pair, value */
#define	WINSYNCUSRSTD	3		/* Use user supplied type and value */
#define WINSYNCUSRDFN	4		/* Use all user supplied data */
	int	sync_type;		/* Sync Event Pair Type Value */
#define	WIN_SYNC_SET	1		/* Normal (expected) Sync Point */
#define	WIN_SYNC_INTR	2		/* Playback Interrupt Sync Point */
#define	WIN_SYNC_ERR	3		/* Sync Error Sync Point */
#define WIN_SYNC_MENU	4		/* SunView menu map sync point */
#define WIN_SYNC_TTY	5		/* SunView tty ready sync pt */
#define WIN_SYNC_TEXT	6		/* SunView text ready sync pt */
#define WIN_SYNC_WARPX  7               /* Set Mouse Warp x position */
#define WIN_SYNC_WARPY  8               /* Set Mouse Warp y position */
#define WIN_SYNC_ALERT  9               /* Alerts windowing sync pt */
#define WIN_SYNC_STACK  10              /* Stacking Menu map sync pt */
#define WIN_SYNC_PROMPT 11              /* Prompt box sync pt */
#define WIN_SYNC_USER1	128             /* First User defined sync type */
	int	sync_pair;		/* Sync Event Win Device Id (pair) */
	int	sync_value;		/* Sync point global id or value */
	int	sync_flag;		/* Syncing flag to sync on rec/play */
#define	WINSYNCREC	0x01		/* Set sync on record only */
#define	WINSYNCPLAY	0x02		/* Set sync on playback only */
} Winrecplay_syncbuf;
