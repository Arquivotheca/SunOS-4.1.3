#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)iconedit_main.c 1.1 92/07/30";
#endif
#endif

/**************************************************************************/
/*                       iconedit_main.c                                  */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/*                 Bitmap editor for cursors and icons.                   */
/**************************************************************************/

#include "iconedit.h" 
#include <locale.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

/**************************************************************************/
/* icon                                                                   */
/**************************************************************************/

static short    icon_data[] = {
#include <images/iconedit.icon>
};
mpr_static(iced_icon_pixrect, 64, 64, 1, icon_data);


/************************************************************************/
/* miscellaneous globals                                                */
/************************************************************************/

Frame		iced_base_frame;
short		iced_state	=  -1;	
struct pixrect	*iced_patch_prs[IC_GRAYCOUNT];


int 		iced_mouse_panel_width, iced_mouse_panel_height,
		iced_msg_panel_width, iced_msg_panel_height,
		iced_panel_width, iced_panel_height,
		iced_proof_width, iced_proof_height;

int 		iced_icon_canvas_is_clear = TRUE;

char 		iced_file_name[1024];
static int	file_arg_given = FALSE;


char	*strcpy();
/************************************************************************/
/* main                                                                 */
/************************************************************************/

#ifdef STANDALONE
main(argc,argv) 
#else
iconedit_main(argc,argv) 
#endif STANDALONE
int argc; 
char **argv;
{
   int panel_device_number;

#ifdef	GPROF
	if (argc > 1 && strcmp(argv[argc-1], "-gprof") == 0) {
	    moncontrol(1);
	    /* Pull the -gprof out of argc/v */
	    argc--;
	    argv[argc] = (char *)0;
	} else {
	    moncontrol(0);
	}
#endif	GPROF

   setlocale(LC_ALL, "");
   init_misc();
   init_base_frame(argc, argv);
   iced_init_mouse_panel();
   iced_init_msg_panel();
   iced_init_canvas();		
   iced_init_panel();
   iced_init_proof();
   iced_init_browser();
   (void)window_fit(iced_base_frame);

   panel_device_number = (int) window_get(iced_panel, WIN_DEVICE_NUMBER);
   (void)window_set(iced_canvas,      WIN_INPUT_DESIGNEE, panel_device_number, 0);
   (void)window_set(iced_proof,       WIN_INPUT_DESIGNEE, panel_device_number, 0);
   (void)window_set(iced_mouse_panel, WIN_INPUT_DESIGNEE, panel_device_number, 0);
   (void)window_set(iced_msg_panel,   WIN_INPUT_DESIGNEE, panel_device_number, 0);

   iced_set_state(ICONIC);

   if (file_arg_given)  {
      (void)panel_set(iced_fname_item, PANEL_VALUE, iced_file_name, 0);
      iced_load_proc(iced_fname_item);
   }
   
   window_main_loop(iced_base_frame);
   
	EXIT(0);
}

/************************************************************************/
/* init_base_frame                                                      */
/************************************************************************/

static
init_base_frame(argc, argv)
int argc;
char **argv;
{
   argc--;  argv++;             /*      skip over program name  */

   iced_base_frame = window_create((Window)0, FRAME,
		FRAME_ARGC_PTR_ARGV, 		&argc, argv,
                FRAME_LABEL,          		"iconedit",
                FRAME_SUBWINDOWS_ADJUSTABLE,   	FALSE,
                FRAME_ICON,         		icon_create(ICON_IMAGE, 
						   &iced_icon_pixrect,0),
                WIN_ERROR_MSG, 		"Unable to create iced base frame\n",
                0);
   if (iced_base_frame == NULL) {
                (void)fprintf(stderr,"Unable to create iced base frame\n");
                exit(1);
   }

   while (argc--) {
      if (!file_arg_given)  {
         (void)strcpy(iced_file_name, *(argv++));
         file_arg_given = TRUE;
      }  else {
         (void)fprintf(stderr, "iconedit: unrecognized argument '%s'\n", *argv++);
         exit(1);
      }
   }
}

/************************************************************************/
/* initialization routines                                              */
/************************************************************************/

static
init_misc()
{
   init_patches();
   init_fonts();
   init_sw_dimensions();
}

static
init_patches() 
{
static struct pixrect	*patch_sources[IC_GRAYCOUNT]  =  {
	&iced_white_patch, &iced_gray25_patch, &iced_root_gray_patch,
	&iced_gray50_patch, &iced_gray75_patch, &iced_black_patch
};
   register int i;
   for (i = 0; i < IC_GRAYCOUNT; i++)  {
      iced_patch_prs[i] = mem_create(64, 64, 1);
      (void)pr_replrop(iced_patch_prs[i], 0, 0, 64, 64, 
		 PIX_SRC, patch_sources[i], 0, 0);
   }
}

static
init_fonts()
{
   if (!(iced_font       = pf_open(STANDARD_FONT)))  die(STANDARD_FONT);
   if (!(iced_bold_font  = pf_open(BOLD_FONT)))      die(BOLD_FONT);
   if (!(iced_screenr7   = pf_open(F_SCREEN_R_7)))   die(F_SCREEN_R_7);
   if (!(iced_screenr11  = pf_open(F_SCREEN_R_11)))  die(F_SCREEN_R_11);
   if (!(iced_screenr12  = pf_open(F_SCREEN_R_12)))  die(F_SCREEN_R_12);
   if (!(iced_screenb14  = pf_open(F_SCREEN_B_14)))  die(F_SCREEN_B_14);
   if (!(iced_cmrr14     = pf_open(F_CMR_R_14)))     die(F_CMR_R_14);
   if (!(iced_cmrb14     = pf_open(F_CMR_B_14)))     die(F_CMR_B_14);
   if (!(iced_gallantr19 = pf_open(F_GALLANT_R_19))) die(F_GALLANT_R_19);

   iced_screenr14           = iced_font;
   iced_screenb12           = iced_bold_font;
}

static
init_sw_dimensions()
{
   struct pr_size char_size;

   char_size = pf_textwidth(1, iced_font, "n");

   iced_mouse_panel_width	= CANVAS_SIDE;
   iced_mouse_panel_height	= 40;
   iced_msg_panel_width		= char_size.x * 30;;
   iced_msg_panel_height	= iced_mouse_panel_height;
   iced_panel_width		= iced_msg_panel_width;
   iced_panel_height		= CANVAS_SIDE - 100;	
   iced_proof_width		= iced_msg_panel_width;
   iced_proof_height		= 95;
}

static
die(msg) char *msg; {
   (void)fprintf(stderr, "%s\n", msg);
   exit(1);
}

