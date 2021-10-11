/*	@(#)win_input.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * SunWindows related input definitions.
 */

#ifndef	sunwindow_win_input_DEFINED
#define	sunwindow_win_input_DEFINED

#ifdef	KERNEL
#include "../sundev/vuid_event.h"
#else
#include <sundev/vuid_event.h>
#endif	KERNEL

/*
 * SunView semantic events (see <sundev/vuid_event.h>)
 */

#define SUNVIEW_FIRST		vuid_first(SUNVIEW_DEVID)

#define ACTION_NULL_EVENT		(SUNVIEW_FIRST+0)	/* 31744 */
#define ACTION_ERASE_CHAR_BACKWARD		(SUNVIEW_FIRST+1)
#define ACTION_ERASE_CHAR_FORWARD		(SUNVIEW_FIRST+2)
#define ACTION_ERASE_WORD_BACKWARD		(SUNVIEW_FIRST+3)
#define ACTION_ERASE_WORD_FORWARD		(SUNVIEW_FIRST+4)
#define ACTION_ERASE_LINE_BACKWARD		(SUNVIEW_FIRST+5)
#define ACTION_ERASE_LINE_END			(SUNVIEW_FIRST+6)
#define ACTION_ERASE_SPARE1			(SUNVIEW_FIRST+7)

#define ACTION_GO_CHAR_BACKWARD		(SUNVIEW_FIRST+8)	/* 31752 */
#define ACTION_GO_CHAR_FORWARD			(SUNVIEW_FIRST+9)
#define ACTION_GO_WORD_BACKWARD		(SUNVIEW_FIRST+10)
#define ACTION_GO_WORD_FORWARD			(SUNVIEW_FIRST+11)
#define ACTION_GO_WORD_END			(SUNVIEW_FIRST+12)
#define ACTION_GO_LINE_BACKWARD		(SUNVIEW_FIRST+13)
#define ACTION_GO_LINE_FORWARD			(SUNVIEW_FIRST+14)
#define ACTION_GO_LINE_END			(SUNVIEW_FIRST+15)
#define ACTION_GO_LINE_START			(SUNVIEW_FIRST+16)	/* 31760 */
#define ACTION_GO_COLUMN_BACKWARD		(SUNVIEW_FIRST+17)
#define ACTION_GO_COLUMN_FORWARD		(SUNVIEW_FIRST+18)
#define ACTION_GO_DOCUMENT_START		(SUNVIEW_FIRST+19)
#define ACTION_GO_DOCUMENT_END			(SUNVIEW_FIRST+20)
#define ACTION_GO_SPARE1			(SUNVIEW_FIRST+21)
#define ACTION_GO_SPARE2			(SUNVIEW_FIRST+22)

#define ACTION_STOP			(SUNVIEW_FIRST+23)	/* 31767 */
#define ACTION_AGAIN			(SUNVIEW_FIRST+24)
#define ACTION_PROPS			(SUNVIEW_FIRST+25)
#define ACTION_UNDO			(SUNVIEW_FIRST+26)
#define ACTION_REDO			(SUNVIEW_FIRST+27)
#define ACTION_FRONT			(SUNVIEW_FIRST+28)
#define ACTION_BACK			(SUNVIEW_FIRST+29)
#define ACTION_COPY			(SUNVIEW_FIRST+30)
#define ACTION_OPEN			(SUNVIEW_FIRST+31)
#define ACTION_CLOSE			(SUNVIEW_FIRST+32)
#define ACTION_PASTE			(SUNVIEW_FIRST+33)
#define ACTION_FIND_BACKWARD		(SUNVIEW_FIRST+34)
#define ACTION_FIND_FORWARD		(SUNVIEW_FIRST+35)
#define ACTION_REPLACE			(SUNVIEW_FIRST+36)
#define ACTION_CUT			(SUNVIEW_FIRST+37)

#define ACTION_SELECT_FIELD_BACKWARD	(SUNVIEW_FIRST+38)	/* 31782 */
#define ACTION_SELECT_FIELD_FORWARD	(SUNVIEW_FIRST+39)
#define ACTION_COPY_THEN_PASTE		(SUNVIEW_FIRST+40)
#define ACTION_STORE			(SUNVIEW_FIRST+41)
#define ACTION_LOAD			(SUNVIEW_FIRST+42)
#define ACTION_GET_FILENAME		(SUNVIEW_FIRST+43)
#define ACTION_SET_DIRECTORY		(SUNVIEW_FIRST+44)
#define ACTION_DO_IT			(SUNVIEW_FIRST+45)
#define ACTION_HELP			(SUNVIEW_FIRST+46)
#define ACTION_INSERT			(SUNVIEW_FIRST+47)
#define ACTION_INVOKE			(SUNVIEW_FIRST+48)
#define ACTION_EXPAND			(SUNVIEW_FIRST+49)
#define ACTION_MATCH_DELIMITER		(SUNVIEW_FIRST+50)	/* 31794 */
#define ACTION_CAPS_LOCK		(SUNVIEW_FIRST+51)
#define ACTION_SPARE1			(SUNVIEW_FIRST+52)
#define ACTION_SPARE2			(SUNVIEW_FIRST+53)
#define ACTION_QUOTE			(SUNVIEW_FIRST+54)
#define ACTION_EMPTY			(SUNVIEW_FIRST+55)

#define SUNVIEW_LAST			ACTION_EMPTY

/*
 * Workstation device related definitions (see <sundev/vuid_event.h>)
 * that are SunWindows window device related.
 */
#define	LOC_MOVE		(VKEY_FIRSTPSEUDO+0)		/* 32512 */
#define	LOC_STILL		(VKEY_FIRSTPSEUDO+1)		/* 32513 */
#define	LOC_WINENTER		(VKEY_FIRSTPSEUDO+2)		/* 32514 */
#define	LOC_WINEXIT		(VKEY_FIRSTPSEUDO+3)		/* 32515 */
#define	LOC_MOVEWHILEBUTDOWN	(VKEY_FIRSTPSEUDO+4)		/* 32516 */
#define	LOC_DRAG		LOC_MOVEWHILEBUTDOWN		/* 32516 */
#define	WIN_REPAINT		(VKEY_FIRSTPSEUDO+5)		/* 32517 */
#define	WIN_RESIZE		(VKEY_FIRSTPSEUDO+6)		/* 32518 */
#define	LOC_RGNENTER		(VKEY_FIRSTPSEUDO+7)		/* 32519 */
#define	LOC_RGNEXIT		(VKEY_FIRSTPSEUDO+8)		/* 32520 */
#define	WIN_UNUSED_9		(VKEY_FIRSTPSEUDO+9)		/* 32521 */
#define	WIN_STOP		(VKEY_FIRSTPSEUDO+10)		/* 32522 */
#define	LOC_TRAJECTORY		(VKEY_FIRSTPSEUDO+11)		/* 32523 */
#define	KBD_USE			(VKEY_FIRSTPSEUDO+12)		/* 32524 */
#define	KBD_DONE		(VKEY_FIRSTPSEUDO+13)		/* 32525 */
#define	KBD_REQUEST		(VKEY_FIRSTPSEUDO+14)		/* 32526 */
#define	WIN_UNUSED_15		(VKEY_FIRSTPSEUDO+15)		/* 32527 */

/*
 * The inputmask consists of a input code array + flags that indicates those
 * user actions to be placed in the input queue.
 */

#define	BITSPERBYTE	8

#define VKEY_CODES      VKEY_KBD_CODES
				/* The number of the virtual keyboard device
				   codes that are contained within the input
				   mask. */

typedef	struct inputmask {
	short	im_flags;
#define	IM_NEGEVENT	(0x01)	/* send input negative events too */
#define	IM_POSASCII	(0x02)	/* OBSOLETE:no neg ASCII even if IM_NEGEVENT */
#define	IM_ANSI		(0x04)	/* OBSOLETE: ansi with funcs encoded in ESC[ */
#define	IM_UNENCODED	(0x08)	/* OBSOLETE: pure device codes */
#define	IM_ASCII	(0x10)	/* enable ASCII codes 0-127 */
#define	IM_META		(0x20)	/* enable META codes 128-255 */
#define	IM_NEGASCII	(0x40)	/* enable negative ASCII codes 0-127 */
#define	IM_NEGMETA	(0x80)	/* enable negative META codes 128-255 */
#define	IM_TOP		(0x100)	/* enable TOP codes 256-511 */
#define	IM_NEGTOP	(0x200)	/* enable negative TOP codes 256-511 */
#define	IM_INTRANSIT	(0x400)	/* don't surpress locator events when
				   in-transit over window */
#define IM_ISO		(0x800)	/* enable ISO codes 512-767 (backwards compat) */
#define	IM_EUC		(0x1000)/* enable EUC codes 0-255 */
#define	IM_NEGEUC	(0x2000)/* enable negative EUC codes 0-255 */
#define	IM_CODEARRAYSIZE (VKEY_CODES/((sizeof(char))*BITSPERBYTE))
	char	im_inputcode[IM_CODEARRAYSIZE];
	short	im_shifts;		/* OBSOLETE */
#define	IM_SHIFTARRAYSIZE (sizeof(short)*BITSPERBYTE)
	short	im_shiftcodes[IM_SHIFTARRAYSIZE]; /* OBSOLETE */
} Inputmask;

#define	INPUTMASK_NULL	((Inputmask *)0)

#define isworkstationdevid(code) \
	( (( (code) >> 8 ) & 0xff) == WORKSTATION_DEVID )
#define	win_setinputcodebit(im,code) \
	(im)->im_inputcode[((code)-VKEY_FIRST)/BITSPERBYTE] |= \
		(isworkstationdevid(code))? \
	    (1<<((BITSPERBYTE-1)-(((code)-VKEY_FIRST)%BITSPERBYTE))): \
	    0
#define	win_unsetinputcodebit(im,code) \
	(im)->im_inputcode[((code)-VKEY_FIRST)/BITSPERBYTE] &= \
		(isworkstationdevid(code))? \
	    (~(1<<((BITSPERBYTE-1)-(((code)-VKEY_FIRST)%BITSPERBYTE)))): \
	    0xff
#define	win_getinputcodebit(im,code) \
	((isworkstationdevid(code))? \
	((im)->im_inputcode[((code)-VKEY_FIRST)/BITSPERBYTE] & \
	    (1<<((BITSPERBYTE-1)-(((code)-VKEY_FIRST)%BITSPERBYTE)))): 0)


/*
 * An input event structure is that which is filled in on a readrecord call.
 * ie_code contains the input code involved.  IE_NEGEVENT is used to indicate
 * a negative event.
 *
 * ie_shiftmask contains selected shift state.
 *
 * The locator position (window relative) is placed in ie_loc*.
 * The time of the input event is put in ie_time (seconds & microseconds).
 * Time zone and daylight savings flag available via system call.
 */

typedef	struct	inputevent {
	short	ie_code;		/* input code */
	short	ie_flags;
#define	IE_NEGEVENT	(0x01)		/* input code is negative */
	short	ie_shiftmask;		/* input code shift state */
#ifndef	CAPSLOCK	/* Same as sundev/kbd.h */
#define	CAPSLOCK	0		/* Caps Lock key */
#define	CAPSMASK	0x0001
#define	SHIFTLOCK	1		/* Shift Lock key */
#define	LEFTSHIFT	2		/* Left-hand shift key */
#define	RIGHTSHIFT	3		/* Right-hand shift key */
#define	SHIFTMASK	0x000E
#define	LEFTCTRL	4		/* Left-hand (or only) control key */
#define	RIGHTCTRL	5		/* Right-hand control key */
#define	CTRLMASK	0x0030
/*	TOP		7		do not use! */
/*	TOPMASK		0x0080		UPMASK in keyboard driver */
/*	CMD		8		reserved */
/*	CMDMASK		0x0100		reserved */
#define	ALTGRAPH	9		/* Alt Graph key */
#define	ALTGRAPHMASK	0x0200
#define	ALT		10		/* Alt key */
#define	ALTMASK		0x0400
#define	NUMLOCK		11		/* Num Lock key */
#define	NUMLOCKMASK	0x0800
#endif	CAPSLOCK
#ifndef	META_SHIFT_MASK
#define	META_SHIFT_MASK	0x0040		/* Meta shift mask */
					/* Same as 'unused...	0x0040' in */
					/* sundev/kbd.h */
#endif	META_SHIFT_MASK
	short	ie_locx, ie_locy;	/* locator (usually a mouse) position */
	struct	timeval ie_time;	/* time of event */
} Event;

#define	EVENT_NULL	((Event *)0)

#define event_init(ie)	 	 ((ie)->ie_code = (ie)->ie_flags = \
				  (ie)->ie_shiftmask = (ie)->ie_locx = \
				  (ie)->ie_locy = (ie)->ie_time.tv_sec = \
				  (ie)->ie_time.tv_usec = 0)

#define	win_inputnegevent(ie)	((ie)->ie_flags&IE_NEGEVENT)
#define	win_inputposevent(ie)	(!((ie)->ie_flags&IE_NEGEVENT))

#define	ANSI_ENCODED	'z'	/* OBSOLETE */
#define	win_inputdeviceid(code)		(((code)>>8)&0Xff00)	/* OBSOLETE */
#define	win_inputphysicalkeystation(code)	 ((code)&0Xff)	/* OBSOLETE */

#define	event_id(event)		((event)->ie_code)
#define event_flags(event)		((event)->ie_flags)
#define	event_shiftmask(event)		((event)->ie_shiftmask)
#define	event_x(event)			((event)->ie_locx)
#define	event_y(event)			((event)->ie_locy)
#define	event_time(event)		((event)->ie_time)

/*
 * For keymapping squirrel the semantic event code into the 9 high-order
 * bits of the ie_shiftmask
 */
#define IE_SHIFTMASKSIZE 8
#define event_action_id(event) (\
	(u_short)SUNVIEW_FIRST|event_action_bits(event) \
	)
#define event_action_bits(event) (\
	((u_short)((event)->ie_shiftmask))>>IE_SHIFTMASKSIZE \
	)
#define event_action(event) \
	((event_action_bits(event)==0)?event_id(event):event_action_id(event))
#define event_set_action(event, action) \
	(event)->ie_shiftmask = \
	((u_short)(action) << IE_SHIFTMASKSIZE)|((event)->ie_shiftmask & 0xff)
	
#define event_clear_action(event)	\
	((event)->ie_shiftmask &= 0xff)

#define	event_is_up(event)		(win_inputnegevent((event)))
#define	event_is_down(event)		(win_inputposevent((event)))
#define	event_shift_is_down(event)	(event_shiftmask(event) & SHIFTMASK)
#define	event_ctrl_is_down(event)	(event_shiftmask(event) & CTRLMASK)
#define	event_meta_is_down(event)	(event_shiftmask(event)&META_SHIFT_MASK)
#define	event_alt_is_down(event)	(event_shiftmask(event) & ALTMASK)

#define	event_set_id(event, code) 	\
   event_id(event) = (code), event_clear_action(event);
					 
#define	event_set_shiftmask(event, shiftmask)	event_shiftmask(event) = (shiftmask)
#define	event_set_x(event, x)		event_x(event) = (x)
#define	event_set_y(event, y)		event_y(event) = (y)
#define	event_set_time(event, time)	event_time(event) = (time)

#define	event_set_up(event)		(event)->ie_flags |= IE_NEGEVENT
#define	event_set_down(event)		(event)->ie_flags &= ~IE_NEGEVENT


#define	event_is_button(event)		\
   ((event_id(event) >= BUT_FIRST) && (event_id(event) <= BUT_LAST))
				 	 
#define	event_is_euc(event)		\
   ((event_id(event) >= EUC_FIRST) && (event_id(event) <= EUC_LAST))
				 	 
#define	event_is_ascii(event)		\
   ((event_id(event) >= ASCII_FIRST) && (event_id(event) <= ASCII_LAST))
				 	 
#define	event_is_meta(event)		\
   ((event_id(event) >= META_FIRST) && (event_id(event) <= META_LAST))
				 	 
#define	event_is_iso(event)		\
   ((event_id(event) >= ISO_FIRST) && (event_id(event) <= ISO_LAST))
				 	 
#define	event_is_key_left(event)	\
   ((event_id(event) >= KEY_LEFTFIRST) && (event_id(event) <= KEY_LEFTLAST))
				 	 
#define	event_is_key_right(event)	\
   ((event_id(event) >= KEY_RIGHTFIRST) && (event_id(event) <= KEY_RIGHTLAST))
 
#define	event_is_key_top(event)	\
   ((event_id(event) >= KEY_TOPFIRST) && (event_id(event) <= KEY_TOPLAST))
				 	 
#define	event_is_key_bottom(event)	\
   ((event_id(event) == KEY_BOTTOMLEFT) || (event_id(event) == KEY_BOTTOMRIGHT))
 

/*
 *	for mouse control (button order & scaling)
 */

#define	WS_ORDER_LMR		0
#define	WS_ORDER_LRM		1
#define	WS_ORDER_MLR		2
#define	WS_ORDER_MRL		3
#define	WS_ORDER_RLM		4
#define	WS_ORDER_RML		5


typedef struct {
    unsigned short  ceiling;
    unsigned short  factor;
}               Ws_scale;

#define WS_SCALE_MAX_COUNT      16
#define WS_SCALE_MAX_CEILING 0xffff

typedef struct {
    Ws_scale        scales[WS_SCALE_MAX_COUNT];
}               Ws_scale_list;

#ifdef	cplus
/*
 * C Library routines dealing this type of input
 */
/* Input "read" routine */
int	input_readevent(int fd, struct inputevent *event);

/* Input mask control */
void	input_imnull(struct inputmask *im);
void	input_imall(struct inputmask *im);
void	win_get_kbd_mask(int windowfd, struct inputmask *im);
void	win_set_kbd_mask(int windowfd, struct inputmask *im);
void	win_get_pick_mask(int windowfd, struct inputmask *im);
void	win_set_pick_mask(int windowfd, struct inputmask *im);
void	win_get_designee(int windowfd, *nextwindownumber);
void	win_set_designee(int windowfd, nextwindownumber);

/* Input state query */
int	win_get_vuid_value(int windowfd, unsigned short id);

/* Input synchronization release */
void	win_release_event_lock(int windowfd);

/* Keyboard input focus access */
void	win_refuse_kbd_focus(int windowfd);
int	win_set_kbd_focus(int windowfd, number);
int	win_get_kbd_focus(int windowfd);

/* Physical input device control */
int	win_remove_input_device(int windowfd; char *name);
int	win_enum_input_device(int windowfd; int (*func)(); caddr_t data);
		/* Returns -1 if error enumerating, 0 if went smoothly and
		 * 1 if func terminated enumeration. */
int	win_is_input_device(int windowfd; char *name);
int	win_set_input_device(int windowfd, inputfd; char *name);

/* Audible/Visible bell control */
void	win_bell(int windowfd, struct timeval wait_tv, Pixwin *pw);

/* mouse control */
int	win_get_button_order(int windowfd);
int	win_set_button_order(int windowfd, order);
int	win_get_scaling(int windowfd; Ws_scale_list *buffer);
int	win_set_scaling(int windowfd; Ws_scale_list *buffer);

#endif

#endif	sunwindow_win_input_DEFINED
