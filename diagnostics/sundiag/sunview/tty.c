#ifndef	lint
static	char sccsid[] = "@(#)tty.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <curses.h>
#include "sundiag.h"
#include "struct.h"
#include "tty.h"
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#ifdef sun4
#include <mon/idprom.h>
#endif

/* global's */
WINDOW	*tty_control[TTY_WINNO];/* control windows,up to TTY_WINNO pages each */
WINDOW	*tty_status[TTY_WINNO];	/* status windows, up to TTY_WINNO pages each */
WINDOW	*tty_option=NULL;	/* option window(share just one ) */
WINDOW	*tty_other=NULL;	/* other window(share just one) */
WINDOW	*control_button=NULL;	/* the window to display command button */
WINDOW	*status_button=NULL;	/* the window to display test status */
WINDOW	*console_tty=NULL;	/* console/error message window(to draw box) */
WINDOW	*message_tty=NULL;	/* console/error message subwindow */
int	tty_ppid=0;		/* the parent pid of the sundiag */
int	window_type=CONTROL_TTY;/* type of window currently displayed */
int	control_index;		/* index of current control window displayed */
int	status_index;		/* index of current status window displayed */
int	control_max;		/* the max. index of control windows(#-1) */
int	status_max;		/* the max. index of status windows(#-1) */
int	status_page;		/* current status page */
int	status_row;		/* current status row */
char	*arg[20];		/* up to 20 command line arguments */
int	arg_no;			/* number of arguments available */
char	com_line[82];	/* the image of the command line(NULL-terminated) */

/* external's */
extern	char	*build_sel_label();	/* in tests.c */
extern	char	get_com();		/* in ttyprocs.c */
extern	char	*strtok();
extern	char	*sprintf();
extern	Notify_value	kill_proc();	/* in sundiag.c */
extern	Panel   init_opt_panel();

/* static's */
static	int	command_row;	/* the row to put the command promt */
static	int	message_row;	/* the row to put the error message */
static	char	kill_char;	/* the line kill character */
static	char	del_char;	/* the character erase character */
static	int	status_lines;	/* number of lines available for test status */
static	int	other_type;	/* who is using the "other window"(see below) */
#define	OTHER_OPTION	  1
#define	OTHER_OPFILE	  2
#define OTHER_LOGFILE	  3
#define OTHER_EEPROM	  4
#define OTHER_SCHEDULE	  5
#define OTHER_PROCESSORS  6

/* forward declarations */
extern	switch_to_control();
static	switch_to_status();

static	struct	com_tbl	control_tbl1[]=
{
  {"start", 's'},
  {"options", 'o'},
  {"logfiles", 'l'},
  {"status", 'a'},
  {"optfiles", 'f'},
  {"stop", 't'},
  {"quit", 'q'},
  {"reset", 'r'},
  {"suspend", 'p'},
  {"resume", 'u'},
  {"eeprom", 'e'},
  {"schedule", 'm'},
  {"processors", 'c'},
  {"intervention", 'i'},
  {"help", 'h'},
  {"next", 'n'},
  {"", '\0'}
};

static	struct	com_tbl	control_tbl2[]=
{
  {"tests", 'c'},
  {"", '\0'}
};

static	struct	com_tbl	control_tbl3[]=
{
  {"default", 'd'},
  {"all", 'a'},
  {"none", 'n'},
  {"", '\0'}
};

static	struct	com_tbl status_tbl[]=
{
  {"done", 'd'},
  {"next", 'n'},
  {"help", 'h'},
  {"", '\0'}
};

static	struct	com_tbl	popup_tbl[]=
{
  {"default", 'f'},
  {"done", 'd'},
  {"help", 'h'},
  {"", '\0'}
};

/******************************************************************************
 * init_tty_control(), initializes all the control windows needed.	      *
 ******************************************************************************/
static	init_tty_control()
{
  int	lines;			/* # of lines available for displaying tests */
  int	pages;
  int	group;			/* current group id(index to groups[] */
  int	cur_test_i;		/* current test index(to tests[]) */
  int	select_v;
  int	which_line;
  char	temp_buf[82];

  lines = LINES-12;
  for (pages=0, cur_test_i=0, group= -1; pages < TTY_WINNO; ++pages)
  {
    tty_control[pages] = newwin(lines, COLS-2, 8, 1);
    if (cur_test_i == exist_tests) break;		/* done */
    for (which_line=0; which_line != lines; ++which_line)
    {
      if (tests[cur_test_i]->group != group)
      {
	group = tests[cur_test_i]->group;
        groups[group].first = cur_test_i;
	/* keep the first test in the group */

	(void)mvwaddstr(tty_control[pages], which_line++,
					16, groups[group].c_label);

        if (which_line == lines)		/* hit the end */
	{
	  if (pages+1 == TTY_WINNO) break;	/* use up */
	  tty_control[++pages] = newwin(lines, COLS-2, 8, 1);
	  which_line = 0;
	}
      }

      if (tests[cur_test_i]->which_test == 1)
      {
	if (tests[cur_test_i]->test_no > 1)	/* multiple tests */
	{
	  if (tests[cur_test_i]->type != 2)
	  /* it is not a disabled intervention tests */
	    select_v = tests[cur_test_i]->dev_enable;
	  else
	    select_v = FALSE;
	}
	else
	  select_v = (tests[cur_test_i]->enable && tests[cur_test_i]->type!=2);
	
	(void)sprintf(temp_buf, "%s%-30s%s", select_v?"->":"  ",
	  build_sel_label(cur_test_i),
		tests[cur_test_i]->popup?"[OPTION]":"");

	(void)mvwaddstr(tty_control[pages], which_line, TEST_COL, temp_buf);
	tests[cur_test_i]->select = (Panel_item)(which_line | (pages << 8));
      }
      else
	--which_line;			/* reuse this line */

      if (++cur_test_i == exist_tests) break;		/* no more */
    }
    if (cur_test_i == exist_tests) break;		/* done */
  }
  control_max = pages;

  control_button = newwin(5, COLS-2, 3, 1);
  (void)sprintf(temp_buf, "[%s]  [%s]  [%s]  [%s]  [%s]  [%s]  [%s]", "START",
	"OPTIONS", "STATUS", "SCHEDULE", "OPTFILES", "STOP", "QUIT");
  (void)mvwaddstr(control_button, 0, 4, temp_buf);
  (void)sprintf(temp_buf, "[%s]  [%s]  [%s]  [%s]  [%s]  [%s]  [%s]", "RESET",
	"SUSPEND", "RESUME", " EEPROM ", "LOGFILES", "HELP", "NEXT");
  (void)mvwaddstr(control_button, 1, 4, temp_buf);
  (void)sprintf(temp_buf, "[%s]", "PROCESSORS");

  (void)mvwaddstr(control_button, 2, 4, temp_buf);
  (void)sprintf(temp_buf, "[INTERVENTION]: %-18s [TESTS]: %-20s", 
	intervention?"Enable":"Disable",
	select_value!=0?
	(select_value>1?"ALL [DEFAULT/NONE]":"NONE [ALL/DEFAULT]"):
	"DEFAULT [NONE/ALL]");
  (void)mvwaddstr(control_button, 3, 6, temp_buf);
}

/******************************************************************************
 * get_next_page(), set/initialize next available status page.		      *
 ******************************************************************************/
get_next_page()
{
  if (status_row >= status_lines)
  {
    ++status_page;
    status_row = 0;

    if (tty_status[status_page] == NULL)	/* not created yet */
      tty_status[status_page] = newwin(status_lines, COLS-2, 6, 1);

    (void)wclear(tty_status[status_page]);
  }
}
  
/******************************************************************************
 * init_tty_option(), initializes the option window.			      *
 ******************************************************************************/
static	init_tty_option()
{
  tty_option = newwin(LINES-7, COLS-2, 3, 1);
  (void)wclear(tty_option);
}

/******************************************************************************
 * init_tty_other(), initializes the other window.			      *
 ******************************************************************************/
static	init_tty_other()
{
  tty_other = newwin(LINES-7, COLS-2, 3, 1);
  (void)wclear(tty_other);
}

/******************************************************************************
 * init_tty_status(), initializes status_button and console_tty windows.      *
 ******************************************************************************/
static	init_tty_status()
{
  status_button = newwin(2, COLS-2, 3, 1);
  (void)wclear(status_button);
  (void)mvwaddstr(status_button, 0, SYS_PASS_COL, "[NEXT]  [DONE]  [HELP]");
				/* command to go back to control window */
  time_display();		/* initilaize the "Elapsed time" couter */

  console_tty = newwin(6, COLS-2, LINES-10, 1);
  /* this window is opened just for displaying the "box" */
  box(console_tty, '|', '-'); 
  mvwaddch(console_tty, 0, 0, ' ');
  mvwaddch(console_tty, 0, COLS-3, ' ');
  mvwaddch(console_tty, 5, 0, ' ');
  mvwaddch(console_tty, 5, COLS-3, ' ');
  message_tty = subwin(console_tty, 4, COLS-4, LINES-9, 2);
  (void)wclear(message_tty);
  scrollok(message_tty, TRUE);	/* allow scrolling */

  status_lines = LINES-3-3-6-4;

  tty_status[0] = newwin(status_lines, COLS-2, 6, 1);	/* get the first page */
}

/******************************************************************************
 * init_ttys(), creates/initializes the screens/windows needed.		      *
 ******************************************************************************/
init_ttys()
{
  char	temp_buf[82];
  int	i;

  (void)initscr();

  if (COLS < MIN_WIDTH)
  {
    (void)fprintf(stderr,
      "Sundiag: The screen needs to have at least %d columns.\n", MIN_WIDTH);
    sundiag_exit(1);
  }
  if (LINES < MIN_HEIGHT)
  {
    (void)fprintf(stderr,
      "Sundiag: The screen needs to have at least %d rows.\n", MIN_HEIGHT);
    sundiag_exit(1);
  }

  cbreak();
  noecho();
  nonl();
  clear();
  kill_char = killchar();
  del_char = erasechar();

  box(stdscr, '|', '-');
  (void)sprintf(temp_buf, "Sundiag %s  CPU:%s", version, cpuname);
  mvaddstr(1,2,temp_buf);
  move(2,1);
  for (i=COLS-2; i!=0; --i) addch('-');
  command_row = LINES-3;
  message_row =command_row+1;
  move(command_row-1, 1);
  for (i=COLS-2; i!=0; --i) addch('-');
  mvaddstr(message_row, 2, "Message: ");
  mvaddstr(command_row, 2, "Command: ");
  (void)refresh();

  init_tty_control();		/* initialize the control window(s) */
  init_tty_status();		/* initialize the status window(s) */
  init_tty_option();		/* initialize the option window */
  init_tty_other();		/* initialize the "other" window */

  if (window_type == STATUS_TTY)
    switch_to_status();
  else
    switch_to_control();
}

/******************************************************************************
 * tty_message(), print the passed message in the "message:" line	      *
 * Input: msg, the message to be printed.				      *
 * Note: the message line will be cleared if msg == NULL.		      *
 ******************************************************************************/
#define	BEG_MESSAGE	11		/* starting message(status) line */

tty_message(msg)
char	*msg;				/* message to be displayed */
{
  int	keep_x, keep_y;

  getyx(stdscr, keep_y, keep_x);	/* save the current position */
  move(message_row, BEG_MESSAGE-1);	/* get rid of possible garbage too */
  clrtoeol();				/* remove the old message first */
  mvaddch(message_row, COLS-1, '|');
  move(message_row, BEG_MESSAGE);

  if (msg != NULL)			/* just clear the message if NULL */
					/* refresh() needs to be called later */
  {
    if (strlen(msg) > MIN_WIDTH-4)
      *(msg+MIN_WIDTH-4) = '\0';	/* truncate them off */
    addstr(msg);
    addch(0x07);			/* bell */
  }

  move(keep_y, keep_x);
  (void)refresh();			/* display it */
}

/******************************************************************************
 * console_message(), print the passed message in the "console box" of the    *
 * status window.							      *
 * Input: msg, the message to be printed.				      *
 ******************************************************************************/
console_message(msg)
char	*msg;
{
  if (message_tty == NULL) return;	/* not initialized yet */

  while (*msg != '\0')
  {
    if (*msg != '\r') (void)waddch(message_tty, *msg);
    ++msg;
  }
  if (window_type == STATUS_TTY)
  {
    (void)wrefresh(message_tty);	/* display the message */
    (void)refresh();			/* restore the cursor */
  }
}

/******************************************************************************
 * tty_test_sel_proc, the tty version of the test selection procedure.	      *
 * input: devname, the device name.					      *
 ******************************************************************************/
tty_test_sel_proc(devname)
char	*devname;
{
  int	test_id, group_id;
  int	value;

  for (test_id=0; test_id != exist_tests; ++test_id)
  /* gone through all existing devices to find the specified device */
    if (strcmp(devname, tests[test_id]->devname) == 0) break;

  if (test_id == exist_tests)		/* unknown test */
  {
    tty_message(com_err);
    return;
  }

  if (tests[test_id]->type == 2)
  {
	(void)confirm(
	"Please enable the 'Intervention' mode before select this test.", TRUE);
	return;	/* return if disabled intervention/manufacturing tests */
  }

  if (tests[test_id]->test_no > 1)
  {
    value = !tests[test_id]->dev_enable;
    multi_tests(test_id, value);
    /* handle multiple tests for a device */
  }
  else
  {
    tests[test_id]->enable = !tests[test_id]->enable;
    value = tests[test_id]->enable;

    if (tests[test_id]->enable)		/* if test is to be enabled */
    {
      print_status();			/* display it on status subwindow */

      if (running == GO || running == SUSPEND)	/* if tests are running */
        start_log(test_id);		/* log to information file */
    }
    else				/* if test is to be disabled */
    {
      if (tests[test_id]->pid == 0)	/* if test is not currently running */
	print_status();			/* just remove it */
      else
	(void)kill(tests[test_id]->pid, SIGINT);

      if (running == GO || running == SUSPEND)	/* if tests are running */
        stop_log(test_id);		/* log to information file */
    }
  }

  display_enable(test_id, value);	/* display it */

  group_id = tests[test_id]->group;	/* get the group number of the test */
  test_id = groups[group_id].first;	/* get the first test in the group */
  for (; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->group != group_id) break;	/* none was enabled */

    if (tests[test_id]->dev_enable && (tests[test_id]->enable ||
		tests[test_id]->test_no > 1) && tests[test_id]->type != 2)
    {
	if (!groups[group_id].enable)
	  groups[group_id].enable = TRUE;
	return;				/* done */
    }
  }

  if (groups[group_id].enable)
    groups[group_id].enable = FALSE;
}

/******************************************************************************
 * switch_to_control(), switch the screen to dispaly control windows.	      *
 ******************************************************************************/
switch_to_control()
{
  window_type = CONTROL_TTY;
  control_index=0;		/* start from 0 */
  touchwin(control_button);
  touchwin(tty_control[control_index]);
  (void)wrefresh(control_button);
  (void)wrefresh(tty_control[control_index]);
  (void)refresh();		/* restore the cursor */
}

/******************************************************************************
 * switch_to_status(), switch the screen to display status windows.	      *
 ******************************************************************************/
static	switch_to_status()
{
  window_type = STATUS_TTY;
  status_index=0;		/* start from 0 */
  touchwin(status_button);
  touchwin(tty_status[0]);
  touchwin(console_tty);
  (void)wrefresh(status_button);
  (void)wrefresh(tty_status[0]);
  (void)wrefresh(console_tty);
  (void)refresh();		/* restore the cursor */
}

/******************************************************************************
 * proc_control_com(), processes control window command.		      *
 ******************************************************************************/
static	char	*inactivated_msg="Inactivated while running tests!";
#define ID_MACH_SHIFT           24      /* machine id in MSB of gethostid() */
#define ID_MACH_MASK            0xff
static	proc_control_com()		/* process control window command */
{
  unsigned long machine_id;

  if (arg_no == 1)
  {
      switch (get_com(control_tbl1, arg[0]))
      {
        case 'a':			/* go to status window */
	  switch_to_status();
	  return;

	case 'e':			/* check EEPROM */
#ifdef sun4	/* Check for Sun4c */
	if ( cpu_is_sun4c() )
		return;
#endif sun4
#ifdef sun386
	  tty_message("The EEPROM feature is not available on Sun386i!");
	  return;
#else  sun386
	  if (running == GO)
	    tty_message(inactivated_msg);
	  else
	  {
	    window_type = OTHER_TTY;
	    other_type = OTHER_EEPROM;
	    (void)wclear(tty_other);
	    eeprom_get_proc();
	    tty_eeprom();		/* display the eeprom settings */
	  }
	  return;
#endif	sun386

	case 'f':			/* option files */
	  if (running == GO)
	    tty_message(inactivated_msg);
	  else
	  {
	    window_type = OTHER_TTY;
	    other_type = OTHER_OPFILE;
	    (void)wclear(tty_other);
	    tty_opfiles(TRUE);
	  }
	  return;

        case 'c':                       /* test times */
          if (running == GO)
            tty_message(inactivated_msg);
          else
          {   
            window_type = OTHER_TTY;
            other_type = OTHER_PROCESSORS;
            (void)wclear(tty_other);
            tty_processors();           /* display the processors enable menu */
          }
          return;

	case 'i':			/* intervention enable/disable */
	  interven_proc((Panel_item)0, !intervention, (Event *)0);
	  return;
	    
	case 'l':			/* log files */
	  if (running == GO)
	    tty_message(inactivated_msg);
	  else
	  {
	    window_type = OTHER_TTY;
	    other_type = OTHER_LOGFILE;
	    (void)wclear(tty_other);
	    tty_log_files();
	  }
	  return;

	case 'm':			/* test times */
	  if (running == GO)
	    tty_message(inactivated_msg);
	  else
	  {
	    window_type = OTHER_TTY;
	    other_type = OTHER_SCHEDULE;
	    (void)wclear(tty_other);
	    tty_schedule();		/* display the schedule menu */
	  }
	  return;

        case 'n':			/* display next control window */
	  if (control_max == 0)
	  {
	    tty_message("No next control screen!");
	    return;
	  }

	  if (control_index == control_max)
	    control_index = 0;
	  else
	    ++control_index;

	  touchwin(tty_control[control_index]);
	  (void)wrefresh(tty_control[control_index]);
	  (void)refresh();		/* restore the cursor */
	  return;

	case 'o':			/* system options */
	  if (running == GO)
	    tty_message(inactivated_msg);
	  else
	  {
	    window_type = OTHER_TTY;
	    other_type = OTHER_OPTION;
	    (void)wclear(tty_other);
	    tty_options();		/* display the current options */
	  }
	  return;

	case 'p':			/* suspend tests */
	  suspend_proc();
	  return;

	case 'q':			/* quit sundiag */
	  (void)kill_proc();		/* no return */

	case 'r':
	  reset_proc();			/* reset tests */
	  return;

	case 's':			/* start tests */
	  start_proc();
	  return;

	case 't':
	  stop_proc();			/* stop tests */
	  return;

	case 'u':			/* resume tests */
	  resume_proc();
	  return;

	case 'h':			/* help */
	  tty_help();
	  return;

	default:
      	  tty_test_sel_proc(arg[0]);
	  return;
      }
  }
  else if (arg_no == 2)
  {
    switch (get_com(control_tbl2, arg[0]))
    {
	case 'c':			/* global test selection */
	  switch (get_com(control_tbl3, arg[1]))
	  {
	    case 'd':
	      select_proc((Panel_item)0, SEL_DEF, (Event *)0);
	      return;

	    case 'a':
	      select_proc((Panel_item)0, SEL_ALL, (Event *)0);
	      return;

	    case 'n':
	      select_proc((Panel_item)0, SEL_NON, (Event *)0);
	      return;

	    default:
	      break;
	  }

	default:
	  tty_test_opt_proc(arg[0], arg[1]);
	  return;
    }
  }

  tty_message(format_err);
}

/******************************************************************************
 * proc_status_com(), processes status window command.			      *
 ******************************************************************************/
static	proc_status_com()		/* process status window command */
{
  if (arg_no == 1)
  {
    switch (get_com(status_tbl, arg[0]))
    {
      case 'd':				/* go back to control window */
	switch_to_control();
	return;

      case 'n':				/* display next status window */
	if (status_max == 0)
	{
	  tty_message("No next status screen!");
	  return;
	}

	if (status_index == status_max)
	  status_index = 0;
	else
	  ++status_index;

	touchwin(tty_status[status_index]);
	(void)wrefresh(tty_status[status_index]);
	(void)refresh();		/* restore the cursor */
	return;

      case 'h':				/* help */
	tty_help();
	return;

      default:
	break;
    }
  }

  tty_message(com_err);
}

/******************************************************************************
 * proc_option_com(), processes option window command.			      *
 ******************************************************************************/
static	proc_option_com()		/* process option window command */
{
  if (arg_no == 1)
    switch (get_com(popup_tbl, arg[0]))
    {
      case 'd':				/* go back to control window */
	switch_to_control();
	return;

      case 'f':				/* default options */
	option_default_proc((Panel_item)option_id);	/* process default */
	(void)init_opt_panel(option_id);	/* redisplay the options */
	return;

      case 'h':				/* help */
	tty_help();
	return;

      default:
	break;
    }

  if (tty_test_option_proc())		/* process individual test option */
    (void)init_opt_panel(option_id);	/* redisplay(update) the options */
}

/******************************************************************************
 * proc_help_com(), processes help window command.			      *
 ******************************************************************************/
static	proc_help_com()			/* process help window command */
{
}

/******************************************************************************
 * proc_other_com(), processes other window command.			      *
 ******************************************************************************/
static	proc_other_com()		/* process other window command */
{
  switch (other_type)
  {
    case OTHER_OPTION:			/* handling "system options" */
	tty_option_proc();
	break;

    case OTHER_OPFILE:			/* handling "option files" */
	tty_opfile_proc();
	break;

    case OTHER_LOGFILE:			/* handling "log files" */
	tty_logfile_proc();
	break;
    case OTHER_EEPROM:			/* handling "eeprom entries" */
	tty_eeprom_proc();
	break;
    case OTHER_PROCESSORS:              /* handling "processors entries" */
        tty_processors_proc();
        break;
    case OTHER_SCHEDULE:			/* handling "ttime entries" */
	tty_schedule_proc();
	break;
  }
}

/******************************************************************************
 * com_parser(), executes the user's command.				      *
 ******************************************************************************/
com_parser(command)
char	*command;			/* the command line */
{
  char	*ptr;

  if (*command == '\0') return;	/* nothing in there */

  arg_no = 0;
  if ((ptr=strtok(command, " \t")) != NULL)
  {
    do
      arg[arg_no++] = ptr;		/* parse out the tokens */
    while ((ptr=strtok((char *)NULL, " \t")) != NULL && arg_no < 20);
  } 

  switch (window_type)
  {
    case CONTROL_TTY:			/* control window is up */
      proc_control_com();		/* process control window command */
      break;

    case STATUS_TTY:			/* status window is up */
      proc_status_com();		/* process status window command */
      break;

    case OPTION_TTY:			/* option window is up */
      proc_option_com();		/* process option window command */
      break;

    case HELP_TTY:			/* help window is up */
      proc_help_com();			/* process help window command */
      break;

    case OTHER_TTY:			/* other windows is up */
      proc_other_com();			/* process other window command */
      break;
  }
}

/******************************************************************************
 * message_input(), the event handler for the "pipe" read data available.     *
 ******************************************************************************/
Notify_value	message_input()
{
  char	temp_buf[82];
  int	n;

  n = read(pfd[0], temp_buf, 81);	/* read as much as we can */
  if (n > 0)
  {
    temp_buf[n] = '\0';	/* NULL-terminated */
    console_message(temp_buf);		/* display the message */
  }

  return(NOTIFY_DONE);
}

/******************************************************************************
 * console_input(), the event handler for pseudo console data available.      *
 ******************************************************************************/
Notify_value	console_input()
{
  char	temp_buf[82];
  int	n;

  n = read(pty_fd, temp_buf, 81);	/* read as much as we can */
  if (n > 0)
  {
    temp_buf[n] = '\0';	/* NULL-terminated */
    console_message(temp_buf);		/* display the message */
  }

  return(NOTIFY_DONE);
}

/******************************************************************************
 * handle_input(), the event handler for the "stdin" input pending.	      *
 ******************************************************************************/
#define	BEG_COMMAND	11		/* starting command line prompt */
static	int	ind=BEG_COMMAND;	/* current command line column number */

Notify_value	handle_input()
{
  char	ch;
  int	i, j;

  if ((ch=getch()) != ERR)
  {
    if (ind == BEG_COMMAND)		/* clear the last command */
    {
      move(command_row, ind-1);		/* get rid of possible garbage too */
      clrtoeol();
      mvaddch(command_row, COLS-1, '|');
      move(message_row, BEG_MESSAGE);
      if (inch() != ' ') tty_message((char *)NULL);
      move(command_row, ind);
    }

    if (ch >= ' ' && ch <= '~' && ind < MIN_WIDTH-4)
    {
      ++ind;
      addch(ch);
    }
    else if (ch == '\r')		/* Return key */
    {
      for (i=BEG_COMMAND, j=0; i != ind; ++i, ++j)
      {
	move(command_row, i);
	com_line[j] = inch();		/* read them back from glass */
      }
      com_line[j] = '\0';		/* NULL-terminated */

      ind = BEG_COMMAND;
      move(command_row, ind);

      com_parser(com_line);		/* execute the command */
    }
    else if (ch == del_char)		/* del-char key */
    {
      if (ind > BEG_COMMAND)
      {
	--ind;
        move(command_row, ind);
	addch(' ');
	move(command_row, ind);
      }
    }
    else if (ch == kill_char)		/* del-line key */
    {
      ind = BEG_COMMAND;
      move(command_row, ind-1);		/* get rid of possible garbage too */
      clrtoeol();
      mvaddch(command_row, COLS-1, '|');
      move(command_row, ind);
    }
    else if (ch == 0x0c)		/* ctrl-L, redisplay */
      (void)wrefresh(curscr);		/* redraw the screen */
    else if (ch == 0x18 || ch == -1)	/* ctrl-X(or ATS mode), background it */
    {
	ind = BEG_COMMAND;
	puts(CL);
	resetty();
	unset_input_notify();		/* unregistered before closing */

	(void)close(0);	/* redirect stdin, stdout, and stderr to /dev/null */
	(void)open("/dev/null", O_RDWR);
	(void)dup2(0, 1);
	(void)dup2(0, 2);

	(void)kill(tty_ppid, SIGTERM);	/* wake up shell */
	tty_ppid = 0;
	(void)signal(SIGHUP, SIG_IGN);	/* ignore SIGHUP */

	return(NOTIFY_DONE);
    }

    (void)refresh();
  }

  return(NOTIFY_DONE);
}

/******************************************************************************
 * tty_int_sel(), updates both "intervention:" and "test selection:".	      *
 ******************************************************************************/
tty_int_sel()
{
  char	temp_buf[82];

  (void)sprintf(temp_buf, "[INTERVENTION]: %-18s [TESTS]: %-20s", 
	intervention?"Enable":"Disable",
	select_value!=0?
	(select_value>1?"ALL [DEFAULT/NONE]":"NONE [ALL/DEFAULT]"):
	"DEFAULT [NONE/ALL]");
  (void)mvwaddstr(control_button, 3, 6, temp_buf);

  if (window_type == CONTROL_TTY)
  {
    (void)wrefresh(control_button);
    (void)refresh();			/* restore cursor */
  }
}

/******************************************************************************
 * display_enable(), dispaly "->" for the specified enabled test.	      *
 * Input: test_id, the test number to be enabled.			      *
 *	  enable, TRUE, if to be enabled; FALSE, if to be disabled.	      *
 ******************************************************************************/
display_enable(test_id, enable)
int	test_id;
int	enable;
{
  int	row;
  int	page;
  char	*temp;

  row = (int)tests[test_id]->select & 0xff;
  page = (int)tests[test_id]->select >> 8;

  if (enable)
    temp = "->";
  else
    temp = "  ";		/* erase the "->" mark */

  (void)mvwaddstr(tty_control[page], row, TEST_COL, temp);
  if (window_type == CONTROL_TTY && control_index == page)
  {
    (void)wrefresh(tty_control[page]);
    (void)refresh();		/* restore cursor */
  }
}

/******************************************************************************
 * term_tty(), cleans up curses routine before exit. it restores the terminal *
 * to the state it was before initscr().				      *
 ******************************************************************************/
term_tty()
{
  clear();				/* clear the screen before exit */
  (void)refresh();
  endwin();				/* restore terminal state */
}

/******************************************************************************
 * restore_term_tty_state(), cleans up curses routine before exiting from an  *
 * abnormal condition. It restores the terminal (shelltool/cmdtool)           *
 * to the state it was before the initscr().                                  *
 ******************************************************************************/restore_term_tty_state()
{
  clear();                              /* clear the screen before exit */
  scroll();                             /* scrolls 1 line for cmdtool */
  endwin();                             /* restore terminal state */
}

