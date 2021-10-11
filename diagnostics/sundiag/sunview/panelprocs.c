#ifndef lint
static	char sccsid[] = "@(#)panelprocs.c 1.40 91/04/09 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "sundiag.h"
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <fcntl.h>
#include <vfork.h>
#include "struct.h"
#include "procs.h"
#include "sundiag_msg.h"

int  running=IDLE;			/* to keep the system status */
int  selection_flag=TRUE;

extern char *malloc();
extern char *strcpy();
extern char *sprintf();
extern FILE *fopen();
extern time_t	time();
extern Panel_item	test_item;			/* defined in panel.c */
extern Pixrect	*start_button, *stop_button;		/* defined in panel.c */
extern Pixrect	*reset_button, *suspend_button;
extern Pixrect	*resume_button;
extern Pixrect	*ttime_button, *ttime_button_org;	/* defined in panel.c */
extern Pixrect	*options_button, *options_button_org;
extern Pixrect	*opf_button, *opf_button_org;
extern Pixrect	*eeprom_button, *eeprom_button_org;
extern Pixrect  *cpu_button, *cpu_button_org;

/* forward references */
extern frame_destroy_proc();
extern load_option_files();
extern store_option_files();
extern list_option_files();
extern remove_option_files();

static Frame display_frame=NULL;
static Textsw display_sw=NULL;
static char *cur_logname=" ";
static Frame of_frame=NULL;
static Panel of_panel;
static Panel_item	core_item;
static Panel_item	single_item;
static Panel_item	quick_item;
static Panel_item	verbose_item;
static Panel_item	trace_item;
static Panel_item	auto_item;
static Panel_item	run_error_item;
static Panel_item	max_passes_item;
static Panel_item	max_errors_item;
static Panel_item	max_tests_item;
static Panel_item	email_item;
static Panel_item	address_item;
static Panel_item	log_period_item;
static Panel_item	printer_name_item;
static Panel_item of_fname_item;
static Frame o_frame=NULL;
static Panel o_panel;
static short gray25[4]  =  {                    /*  25 % gray pattern   */
       0x8888, 0x2222, 0x4444, 0x1111
};
static mpr_static(gray25_pr, 16, 4, 1, gray25);

/******************************************************************************
 * Grays out the image of the button on the screen.			      *
 * Input: ip, the handle of the button to be grayed.			      *
 ******************************************************************************/
static	gray_button(ip)
Panel_item	ip;
{
   Pixrect *image_mpr;

   image_mpr = (Pixrect *)panel_get(ip, PANEL_LABEL_IMAGE);

   /* gray out the button label */
   (void)pr_replrop(image_mpr, 0, 0, image_mpr->pr_width, image_mpr->pr_height,
                    PIX_SRC | PIX_DST,
                    (Pixrect *)(LINT_CAST(&gray25_pr)), 0, 0);

   (void)panel_paint(ip, PANEL_NO_CLEAR);
}

/******************************************************************************
 * Grays out all the buttons which are not accessible while the tests are     *
 * running(i.e. the System status is "testing").			      *
 ******************************************************************************/
static	gray_all_buttons()
{
  gray_button(ttime_item);
  gray_button(options_item);
  gray_button(option_files_item);
  if (strcmp(cpuname, "SPARCsystem 600 MP"))
  {
      gray_button(eeprom_item);
  }
  gray_button(cpu_item);
}

/******************************************************************************
 * Un-grays all the buttons which are not accessible while the tests are      *
 * running(i.e. the System status is "testing").			      *
 ******************************************************************************/
light_all_buttons()
{
  (void)pr_rop(ttime_button, 0, 0, ttime_button->pr_width,
	ttime_button->pr_height, PIX_SRC, ttime_button_org, 0, 0);
  (void)pr_rop(options_button, 0, 0, ttime_button->pr_width,
	ttime_button->pr_height, PIX_SRC, options_button_org, 0, 0);
  (void)pr_rop(opf_button, 0, 0, ttime_button->pr_width,
	ttime_button->pr_height, PIX_SRC, opf_button_org, 0, 0);
  if (strcmp(cpuname, "SPARCsystem 600 MP"))
  {
      (void)pr_rop(eeprom_button, 0, 0, ttime_button->pr_width,
	    ttime_button->pr_height, PIX_SRC, eeprom_button_org, 0, 0);
  }
  (void)pr_rop(cpu_button, 0, 0, ttime_button->pr_width,
        ttime_button->pr_height, PIX_SRC, cpu_button_org, 0, 0);

  (void)panel_paint(ttime_item, PANEL_CLEAR);
  (void)panel_paint(options_item, PANEL_CLEAR);
  (void)panel_paint(option_files_item, PANEL_CLEAR);
  if (strcmp(cpuname, "SPARCsystem 600 MP"))
  {
      (void)panel_paint(eeprom_item, PANEL_CLEAR);
  }
  (void)panel_paint(cpu_item, PANEL_CLEAR);
}

/******************************************************************************
 * Notify procedure for "Start Tests" button.				      *
 ******************************************************************************/
extern	int	vmem_wait;		/* defined in tests.c */
extern	int	fb_delay;		/* defined in tests.c */

start_proc()
{
  disable_batch();	/* disable batch processing for now */
  if (running != IDLE)
  {
    (void)confirm("Already started testing!", TRUE);
    return;
  }
  vmem_wait = 0;	/* initialize the vmem wait counter */
  fb_delay = 0;		/* initialize the frame buffer delay */
  time_display();	/* redisplay the "Elapsed Time" */
  last_elapse = (long)time((time_t *)0);  /* get the current time of the day */
  running = GO;
  status_display();

  logmsg(start_all_info, 1, START_ALL_INFO); /* *** start all tests *** */
  log_status();	/* also log current status to information file */

  if (tty_mode) return;	/* that's it for TTY mode */

  if (o_frame != NULL)	/* close the system option popup if it was opened */
    frame_destroy_proc(o_frame);
  if (of_frame != NULL)	/* close the option file popup if it was opened */
    frame_destroy_proc(of_frame);

  (void)panel_set(test_item,
    PANEL_LABEL_IMAGE, stop_button,
    PANEL_NOTIFY_PROC, stop_proc, 0);

  (void)panel_set(reset_item,
    PANEL_LABEL_IMAGE, suspend_button,
    PANEL_NOTIFY_PROC, suspend_proc, 0);

  gray_all_buttons();	/* Gray out the unaccessible buttons */
}
 
/******************************************************************************
 * Notify procedure for "Stop Tests" button.				      *
 ******************************************************************************/
stop_proc()
{
  disable_batch();	/* disable batch processing for now */
  if (running != GO && running != SUSPEND && running != KILL)
  {
    (void)confirm("No tests are running!", TRUE);
    return;
  }

  if (running == GO || running == KILL)
  {
    elapse_count += (int)time((time_t *)0) - last_elapse;
				/* increment the elapsed seconds */
    (void)time_display();	/* display elapse time on status subwindow */
  }

  kill_all_tests();	/* kill all tests before quiting */

  status_display();
  logmsg(stop_all_info, 1, STOP_ALL_INFO); /* *** Stop all tests *** */
}
 
/******************************************************************************
 * Notify procedure for "Reset" button.					      *
 * Note: the "Reset" button will be disabled when tests are running.	      *
 ******************************************************************************/
reset_proc()
{
  int	test_id;

  if (running != IDLE)
  {
    (void)confirm("Can't reset while running tests!", TRUE);
    return;
  }

  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    tests[test_id]->pass = 0;
    tests[test_id]->error = 0;
  }

  logmsg(reset_pecount_info, 1, RESET_PECOUNT_INFO); 
				/* *** Reset pass/error counts *** */

  elapse_count = 0;		/* also reset the elaspse counter */
  time_display();		/* update it on screen too */
  print_status();		/* refresh the status */
}

/******************************************************************************
 * log_print(), prints the specified log file.				      *
 ******************************************************************************/
static	int	lpr_object;
static	int	*lpr_handle = &lpr_object;
/*ARGSUSED*/
static	void	log_print(filename)
char	*filename;
{
  char	buff[162];
  int	pid;

  (void)sprintf(buff, "fold %s | lpr -p -P%s -T %s",
				filename, printer_name, filename);

  if ((pid=vfork()) == 0)		/* child */
  {
    (void)execl("/bin/csh", "csh", "-c", buff, (char *)0);
    _exit(127);
  }

  if (pid != -1)			/* succeeded */
    (void)notify_set_wait3_func((Notify_client)lpr_handle,
			notify_default_wait3, pid);
}

/******************************************************************************
 * Notify procedure for "Log Files" button.				      *
 ******************************************************************************/
static	char	*unix_msg="/usr/adm/messages";
static	char	*unix_msg1="/usr/adm/messages.0";
static	char	*unix_msg2="/usr/adm/messages.1";
static	char	*unix_tmp_msg="/tmp/unix.msg";
static	display_log();	/* forward reference */
static	append_text();	/* forward reference */
log_files_menu_proc(item, event)
Panel_item item;
Event   *event;
{
  int	log_file_action;
  FILE	*tmp;
 
        if (event_id(event) == MS_LEFT && event_is_down(event))
	{
	  display_log(error_file);	/* left button to display ERROR log */
	  cur_logname = error_file;
	  (void)window_set(display_frame, FRAME_LABEL,
				cur_logname, WIN_SHOW, TRUE, 0);
	}
        else if (event_id(event) == MS_RIGHT && event_is_down(event))
	{
            log_file_action = (int)menu_show(log_files_menu,
                sundiag_control, event, 0);	/* display the menu */

            switch(log_file_action)
	    {
                case 1:			/* display ERROR log file */
		    display_log(error_file);
		    cur_logname = error_file;
		    (void)window_set(display_frame, FRAME_LABEL,
				cur_logname, WIN_SHOW, TRUE, 0);
                    break;
                case 2:			/* remove ERROR log file */
                    if (confirm_yes("OK to remove sundiag error logs?"))
		    {
			(void)fclose(error_fp);
			if (unlink(error_file) != 0)
			  perror("unlink(error_file)");
			error_fp = fopen(error_file, "a");

			if (strcmp(cur_logname, error_file) == 0)
			  (void)window_set((Frame)display_sw, TEXTSW_FILE,
								error_file, 0);
			  /* reopen(in case it was opened now) */
		    }
		    break;
		case 3:			/* print ERROR log file */
		    log_print(error_file);
		    break;
                case 4:			/* display INFO log file */
		    display_log(info_file);
		    cur_logname = info_file;
		    (void)window_set(display_frame, FRAME_LABEL,
				cur_logname, WIN_SHOW, TRUE, 0);
		    break;
                case 5:			/* remove INFO log file */
                    if (confirm_yes("OK to remove sundiag information logs?"))
		    {
			(void)fclose(info_fp);
			if (unlink(info_file) != 0)
			  perror("unlink(info_file)");
			info_fp = fopen(info_file, "a");

			if (strcmp(cur_logname, info_file) == 0)
			  (void)window_set((Frame)display_sw, TEXTSW_FILE,
								info_file, 0);
			  /* reopen(in case it was opened now) */
		    }
		    break;
		case 6:			/* print INFO log file */
		    log_print(info_file);
		    break;
                case 7:			/* display UNIX log file */
		    if (access(unix_msg, F_OK) != 0)		/* not exist */
		      if ((tmp=fopen(unix_msg, "a")) != NULL)
			(void)fclose(tmp);			/* create it */
		    display_log(unix_msg);
		    cur_logname = unix_msg;

		    (void)window_set(display_sw,
				TEXTSW_INSERTION_POINT, 0, 0);
		    (void)window_set(display_sw,
				TEXTSW_INSERT_FROM_FILE, unix_msg2,
				TEXTSW_INSERT_FROM_FILE, unix_msg1, 0);
		    (void)unlink(unix_tmp_msg);
		    (void)textsw_store_file(display_sw, unix_tmp_msg, 0, 0);
		    (void)unlink(unix_tmp_msg);
		    (void)window_set(display_frame, FRAME_LABEL,
				unix_tmp_msg, WIN_SHOW, TRUE, 0);
		    break;
                case 8:			/* remove UNIX log file */
                    if (confirm_yes("OK to remove Unix logs?"))
		    {
			(void)unlink(unix_msg1);
			(void)unlink(unix_msg2);
			(void)unlink(unix_tmp_msg);

			tmp = fopen(unix_msg, "w");
			fclose(tmp);

			if (strcmp(cur_logname, unix_msg) == 0)
			  (void)window_set((Frame)display_sw, TEXTSW_FILE,
							unix_tmp_msg, 0);
			  /* reopen(in case it was opened now) */
		    }
		    break;
		case 9:			/* print UNIX log file */
		    log_print(unix_msg);
		    break;
            }
        }
	else
            (void)panel_default_handle_event(item, event);
}

/******************************************************************************
 * Displays(shows) the specified log file in the popup text subwindow.	      *
 * Input: log_name, the name of the log file to be displayed.		      *
 ******************************************************************************/
static	display_log(log_name)
char *log_name;
{

	if (display_sw == (Textsw)NULL)
	/* if the text subwindow was not created yet */
	{
          display_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL,	TRUE,
	    WIN_X,		(int)(STATUS_WIDTH*frame_width)+15,
	    WIN_Y,		40,
            0);

          display_sw = (Textsw)window_create(display_frame, TEXTSW,
	    WIN_WIDTH,			ATTR_COLS(80),
	    WIN_HEIGHT,			ATTR_ROWS(32),
	    0);

	  window_fit(display_frame);
	}

	/* always reopen the file for TEXT subwindow */
        (void)window_set((Frame)display_sw, TEXTSW_FILE, log_name,
	    TEXTSW_FIRST, 0,
	    0);
}

/******************************************************************************
 * append_text(), appends the contents of the passed file into the TEXT	      *
 * subwindow(display_sw).						      *
 * input: the name of the file to be appended.				      *
 ******************************************************************************/
static	append_text(filename)
char *filename;
{
  int	fd, count;
  char	read_buf[514];

    if ((fd=open(filename, O_RDONLY)) == -1) return;

    (void)window_set(display_sw, TEXTSW_INSERTION_POINT, TEXTSW_INFINITY, 0);

    while ((count=read(fd, read_buf, 512)) == 512)
      (void)textsw_insert(display_sw, read_buf, 512);

    if (count != 0)
      (void)textsw_insert(display_sw, read_buf, count);

    (void)close(fd);
}

/******************************************************************************
 * Notify procedure for "Option Files" button.				      *
 ******************************************************************************/
/*****	forward references *****/
static	Panel_setting fname_proc();
static	done_option_files();

/*ARGSUSED*/
option_files_proc(item, event)
Panel_item item;
Event	   *event;
{
	if (running == GO) return;	/* not accessible when testing */

	/* o_frame & of_frame can't be up at the same time */
	if (o_frame != NULL)
	  frame_destroy_proc(o_frame);

	of_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_LABEL,	"Option File Menu",
	    WIN_X,	(int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
	    WIN_Y,	20,
            FRAME_DONE_PROC, frame_destroy_proc,
	    0);

	of_panel = window_create(of_frame, PANEL,
	    0);

	of_fname_item = panel_create_item(of_panel, PANEL_TEXT,
	    PANEL_LABEL_STRING,		"Option File Name: ",
	    PANEL_VALUE,		option_fname,
	    PANEL_VALUE_DISPLAY_LENGTH,	22,
	    PANEL_ITEM_X,               ATTR_COL(0),
            PANEL_ITEM_Y,               ATTR_ROW(1),
	    PANEL_NOTIFY_PROC,		fname_proc,
            0);

	(void)panel_create_item(of_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,  panel_button_image(of_panel,
					"Load", 8, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(0),
	    PANEL_ITEM_Y,		ATTR_ROW(3),
	    PANEL_NOTIFY_PROC,		load_option_files,

	    0);

	(void)panel_create_item(of_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,  panel_button_image(of_panel,
					"Store", 8, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(12),
	    PANEL_ITEM_Y,		ATTR_ROW(3),
	    PANEL_NOTIFY_PROC,		store_option_files,
	    0);

	(void)panel_create_item(of_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,  panel_button_image(of_panel,
					"Done", 8, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(24),
	    PANEL_ITEM_Y,		ATTR_ROW(3),
	    PANEL_NOTIFY_PROC,		done_option_files,
	    0);

	(void)panel_create_item(of_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,  panel_button_image(of_panel,
					"List", 8, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(0),
	    PANEL_ITEM_Y,		ATTR_ROW(4),
	    PANEL_EVENT_PROC,		list_option_files,
	    0);

	(void)panel_create_item(of_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,  panel_button_image(of_panel,
					"Remove", 8, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(12),
	    PANEL_ITEM_Y,		ATTR_ROW(4),
	    PANEL_NOTIFY_PROC,		remove_option_files,
	    0);

	window_fit(of_panel);
        window_fit(of_frame);
        (void)window_set(of_frame, WIN_SHOW, TRUE, 0);
}

/******************************************************************************
 * Gets names of all files in the specified directory.			      *
 * Input: directory, the name of the directory to be searched.		      *
 * Output: pointer to the array of file names.				      *
 ******************************************************************************/
char **get_file_names(directory)
char *directory;
{
  static char *list[BUFSIZ+1];
  DIR *dirp;
  struct direct *dp;
  int i = 0;
         
  dirp = opendir(directory);
  if (dirp != NULL)
  {
        while (dp = readdir(dirp))
	{
            if ((strcmp(dp->d_name, ".") != 0) &&
            	(strcmp(dp->d_name, "..") != 0))
	    {
                list[i] = malloc((unsigned)strlen(dp->d_name)+4);
		(void)strcpy(list[i++], dp->d_name);
	    }
        }
    (void)closedir(dirp);
  }
  list[i] = NULL;
  return(list);
}

/******************************************************************************
 * Panel notify procedure for the "file name:" text item in "Option Files"    *
 * popup window.						              *
 ******************************************************************************/
static Panel_setting fname_proc(item, event)
Panel_item	item;
Event		*event;
{
  if (event_id(event) == '\r')		/* the Return key was hit */
    (void)strcpy(option_fname, (char *)panel_get_value(item));

  return(panel_text_notify(item, event));
}

/******************************************************************************
 * Panel notify procedure for the "Load" button item in "Option Files"	      *
 * popup window.						              *
 ******************************************************************************/
load_option_files()
{
  char	*tmp;

  if (!tty_mode)
  {
    tmp = (char *)panel_get_value(of_fname_item);
    if (*tmp == '\0') return;	/* no file specified */
    (void)strcpy(option_fname, tmp);
  }

  (void)load_option(option_fname, 1);
}

/******************************************************************************
 * Panel notify procedure for the "Store" button item in "Option Files"	      *
 * popup window.						              *
 ******************************************************************************/
store_option_files()
{
  char	temp[82], *tmp;

  if (!tty_mode)
  {
    tmp = (char *)panel_get_value(of_fname_item);
    while (*tmp == ' ') ++tmp;	/* skip leading spaces */
    if (*tmp == '\0') return;	/* no file specified */

    (void)strcpy(option_fname, tmp);
  
    (void)sprintf(temp, "%s/%s", OPTION_DIR, option_fname);
    if (access(temp, F_OK) == 0)	/* the option file already exists */
      if (!confirm_yes("OK to reuse existing option file?")) return;
  }

  (void)store_option(option_fname);
}

/******************************************************************************
 * Panel notify procedure for the "List" button item in "Option Files"	      *
 * popup window.						              *
 ******************************************************************************/
list_option_files(item, event)
Panel_item	item;
Event		*event;
{
  int	name_index;
  int	i=1;
  char	**namelist, **keeplist;
  Menu	of_menu;

  if (event_id(event) == MS_RIGHT && event_is_down(event))
  {
    of_menu = menu_create(MENU_NCOLS, 3, 0);
    namelist = get_file_names(OPTION_DIR);
    keeplist = namelist;	/* keep the head pointer */

    if (*namelist == NULL)
    {
	(void)menu_set(of_menu, MENU_NCOLS, 1, 0);
	(void)menu_set(of_menu, MENU_STRING_ITEM, "??? no option files", 1, 0);
    }
    else
      while (*namelist)
        (void)menu_set(of_menu, MENU_STRING_ITEM, *namelist++, i++, 0);

    name_index = (int)menu_show(of_menu, sundiag_control, event, 0);
    if (name_index != 0 && *keeplist != NULL)	/* one file name was selected */
    {
	(void)strcpy(option_fname, *(keeplist+name_index-1));/* keep the name */
	(void)panel_set(of_fname_item, PANEL_VALUE, option_fname, 0);
					/* display it too */
    }
    menu_destroy(of_menu);

    namelist = keeplist;
    while (*namelist)
      free(*namelist++);
  }
  else
	(void)panel_default_handle_event(item, event);
}

/******************************************************************************
 * Panel notify procedure for the "Remove" button item in "Option Files"      *
 * popup window.						              *
 ******************************************************************************/
remove_option_files()
{
  char	temp[82], *tmp;

  if (!tty_mode)
  {
    tmp = (char *)panel_get_value(of_fname_item);
    if (*tmp == '\0') return;	    /* no file specified */

    (void)sprintf(temp, "%s/%s", OPTION_DIR, tmp);

    if (access(temp, F_OK) == 0)    /* the specified option file does exist */
      if (confirm_yes("OK to remove sundiag option file?"))
        if (!unlink(temp))	    /* successful */
	  (void)panel_set(of_fname_item, PANEL_VALUE, "", 0);
  }
  else
  {
    (void)sprintf(temp, "%s/%s", OPTION_DIR, option_fname);
    if (unlink(temp))
      (void)confirm("The specified option file was not removed!", TRUE);
  }
}

/******************************************************************************
 * Panel notify procedure for the "Done" button item in "Option Files"	      *
 * popup window.						              *
 ******************************************************************************/
static done_option_files()
{
  (void)strcpy(option_fname, (char *)panel_get_value(of_fname_item));
  (void)window_set(of_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(of_frame);
  of_frame = NULL;
}

/******************************************************************************
 * Notify procedure for the "Options" button.				      *
 ******************************************************************************/
/***** global flag(switch) variables. *****/
int	core_file=0;	/* generate core files? */
int	single_pass=0;	/* single pass for all enabled tests? */
int	quick_test=0;	/* run tests with "q(uick)" option? */
int	verbose=0;	/* run tests in verbose mode? */
int	trace=0;	/* run tests in trace mode? */
int	auto_start=0;	/* automatically start testing when booted? */
int	run_error=0;	/* enable run_on_error mode? */
int	max_errors=1;	/* max. # of errors allowed if run_on_error enabled */
int	max_tests=2;	/* max. # of tests in a group */
int	log_period=0;	/* period(in minutes) to log the staus, or send email */
int	send_email=0;	/* automatically send mail on conditions */
int	max_sys_pass=0;	/* to keep maximum system pass count */
char	eaddress[52]="root";	/* to contain the email address */

/***** forward references *****/
static option_done_proc();
static option_default_proc();

/*ARGSUSED*/
options_proc()
{
  char  tmp1[12];
  char	tmp2[12];
  char	tmp3[12];
  int	which_row=0;

	if (running == GO) return;

	/* repaint the system option popup */
	if (o_frame != NULL)
	  frame_destroy_proc(o_frame);

	/* o_frame & of_frame can't be up at the same time */
	if (of_frame != NULL)
	  frame_destroy_proc(of_frame);

	o_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_LABEL,	"System Option Menu",
	    WIN_X,	(int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
	    WIN_Y,	20,
            FRAME_DONE_PROC, frame_destroy_proc, 0);

	o_panel = window_create(o_frame, PANEL, 0);

	core_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Core Files:   ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		core_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	single_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Single Pass:  ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		single_pass,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	quick_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Quick Test:   ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		quick_test,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	verbose_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Verbose:      ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		verbose,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	trace_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "trace:        ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		trace,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	auto_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Auto Start:   ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		auto_start,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	run_error_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Run On Error: ",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		run_error,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	email_item = panel_create_item(o_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Send Email:   ",
	    PANEL_CHOICE_STRINGS,
		"Disable", "On_Error", "Periodically", "Both", 0,
	    PANEL_VALUE,		send_email,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	(void)sprintf(tmp1, "%u", max_sys_pass);
	max_passes_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Max. # of Passes:  ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	5,
	    PANEL_VALUE_STORED_LENGTH,	5,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	(void)sprintf(tmp1, "%u", max_errors);
	max_errors_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Max. # of Errors:  ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	5,
	    PANEL_VALUE_STORED_LENGTH,	5,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	(void)sprintf(tmp2, "%u", max_tests);
	max_tests_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Concurrent Tests #:",
	    PANEL_VALUE,		tmp2,
	    PANEL_VALUE_DISPLAY_LENGTH,	5,
	    PANEL_VALUE_STORED_LENGTH,	5,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	(void)sprintf(tmp3, "%u", log_period);
	log_period_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Log/Email Period:  ",
	    PANEL_VALUE,		tmp3,
	    PANEL_VALUE_DISPLAY_LENGTH,	8,
	    PANEL_VALUE_STORED_LENGTH,	10,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	address_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Email Address:",
	    PANEL_VALUE,		eaddress,
	    PANEL_VALUE_DISPLAY_LENGTH,	12,
	    PANEL_VALUE_STORED_LENGTH,	20,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	printer_name_item = panel_create_item(o_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Printer Name: ",
	    PANEL_VALUE,		printer_name,
	    PANEL_VALUE_DISPLAY_LENGTH,	12,
	    PANEL_VALUE_STORED_LENGTH,	20,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	(void)panel_create_item(o_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,		panel_button_image(o_panel,
					"Default", 7, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(1),
	    PANEL_ITEM_Y,		ATTR_ROW(which_row),
	    PANEL_NOTIFY_PROC,		option_default_proc,
	    0);

	(void)panel_create_item(o_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,		panel_button_image(o_panel,
					"Done", 7, (Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,		option_done_proc,
	    0);

	window_fit(o_panel);
        window_fit(o_frame);
        (void)window_set(o_frame, WIN_SHOW, TRUE, 0);
}

/******************************************************************************
 * Panel notify procedure for the "defualt" button item in "System Options"   *
 * popup subwindow.							      *
 ******************************************************************************/
static option_default_proc()
{
  char	*tmp, *getenv();

  (void)panel_set(core_item, PANEL_VALUE, 0, 0);
  (void)panel_set(single_item, PANEL_VALUE, 0, 0);
  (void)panel_set(quick_item, PANEL_VALUE, 0, 0);
  (void)panel_set(verbose_item, PANEL_VALUE, 0, 0);
  (void)panel_set(trace_item, PANEL_VALUE, 0, 0);
  (void)panel_set(auto_item, PANEL_VALUE, 0, 0);
  (void)panel_set(run_error_item, PANEL_VALUE, 0, 0);
  (void)panel_set(max_passes_item, PANEL_VALUE, "0", 0);
  (void)panel_set(max_errors_item, PANEL_VALUE, "1", 0);
  (void)panel_set(max_tests_item, PANEL_VALUE, "2", 0);
  (void)panel_set(email_item, PANEL_VALUE, 0, 0);
  (void)panel_set(log_period_item, PANEL_VALUE, "0", 0);
  (void)panel_set(address_item, PANEL_VALUE, "root", 0);
  (void)panel_set(printer_name_item, PANEL_VALUE,
	(tmp=getenv("PRINTER"))!=NULL?tmp:"lp", 0);
}

/******************************************************************************
 * Panel notify procedure for the "Done" button item in "System Options"      *
 * popup subwindow.							      *
 ******************************************************************************/
static option_done_proc()
{
  core_file = (int)panel_get_value(core_item);
  single_pass = (int)panel_get_value(single_item);
  quick_test = (int)panel_get_value(quick_item);
  verbose = (int)panel_get_value(verbose_item);
  trace = (int)panel_get_value(trace_item);
  auto_start = (int)panel_get_value(auto_item);
  run_error = (int)panel_get_value(run_error_item);
  max_sys_pass = atoi(panel_get_value(max_passes_item));
  max_errors = atoi(panel_get_value(max_errors_item));
  max_tests = atoi(panel_get_value(max_tests_item));
  send_email = (int)panel_get_value(email_item);
  log_period = atoi(panel_get_value(log_period_item));
  (void)strcpy(eaddress, panel_get_value(address_item));
  (void)strcpy(printer_name, panel_get_value(printer_name_item));

  set_max_tests(max_tests);	/* set the max. # of tests per group */

  (void)window_set(o_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(o_frame);
  o_frame = NULL;
}

/******************************************************************************
 * Notify procedure for the "Print Screen" button.			      *
 ******************************************************************************/
static	int	fork_object;
static	int	*fork_handle = &fork_object;
/*ARGSUSED*/
pscrn_proc()
{
  char	buff[162];
  int	pid;

  (void)sprintf(buff, "./diagscrnprnt %s",printer_name);

  if ((pid=vfork()) == 0)		/* child */
  {
    (void)execl("/bin/csh", "csh", "-c", buff, (char *)0);
    _exit(127);
  }

  if (pid != -1)			/* succeeded */
    (void)notify_set_wait3_func((Notify_client)fork_handle,
			notify_default_wait3, pid);
}

/******************************************************************************
 * Notify procedure for the "Suspend" button.				      *
 ******************************************************************************/
/*ARGSUSED*/
suspend_proc()
{
  int	test_id;

  disable_batch();	/* disable batch processing for now */
  if (running != GO)
  {
    (void)confirm("Can't suspend unless tests are running!", TRUE);
    return;
  }

  running = SUSPEND;

  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->pid != 0)	/* it is running */
	(void)kill(tests[test_id]->pid, SIGSTOP);
  }

  elapse_count += (int)time((time_t *)0) - last_elapse;
				/* increment the elapsed seconds */
  (void)time_display();		/* display elapse time on status subwindow */

  if (!tty_mode)
  {
    (void)panel_set(reset_item,
      PANEL_LABEL_IMAGE, resume_button,
      PANEL_NOTIFY_PROC, resume_proc, 0);

    light_all_buttons();
  }

  status_display();
}

/******************************************************************************
 * Notify procedure for the "Resume" button.				      *
 ******************************************************************************/
/*ARGSUSED*/
resume_proc()
{
  int	test_id;

  disable_batch();	/* disable batch processing for now */
  if (running != SUSPEND)
  {
    (void)confirm("Can't resume unless tests are suspended!", TRUE);
    return;
  }

  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->pid != 0)	/* it is loaded */
	(void)kill(tests[test_id]->pid, SIGCONT);
  }

  last_elapse = (int)time((time_t *)0);	/* get current time of the day */
  running = GO;

  if (!tty_mode)
  {
    	
    if (o_frame != NULL)  /* close the system option popup if it was opened */
      frame_destroy_proc(o_frame);
    if (of_frame != NULL) /* close the option file popup if it was opened */
      frame_destroy_proc(of_frame);

    (void)panel_set(reset_item,
      PANEL_LABEL_IMAGE, suspend_button,
      PANEL_NOTIFY_PROC, suspend_proc, 0);

    gray_all_buttons();
  }

  status_display();
}

/******************************************************************************
 * Notify procedure for the "Quit" button.				      *
 ******************************************************************************/
quit_proc()
{
        (void)window_destroy(sundiag_frame);	/* quit sundiag */
}

/******************************************************************************
 * Notify procedure for the "Intervention" cycle.			      *
 ******************************************************************************/
/***** global variables *****/
int	intervention=0;

/*ARGSUSED*/
interven_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  int	test_id, group_id, id;
  int	update_flag=0;

  intervention = value;
  if (tty_mode)
    tty_int_sel();

  if (intervention)			/* enable the intervention mode */
  {
      for (test_id=0; test_id != exist_tests; ++test_id)
        if (tests[test_id]->type == 2)
	{
	  tests[test_id]->type = 12;		/* enable the tests */
	  if ((tests[test_id]->dev_enable && tests[test_id]->enable) ||
		(tests[test_id]->dev_enable && tests[test_id]->test_no != 1))
	  {
	    if (tests[test_id]->which_test == 1)
	    {
	      if (!tty_mode)
	      {
	        (void)panel_set(groups[tests[test_id]->group].select,
				PANEL_TOGGLE_VALUE, 0, TRUE, 0);
	        (void)panel_set(tests[test_id]->select,
				PANEL_TOGGLE_VALUE, 0, TRUE, 0);
	        (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);
						/* no slection mark */
	      }
	      else
		display_enable(test_id, TRUE);

	      selection_flag = FALSE;
	    }
	    update_flag = 1;
	  }
	}
  }
  else
  {
      for (test_id=0; test_id != exist_tests; ++test_id)
        if (tests[test_id]->type == 12)
	{
	  tests[test_id]->type = 2;		/* disable the tests */
	  if (tests[test_id]->pid != 0)		/* test is currently running */
	    (void)kill(tests[test_id]->pid, SIGINT);	/* kill it */

	  if ((tests[test_id]->dev_enable && tests[test_id]->enable) ||
		(tests[test_id]->dev_enable && tests[test_id]->test_no != 1))
	  {
	    if (tests[test_id]->which_test == 1)
	      if (!tty_mode)
	        (void)panel_set(tests[test_id]->select,
		  PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	      else
		display_enable(test_id, FALSE);

	    update_flag = 1;

	    group_id = tests[test_id]->group;
	    id = groups[group_id].first;
	    for (; id != exist_tests; ++id)
  	    {
		if (tests[id]->group != group_id) /* none was enabled */
		  if (!tty_mode)
		    (void)panel_set(groups[group_id].select,
				PANEL_TOGGLE_VALUE, 0, FALSE, 0);

		if (tests[id]->enable && tests[id]->type != 2)
				/* at least one is enabled */
		  break;				/* done */
	    }
	    if (id == exist_tests)		/* none was enabled */
		(void)panel_set(groups[group_id].select,
                                PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	  }
	}
  }

  if (!ats_nohost)
      (void)panel_set(mode_item, PANEL_VALUE, intervention, 0);
  /* update the cycle on the screen too */

  if (update_flag)
    print_status();	/* update status panel also */
}

/******************************************************************************
 * Notify procedure for the "Test selections" toggle.			      *
 ******************************************************************************/
/***** global variables *****/
int	select_value=SEL_DEF;		/* default to Default(SEL_DEF) */

/*ARGSUSED*/
select_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  int	test_id, group_id;
  int	tmp;

  select_value = value;

  if (!tty_mode)
  {
    if (option_frame != NULL)	/* destroy the popup test option frame */
      frame_destroy_proc(option_frame);
    (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_INVERTED, 0);
  }
  else
    tty_int_sel();		/* update the control panel */

  selection_flag = TRUE;

  switch (value)
  {
    case SEL_DEF:
	/* disable all the tests first */
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)  /* clear the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = FALSE;
		if (!tty_mode)
		  (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
						0, FALSE, 0);
	    }

	    tests[test_id]->enable = FALSE;
	    if (tests[test_id]->test_no > 1)
	    {
		tests[test_id]->dev_enable = FALSE;
		if (tests[test_id]->which_test > 1) continue;
	    }

	    if (!tty_mode)
	      (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, FALSE, 0);
	    else
	      display_enable(test_id, FALSE);
	}

	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	  if (tests[test_id]->type == 0)	/* this is a default test */
	  {
	    group_id = tests[test_id]->group;
	    if (!groups[group_id].enable)
	    {
		groups[group_id].enable = TRUE;
		if (!tty_mode)
		  (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
	    }
	    tests[test_id]->enable = TRUE;

	    if (tests[test_id]->test_no > 1)
	    {
		tests[test_id]->dev_enable = TRUE;
		for (tmp=test_id; tests[tmp]->which_test != 1; --tmp);
		if (!tty_mode)
	          (void)panel_set(tests[tmp]->select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
		else
		  display_enable(tmp, TRUE);
	    }
	    else
	    {
	      if (!tty_mode)
	        (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
	      else
		display_enable(test_id, TRUE);
	    }
	  }
	}

	if (running == GO)
	{
	  for (test_id=0; test_id != exist_tests; ++test_id)
	    if ((!tests[test_id]->enable) && tests[test_id]->pid != 0) 
		(void)kill(tests[test_id]->pid, SIGINT);
	}

	print_status();
	break;

    case SEL_NON:
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)  /* clear the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = FALSE;
		if (!tty_mode)
		  (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
						0, FALSE, 0);
	    }
	    tests[test_id]->enable = FALSE;
	    if (tests[test_id]->test_no > 1)
	    {
		tests[test_id]->dev_enable = DISABLE;
		if (tests[test_id]->which_test == 1)
		  if (!tty_mode)
		    (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, FALSE, 0);
		  else
		    display_enable(test_id, FALSE);
	    }
	    else
	    {
	      if (!tty_mode)
	        (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, FALSE, 0);
	      else
		display_enable(test_id, FALSE);
	    }

	    if (tests[test_id]->pid != 0)
		(void)kill(tests[test_id]->pid, SIGINT);
	}

	print_status();
	break;

    case SEL_ALL:
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)	/* set the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = TRUE;
		(void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
	    }
	    tests[test_id]->enable = TRUE;
	    if (tests[test_id]->test_no > 1)
	    {
		tests[test_id]->dev_enable = ENABLE;
		if (tests[test_id]->which_test == 1)
		  if (!tty_mode)
		    (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
		  else
		    display_enable(test_id, TRUE);
	    }
	    else
	    {
	      if (!tty_mode)
	        (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
						0, TRUE, 0);
	      else
		display_enable(test_id, TRUE);
	    }
	}

	print_status();		/* display them */
	break;

    default:
	break;
  }
}

/******************************************************************************
 * Destroys the frame without comfirmer, also is the FRAME_DONE_PROC for      *
 * some frames that like to be destroyed without confirmer.		      *
 * Input: done_frame, the handle of the frame to be destroyed.		      *
 ******************************************************************************/
frame_destroy_proc(done_frame)
Frame	done_frame;
{
  (void)window_set(done_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(done_frame);

  if (done_frame == o_frame) o_frame = NULL;
  else if (done_frame == of_frame) of_frame = NULL;
  else if (done_frame == option_frame) option_frame = NULL;
}
