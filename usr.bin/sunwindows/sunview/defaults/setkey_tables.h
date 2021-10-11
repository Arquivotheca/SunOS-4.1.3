/*	@(#)setkey_tables.h 1.1 92/07/30	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <sundev/msio.h>
#include <sunwindow/win_input.h>

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

typedef enum { Unknown_kbd, Klunker_kbd, Sun1_kbd, Sun2_kbd, Sun3_kbd, Sun4_kbd
}	Keybd_type;

typedef struct {
    int		    key;
    int		    code;
}	Pair;

typedef struct {
    Pair           *down_only;
    int             count_downs;
    Pair           *ups;
    int             count_ups;
}	Key_info;

typedef struct {
    Key_info       *standard;
    Key_info       *standard_noarrow;
    Key_info       *lefty;
    Key_info       *lefty_noarrow;
}	Keybd_info;


extern Keybd_info    ktbl_klunker_info, ktbl_no_sunview_info,
		     ktbl_sun1_info, ktbl_sun2_info, ktbl_sun3_info,
		     ktbl_sun4_info;

