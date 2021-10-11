/*	@(#)fv.port.h 1.1 92/07/30 SMI	*/
#ifdef OS3
#define mpr_static_static	mpr_static
#define dirent			direct
#endif

#ifdef PROTO
#define OPENLOOK 1
#endif
#ifndef SV1
#define OPENLOOK 1
#endif

#ifdef PROTO
# define MY_MENU(M)		PANEL_PULLDOWN, PANEL_MENU, M
# define MY_BUTTON_IMAGE(P,S)	PANEL_LABEL_STRING, S
# define PAINTWIN		Pixwin *
# define PANEL_CHOICE_BUG	0
# define CHOICE_OFFSET		20
# define CHECK_OFFSET		0
# define FRAME_PROPS_PUSHPIN_IN	FRAME_STAY_UP		
# define FRAME_CMD_PUSHPIN_IN	FRAME_STAY_UP		
#else
# define CHOICE_OFFSET		0
# define CHECK_OFFSET		5
# define FRAME_PROPS_RESET	0
# define FRAME_PROPS_SET	1
# ifdef SV1
#  define MY_MENU(M)		PANEL_BUTTON
#  define MY_BUTTON_IMAGE(P,S)	PANEL_LABEL_IMAGE, panel_button_image(P, S, 0, 0)
#  define PAINTWIN		Pixwin *
#  define PANEL_CHOICE_BUG	3
# else
#  define FRAME_MESSAGE		FRAME_LEFT_FOOTER
#  define Window		Vu_window
#  define MY_MENU(M)		PANEL_BUTTON, PANEL_ITEM_MENU, M
#  define MY_BUTTON_IMAGE(P,S)	PANEL_LABEL_STRING, S
#  define SCROLL_VIEW_START	SCROLLBAR_VIEW_START
#  define SCROLL_PIXELS_PER_UNIT	SCROLLBAR_PIXELS_PER_UNIT
#  define SCROLL_RECT		SCROLLBAR_RECT
#  define SCROLL_REQUEST	SCROLLBAR_REQUEST
#  define PAINTWIN		Vu_Window
#  define PANEL_CHOICE_BUG	0
#  ifndef FRAME_LABEL
#   define FRAME_LABEL		VU_LABEL
#  endif
#  ifndef PANEL_SHOW_ITEM
#   define PANEL_SHOW_ITEM	VU_SHOW
#  endif
#  ifndef WIN_SHOW
#   define WIN_SHOW		VU_SHOW
#  endif
# endif
#endif
