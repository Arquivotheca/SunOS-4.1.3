#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)setkeys.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * setkeys.c	modify codes emitted by a keyboard
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>



#define FALSE	0
#define TRUE	1
#define STANDARD	0
#define NONSTANDARD	1
#define  RIGHTWIDTH_OTHR  22
#define  RIGHTWIDTH_1	29
#define  LEFTWIDTH_SUN 21
#define  LEFTWIDTH_KLUNK  22
#define  BLANKS  "      "

typedef enum { Show, Reset, Limbo, Standard, Remap, Fail} Parse_result;

typedef enum { Unknown_kbd, Klunker_kbd, Sun1_kbd, Sun2_kbd, Sun3_kbd, Sun4_kbd }
		Keybd_type;


typedef struct {
	int	key, code;
}   Pair;

typedef struct {
	Pair	*down_only;
	int	 count_downs;
	Pair	*ups;
	int	 count_ups;
}   Key_info;

typedef struct {
	Key_info *standard;
	Key_info *standard_noarrow;
	Key_info *lefty;
	Key_info *lefty_noarrow;
}   Keybd_info;


static Pair	 klunker_standard_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad	*/
		   { 2, LF(11)}, { 3, LF( 2)},
    { 25, LF(3)},  {26, LF(12)}, {27, LF( 4)},
    { 49, LF(5)},  {50, LF(13)}, {51, LF( 6)},
    { 72, LF(7)},  {73, LF(14)}, {74, LF( 8)},
    { 95, LF(9)},  {96, LF(15)}, {97, LF(10)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, RF( 1)}, {22, RF( 2)}, {23, RF( 3)},	/* Right function pad	*/
    { 45, RF( 4)}, {46, RF( 5)}, {47, RF( 6)},
    { 68, RF( 7)}, {69, RF( 8)}, {70, RF( 9)},
    { 91, RF(10)}, {92, RF(11)}, {93, RF(12)},
    {114, RF(13)},	 	 {116, RF(15)},

    {124, BF(2)}				/* "Right" => Next	*/
};

static Pair	 klunker_standard_ups[] = {
    /*	UP_KEYS give a code on button-up	*/
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_standard = {
    klunker_standard_downs, sizeof(klunker_standard_downs) / sizeof(Pair),
    klunker_standard_ups, sizeof(klunker_standard_ups) / sizeof(Pair)
};

static Pair	 klunker_lefty_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		   { 2,  RF( 2)}, { 3,  RF( 3)},
    { 25, RF( 4)}, {26,  RF( 5)}, {27,  RF( 6)},
    { 49, RF( 7)}, {50,  STRING+UPARROW},
				  {51,  RF( 9)},
    { 72, STRING+LEFTARROW},
		   {73,  RF(11)}, {74,  STRING+RIGHTARROW},
    { 95, RF(13)}, { 96, STRING+DOWNARROW},
				  { 97, RF(15)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, LF( 2)}, {22, LF(11)},		/* Right function pad */
				 {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)}, {46, LF(12)}, {47, LF( 3)},
    { 68, LF( 6)}, {69, LF(13)}, {70, LF( 5)},
    { 91, LF( 8)}, {92, LF(14)}, {93, LF( 7)},
    {114, LF(10)},	 	 {116, LF(9)},

    {124, 0x7f}					/* "Right" => DEL	*/
};

static Pair	 klunker_lefty_ups[] = {
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {113, SHIFTKEYS+RIGHTCTRL},			/* "Back Tab" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_lefty = {
    klunker_lefty_downs, sizeof(klunker_lefty_downs) / sizeof(Pair),
    klunker_lefty_ups, sizeof(klunker_lefty_ups) / sizeof(Pair)
};

static Pair	 klunker_left_noarrow_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		   { 2, RF( 2)}, { 3, RF( 3)},
    { 25, RF( 4)}, {26, RF( 5)}, {27, RF( 6)},
    { 49, RF( 7)}, {50, RF( 8)}, {51, RF( 9)},
    { 72, RF(10)}, {73, RF(11)}, {74, RF(12)},
    { 95, RF(13)}, {96, RF(14)}, {97, RF(15)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, LF( 2)}, {22, LF(11)},		/* Right function pad */
				 {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)}, {46, LF(12)}, {47, LF( 3)},
    { 68, LF( 6)}, {69, LF(13)}, {70, LF( 5)},
    { 91, LF( 8)}, {92, LF(14)}, {93, LF( 7)},
    {114, LF(10)},	 	 {116, LF(9)},

    {124, 0x7f}					/* "Right" => DEL	*/
};

static Pair	 klunker_left_noarrow_ups[] = {
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {113, SHIFTKEYS+RIGHTCTRL},			/* "Back Tab" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_left_noarrow = {
    klunker_left_noarrow_downs,
    sizeof(klunker_left_noarrow_downs) / sizeof(Pair),
    klunker_left_noarrow_ups,
    sizeof(klunker_left_noarrow_ups) / sizeof(Pair)
};

static Keybd_info	klunker = {
    &klunker_standard, &klunker_standard, &klunker_lefty, &klunker_left_noarrow
};


static Pair	sun1_standard_downs[] = {
    {15, LF(2)},  {16, RF(1)},  {17, LF(1)},  {18, RF(2)},
    {35, LF(4)},  {36, RF(3)},  {37, LF(3)},  {38, RF(4)},
    {53, LF(6)},  {54, RF(5)},  {55, LF(5)},  {56, RF(6)},
    {72, LF(8)},  {73, RF(7)},  {74, LF(7)},  {75, HOLE},
    {90, LF(10)}, {91, HOLE},   {92, LF(9)},  {93, RF(8)}
};

static Key_info	 sun1_standard = {
    sun1_standard_downs, sizeof(sun1_standard_downs) / sizeof(Pair), 0, 0
};

static Pair	sun1_noarrow_downs[] = {
    {10, TF(1)}, {11, TF(2)}, {12, TF(3)}, {13, TF(4)},

    {15, LF(2)},  {16, RF(1)},  {17, LF(1)},  {18, RF(2)},
    {35, LF(4)},  {36, RF(3)},  {37, LF(3)},  {38, RF(4)},
    {53, LF(6)},  {54, RF(5)},  {55, LF(5)},  {56, RF(6)},
    {72, LF(8)},  {73, RF(7)},  {74, LF(7)},  {75, HOLE},
    {90, LF(10)}, {91, HOLE},   {92, LF(9)},  {93, RF(8)}
};

static Key_info	 sun1_standard_noarrow = {
    sun1_noarrow_downs, sizeof(sun1_noarrow_downs) / sizeof(Pair), 0, 0
};
static Key_info	 sun1_lefty = {
    sun1_standard_downs, sizeof(sun1_standard_downs) / sizeof(Pair), 0, 0
};
static Key_info	 sun1_left_noarrow = {
    sun1_noarrow_downs, sizeof(sun1_noarrow_downs) / sizeof(Pair), 0, 0
};

static Keybd_info	sun1 = {
    &sun1_standard, &sun1_standard_noarrow, &sun1_lefty, &sun1_left_noarrow
};


static Key_info	 sun2_standard = {
    0, 0, 0, 0
};

static Pair	sun2_standard_noarrow_downs[] = {
		    {69, RF( 8)},
    { 91, RF(10)},		  {93, RF(12)},
		   {113, RF(14)}
};

static Key_info	 sun2_standard_noarrow = {
    sun2_standard_noarrow_downs,
    sizeof(sun2_standard_noarrow_downs) / sizeof(Pair),
    0, 0
};

static Pair	sun2_lefty_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 2, LF(11)}, { 3, RF( 1)},
    { 25, RF( 6)},  {26, RF( 4)}, {27, LF(12)},
    { 49, RF( 9)},  {50, LF(13)}, {51, RF( 7)},
    { 72, RF(12)},  {73, RF(10)}, {74, LF(14)},
    { 95, RF(15)},  {96, LF(15)}, {97, RF(13)},				
						/* Right function pad */
    { 21, LF( 2)},  {22, STRING+LEFTARROW},
				  {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)},  {46, STRING+RIGHTARROW},
				  {47, LF( 3)},
    { 68, LF( 6)},  {69, STRING+UPARROW},
				  {70, LF( 5)},
    { 91, LF( 8)},  {92, RF(11)}, {93, LF( 7)},
    {112, LF(10)}, {113, STRING+DOWNARROW},
				  {114, LF(9)}
};

static Pair	sun2_lefty_ups[] = {
    {111, SHIFTKEYS+RIGHTCTRL}			/* "Line Feed" => CTRL	*/

};

static Key_info	 sun2_lefty = {
    sun2_lefty_downs, sizeof(sun2_lefty_downs) / sizeof(Pair),
    sun2_lefty_ups, sizeof(sun2_lefty_ups) / sizeof(Pair)
};

static Pair	sun2_left_noarrow_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 2, LF(11)}, { 3, RF( 1)},
    { 25, RF( 6)},  {26, RF( 4)}, {27, LF(12)},
    { 49, RF( 9)},  {50, LF(13)}, {51, RF( 7)},
    { 72, RF(12)},  {73, RF(10)}, {74, LF(14)},
    { 95, RF(15)},  {96, LF(15)}, {97, RF(13)},				
						/* Right function pad */
    { 21, LF( 2)},  {22, RF( 2)}, {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)},  {46, RF( 5)}, {47, LF( 3)},
    { 68, LF( 6)},  {69, RF( 8)}, {70, LF( 5)},
    { 91, LF( 8)},  {92, RF(11)}, {93, LF( 7)},
    {112, LF(10)}, {113, RF(14)}, {114, LF(9)},
};

static Pair	sun2_left_noarrow_ups[] = {
    {111, SHIFTKEYS+RIGHTCTRL}			/* "Line Feed" => CTRL	*/

};

static Key_info	 sun2_left_noarrow = {
    sun2_left_noarrow_downs, sizeof(sun2_left_noarrow_downs) / sizeof(Pair),
    sun2_left_noarrow_ups, sizeof(sun2_left_noarrow_ups) / sizeof(Pair)
};

static Keybd_info	sun2_info = {
    &sun2_standard, &sun2_standard_noarrow, &sun2_lefty, &sun2_left_noarrow
};

static Keybd_info	sun3_info = {
    &sun2_standard, &sun2_standard_noarrow, &sun2_left_noarrow
};


static Pair		limbo_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 3, TF(11)},
    { 25, LF(12)},  {26, TF(12)},
    { 49, LF(13)},  {51, TF(13)},
    { 72, LF(14)},  {73, TF(14)},
    { 95, LF(15)},  {97, TF(15)},
    
    {  5, LF(11)}				/* F1 (Caps)		*/
};

static Key_info		limbo_info = {
    limbo_downs, sizeof(limbo_downs) / sizeof(Pair), 0, 0
};

static char          *progname = "setkeys";
static char          *nullstr = "";
static char	     *remap_filename;

#ifdef DEBUG
static int            debug_setkeys = TRUE;
#else
static int            debug_setkeys = FALSE;
#endif DEBUG

static Keybd_type     get_type();
static Parse_result   parse_args();

static int            setkey_local();

static void           fold(),
		      lose(),
		      reset_keyboard(),
		      set_all_keys(),
	              remap_keys();		

static char  *leftStr[5] =  {"STOP  ","PROPS ","EXPOSE","OPEN  ","FIND  "};
static char  *rightStr[5] = {"AGAIN ","UNDO  ","PUT   ","GET   ","DELETE"};

#ifdef STANDALONE
main(argc, argv)
#else
setkeys_main(argc, argv)
#endif STANDALONE
    int                   argc;
    char                **argv;
{
    int                   keyboard;
    Key_info             *info_ptr;
    Keybd_type            keybd_type;
    int			  non_standard;

    if ((keyboard = open("/dev/kbd", O_RDONLY, 0)) < 0) {
	(void)fprintf(stderr, "Couldn't open /dev/kbd\n");
	perror(progname);
	return;
    }
    keybd_type = get_type(keyboard);
    switch (parse_args(argc, argv, &keybd_type, &info_ptr)) {

      case Fail:
	(void)fprintf(stderr,
	"Usage: setkeys [reset | nosunview | show | -f <filename> | [[lefty] [noarrow]]]\n");
	exit(1);

      case Show:
	/* Determine if this is a lefty's keyboard */
	non_standard = is_non_standard(keyboard,keybd_type);
	print_func_key(keybd_type,non_standard);
	exit(0);

      case Reset:
	reset_keyboard(keyboard, keybd_type);
	exit(0);

      case Limbo:{
	    int                   count;
	    Pair                 *pairs;

	    pairs = info_ptr->down_only;
	    count = info_ptr->count_downs;
	    while (count-- > 0) {
		if (setkey_local(keyboard, 0, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CTRLMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, SHIFTMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CAPSMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
	    }
	    break;
	}

      case Standard:{
	    int                   count;
	    Pair                 *pairs;

	    pairs = info_ptr->down_only;
	    count = info_ptr->count_downs;
	    while (count-- > 0) {
		if (setkey_local(keyboard, 0, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CTRLMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, SHIFTMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CAPSMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
	    }
	    pairs = info_ptr->ups;
	    count = info_ptr->count_ups;
	    while (count-- > 0) {
		if (setkey_local(keyboard, 0, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CTRLMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, SHIFTMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
		if (setkey_local(keyboard, CAPSMASK, pairs[count].key,
			   pairs[count].code, nullstr) == -1)
		    lose();
	    }
	    break;
	}
       case Remap:
	 remap_keys(keyboard);
      	 break;
    }
    exit(0);
}

static void
lose()
{
    perror(progname);
    exit(2);
}

static Keybd_type
get_type(kbd)
	int	kbd;

{
	int	type;

	if (ioctl(kbd, KIOCTYPE, &type) == -1) {
		perror("kioctype");
		exit(2);
	}
	switch (type) {
	    case KB_KLUNK:
	    		return Klunker_kbd;
	    case KB_VT100:
	    		return Sun1_kbd;
	    case KB_SUN2:
	    		return Sun2_kbd;
	    case KB_SUN3:
	    		return Sun3_kbd;
            case KB_SUN4:
			return Sun4_kbd;		
	    case KB_ASCII:
	    case -1:
	    default:	return Unknown_kbd;
	}
}


static Parse_result
parse_args(argc, argv, type_p, info_p)
    int                   argc;
    char                **argv;
    Keybd_type           *type_p;
    Key_info            **info_p;
{
    int			  lefty = FALSE;
    int			  limbo = FALSE;
    int			  noarrows = FALSE;
    int			  reset = FALSE;
    int			  show = FALSE;
    int                   remap = FALSE;	
    Keybd_info           *keybd_info;

    while (--argc) {
	fold(*++argv);
	if (!strcmp(*argv, "klunker")) {
	    *type_p = Klunker_kbd;
	    continue;
	}
	if (!strcmp(*argv, "sun1")) {
	    *type_p = Sun1_kbd;
	    continue;
	}
	if (!strcmp(*argv, "sun2")) {
	    *type_p = Sun2_kbd;
	    continue;
	}
	if (!strcmp(*argv, "sun3")) {
	    *type_p = Sun3_kbd;
	    continue;
	}
	if (!strcmp(*argv, "sun4")) {
            *type_p = Sun4_kbd;
	    continue;
	}		
	if (!strcmp(*argv, "lefty")) {
	    lefty = TRUE;
	    continue;
	}
	if (!strcmp(*argv, "debug")) {
	    debug_setkeys = TRUE;
	    continue;
	}

	if (!strcmp(*argv, "nosunview")  ||
	    !strcmp(*argv, "no-sunview") ||
	    !strcmp(*argv, "no_sunview")   ) {
	    limbo = TRUE;
	    continue;
	}
	if (!strcmp(*argv, "noarrow")   ||
	    !strcmp(*argv, "no-arrow")  ||
	    !strcmp(*argv, "no_arrow")  ||
	    !strcmp(*argv, "noarrows")  ||
	    !strcmp(*argv, "no-arrows") ||
	    !strcmp(*argv, "no_arrows")   ) {
	    noarrows = TRUE;
	    continue;
	}
	if (!strcmp(*argv, "reset")) {
	    reset = TRUE;
	    continue;
	}
	if (!strcmp(*argv,"show") 	||
	    !strcmp(*argv,"Show")	)  {
		show = TRUE;
		continue;
	}
	if (!strcmp(*argv, "-f")) {
                remap = TRUE;
                if (!--argc) {
                        (void)fprintf(stderr, "Must specify filename after -f\n");
                        return Fail;
                }
                fold(*++argv);
                remap_filename = *argv;
                continue;
        } 
	(void)fprintf(stderr, "Unknown parameter \"%s\"\n", *argv);
	return Fail;
    } /* end of while (--argc) */

    switch (*type_p) {
      case Klunker_kbd:
	keybd_info = &klunker;
	break;
      case Sun1_kbd:
	keybd_info = &sun1;
	break;
      case Sun2_kbd:
	keybd_info = &sun2_info;
	break;
      case Sun3_kbd:
      case Sun4_kbd:
	keybd_info = &sun3_info;
	break;
    }
    if (lefty) {
	if (noarrows) {
	    *info_p = keybd_info->lefty_noarrow;
	} else {
	    *info_p = keybd_info->lefty;
	}
    } else {
	if (noarrows) {
	    *info_p = keybd_info->standard_noarrow;
	} else {
	    *info_p = keybd_info->standard;
	}
    }
    if (limbo) {
	if (lefty || noarrows || reset || show) {
	    (void)fprintf(stderr, "No other options compatible with no-sunview.\n");
	    return Fail;
	}
	if (*type_p == Klunker_kbd || *type_p == Sun1_kbd) {
	    (void)fprintf(stderr,
	            "No-sunview is valid only on a Sun-2, Type 3 or Type 4 keyboard.\n");	
	    return Fail;
	}
	*info_p = &limbo_info;
	return Limbo;
    }
    if (reset) {
	if (lefty || limbo || noarrows || show) {
	    (void)fprintf(stderr, "No other options compatible with reset.\n");
	    return Fail;
	}
	return Reset;
    }
    if (show)  {
	if (lefty || limbo || noarrows || reset)  {
	    (void)fprintf(stderr, "No other options compatible with show.\n");
	    return Fail;
	}
	return Show;
    }
    if (remap) {
        if (lefty || limbo || noarrows || reset || show) {
            (void)fprintf(stderr, "No other options compatible with -f.\n");
            return Fail;
        }
        return Remap;
    } else
	return Standard;
}

static void
fold(cp)
    register char	*cp;
{
    register char	 c;

    while (c = *cp) {
	if (isupper(c))
	    *cp = tolower(c);
	cp++;
    }
}

extern struct keyboard	*keytables[];		/*  in keytables.c  */
extern char		 keystringtab[16][KTAB_STRLEN];  

static void
reset_keyboard(keyboard, keybd_type)
    int                   keyboard;
    Keybd_type            keybd_type;
{
    register struct keyboard	 *kb;

    switch (keybd_type) {
      case Klunker_kbd:
	kb = keytables[0];
	break;
      case Sun1_kbd:
	kb = keytables[1];
	break;
      case Sun2_kbd:
	kb = keytables[2];
	break;
      case Sun3_kbd:
	kb = keytables[3];
	break;
      case Sun4_kbd:
        kb = keytables[4];
        break;
    }
    set_all_keys(keyboard, kb->k_normal, 0);
    set_all_keys(keyboard, kb->k_caps, CAPSMASK);
    set_all_keys(keyboard, kb->k_shifted, SHIFTMASK);
    set_all_keys(keyboard, kb->k_control, CTRLMASK);
    set_all_keys(keyboard, kb->k_up, UPMASK);
}

static void
set_all_keys(kbd_fd, km, keystate)
    register int		  kbd_fd, keystate;
    register struct keymap	 *km;
{
    register int		  i, code;
    register char                *str;

    for (i = 0; i < 128; i++) {
	code = km->keymap[i]; 
	if (code >= STRING && code <= STRING + 15)
	    str = keystringtab[code - STRING];
	else
	    str = nullstr;
	(void)setkey_local(kbd_fd, keystate, i, code, str);
    }
}

static void
remap_keys(keyboard)
        int             keyboard;
{
        char table_string[16], line[80];
        int     cnt, table, key, entry;
        FILE *file;

        if ((file = fopen(remap_filename, "r")) == NULL) {
                fprintf(stderr, "Unable to open %s\n", remap_filename);
                lose();
        }

        while (TRUE) {
                fgets(line, 80, file);
                if (line[0] == '#')
                        continue;       /* ignore comment lines */
                cnt = sscanf(line, "%s %d %x\n", table_string, &key, &entry);
                if (cnt != 3) {
                        fprintf(stderr,
                                "setkeys: Improper format (field count = %d) in %s\n",                
                                cnt, remap_filename);
                        goto close_setkeys;
                }
                if (!strcmp(table_string, "END")) {
close_setkeys:  
                        if (fclose(file))
                                lose();
                        return;
                } else if (!strcmp(table_string, "BASE"))
                        table = 0;
                else if (!strcmp(table_string, "CTRL"))
                        table = CTRLMASK;
                else if (!strcmp(table_string, "SHIFT"))
                        table = SHIFTMASK;
                else if (!strcmp(table_string, "CAPS"))
                        table = CAPSMASK;
                else if (!strcmp(table_string, "UP"))
                        table = UPMASK;
                else if (!strcmp(table_string, "ALTG"))
                        table = ALTGRAPHMASK;
                else {
                        fprintf(stderr,
                                "setkeys: Unknown keytable identifier '%s' in %s\n",                  
                                table_string, remap_filename);
                        goto close_setkeys;
                }
                if (setkey_local(keyboard, table, key, entry, nullstr)
                        == -1)
                        lose();
        }
}
 

static int
setkey_local(keyboard, table, position, code, string)
    int             keyboard, table, position, code;
    char           *string;
{
    struct kiockeymap  key;

    if (debug_setkeys) {
	(void)printf("Set station %d in %s table to 0x%x (%d).\n",
	       position,
	       ((table == 0)	 	 ? "base"	 :
		(table == ALTGRAPHMASK) ? "alt-graph" :
		(table == CAPSMASK)	 ? "caps-locked" :
		(table == SHIFTMASK) 	 ? "shifted"	 :
		(table == CTRLMASK)	 ? "controlled"	 :
		(table == UPMASK)	 ? "key-up"	 :  "unknown"),
	       code, code);
	if (code >= STRING && code <= STRING + 15) {
	    (void)printf("\t(string[%d]: \"%s\")\n",code - STRING, string);
	}
 	return 0;
    } else {
	key.kio_tablemask = table;
	key.kio_station = position;
	key.kio_entry = code;
	(void)strcpy(key.kio_string, string);
	return ((ioctl((keyboard), KIOCSKEY, &key)));
    }
}

static
print_func_key(keybd_type,lefty)
Keybd_type  keybd_type;
int  lefty;
{
    int   result,column;
    char  buffer[1024];
    int   strt_col,i;

    result = tgetent(buffer,"sun");
    if (result == -1)  {
	(void)printf("Can't open termcap file\n");
	exit(1);
    }   else   if (result == 0)  {
	(void)printf("No entry for sun\n");
	exit(1);
    }   else   if (result != 1)  {
	(void)printf("Unspecified error\n");
	exit(1);
    }
    column = tgetnum("co");
    putchar('\n');
    for (i = 0; i < column-1; i++)  {
	putchar('=');
    }
    putchar ('\n');
    switch (keybd_type)  {
	case Sun1_kbd:	(void)printf("Keyboard type is Sun1\n"); 
			if ((strt_col = column - RIGHTWIDTH_1 -1) < 0)  {
			    too_narrow();
			    exit(1);
			}   else   {
			    print_sun1(lefty,strt_col);
			}
			break;
	case Sun2_kbd:
	case Sun3_kbd:
        case Sun4_kbd:	if (keybd_type == Sun2_kbd)  {
			    (void)printf("Keyboard type is Sun-2\n");
			}   else if (keybd_type == Sun3_kbd)  {
                            (void)printf("Keyboard type is Type 3\n");
                        }   else { 
			    (void)printf("Keyboard type is Type 4\n");
			}
			if (lefty)  {
			    if ((strt_col = column - RIGHTWIDTH_OTHR-1)<0)  {
				too_narrow();
				exit(1);
			    } 
			    (void)printf("Lefthand setting\n");
			}   else   {
			    if (column - LEFTWIDTH_SUN < 0)  {
				too_narrow();
				exit(1);
			    }    else   {
				strt_col = 0;
			    }
			    (void)printf("Righthand setting\n");
			}
			print_sun2(lefty,strt_col);
			break;
	case Klunker_kbd:	(void)printf("Keyboard type is Klunker\n");
			if (lefty)  {
			    if ((strt_col = column - RIGHTWIDTH_OTHR-1)<0)  {
				too_narrow();
				exit(1);
			    } 
			    (void)printf("Lefthand setting\n");
			}   else   {
			    if (column - LEFTWIDTH_KLUNK < 0)  {
				too_narrow();
				exit(1);
			    }    else   {
				strt_col = 0;
			    }
			    (void)printf("Righthand setting\n");
			}
			print_klunk(lefty,strt_col);
			break;
    }
}

static
print_sun1(not_set,strt_col)
{

    int  i,j;
    char **left,**right;
    int  length;

    if (not_set)  {
	(void)printf("Function keys have not been set up using 'setkeys'\n");
	return;
    }
    left = rightStr;
    right = leftStr;
    (void)strcpy(leftStr[0],BLANKS);
    length = RIGHTWIDTH_1;

    put_blank(strt_col);
    for (i = 0; i < length; i++)  {
	putchar('-');
    }
    putchar('\n');
    for (i = 0; i < 5; i++)  {
	put_blank(strt_col);
	putchar('|');
	(void)printf("%s",left[i]);
	if (i == 4)  {
	    putchar(' ');
	}   else   {
	    putchar('|');
	}
	(void)printf("%s",BLANKS);
	putchar('|');
	(void)printf("%s",right[i]);
	putchar('|');
	(void)printf("%s",BLANKS);
	putchar('|');
	putchar('\n');
	put_blank(strt_col);
	if (i == 3)  {
	    for (j = 0; j < length - 7; j++)  {
		putchar('-');
	    }
	    (void)printf("%s",BLANKS);
	    putchar('|');
	}   else   {
	    for (j = 0; j < length; j++)  {
		putchar('-');
	    }
	}
	putchar('\n');
    } 
    
}

static
print_sun2(lefty,strt_col)
int  lefty;
int  strt_col;
{
#define  NULLSTR  "\0"
    int i, j;
    char  **left,**right;
    int   length;

    if (lefty) {
	left = rightStr;
	right = leftStr;
	length = RIGHTWIDTH_OTHR;
    }   else   {
	left = leftStr;
	right = rightStr;
	length = LEFTWIDTH_SUN;
    }
    put_blank(strt_col);
    for (i = 0; i < length; i++)  {
	putchar('-');
    }
    putchar('\n');
    for (i = 0; i < 5; i++)  {
	put_blank(strt_col);
	putchar('|');
	(void)printf("%s",left[i]);
	putchar('|');
	if (lefty)  {
	    (void)printf("%s",BLANKS);
	    putchar('|');
	}
	(void)printf("%s",right[i]);
	if (!lefty)  {
	    (void)printf("%s",BLANKS);
	}
	putchar('|');
	putchar('\n'); 
	put_blank(strt_col);  
	for (j = 0; j < length; j++)  {
	    putchar('-');
        }
	putchar('\n');
    } 
}

static
put_blank(strt_col)
int  strt_col;
{
    int  i;

    for (i = 0; i < strt_col; i++)  {
	putchar(' ');
    }
}

static
print_klunk(lefty,strt_col)
int  lefty;
int  strt_col;
{
    int i, j;
    char  **left,**right;
    int   length;

    if (lefty)  {
	left = rightStr;
	right = leftStr;
	length = RIGHTWIDTH_OTHR;
    }   else   {
	left = leftStr;
	right = rightStr;
	length = LEFTWIDTH_KLUNK;
    }
    put_blank(strt_col);
    for (i = 0; i < length; i++)  {
	putchar('-');
    }
    putchar('\n');
    for (i = 0; i < 5; i++)  {
	put_blank(strt_col);
	putchar('|');
	(void)printf("%s",left[i]);
	putchar('|');
	(void)printf("%s",BLANKS);
	putchar('|');
	(void)printf("%s",right[i]);
	putchar('|');
	putchar('\n'); 
	put_blank(strt_col);  
	for (j = 0; j < length; j++)  {
	    putchar('-');
        }
	putchar('\n');
    } 
}

static
is_non_standard(kb_des,type)
int	kb_des;
Keybd_type type;
{
    struct kiockeymap key;

    /* If the keyboard is Sun1, then find out if the user has set the 
       function keys already.  Otherwise,
       find out whether it's a lefty or righty's keyboard by testing which
       key is at the L5 spot for the righty setting, if it's L5, then it's
       a regular setting, anything else indicates lefty setting*/ 

    key.kio_tablemask = 0;
    if (type == Sun1_kbd)  {
	key.kio_station = 55;
    }   else   {
	key.kio_station = 49;
    }
    if (ioctl(kb_des,KIOCGKEY,&key)  < 0)  {
	(void)printf("IOCTL error, invalid argument\n");
	exit(1);
	/* NOTREACHED */
    }   else   {
	if (key.kio_entry == LF(5))  {
	    return(STANDARD);
	}   else   {
	    return(NONSTANDARD);
	}
    }
} 

static
too_narrow()
{
    (void)printf("Increase the width of the window for proper display please\n");
}
