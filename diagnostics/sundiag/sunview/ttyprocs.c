#ifndef	lint
static	char sccsid[] = "@(#)ttyprocs.c 1.1 92/07/30 Copyright Sun Micro";
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

int	option_id;	/* test i.d. of the current option popup */
char	*com_err="Command error!";	/* command error message */
char	*format_err="Command format error!";  /* format error message */

extern 	char 	run_time_file[];
extern  char	**get_file_names();
extern	char	*strcpy();
extern	char	*sprintf();
extern	char	*getenv();
extern	Panel   init_opt_panel();
extern	Notify_value	kill_proc();
typedef void	(*void_func)();		/* used to shut up lint */

static	int	max_opfile=0;	/* max. page # of option filename list */
static	int	cur_opfile=0;	/* current page # of displayed filename list */
static	char	tty_logfile='e';/* current log file to be acted on */
static  char	**name_list=NULL;
static	char	term_type[22];

static	struct com_tbl	option_tbl1[]=
{
  {"default", 'f'},
  {"done", 'd'},
  {"corefile", 'o'},
  {"singlepass", 's'},
  {"quicktest", 'q'},
  {"verbose", 'v'},
  {"trace", 't'},
  {"autostart", 'a'},
  {"runerror", 'r'},
  {"sendmail", 'm'},
  {"help", 'h'},
  {"", '\0'}
};

static	struct com_tbl	option_tbl2[]=
{
  {"maxpasses", 'x'},
  {"maxerrors", 'y'},
  {"concurrent", 'c'},
  {"logperiod", 'l'},
  {"eaddress", 'e'},
  {"printer", 'p'},
  {"", '\0'}
};

static	struct com_tbl	optfile_tbl1[]=
{
  {"done", 'd'},
  {"load", 'l'},
  {"store", 's'},
  {"remove", 'r'},
  {"next", 'n'},
  {"help", 'h'},
  {"", '\0'}
};

static	struct com_tbl	optfile_tbl2[]=
{
  {"name", 'm'},
  {"", '\0'}
};

static	struct com_tbl	logfile_tbl1[]=
{
  {"done", 'd'},
  {"display", 'l'},
  {"remove", 'r'},
  {"print", 'p'},
  {"help", 'h'},
  {"", '\0'}
};

static	struct com_tbl	logfile_tbl2[]=
{
  {"name", 'm'},
  {"", '\0'}
};

static	struct com_tbl	logfile_tbl3[]=
{
  {"error", 'e'},
  {"info", 'i'},
  {"unix", 'u'},
  {"", '\0'}
};

static	struct com_tbl	eeprom_tbl1[]=
{
  {"default", 'f'},
  {"done", 'd'},
  {"help", 'h'},
  {"cancel", 'c'},
  {"assertscsia", 'a'},
  {"assertscsib", 'b'},
  {"consoletype", 'x'},
  {"screensize", 'z'},
  {"keyclick", 'k'},
  {"watchdogreboot", 'w'},
  {"unixbootpath", 'u'},
  {"customlogo", 'l'},
  {"", '\0'}
};
static	struct com_tbl	eeprom_tbl2[]=
{
  {"unixbootdevice", 'o'},
  {"bannerstring", 's'},
  {"diagbootpath", 'p'},
  {"diagbootdev", 'g'},
  {"keyboardtype", 't'},
  {"memorysize", 'm'},
  {"memorytestsize", 'i'},
  {"columns", 'n'},
  {"rows", 'r'},
  {"", '\0'}
};

static struct com_tbl processors_tbl1[] =
{
  {"default", 'f'},
  {"done", 'd'},
  {"help", 'h'},
  {"", '\0'}
};

static struct com_tbl processors_tbl2[16];

static	struct com_tbl	schedule_tbl1[]=
{
  {"done", 'd'},
  {"cancel", 'c'},
  {"scheduler", 's'},
  {"help", 'h'},
  {"", '\0'}
};
static	struct com_tbl	schedule_tbl2[]=
{
  {"startdate", 'r'},
  {"starttime", 't'},
  {"stopdate", 'o'},
  {"stoptime", 'm'},
  {"", '\0'}
};
/******************************************************************************
 * tty_options(), display the current options on the "other window".	      *
 ******************************************************************************/
#define	OPTION_COL	24	
tty_options()
{
  char	*enable, *disable;
  char	*tmp;
  int	row;

  mvwaddstr(tty_other, 0, OPTION_COL, "<< System Option Menu >>");
  mvwaddstr(tty_other, 2, OPTION_COL, "[DEFAULT(f)]  [DONE(d)]  [HELP(h)]");

  enable = "Enable";
  disable = "Disable";
  row = 4;

  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[COREFILE(o)]:    %-7s", core_file?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[SINGLEPASS(s)]:  %-7s", single_pass?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[QUICKTEST(q)]:   %-7s", quick_test?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[VERBOSE(v)]:     %-7s", verbose?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[TRACE(t)]:       %-7s", trace?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[AUTOSTART(a)]:   %-7s", auto_start?enable:disable);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[RUNERROR(r)]:    %-7s", run_error?enable:disable);
  switch (send_email)
  {
    case 1: tmp = "On_Error";
	    break;
    case 2: tmp = "Periodically";
	    break;
    case 3: tmp = "Both";
	    break;
    default: tmp = disable;
  }
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[SENDMAIL(m)]:    %-12s", tmp);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[MAXPASSES(x)]:   %-5u", max_sys_pass);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[MAXERRORS(y)]:   %-5u", max_errors);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[CONCURRENT(c)]:  %-5u", max_tests);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[LOGPERIOD(l)]:   %-10u", log_period);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[EADDRESS(e)]:    %-20s", eaddress);
  (void)mvwprintw(tty_other, row++, OPTION_COL,
			"[PRINTER(p)]:     %-20s", printer_name);

  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();			/* restore the cursor */
}

/******************************************************************************
 * tty_log_files(), display the log file menu.				      *
 ******************************************************************************/
#define	LOGFILE_COL	24	
tty_log_files()
{
  char	temp[82];

  mvwaddstr(tty_other, 0, LOGFILE_COL, "<< Log File Menu >>");
  mvwaddstr(tty_other, 2, LOGFILE_COL-6, "[DONE(d)] [HELP(h)]");
  mvwaddstr(tty_other, 3, LOGFILE_COL-6, "[DISPLAY(l)] [REMOVE(r)] [PRINT(p)]");
  (void)sprintf(temp, "[NAME(m)]: %s",
	    tty_logfile=='e'?
	    "ERROR [INFO(i)/UNIX(u)]":
	    (tty_logfile=='i'?
	    "INFO [UNIX(u)/ERROR(e)]":"UNIX [ERROR(e)/INFO(i)]"));
  mvwaddstr(tty_other, 5, LOGFILE_COL-6, temp);

  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();			/* restore the cursor */
}

/******************************************************************************
 * tty_opfiles(), display the option file menu.				      *
 ******************************************************************************/
#define	OPFILE_COL	24	
tty_opfiles(list)
int	list;		/* TRUE, if needs to update the available list */
{
  int	row, col;
  int	x, y, len;
  int	total, index;
  int	i, j;
  char	**temp;

  mvwaddstr(tty_other, 0, OPFILE_COL, "<< Option File Menu >>");
  mvwaddstr(tty_other, 2, OPFILE_COL-10, "[DONE(d)] [HELP(h)]");
  mvwaddstr(tty_other, 3, OPFILE_COL-10,
		"[LOAD(l)] [STORE(s)] [REMOVE(r)] [NEXT(n)]");

  (void)mvwprintw(tty_other, 4, OPFILE_COL-10, "[NAME(m)]: %-20s",option_fname);

  if (list)
  {
    if (name_list != NULL)
      while (*name_list != NULL) free(*name_list++);
    name_list = get_file_names(OPTION_DIR);
    temp = name_list;			/* keep the head */

    row = 7;				/* starting row */
    (void)wmove(tty_other, row, 0);
    (void)wclrtobot(tty_other);		/* clear the rest of the window */
    if (*temp == NULL)			/* didn't find any */
    {
      max_opfile = 0;
      cur_opfile = 0;
      mvwaddstr(tty_other, row, 4, "<none>");
    }
    else
    {
      x = 0;
      total = 0;
      while (*temp != NULL)
      {
        if ((len=strlen(*temp++)) > x) x = len; /* get the longest filename */
        ++total;		/* increment total # of files found */
      }

      len = x+2;		/* number of columns for each filename */
      x = (COLS-6)/len;		/* # of filenames to be displayed in a line */
      y = LINES - 14;		/* available lines */

      max_opfile = (total-1)/(x*y);	/* max. page # */
      if (cur_opfile > max_opfile) cur_opfile = max_opfile;
      index = cur_opfile*x*y;		/* starting filename index */

      for (i=0; i < y; ++i, ++row)
      {
	col = 4;
	for (j=0; j < x && index < total; ++j, ++index, col+=len)
	  mvwaddstr(tty_other, row, col, *(name_list+index));
	if (index == total) break;	/* done */
      }
    }
  }

  (void)mvwaddstr(tty_other, 6, 4, "Available option files:");
  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();			/* restore the cursor */
}

/******************************************************************************
 * get_com(), search the passed command table and return a single character   *
 *	sharthand command to the caller.				      *
 * Input: the pointer to the command table, and the to-be-searched command.   *
 * output: a single character short hand command, or NULL if no match.	      *
 ******************************************************************************/
char	get_com(table, command)
struct	com_tbl	*table;			/* command table */
char	*command;			/* to-be-searched command */
{
  if (strlen(command) == 1) return(*command);	/* already single character */

  while (table->shortname != NULL)
  {
    if (strcmp(command, table->fullname) == 0)	/* found it */
      return(table->shortname);
    ++table;
  }
  return('\0');
}


/******************************************************************************
 * attach to the specified(in ATTACH_TTY) tty(foreground it).		      *
 ******************************************************************************/
#define	ATTACH_TTY	"/tmp/sundiag.tty"
Notify_value	foreground_proc()
{
	FILE	*fp;
	int	i, tmp_pid;
	char	tmp[26];

	if ((fp=fopen(ATTACH_TTY, "r")) == NULL)  /* open the info. file */
	  return(NOTIFY_DONE);		/* ignored if failed */
	(void)fgets(tmp, 20, fp);	/* get the graber's pid */
	tmp_pid = atoi(tmp);	/* save the pid so we can kill it later */
	if (kill(tmp_pid, 0) != 0)	/* check if this is a valid pid */
	{
	  (void)fclose(fp);
	  return(NOTIFY_DONE);		/* ignored if it is not a valid pid */
	}

	term_tty();			/* safe to clean up the curses now */

	if (tty_ppid != 0)		/* grab it from other tty */
	{
	  (void)kill(tty_ppid, SIGKILL);/* wake up that shell */
	  unset_input_notify();		/* unregistered before closing */
	}
	tty_ppid = tmp_pid;

	(void)setpgrp(0, tty_ppid);
	(void)fgets(tmp, 20, fp);	/* get the tty name */
	i = strlen(tmp);
	tmp[i-1] = '\0';		/* remove the newline */
	(void)fgets(term_type, 20, fp);	/* get the new terminal type */
	(void)fclose(fp);

	/* redirect the stdin, stdout, and stderr to the new tty */
	(void)close(0);			/* so we can reopen it */
	(void)open(tmp, O_RDWR);
	(void)dup2(0, 1);
	(void)dup2(0, 2);

	set_input_notify();		/* register the input notifier again */

	(void)signal(SIGHUP, (void_func)kill_proc);	/* catch SIGHUP */

	(void)delwin(message_tty);	/* free the resources */
	(void)delwin(console_tty);
	(void)delwin(status_button);
	(void)delwin(control_button);
	(void)delwin(tty_other);
	(void)delwin(tty_option);
	for (i=0; tty_control[i] != NULL; ++i)
	{
	  (void)delwin(tty_control[i]);
	  tty_control[i] = NULL;
	}
	for (i=0; tty_status[i] != NULL; ++i)
	{
	  (void)delwin(tty_status[i]);
	  tty_status[i] = NULL;
	}

	LINES = COLS = 0;	/* so that setterm() will reset it */
	if (term_type[0] != '\0')
	{
	  (void)sprintf(tmp, "TERM=%s", term_type);
	  (void)putenv(tmp);
	}

	test_items();
	init_ttys();		/* initialize the TTY screens */
	print_status();		/* display the status screen */

	return(NOTIFY_DONE);
}

/******************************************************************************
 * tty_option_proc(), process the system options menu commands.		      *
 ******************************************************************************/
tty_option_proc()
{
    if (arg_no == 1)			/* command without arguments */
      switch (get_com(option_tbl1, arg[0]))
      {
	case 'f':			/* default */
	  core_file = 0;
	  single_pass = 0;
	  quick_test = 0;
	  verbose = 0;
	  trace = 0;
	  auto_start = 0;
	  run_error = 0;
	  send_email = 0;
	  max_errors = 1;
	  max_sys_pass = 0;
	  max_tests = 2;
	  log_period = 0;
	  (void)strcpy(eaddress, "root");
	  (void)strcpy(printer_name, "lp");
	  break;
	case 'd':			/* done */
	  switch_to_control();
	  return;
	case 'o':			/* core file */
	  core_file = !core_file;
	  break;
	case 's':			/* single pass */
	  single_pass = !single_pass;
	  break;
	case 'q':			/* quick test */
	  quick_test = !quick_test;
	  break;
	case 'v':			/* verbose */
	  verbose = !verbose;
	  break;
	case 't':			/* trace */
	  trace = !trace;
	  break;
	case 'a':			/* auto start */
	  auto_start = !auto_start;
	  break;
	case 'r':			/* run on error */
	  run_error = !run_error;
	  break;
	case 'm':			/* send email */
	  if (++send_email > 3) send_email = 0;
	  break;
	case 'h':			/* help */
	  tty_help();
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else if (arg_no == 2)		/* command with one argument */
      switch (get_com(option_tbl2, arg[0]))
      {
	case 'x':			/* max. pass # */
	  max_sys_pass = atoi(arg[1]);
	  break;
	case 'y':			/* max. errors # */
	  max_errors = atoi(arg[1]);
	  break;
	case 'p':			/* printer name */
	  (void)strcpy(printer_name, arg[1]);
	  break;
	case 'c':			/* concurrent tests # */
	  max_tests = atoi(arg[1]);
	  set_max_tests(max_tests);	/* set the max. # of tests per group */
	  break;
	case 'l':			/* log/email period */
	  log_period = atoi(arg[1]);
	  break;
	case 'e':			/* email address */
	  (void)strcpy(eaddress, arg[1]);
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else
    {
      tty_message(com_err);
      return;
    }

    tty_options();			/* update the system option window */
}

/******************************************************************************
 * tty_opfile_proc(), process the option file commands.			      *
 ******************************************************************************/
tty_opfile_proc()
{
    if (arg_no == 1)			/* command without arguments */
      switch (get_com(optfile_tbl1, arg[0]))
      {
	case 'd':			/* done */
	  cur_opfile = 0;		/* reset it for the next time */
	  switch_to_control();
	  break;
	case 'l':			/* load from option file */
	  load_option_files();
	  break;
	case 's':			/* store to option file */
	  store_option_files();
	  tty_opfiles(TRUE);
	  break;
	case 'r':			/* remove the specified option file */
	  remove_option_files();
	  tty_opfiles(TRUE);
	  break;
	case 'n':		/* list next page of option file names */
	  if (max_opfile == 0)
	  {
	    tty_message("No next option file screen!");
	    return;
	  }
	  if (++cur_opfile > max_opfile) cur_opfile = 0;
	  tty_opfiles(TRUE);
	  break;
	case 'h':			/* help */
	  tty_help();
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else if (arg_no == 2)		/* command with one argument */
      switch (get_com(optfile_tbl2, arg[0]))
      {
	case 'm':			/* assign option file name */
	  (void)strcpy(option_fname, arg[1]);
	  tty_opfiles(FALSE);
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else
    {
      tty_message(format_err);
      return;
    }
}

/******************************************************************************
 * tty_logfile_proc(), process the log file commands.			      *
 ******************************************************************************/
tty_logfile_proc()
{
  FILE	*tmp;
  char	temp[162], *editor, *file;

    if (arg_no == 1)			/* command without arguments */
      switch (get_com(logfile_tbl1, arg[0]))
      {
	case 'd':			/* done */
	  switch_to_control();
	  return;

	case 'l':			/* display logs */
	  editor = getenv("EDITOR");
	  if (editor == NULL)
	      editor = "view";		/* use "view" as the default editor */

	  switch (tty_logfile)
	  {
	    case 'e':			/* error logs */
		(void)sprintf(temp, "%s %s", editor, error_file);
		break;
	    case 'i':			/* info. logs */
		(void)sprintf(temp, "%s %s", editor, info_file);
		break;
	    case 'u':			/* UNIX logs */
		(void)sprintf(temp, "%s %s", editor, "/usr/adm/messages");
		break;
	    default:			/* for safty sake */
		return;
	  }
	  nl();
	  (void)system(temp);
	  (void)wrefresh(curscr);
	  nonl();
	  break;

	case 'r':			/* remove logs */
	  switch (tty_logfile)
	  {
	    case 'e':			/* error logs */
		(void)fclose(error_fp);
		if (unlink(error_file) != 0)
		  tty_message("Error logs not removed!");
		error_fp = fopen(error_file, "a");
		break;
	    case 'i':			/* info. logs */
		(void)fclose(info_fp);
		if (unlink(info_file) != 0)
		  tty_message("Info. logs not removed!");
		error_fp = fopen(error_file, "a");
		break;
	    case 'u':			/* UNIX logs */
		if ((tmp=fopen("/usr/adm/messages", "w")) != NULL)
		  (void)fclose(tmp);/* reopen for writing(truncated) */
		else
		  tty_message("UNIX logs not removed!");
		break;
	    default:			/* for safty sake */
		break;
	  }
	  break;

	case 'p':			/* print logs */
	  file = NULL;
	  switch (tty_logfile)
	  {
	    case 'e':			/* error logs */
		file = error_file;
		break;
	    case 'i':			/* info. logs */
		file = info_file;
		break;
	    case 'u':			/* UNIX logs */
		file = "/usr/adm/messages";
		break;
	    default:			/* for safty sake */
		break;
	  }

	  if (file != NULL)
	  {
  	    (void)sprintf(temp, "fold %s | lpr -p -P%s -T %s",
				file, printer_name, file);
	    (void)system(temp);
	  }
	  break;

	case 'h':			/* help */
	  tty_help();
	  break;

	default:
	  tty_message(com_err);
	  return;
      }
    else if (arg_no == 2)		/* command with one argument */
      switch (get_com(logfile_tbl2, arg[0]))
      {
	case 'm':			/* name command */
	  switch (get_com(logfile_tbl3, arg[1]))
	  {
	    case 'e':			/* error logs */
		tty_logfile = 'e';
		break;
	    case 'i':			/* info. logs */
		tty_logfile = 'i';
		break;
	    case 'u':			/* UNIX logs */
		tty_logfile = 'u';
		break;
	    default:
		tty_message(com_err);
		return;
	  }

	  (void)sprintf(temp, "[NAME(m)]: %s",
	    tty_logfile=='e'?
	    "ERROR [INFO(i)/UNIX(u)]":
	    (tty_logfile=='i'?
	    "INFO [UNIX(u)/ERROR(e)]":"UNIX [ERROR(e)/INFO(i)]"));
	  mvwaddstr(tty_other, 5, LOGFILE_COL-6, temp);
	  (void)wrefresh(tty_other);
	  (void)refresh();		/* restore the cursor */
	  
	  break;

	default:
	  tty_message(com_err);
	  break;
      }
    else
      tty_message(format_err);
}

/******************************************************************************
 * tty_test_opt_proc, the tty version of the test option procedure.	      *
 * input: arg[0](device name), and arg[1]("o", or "option").		      *
 ******************************************************************************/
tty_test_opt_proc(devname, option)
char	*devname, *option;
{
  int	test_id;

  if (strcmp(option, "option") != 0 && strcmp(option, "o") != 0)
  {
    tty_message(com_err);
    return;
  }

  for (test_id=0; test_id != exist_tests; ++test_id)
  /* gone through all existing devices to find the specified device */
    if (strcmp(devname, tests[test_id]->devname) == 0) break;

  if (test_id == exist_tests)		/* unknown test */
  {
    tty_message(com_err);
    return;
  }

  if (!tests[test_id]->popup)		/* no option popup for this test */
  {
    tty_message("No option menu for this device!");
    return;
  }

  option_id = test_id;
  window_type = OPTION_TTY;
  (void)wclear(tty_option);

  panel_printf((Panel_item)0, "[DEFAULT(f)] [DONE(d)] [HELP(h)]", 1, -2);

  (void)init_opt_panel(test_id);	/* initialize and display the rest */
}

/******************************************************************************
 * tty_help, the tty version of the on-line help.			      *
 ******************************************************************************/
tty_help()
{
  tty_message("The HELP command is not implemented yet!");
}


/******************************************************************************
 * init_tty_windows, initializes the control/status window arrays to NULL.    *
 ******************************************************************************/
init_tty_windows()
{
  int	i;

  for (i=0; i != TTY_WINNO; ++i)
  {
    tty_control[i] = NULL;
    tty_status[i] = NULL;
  }
}

/******************************************************************************
 * tty_eeprom(), display the eeprom tool.	      *
 ******************************************************************************/
#define	EEPROM_COL1	10
#define	EEPROM_COL2	44
tty_eeprom()
{
#ifndef	sun386
  char	*true, *false;
  char	*tmp;
  int	row;

#ifdef sun4	/* Check for Sun4c */
	if ( cpu_is_sun4c() )
		return;
#endif sun4

  mvwaddstr(tty_other, 0, 24, "<< EEPROM Option Menu >>");
  mvwaddstr(tty_other, 2, 24, "[DEFAULT(f)] [DONE(d)] [CANCEL(c)] [HELP(h)]");
  mvwprintw(tty_other, 4, 24, "LASTUPDATE: %-32s",hwupdate_file);

  true = "True";
  false = "False";
  row = 6;

  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[ASSERTSCSIA(a)]:    %-7s", ttya_rtsdtr_file?true:false);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[ASSERTSCSIB(b)]:    %-7s", ttyb_rtsdtr_file?true:false);
  switch(console_file)
  {
	case 1: tmp = "ttya        ";
		break;
	case 2: tmp = "ttyb        ";
		break;
	case 3: tmp = "VME fb      ";
		break;
	case 4: tmp = "P4 fb       ";
		break;
	default: tmp = "on-board fb";
  }
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[CONSOLETYPE(x)]:    %-5s", tmp);
  switch(scrsize_file)
  {
	case 1: tmp = "1152x900";
		break;
	case 2: tmp = "1600x1280";
		break;
	case 3: tmp = "1440x1440";
		break;
	default: tmp = "1024x1024";
  }
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[SCREENSIZE(z)]:     %-9s", tmp);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[KEYCLICK(k)]:       %-7s", keyclick_file?true:false);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[WATCHDOGREBOOT(w)]: %-7s", watchdog_reboot_file?true:false);
  switch(default_boot_file)
  {
	case 1: tmp = "eeprom";
		break;
	default: tmp = "poll";
  }
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[UNIXBOOTPATH(u)]:   %-7s", tmp);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[UNIXBOOTDEVICE(o)]: %-9s", bootdev_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[CUSTOMLOGO(l)]:     %-7s", custom_banner_file?true:false);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[BANNERSTRING(s)]:   %-40s", banner_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL1,
			"[DIAGBOOTPATH(p)]:   %-40s", diagpath_file);
   row = 6;
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[DIAGBOOTDEVICE(g)]: %-9s", diagdev_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[KEYBOARDTYPE(t)]:   %-1u", kbdtype_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[MEMORYSIZE(m)]:     %-3u", memsize_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[MEMORYTESTSIZE(i)]: %-3u", memtest_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[COLUMNS(n)]:        %-3u", columns_file);
  (void)mvwprintw(tty_other, row++, EEPROM_COL2,
			"[ROWS(r)]:           %-3u", rows_file);

  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();			/* restore the cursor */
#endif	sun386
}

/******************************************************************************
 * tty_eeprom_proc(), process the eeprom option menu commands.		      *
 ******************************************************************************/
tty_eeprom_proc()
{
#ifndef	sun386
#ifdef sun4	/* Check for Sun4c */
	if ( cpu_is_sun4c() )
		return;
#endif sun4
    if (arg_no == 1)			/* command without arguments */
      switch (get_com(eeprom_tbl1, arg[0]))
      {
	case 'f':			/* default */
	  eeprom_get_proc();
	  break;
	case 'd':			/* done */
	  write_eeprom();
	  fix_chksum();
	  switch_to_control();
	  return;
	case 'c':			/* cancel */
	  switch_to_control();
	  return;
	case 'a':			/* Assert SCSI A DTR/RTS */
	  ttya_rtsdtr_file = !ttya_rtsdtr_file;
	  break;
	case 'b':			/* Assert SCSI B DTR/RTS */
	  ttyb_rtsdtr_file = !ttyb_rtsdtr_file;
	  break;
	case 'x':			/* console file */
	  if (++console_file > 4) console_file = 0;
	  break;
	case 'z':			/* screen size file */
	  if (++scrsize_file > 3) scrsize_file = 0;
	  break;
	case 'k':			/* keyclick file */
	  keyclick_file = !keyclick_file;
	  break;
	case 'w':			/* watchdog reboot file */
	  watchdog_reboot_file = !watchdog_reboot_file;
	  break;
	case 'u':			/* Unix boot path */
	  default_boot_file = !default_boot_file;
	  break;
	case 'l':			/* custom logo file */
	  custom_banner_file = !custom_banner_file;
	  break;
	case 'h':			/* help */
	  tty_help();
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else if (arg_no == 2)		/* command with one argument */
      switch (get_com(eeprom_tbl2, arg[0]))
      {
	case 'o':			/* Unix boot device */
	  (void)strcpy(bootdev_file, arg[1]);
	  break;
	case 's':			/* banner string */
	  (void)strcpy(banner_file, arg[1]);
	  break;
	case 'p':			/* Diagnostics boot path name */
	  (void)strcpy(diagpath_file, arg[1]);
	  break;
	case 'g':			/* Diagniostics boot path */
	  (void)strcpy(diagdev_file, arg[1]);
	  break;
	case 't':			/* Keyboard type */
	  kbdtype_file = atoi(arg[1]);
	  break;
	case 'm':			/* Memory size */
	  memsize_file = atoi(arg[1]);
	  break;
	case 'i':			/* Memory test size */
	  memtest_file = atoi(arg[1]);
	  break;
	case 'n':			/* # of Columns */
	  columns_file = atoi(arg[1]);
	  break;
	case 'r':			/* # of Rows */
	  rows_file = atoi(arg[1]);
	  break;
	default:
	  tty_message(com_err);
	  return;
      }
    else
    {
      tty_message(com_err);
      return;
    }

    tty_eeprom();			/* update the eeprom window */
#endif	sun386
}

/******************************************************************************
 * tty_processors_proc(), process the processors option menu commands.        *
 ******************************************************************************/
tty_processors_proc()
{
    int i, index, num, bit;
    char cmd;
 
    cmd = get_com(processors_tbl1, arg[0]);
    if (arg_no == 1)
        if ((cmd < (number_system_processors +'0')) && (cmd >= '0'))
        {
            index = cmd - '0';
            system_processor_selected[index] = !system_processor_selected[index];
        }
        else
            switch (cmd)
            {
                case 'f':
                    for (bit = 1, num = 0, i = 0; num < number_system_processors; bit <<= 1, i++)
                    {
		        if (system_processors_mask & bit)
                        {
                            system_processor_selected[i] = 0;
                            num++;
                        }
                    }
                    break;
                case 'd':
                    switch_to_control();
                    return;
                case 'h':                       /* help */
                    tty_help();
                    break;
                default:
                    tty_message(com_err);
                    break;
            }
        else
        {
            tty_message(com_err);
            return;
        }
 
    tty_processors();           /* update the schedule window */
}

/******************************************************************************
 * tty_schedule_proc(), process the schedule option menu commands.	      *
 ******************************************************************************/
tty_schedule_proc()
{
   if (arg_no == 1)			/* command without arguments */
      switch (get_com(schedule_tbl1, arg[0]))
      {
	case 'd':		/* done */
	  ttime_process_proc();
	  switch_to_control();
	  return;
	case 'c':		/* cancel */
	  switch_to_control();
	  return;
	case 's':		/* Enable or Disable Scheduler */
	  schedule_file = !schedule_file;
	  break;
	case 'h':			/* help */
	  tty_help();
	  break;
	default:
	  tty_message(com_err);
	  break;
      }
    else if (arg_no == 2)	/* command with one argument */
      switch (get_com(schedule_tbl2, arg[0]))
      {
	case 'r':		/* Start date */
	  (void)strcpy(start_date_file, arg[1]);
	  break;
	case 'x':		/* Start time */
	  (void)strcpy(run_time_file, arg[1]);
	  break;
	case 't':		/* Start time */
	  (void)strcpy(start_time_file, arg[1]);
	  break;
	case 'o':		/* Stop date */
	  (void)strcpy(stop_date_file, arg[1]);
	  break;
	case 'm':		/* Stop time */
	  (void)strcpy(stop_time_file, arg[1]);
	  break;
	default:
	  tty_message(com_err);
	  break;
      }
    else
    {
      tty_message(com_err);
      return;
    }

    tty_schedule();		/* update the schedule window */
}

/******************************************************************************
 * tty_schedule(), display the schedule tool.	      *
 ******************************************************************************/
#define	SCHEDULE_COL	24
tty_schedule()
{
  char	*enable, *disable;
  int	row;

  mvwaddstr(tty_other, 0, SCHEDULE_COL, "<< SCHEDULE Option Menu >>");
  mvwaddstr(tty_other, 2, SCHEDULE_COL, "[DONE(d)] [CANCEL(c)] [HELP(h)]");

  enable = "Enable";
  disable = "Disable";
  row = 6;

  (void)mvwprintw(tty_other, 4, SCHEDULE_COL,
			"[SCHEDULER(s)]:        %-7s", schedule_file?enable:disable);
  (void)mvwprintw(tty_other, row++, SCHEDULE_COL,
			"[RUNTIME  (x)]:        %-9s", run_time_file);
  (void)mvwprintw(tty_other, row++, SCHEDULE_COL,
			"[STARTDATE(r)]:        %-9s", start_date_file);
  (void)mvwprintw(tty_other, row++, SCHEDULE_COL,
			"[STARTTIME(t)]:        %-9s", start_time_file);
  (void)mvwprintw(tty_other, row++, SCHEDULE_COL,
			"[STOPDATE (o)]:        %-9s", stop_date_file);
  (void)mvwprintw(tty_other, row++, SCHEDULE_COL,
			"[STOPTIME (m)]:        %-9s", stop_time_file);

  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();			/* restore the cursor */
}
 
/******************************************************************************
 * tty_processors(), display the processors enable.           *
 ******************************************************************************/
#define PROCESSORS_COL  24
tty_processors()
{
  char  *enable, *disable;
  int   row, i, processor_mask, num;
  unsigned int b;
 
  mvwaddstr(tty_other, 0, PROCESSORS_COL, "<< PROCESSORS Option Menu >>");
  mvwaddstr(tty_other, 2, PROCESSORS_COL, "[DEFAULT(f)] [DONE(d)] [HELP(h)]");
 
  enable = "Enable";
  disable = "Disable";
  row = 5;
 
  for (b = 1, num = 0, i = 0; num < number_system_processors; b <<= 1, i++)
  {
      processor_mask = system_processors_mask & b;
      if (processor_mask)
      {
          (void)mvwprintw(tty_other, row++, OPTION_COL,
                        "[PROCESSOR %d(%c)]:    %-7s",
                        i, i+ '0',
                        system_processor_selected[i]?disable:enable);
          num++;
      }
  }
 
  touchwin(tty_other);
  (void)wrefresh(tty_other);
  (void)refresh();                      /* restore the cursor */
}
