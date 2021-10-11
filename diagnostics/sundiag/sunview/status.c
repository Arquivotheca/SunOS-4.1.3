#ifndef lint
static	char sccsid[] = "@(#)status.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <curses.h>
#include "sundiag.h"
#include "struct.h"
#include "tty.h"
#include "sundiag_msg.h"

#define	GROUP_COL	0		/* group message starting column */
#define	SEL_COL		0		/* test message starting column */
#define	START_ROW	4		/* the row to start status messages */

extern	Icon		passed_icon, failed_icon;
			/* icon objuects, defined in sundiag.c */

extern	print_status();			/* forward declaration */
extern	char	*strcpy();
extern	char	*strcat();
extern	char	*sprintf();

int	fail_flag=0;			/* any enabled tests failed? */
int	sys_error=0;			/* to keep total error count */
int	sys_pass=0;			/* to keep system pass count */

unsigned elapse_count=0;/* to count the elapsed senconds since starting tests */
unsigned last_elapse;	/* time of the day when elapse_count was updated */

static	Panel_item	status_item, pass_item, error_item, elapse_item;
static	int		start_status_row=START_ROW;
			/* current row to start printing the status */

/******************************************************************************
 * Initializes the STATUS subwindow. Called by main().			      *
 ******************************************************************************/
init_status()
{
	status_item = panel_create_item(sundiag_status, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"System status: idle",
			PANEL_ITEM_X,		ATTR_COL(10),
			PANEL_ITEM_Y,		ATTR_ROW(0),
			0);

	pass_item = panel_create_item(sundiag_status, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"System passes: 0",
			PANEL_ITEM_X,		ATTR_COL(1),
			PANEL_ITEM_Y,		ATTR_ROW(1),
			0);

	error_item = panel_create_item(sundiag_status, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"Total errors: 0",
			PANEL_ITEM_X,		ATTR_COL(23),
			PANEL_ITEM_Y,		ATTR_ROW(1),
			0);

	elapse_item = panel_create_item(sundiag_status, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"Elapsed time: 000:00:00",
			PANEL_ITEM_X,		ATTR_COL(10),
			PANEL_ITEM_Y,		ATTR_ROW(2),
			0);

	print_status();			/* also display the initial status */
}

/******************************************************************************
 * Update the "System status" item in the STATUS subwindow according to the   *
 * "running" flag. Also, set the icon to either "failed_icon" or "passed_icon"*
 * according to the "fail_flag" flag.					      *
 ******************************************************************************/
status_display()
{
  char	buff[40];
  int	keep_x, keep_y;

  (void)strcpy(buff, "System status: ");

  switch (running)
  {
    case IDLE:
	(void)strcat(buff, "idle");
	break;
    case GO:
	(void)strcat(buff, "testing");
	break;
    case SUSPEND:
	(void)strcat(buff, "suspended");
	break;
    case KILL:
	(void)strcat(buff, "stopping");
	break;
    default:
	break;
  }

  if (fail_flag)	/* indicate at least one test has failed */
  {
	(void)strcat(buff, "(failed)");
	if (!tty_mode)
	  (void)window_set(sundiag_frame, FRAME_ICON, failed_icon, 0);
  }
  else
    if (!tty_mode)
      (void)window_set(sundiag_frame, FRAME_ICON, passed_icon, 0);

  if (!tty_mode)	/* print on the status screen */
  {
    (void)panel_set(status_item, PANEL_LABEL_STRING, buff, 0);
    if (fail_flag)
      (void)panel_set(status_item, PANEL_LABEL_BOLD, TRUE, 0);
    else 
      (void)panel_set(status_item, PANEL_LABEL_BOLD, FALSE, 0);
  }
  else
  {	
    getyx(stdscr, keep_y, keep_x);
    (void)sprintf(buff, "%-32s", buff);
    mvaddstr(1, SYS_STATUS_COL, buff);
    move(keep_y, keep_x);
    refresh();		/* always on the screen */
  }
}

/******************************************************************************
 * Update the "System passes" item in the STATUS subwindow.		      *
 ******************************************************************************/
pass_display()
{
  char	buff[32];

  (void)sprintf((char *)buff, "System passes: %-4d", sys_pass);
  if (!tty_mode)
    (void)panel_set(pass_item, PANEL_LABEL_STRING, buff, 0);
  else
    (void)mvwaddstr(status_button, 1, SYS_PASS_COL, buff);
    if (max_sys_pass && sys_pass >= max_sys_pass) /* Time to stop */
	if ( running == GO ) {
		if (batch_mode) {
			stop_proc();
			batch_mode = TRUE;
		} else
			stop_proc();
	}
}

/******************************************************************************
 * Update the "Total errors" item in the STATUS subwindow.		      *
 ******************************************************************************/
error_display()
{
  char	buff[32];

  (void)sprintf((char *)buff, "Total errors: %-4d", sys_error);
  if (!tty_mode)
    (void)panel_set(error_item, PANEL_LABEL_STRING, buff, 0);
  else
    (void)mvwaddstr(status_button, 1, TOTAL_ERROR_COL, buff);
}

/******************************************************************************
 * Update the "Elapsed time" item in the STATUS subwindow.		      *
 ******************************************************************************/
time_display()
{
  char	buff[32];
  unsigned hour, minute, second;

  second = elapse_count%60;
  minute = elapse_count/60;
  hour = minute/60;
  minute = minute%60;
  
  (void)sprintf((char *)buff,
		"Elapsed time: %03d:%02d:%02d", hour, minute, second);

  if (!tty_mode)
    (void)panel_set(elapse_item, PANEL_LABEL_STRING, buff, 0);
  else
  {
    (void)mvwaddstr(status_button, 1, ELAPSE_COL, buff);
    if (window_type == STATUS_TTY)
    {
      (void)wrefresh(status_button);
      (void)refresh();			/* restore the cursor */
    }
  }
}

/******************************************************************************
 * Build the test label according to the test i.d.			      *
 * Input: test_id, the internal test i.d.				      *
 ******************************************************************************/
static	char	*build_status_label(test_id)
int	test_id;
{
  char		label_buf[42];
  char		run_flag;
  static char	status_buf[82];

  (void)sprintf((char *)label_buf, "(%s) %s",
			tests[test_id]->devname, tests[test_id]->testname);

  if (tests[test_id]->pid != 0) run_flag = '*';
  else run_flag = ' ';

  (void)sprintf((char *)status_buf, "%c%-16s passes: %-4d errors: %-2d",
	run_flag, label_buf, tests[test_id]->pass, tests[test_id]->error);

  return((char *)status_buf);
}

/******************************************************************************
 * Initializes the PANEL_MESSAGE item for the group label in the STATUS       *
 * subwindow.								      *
 * Input: group_id, the internal group i.d.				      *
 ******************************************************************************/
static	init_group_status(group_id)
int	group_id;			/* index to groups[] */
{
  Panel_item	handle;

  if (!tty_mode)
  {
    handle = panel_create_item(sundiag_status, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	groups[group_id].s_label,
		PANEL_CLIENT_DATA,	group_id,
		PANEL_ITEM_X,		ATTR_COL(GROUP_COL),
		PANEL_ITEM_Y,		ATTR_ROW(start_status_row++),
		0);
    (void)panel_set(handle, PANEL_SHOW_ITEM, TRUE, 0);	/* display it */
    groups[group_id].msg = handle;	/* keep the panel item handle */
  }
  else
  {
    get_next_page();
    (void)mvwaddstr(tty_status[status_page], status_row++, STATUS_1-2,
	groups[group_id].s_label);
  }
}

/******************************************************************************
 * Initializes the PANEL_MESSAGE item for the test label, passes, and errors. *
 * Input: test_id, the internal test i.d.				      *
 ******************************************************************************/
static	init_test_status(test_id)
int	test_id;			/* index to tests[] */
{
  Panel_item	handle;

  if (!tty_mode)
  {
    handle = panel_create_item(sundiag_status, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	build_status_label(test_id),	
		PANEL_CLIENT_DATA,	test_id,
		PANEL_ITEM_X,		ATTR_COL(SEL_COL),
		PANEL_ITEM_Y,		ATTR_ROW(start_status_row++),
		0);
    (void)panel_set(handle, PANEL_SHOW_ITEM, TRUE, 0);	/* display it */
    tests[test_id]->msg = handle;		/* keep the panel item handle */
  }
  else
  {
    get_next_page();
    tests[test_id]->msg = (Panel_item)((status_page << 8) | status_row);
    (void)mvwaddstr(tty_status[status_page], status_row++, STATUS_1,
	build_status_label(test_id));
  }
}

/******************************************************************************
 * Update the pass and error count for the test. Also, update the "system     *
 * status" and "System passes" if necessary.				      *
 * Input: test_id, the internal test i.d. of the test.			      *
 ******************************************************************************/
update_status(test_id)
int	test_id;
{
  int	least;
  int	i, x, y;

  if (tests[test_id]->msg == NULL) return;  /* invalid handle(not on screen) */

  if (!tty_mode)
    (void)panel_set(tests[test_id]->msg, PANEL_LABEL_STRING,
				build_status_label(test_id), 0);	
  else
  {
    x = (int)tests[test_id]->msg & 0xff;	/* row # */
    y = (int)tests[test_id]->msg >> 8;		/* page # */
    (void)mvwaddstr(tty_status[y], x, STATUS_1, build_status_label(test_id));
    if (window_type == STATUS_TTY && status_index == y)
      (void)wrefresh(tty_status[y]);
  }

  if (!fail_flag)		/* no test failed yet */
    if (tests[test_id]->error != 0)
    {
	fail_flag = 1;		/* at least one failed now */
	status_display();	/* show it by updating the "System status" */
    }

  if (tests[test_id]->pass == sys_pass+1)	/* the bottleneck */
  {
    least = sys_pass+1;

    for (i=0; i != exist_tests; ++i)
      if (((tests[i]->enable && tests[i]->dev_enable) || tests[i]->pid != 0) &&
           tests[i]->type != 2)
      /* only if it is enabled or still running(after been killed) */
        if (least > tests[i]->pass) least = tests[i]->pass;

    if (least != sys_pass)
    {
      sys_pass = least;
      pass_display();		/* update the "System passes" */
    }
  }

  if (tty_mode && window_type == STATUS_TTY)
  {
    (void)wrefresh(status_button);
    (void)refresh();		/* restore the cursor */
  }
}

/******************************************************************************
 * Update the pass and error count of all enabled tests.		      *
 ******************************************************************************/
print_status()
{
  Panel_item	item;
  int	group;				/* current group id(index to groups[] */
  int	cur_test_i;			/* current test index(to tests[]) */

  fail_flag = 0;			/* reinitialize the flag */
  sys_error = 0;
  sys_pass = 0;

  if (tty_mode)
  {
    status_page = 0;
    status_row = 0;
    (void)wclear(tty_status[0]);
  }

  if (tty_mode || start_status_row != START_ROW)
  /* need to clear the old status first */
  {
    for (cur_test_i=0; cur_test_i != ngroups; ++cur_test_i)
      if ((item=groups[cur_test_i].msg) != NULL)
      {
	groups[cur_test_i].msg = NULL;
	if (!tty_mode)
	{
	  (void)panel_set(item, PANEL_SHOW_ITEM, FALSE, 0);/* erase it first */
	  (void)panel_destroy_item(item);
	}
      }

    for (cur_test_i=0; cur_test_i != exist_tests; ++cur_test_i)
      if ((item=tests[cur_test_i]->msg) != NULL)
      {
	tests[cur_test_i]->msg = NULL;
	if (!tty_mode)
	{
	  (void)panel_set(item, PANEL_SHOW_ITEM, FALSE, 0);/* erase it first */
	  (void)panel_destroy_item(item);
	}
      }

    start_status_row = START_ROW;
  }

  for (cur_test_i=0; cur_test_i != exist_tests;)
					/* gone through all existing devices */
  {
    if (((tests[cur_test_i]->enable && tests[cur_test_i]->dev_enable) ||
		tests[cur_test_i]->pid != 0) && tests[cur_test_i]->type != 2)
		/* only if it is enabled or still running(after been killed) */
    {
      if (sys_pass == 0) sys_pass = tests[cur_test_i]->pass;

      group = tests[cur_test_i]->group;	/* get the current group */
      init_group_status(group);		/* display the group's status label */

      while (tests[cur_test_i]->group == group)
      {
	if (((tests[cur_test_i]->enable && tests[cur_test_i]->dev_enable) ||
		tests[cur_test_i]->pid != 0) && tests[cur_test_i]->type != 2)
		/* only if it is enabled or still running(after been killed) */
	{
	  init_test_status(cur_test_i);
		/* display the test's msg item */
		/* also initialize the msg item handle */

	  sys_error += tests[cur_test_i]->error;

	  if (sys_pass > tests[cur_test_i]->pass)
	    sys_pass = tests[cur_test_i]->pass;		/* get the smallest */

	  if (!fail_flag)
	    if (tests[cur_test_i]->error != 0)
	      fail_flag = 1;		/* at least one failed now */
	}

	if (++cur_test_i == exist_tests) break;		/* no more */
      }
    }
    else
	++cur_test_i;
  }

  if (tty_mode)
    status_max = status_page;

  status_display();			/* display the testing status line */
  pass_display();			/* display system pass count */
  error_display();			/* display total errors */

  if (tty_mode && window_type == STATUS_TTY)
  {
    (void)wrefresh(status_button);
    (void)wrefresh(tty_status[status_index]);
    (void)refresh();			/* restore the cursor */
  }

  if (!tty_mode)
  {
    item = panel_create_item(sundiag_status, PANEL_MESSAGE,
		PANEL_SHOW_ITEM,	FALSE,
		PANEL_ITEM_X,		ATTR_COL(SEL_COL),
		PANEL_ITEM_Y,		ATTR_ROW(start_status_row),
		0);
    (void)scrollbar_set(status_bar, SCROLL_OBJECT_LENGTH,
		(int)panel_get(item, PANEL_ITEM_Y)-1, 0);
    (void)scrollbar_paint(status_bar);	/* update the bubble */
    (void)panel_destroy_item(item);
  }
}

/******************************************************************************
 * Logs the current test status to INFO file(including "System passes", and   *
 * "Total errors").							      *
 ******************************************************************************/
log_status()
{
  int	cur_test_i;
  char	*temp;
  unsigned hour, minute, second;

  second = elapse_count%60;
  minute = elapse_count/60;
  hour = minute/60;
  minute = minute%60;

  if (fprintf(info_fp,
	" System passes: %d, Total errors: %d, Elapsed time: %03d:%02d:%02d\n",
		sys_pass, sys_error, hour, minute, second) == EOF)
  {
    perror("Writing log file");
    return;
  }

  for (cur_test_i=0; cur_test_i != exist_tests; ++cur_test_i)
  {
    if (tests[cur_test_i]->enable && tests[cur_test_i]->dev_enable &&
         tests[cur_test_i]->type != 2)
    {
	temp = build_status_label(cur_test_i);
	*temp = ' ';		/* don't need the '*' */

	if (fprintf(info_fp, "%s\n", temp) == EOF)
	{
	  perror("Writing log file");
	  break;
	}
    }
  }

  (void)fflush(info_fp);
}

/******************************************************************************
 * Logs the "Enable test" message to the INFO file when a test is enabled     *
 * while the tests are running(System status is "running").		      *
 * Input: cur_test_i, the internal test i.d. of the enabled test.	      *
 ******************************************************************************/
start_log(cur_test_i)
int	cur_test_i;
{
  char	buff[82];
  char	*temp;

  temp = build_status_label(cur_test_i);
  *temp = ' ';			/* don't need the '*' */

  (void)sprintf((char *)buff, enable_test_info, temp); 
					/* "*** Enable test ***\n%s" */

  logmsg(buff, 0, ENABLE_TEST_INFO);		/* log to information file */
}

/******************************************************************************
 * Logs the "Disable test" message to the INFO file when a test is disabled   *
 * while the tests are running(System status is "running").		      *
 * Input: cur_test_i, the internal test i.d. of the disabled test.	      *
 ******************************************************************************/
stop_log(cur_test_i)
int	cur_test_i;
{
  char	buff[82];
  char	*temp;

  temp = build_status_label(cur_test_i);
  *temp = ' ';			/* don't need the '*' */

  (void)sprintf((char *)buff, disable_test_info, temp);  
				/* "*** Disable test ***\n%s" */

  logmsg(buff, 0, DISABLE_TEST_INFO);		/* log to information file */
}
/******************************************************************************
 * Logs the "Failed test" message to both INFO and ERROR files when a test    *
 * is failed.								      *
 * Input: cur_test_i, the internal test i.d. of the failed test.	      *
 ******************************************************************************/
failed_log(cur_test_i)
int	cur_test_i;
{
  char	buff[82];
  char	*temp;

  temp = build_status_label(cur_test_i);
  *temp = ' ';				/* don't need the '*' */

  (void)sprintf((char *)buff, test_failed_info, temp);  
					/* "*** Failed test ***\n%s" */

  logmsg(buff, 1, TEST_FAILED_INFO);	/* log to both INFO and ERROR files */
}


